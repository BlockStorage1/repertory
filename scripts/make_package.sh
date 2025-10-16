#!/usr/bin/env bash

TEMP_DIR=$(mktemp -d)

PROJECT_SCRIPTS_DIR=$(realpath "$0")
PROJECT_SCRIPTS_DIR=$(dirname "${PROJECT_SCRIPTS_DIR}")
. "${PROJECT_SCRIPTS_DIR}/env.sh" "$1" "$2" "$3" "$4" "$5"

function error_exit() {
  echo $1
  rm -rf ${TEMP_DIR}
  exit $2
}

function cleanup_packages() {
  local SOURCE_FILE=$1

  pushd "${PROJECT_DIST_DIR}"
  if [ -f "${SOURCE_FILE}" ]; then
    rm -f "${SOURCE_FILE}" || error_exit "failed to delete file: ${SOURCE_FILE}" 1
  fi

  if [ -f "${SOURCE_FILE}.sha256" ]; then
    rm -f "${SOURCE_FILE}.sha256" || error_exit "failed to delete file: ${SOURCE_FILE}.sha256" 1
  fi

  if [ -f "${SOURCE_FILE}.sig" ]; then
    rm -f "${SOURCE_FILE}.sig" || error_exit "failed to delete file: ${SOURCE_FILE}.sig" 1
  fi
  popd
}

function create_file_validations() {
  local SOURCE_FILE=$1
  sha256sum ${SOURCE_FILE} >${SOURCE_FILE}.sha256 || error_exit "failed to create sha256 for file: ${SOURCE_FILE}" 1
  if [ "${PROJECT_PRIVATE_KEY}" != "" ]; then
    openssl dgst -sha256 -sign "${PROJECT_PRIVATE_KEY}" -out "${SOURCE_FILE}.sig" "${SOURCE_FILE}" || error_exit "failed to create signature for file: ${SOURCE_FILE}" 1
    openssl dgst -sha256 -verify "${PROJECT_PUBLIC_KEY}" -signature "${SOURCE_FILE}.sig" "${SOURCE_FILE}" || error_exit "failed to validate signature for file: ${SOURCE_FILE}" 1
  fi
}

if [ ! -d "${PROJECT_DIST_DIR}" ]; then
  error_exit "dist directory not found: ${PROJECT_DIST_DIR}" 2
fi

cleanup_packages "${PROJECT_OUT_FILE}"
cleanup_packages "${PROJECT_FILE_PART}_setup.exe"
cleanup_packages "${PROJECT_FILE_PART}.dmg"

rsync -av --progress ${PROJECT_DIST_DIR}/ ${TEMP_DIR}/${PROJECT_NAME}/ || error_exit "failed to rsync" 1

for APP in ${PROJECT_APP_LIST[@]}; do
  if [ -d "${TEMP_DIR}/${PROJECT_NAME}/${APP}.app" ]; then
    strip "${TEMP_DIR}/${PROJECT_NAME}/${APP}.app/Contents/MacOS/${APP}${PROJECT_APP_BINARY_EXT}"
  elif [ "${PROJECT_BUILD_SHARED_LIBS}" == "ON" ] && [ "${PROJECT_IS_MINGW}" != "1" ]; then
    strip "${TEMP_DIR}/${PROJECT_NAME}/bin/${APP}${PROJECT_APP_BINARY_EXT}"
  else
    strip "${TEMP_DIR}/${PROJECT_NAME}/${APP}${PROJECT_APP_BINARY_EXT}"
  fi
done

pushd "${TEMP_DIR}/${PROJECT_NAME}/"
IFS=$'\n'
set -f
FILE_LIST=$(find . -type f -not -path "*/.app/*")
for FILE in ${FILE_LIST}; do
  create_file_validations "${FILE}"
done
unset IFS
set +f
popd

pushd "${PROJECT_DIST_DIR}"
tar cvzf "${PROJECT_OUT_FILE}" -C ${TEMP_DIR} . || error_exit "failed to create archive: ${PROJECT_OUT_FILE}" 1
create_file_validations "${PROJECT_OUT_FILE}"
popd

