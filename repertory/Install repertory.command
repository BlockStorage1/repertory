#!/usr/bin/env bash
# No `-e` (we tolerate benign non-zero returns); keep -Euo
set -Euo pipefail

LOG_DIR="/tmp"
LOG_FILE="${LOG_DIR}/Install-$(date +%Y%m%d-%H%M%S).log"
exec > >(tee -a "$LOG_FILE") 2>&1
echo "Logging to: $LOG_FILE"

TARGET_DIR="/Applications"
APP_NAME="repertory.app"

# Embedded at pack time (from CFBundleIdentifier prefix)
LABEL_PREFIX="__LABEL_PREFIX__"
UI_LABEL="${LABEL_PREFIX}.ui"

staged=""
backup=""
snapfile=""
skip_ui_launch=0

log() { printf "[%(%H:%M:%S)T] %s\n" -1 "$*"; }
warn() { log "WARN: $*"; }
die() {
  log "ERROR: $*"
  exit 2
}

here="$(cd "$(dirname "$0")" && pwd)"

# Locate source app on the DMG (supports hidden payload dirs)
src_app="${here}/${APP_NAME}"
if [[ ! -d "$src_app" ]]; then
  src_app="$(/usr/bin/find "$here" -type d -name "$APP_NAME" -print -quit 2>/dev/null || true)"
fi
[[ -d "$src_app" ]] || die "Could not find ${APP_NAME} on this disk image."

app_basename="$(basename "$src_app")"
dest_app="${TARGET_DIR}/${APP_NAME}"

bundle_id_of() { /usr/bin/defaults read "$1/Contents/Info" CFBundleIdentifier 2>/dev/null || true; }
bundle_exec_of() { /usr/bin/defaults read "$1/Contents/Info" CFBundleExecutable 2>/dev/null || echo "${app_basename%.app}"; }
bundle_version_of() {
  /usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$1/Contents/Info.plist" 2>/dev/null ||
    /usr/bin/defaults read "$1/Contents/Info" CFBundleShortVersionString 2>/dev/null || echo "(unknown)"
}
bundle_build_of() {
  /usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$1/Contents/Info.plist" 2>/dev/null ||
    /usr/bin/defaults read "$1/Contents/Info" CFBundleVersion 2>/dev/null || echo "(unknown)"
}

# Require /Applications; prompt for sudo if needed; abort if cannot elevate
USE_SUDO=0
SUDO=""
ensure_target_writable() {
  if mkdir -p "${TARGET_DIR}/.repertory_install_test.$$" 2>/dev/null; then
    rmdir "${TARGET_DIR}/.repertory_install_test.$$" 2>/dev/null || true
    USE_SUDO=0
    SUDO=""
    return 0
  fi
  if command -v sudo >/dev/null 2>&1; then
    log "Elevating privileges to write to ${TARGET_DIR}…"
    sudo -v || die "Administrator privileges required to install into ${TARGET_DIR}."
    USE_SUDO=1
    SUDO="sudo"
    return 0
  fi
  die "Cannot write to ${TARGET_DIR} and sudo is unavailable."
}

# ----- STRICT LABEL PREFIX GATE (fail if invalid) -----
_is_valid_label_prefix() {
  local p="${1:-}"
  [[ -n "$p" ]] && [[ "$p" != "__LABEL_PREFIX__" ]] && [[ "$p" =~ ^[A-Za-z0-9._-]+$ ]] && [[ "$p" == *.* ]]
}
if ! _is_valid_label_prefix "${LABEL_PREFIX:-}"; then
  die "Invalid LABEL_PREFIX in installer (value: \"${LABEL_PREFIX:-}\"). Rebuild the DMG so the installer contains a valid reverse-DNS prefix."
fi