if [ -d "${TEMP_DIR}/${PROJECT_NAME}/${PROJECT_NAME}.app" ]; then
  APP_SRC="${TEMP_DIR}/${PROJECT_NAME}/${PROJECT_NAME}.app"

  DMG_ROOT="${TEMP_DIR}/dmgroot"
  mkdir -p "${DMG_ROOT}" || error_exit "failed to create dmgroot" 1

  INSTALLER="${DMG_ROOT}/Install ${PROJECT_NAME}.command"
  INSTALLER_SRC="${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/Install ${PROJECT_NAME}.command"

  if [ -f "${INSTALLER_SRC}" ]; then
    HIDDEN_DIR="${DMG_ROOT}/.payload"
    mkdir -p "${HIDDEN_DIR}" || error_exit "failed to create payload dir" 1
    APP_DEST_DIR="${HIDDEN_DIR}"
  else
    APP_DEST_DIR="${DMG_ROOT}"
  fi

  rsync -a "${APP_SRC}" "${APP_DEST_DIR}/" || error_exit "failed to stage app bundle" 1

  if [ -f "${INSTALLER_SRC}" ]; then
    chflags hidden "${HIDDEN_DIR}" "${HIDDEN_DIR}/${PROJECT_NAME}.app" 2>/dev/null || true
  fi

  ln -s /Applications "${DMG_ROOT}/Applications" 2>/dev/null || true

  if [ -f "${INSTALLER_SRC}" ]; then
    cp -f "${INSTALLER_SRC}" "${INSTALLER}" || error_exit "failed to copy install command" 1
    chmod +x "${INSTALLER}" || error_exit "failed to chmod install command" 1

    SAFE_PREFIX="$(printf '%s' "${PROJECT_MACOS_BUNDLE_ID}" | sed -e 's/[\/&]/\\&/g')"
    /usr/bin/sed -i '' -e "s|^LABEL_PREFIX=.*$|LABEL_PREFIX=\"${SAFE_PREFIX}\"|g" "${INSTALLER}"

    LABEL_ASSIGNED="$(/usr/bin/awk -F= '/^LABEL_PREFIX=/{sub(/^[^=]*=/,""); gsub(/^"|"$/,""); print; exit}' "${INSTALLER}")"
    if ! /usr/bin/awk -v v="${LABEL_ASSIGNED}" '
        BEGIN {
          if (length(v) == 0) exit 1;
          if (v == "__LABEL_PREFIX__") exit 1;
          if (v !~ /^[A-Za-z0-9._-]+$/) exit 1;
          if (v !~ /\./) exit 1;
          exit 0;
        }'; then
      error_exit "DMG build abort: invalid LABEL_PREFIX written to installer (value: \"${LABEL_ASSIGNED}\"). Check PROJECT_MACOS_BUNDLE_ID and sed substitution." 1
    fi
  fi

  DMG_OUT="${PROJECT_FILE_PART}.dmg"
  hdiutil create \
    -volname "${PROJECT_NAME}" \
    -fs HFS+ \
    -srcfolder "${DMG_ROOT}" \
    -ov -format UDZO \
    "${PROJECT_DIST_DIR}/${DMG_OUT}" || error_exit "hdiutil failed" 1

  pushd "${PROJECT_DIST_DIR}"
  create_file_validations "${DMG_OUT}"
  popd
elif [ "${PROJECT_IS_MINGW}" == "1" ] && [ -f "${PROJECT_DIST_DIR}/../${PROJECT_NAME}.iss" ]; then
  cp -f "${PROJECT_DIST_DIR}/../${PROJECT_NAME}.iss" "${TEMP_DIR}/${PROJECT_NAME}.iss"
  rsync -av --progress --delete ${PROJECT_SOURCE_DIR}/support/3rd_party/*.msi ${TEMP_DIR}/3rd_party/

  pushd "${TEMP_DIR}"
  if [ "${PROJECT_IS_MINGW_UNIX}" == "1" ]; then
    wine64 "c:/Program Files (x86)/Inno Setup 6/iscc.exe" "${PROJECT_NAME}.iss" || exit 1
  else
    iscc "${PROJECT_NAME}.iss" || exit 1
  fi

  pushd "Output"
  create_file_validations "${PROJECT_FILE_PART}_setup.exe"
  cp -f * "${PROJECT_DIST_DIR}/"
  popd

  popd
fi

error_exit "created package ${PROJECT_FILE_PART}" 0