# ----- LaunchServices helpers -----
ls_prune_bundle_id() {
  local bundle_id="$1" keep_path="$2"
  [[ -z "$bundle_id" ]] && return 0
  local search_roots=("/Applications" "$HOME/Applications" "/Volumes")
  if [[ -n "${here:-}" && "$here" == /Volumes/* ]]; then search_roots+=("$here"); fi
  local candidates=""
  for root in "${search_roots[@]}"; do
    [[ -d "$root" ]] || continue
    candidates+=$'\n'"$(/usr/bin/mdfind -onlyin "$root" "kMDItemCFBundleIdentifier == '${bundle_id}'" 2>/dev/null || true)"
  done
  # Include backups adjacent to keep_path (quote-safe)
  local parent_dir="${keep_path%/*.app}"
  candidates+=$'\n'$(/bin/ls -1d "${parent_dir}/"*.bak 2>/dev/null || true)
  local LSREG="/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister"
  printf "%s\n" "$candidates" | /usr/bin/awk 'NF' | /usr/bin/sort -u | while IFS= read -r p; do
    [[ -z "$p" || ! -d "$p" || "$p" == "$keep_path" ]] && continue
    log "Unregistering older LS entry for ${bundle_id}: $p"
    "$LSREG" -u "$p" >/dev/null 2>&1 || true
  done
}

# ----- FUSE unmount (no process killing here) -----
is_mounted() { /sbin/mount | /usr/bin/awk '{print $3}' | /usr/bin/grep -Fx "${1:-}" >/dev/null 2>&1; }
_list_repertory_fuse_mounts() { /sbin/mount | /usr/bin/grep -Ei 'macfuse|osxfuse' | /usr/bin/awk '{print $3}' | /usr/bin/grep -i "repertory" || true; }
_unmount_one() {
  local mnt="${1:-}"
  [[ -n "$mnt" ]] || return 0
  /usr/sbin/diskutil unmount "$mnt" >/dev/null 2>&1 || /sbin/umount "$mnt" >/dev/null 2>&1 || true
  if is_mounted "$mnt"; then
    /usr/sbin/diskutil unmount force "$mnt" >/dev/null 2>&1 || /sbin/umount -f "$mnt" >/dev/null 2>&1 || true
  fi
  for _ in {1..20}; do
    is_mounted "$mnt" || return 0
    sleep 0.25
  done
  return 1
}
unmount_existing_repertory_volumes() {
  # Hard-fail on the first unmount problem.
  while IFS= read -r mnt; do
    [[ -z "$mnt" ]] && continue
    log "Unmounting FUSE mount: $mnt"
    if ! _unmount_one "$mnt"; then
      warn "Failed to unmount $mnt"
      return 1
    fi
  done < <(_list_repertory_fuse_mounts)
  sync || true
  sleep 0.3
  return 0
}

# ----- user LaunchAgents (by LABEL_PREFIX only) -----
get_plist_label() { /usr/bin/defaults read "$1" Label 2>/dev/null || /usr/libexec/PlistBuddy -c "Print :Label" "$1" 2>/dev/null || basename "$1" .plist; }

# Return the executable path a LaunchAgent runs (ProgramArguments[0] or Program).
# Echoes empty string if neither is present.
get_plist_exec_path() {
  local plist="$1" arg0=""
  # Prefer ProgramArguments[0]
  arg0="$(/usr/libexec/PlistBuddy -c 'Print :ProgramArguments:0' "$plist" 2>/dev/null || true)"
  if [[ -z "$arg0" ]]; then
    # Fallback to Program (older style)
    arg0="$(/usr/libexec/PlistBuddy -c 'Print :Program' "$plist" 2>/dev/null || true)"
  fi
  printf '%s\n' "$arg0"
}

snapshot_launchagents_user() {
  local user_agents="$HOME/Library/LaunchAgents"
  snapfile="$(/usr/bin/mktemp "/tmp/repertory_launchagents.XXXXXX")" || snapfile=""
  if [[ -z "$snapfile" ]]; then
    warn "Could not create temporary snapshot file; skipping LaunchAgent restart snapshot."
    return 0
  fi
  [[ -d "$user_agents" ]] || return 0

  # We collect non-UI first, then UI last, to preserve restart order later.
  local tmp_nonui tmp_ui
  tmp_nonui="$(/usr/bin/mktemp "/tmp/repertory_launchagents.nonui.XXXXXX")" || tmp_nonui=""
  tmp_ui="$(/usr/bin/mktemp "/tmp/repertory_launchagents.ui.XXXXXX")" || tmp_ui=""

  /usr/bin/find "$user_agents" -maxdepth 1 -type f -name "${LABEL_PREFIX}"'*.plist' -print 2>/dev/null |
    while IFS= read -r plist; do
      [[ -z "$plist" ]] && continue

      # Label must match the prefix
      local label
      label="$(get_plist_label "$plist")"
      [[ -n "$label" && "$label" == "${LABEL_PREFIX}"* ]] || continue

      # Executable must point into the *installed* app path
      local exec_path
      exec_path="$(get_plist_exec_path "$plist")"
      [[ -n "$exec_path" ]] || continue
      # Normalize: only accept absolute paths under $dest_app (e.g. .../repertory.app/Contents/...)
      if [[ "$exec_path" != "$dest_app/"* ]]; then
        # Not one of ours; skip
        continue
      fi

      # Defer UI label to the end
      if [[ "$label" == "$UI_LABEL" ]]; then
        [[ -n "$tmp_ui" ]] && printf "%s\t%s\n" "$plist" "$label" >>"$tmp_ui"
      else
        [[ -n "$tmp_nonui" ]] && printf "%s\t%s\n" "$plist" "$label" >>"$tmp_nonui"
      fi
    done

  # Stitch together: non-UI first, then UI
  [[ -s "$tmp_nonui" ]] && /bin/cat "$tmp_nonui" >>"$snapfile"
  [[ -s "$tmp_ui" ]] && /bin/cat "$tmp_ui" >>"$snapfile"

  [[ -n "$tmp_nonui" ]] && /bin/rm -f "$tmp_nonui" 2>/dev/null || true
  [[ -n "$tmp_ui" ]] && /bin/rm -f "$tmp_ui" 2>/dev/null || true

  log "Snapshot contains $(/usr/bin/wc -l <"$snapfile" 2>/dev/null || echo 0) LaunchAgent(s)."
}

unload_launchd_helpers_user() {
  local uid user_agents
  uid="$(id -u)"
  user_agents="$HOME/Library/LaunchAgents"
  [[ -d "$user_agents" ]] || return 0

  while IFS= read -r plist; do
    [[ -z "$plist" ]] && continue
    local base label
    base="$(basename "$plist")"
    [[ "$base" == "${LABEL_PREFIX}"* ]] || continue
    label="$(get_plist_label "$plist")"
    [[ -n "$label" && "$label" == "${LABEL_PREFIX}"* ]] || continue
    log "Booting out user label ${label} (${plist})"
    /bin/launchctl bootout "gui/${uid}" "$plist" 2>/dev/null ||
      /bin/launchctl bootout "gui/${uid}" "$label" 2>/dev/null ||
      /bin/launchctl remove "$label" 2>/dev/null || true
  done < <(/usr/bin/find "$user_agents" -maxdepth 1 -type f -name "${LABEL_PREFIX}"'*.plist' -print 2>/dev/null)

  /bin/launchctl list 2>/dev/null | /usr/bin/awk -v pre="$LABEL_PREFIX" 'NF>=3 && index($3, pre)==1 {print $3}' |
    while read -r lbl; do
      [[ -z "$lbl" ]] && continue
      log "Booting out leftover user label: $lbl"
      /bin/launchctl bootout "gui/${uid}" "$lbl" 2>/dev/null || /bin/launchctl remove "$lbl" 2>/dev/null || true
    done
}

restart_launchagents_from_snapshot() {
  [[ -n "${snapfile:-}" && -f "${snapfile}" ]] || return 0

  local uid count=0 ui_seen=0
  uid="$(id -u)"

  # Pass 1: restart all non-UI agents first
  while IFS=$'\t' read -r plist label; do
    [[ -n "$plist" && -n "$label" ]] || continue
    [[ -f "$plist" ]] || continue
    [[ "$label" == "$UI_LABEL" ]] && continue

    log "Re-starting LaunchAgent: ${label}"
    /bin/launchctl bootstrap "gui/${uid}" "$plist" 2>/dev/null || true
    /bin/launchctl kickstart -k "gui/${uid}/${label}" 2>/dev/null || true
    ((count++)) || true
  done <"$snapfile"

  # Give helpers a moment to settle (e.g., automounts)
  sleep 0.3

  # Pass 2: restart the UI agent last (if present in the snapshot)
  while IFS=$'\t' read -r plist label; do
    [[ -n "$plist" && -n "$label" ]] || continue
    [[ -f "$plist" ]] || continue
    [[ "$label" == "$UI_LABEL" ]] || continue

    log "Re-starting UI LaunchAgent last: ${label}"
    /bin/launchctl bootstrap "gui/${uid}" "$plist" 2>/dev/null || true
    /bin/launchctl kickstart -k "gui/${uid}/${label}" 2>/dev/null || true
    ui_seen=1
    ((count++)) || true
  done <"$snapfile"

  log "Re-started ${count} LaunchAgent(s) with prefix ${LABEL_PREFIX}." || true

  if ((ui_seen)); then
    # If the UI label is active, skip manual open(1).
    if /bin/launchctl list | /usr/bin/awk '{print $3}' | /usr/bin/grep -Fxq "$UI_LABEL"; then
      log "UI LaunchAgent (${UI_LABEL}) active after restart; skipping manual UI launch."
      skip_ui_launch=1
    fi
  fi
}

# ----- quarantine helper -----
remove_quarantine() {
  local path="${1:-}"
  if [[ "${USE_SUDO:-0}" == "1" ]]; then
    sudo /usr/bin/xattr -dr com.apple.quarantine "$path" 2>/dev/null || true
  else
    /usr/bin/xattr -dr com.apple.quarantine "$path" 2>/dev/null || true
  fi
}

# ----- process helpers -----
kill_repertory_processes() {
  local exec_name="$1"
  /usr/bin/pkill -TERM -f "$dest_app" >/dev/null 2>&1 || true
  /usr/bin/pkill -TERM -x "$exec_name" >/dev/null 2>&1 || true
  for _ in {1..20}; do
    /usr/bin/pgrep -af "$dest_app" >/dev/null 2>&1 || /usr/bin/pgrep -x "$exec_name" >/dev/null 2>&1 || break
    sleep 0.1
  done
  /usr/bin/pkill -KILL -f "$dest_app" >/dev/null 2>&1 || true
  /usr/bin/pkill -KILL -x "$exec_name" >/dev/null 2>&1 || true
}

# ----- visibility helper -----
unhide_path() {
  local path="$1"
  /usr/bin/chflags -R nohidden "$path" 2>/dev/null || true
  /usr/bin/xattr -d -r com.apple.FinderInfo "$path" 2>/dev/null || true
}

# ----- stage / validate / activate / post-activate -----
stage_new_app() {
  staged="${dest_app}.new-$$"
  log "Staging new app → $staged"
  $SUDO /usr/bin/ditto "$src_app" "$staged" || die "ditto to stage failed"
  remove_quarantine "$staged"
}

validate_staged_app() {
  [[ -f "$staged/Contents/Info.plist" ]] || {
    $SUDO /bin/rm -rf "$staged"
    die "staged app missing Info.plist"
  }
  local exe_name_staged
  exe_name_staged="$(/usr/bin/defaults read "$staged/Contents/Info" CFBundleExecutable 2>/dev/null || echo "${app_basename%.app}")"
  [[ -x "$staged/Contents/MacOS/$exe_name_staged" ]] || {
    $SUDO /bin/rm -rf "$staged"
    die "staged app missing main executable"
  }
}

activate_staged_app() {
  if [[ -d "$dest_app" ]]; then
    backup="${dest_app}.$(date +%Y%m%d%H%M%S).bak"
    log "Moving existing app to backup: $backup"
    $SUDO /bin/mv "$dest_app" "$backup" || {
      $SUDO /bin/rm -rf "$staged"
      die "failed to move existing app out of the way"
    }
  fi
  log "Activating new app → $dest_app"
  if ! $SUDO /bin/mv "$staged" "$dest_app"; then
    warn "Activation failed; attempting rollback…"
    [[ -n "$backup" && -d "$backup" ]] && $SUDO /bin/mv "$backup" "$dest_app" || true
    $SUDO /bin/rm -rf "$staged" || true
    die "install activation failed"
  fi
}

post_activate_cleanup() {
  log "Clearing quarantine on installed app…"
  remove_quarantine "$dest_app"
  log "Clearing hidden flags on installed app…"
  unhide_path "$dest_app"

  local LSREG="/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister"
  [[ -x "$LSREG" ]] && "$LSREG" -f "$dest_app" >/dev/null 2>&1 || true
  local BID
  BID="$(bundle_id_of "$dest_app")"
  ls_prune_bundle_id "$BID" "$dest_app"

  log "Installed ${app_basename}: version=$(bundle_version_of "$dest_app") build=$(bundle_build_of "$dest_app")"
}

launch_ui() {
  log "Launching the new app…"
  /usr/bin/open -n "$dest_app" || warn "open -n by path failed; not falling back to -b to avoid launching a stale copy."
}

remove_backup() {
  [[ -n "$backup" && -d "$backup" ]] && {
    log "Removing backup: $backup"
    $SUDO /bin/rm -rf "$backup" || warn "Could not remove backup (safe to delete manually): $backup"
  }
  log "Done."
}

cleanup_staged() {
  if [[ -n "${staged:-}" && -d "${staged}" ]]; then
    log "Cleaning up staged folder: ${staged}"
    if [[ "${USE_SUDO:-0}" == "1" ]]; then
      sudo /bin/rm -rf "${staged}" 2>/dev/null || true
    else
      /bin/rm -rf "${staged}" 2>/dev/null || true
    fi
  fi
  if [[ -n "${snapfile:-}" && -f "${snapfile}" ]]; then
    /bin/rm -f "${snapfile}" 2>/dev/null || true
  fi
}

main() {
  ensure_target_writable

  local exec_name
  exec_name="$(bundle_exec_of "$src_app")"

  # 1) Snapshot agents we'll restart later
  snapshot_launchagents_user

  # 2) Hard-fail if any FUSE unmount fails
  unmount_existing_repertory_volumes || die "One or more FUSE mounts resisted unmount."

  # 3) Stop user LaunchAgents (do NOT delete plists)
  unload_launchd_helpers_user

  # 4) Kill any remaining repertory processes
  kill_repertory_processes "$exec_name"

  # 5) Stage → validate → activate → post-activate
  stage_new_app
  validate_staged_app
  activate_staged_app
  post_activate_cleanup

  # 6) Re-start previously-running LaunchAgents (so automount helpers come up)
  restart_launchagents_from_snapshot

  # 7) If UI LaunchAgent came back, skip manual launch
  if ((!skip_ui_launch)); then
    launch_ui
  fi

  # 8) Remove backup now that everything is good
  remove_backup
}

trap 'rc=$?; cleanup_staged; if (( rc != 0 )); then echo "Installer failed with code $rc. See $LOG_FILE"; fi' EXIT
main "$@"
