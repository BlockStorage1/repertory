/*
  Copyright <2018-2025> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#if !defined(_WIN32)

#include "utils/unix.hpp"

#include "utils/collection.hpp"
#include "utils/config.hpp"
#include "utils/error.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"

namespace {
[[nodiscard]] auto get_group_list(auto *pass) -> std::vector<gid_t> {
  REPERTORY_USES_FUNCTION_NAME();

  std::vector<gid_t> groups{};
#if defined(__APPLE__)
  constexpr int buffer_count{8};
  constexpr int max_group_count{1024};
  groups.resize(buffer_count);

  std::size_t orig_count{0U};
  while (true) {
    auto group_count{static_cast<int>(groups.size())};
    if (group_count > max_group_count) {
      repertory::utils::error::handle_error(function_name,
                                            "group list has too many groups");
      break;
    }

    auto res{
        getgrouplist(pass->pw_name, static_cast<int>(pass->pw_gid),
                     reinterpret_cast<int *>(groups.data()), &group_count),
    };
    if (res < 0) {
      if (orig_count == 0U) {
        repertory::utils::error::handle_error(
            function_name, std::string{"failed to get group list|error|"} +
                               std::to_string(errno));
      }

      break;
    }

    groups.resize(static_cast<std::size_t>(group_count));
    if (groups.size() == orig_count) {
      break;
    }

    orig_count = groups.size();
  }
#else  // !defined(__APPLE__)
  int group_count{};
  auto res = getgrouplist(pass->pw_name, pass->pw_gid, nullptr, &group_count);
  if (res >= 0) {
    repertory::utils::error::handle_error(
        function_name, std::string{"failed to get group list count|error|"} +
                           std::to_string(errno));
  }

  groups.resize(static_cast<std::size_t>(group_count));
  res = getgrouplist(pass->pw_name, pass->pw_gid, groups.data(), &group_count);
  if (res >= 0) {
    repertory::utils::error::handle_error(
        function_name,
        std::string{"failed to get group list|error|"} + std::to_string(errno));
  }
#endif // !defined(__APPLE__)

  return groups;
}

#if defined(__linux__)
[[nodiscard]] auto sanitize_basename(std::string_view app_name) -> std::string {
  std::string out;
  out.reserve(app_name.size());
  for (const auto &cur_ch : app_name) {
    if ((cur_ch >= 'a' && cur_ch <= 'z') || (cur_ch >= '0' && cur_ch <= '9') ||
        (cur_ch == '-' || cur_ch == '_')) {
      out.push_back(cur_ch);
    } else if (cur_ch >= 'A' && cur_ch <= 'Z') {
      out.push_back(static_cast<char>(cur_ch - 'A' + 'a'));
    } else {
      out.push_back('-'); // replace spaces/symbols
    }
  }

  std::string collapsed;
  collapsed.reserve(out.size());
  bool prev_dash = false;
  for (const auto &cur_ch : out) {
    if (cur_ch == '-') {
      if (not prev_dash) {
        collapsed.push_back(cur_ch);
      }
      prev_dash = true;
    } else {
      collapsed.push_back(cur_ch);
      prev_dash = false;
    }
  }

  if (collapsed.empty()) {
    collapsed = "app";
  }
  return collapsed;
}

[[nodiscard]] auto get_autostart_dir() -> std::string {
  auto config = repertory::utils::get_environment_variable("XDG_CONFIG_HOME");
  if (config.empty()) {
    config = repertory::utils::path::combine(
        repertory::utils::get_environment_variable("HOME"), {".config"});
  }

  return repertory::utils::path::combine(config, {"autostart"});
}

[[nodiscard]] auto desktop_file_path_for(std::string_view app_name)
    -> std::string {
  return repertory::utils::path::combine(
      get_autostart_dir(), {
                               sanitize_basename(app_name) + ".desktop",
                           });
}

[[nodiscard]] auto join_args_for_exec(const std::vector<std::string> &args)
    -> std::string {
  std::string str;
  for (const auto &arg : args) {
    if (not str.empty()) {
      str += ' ';
    }

    auto needs_quotes = arg.find_first_of(" \t\"'\\$`") != std::string::npos;
    if (needs_quotes) {
      str += '"';
      for (const auto &cur_ch : arg) {
        if (cur_ch == '"' || cur_ch == '\\') {
          str += '\\';
        }
        str += cur_ch;
      }
      str += '"';
    } else {
      str += arg;
    }
  }

  return str;
}
#endif // defined(__linux__)
} // namespace

namespace repertory::utils {
#if !defined(__APPLE__)
auto convert_to_uint64(const pthread_t &thread) -> std::uint64_t {
  return static_cast<std::uint64_t>(thread);
}
#endif // !defined(__APPLE__)

#if defined(__linux__)
auto create_autostart_entry(const autostart_cfg &cfg, bool overwrite_existing)
    -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  auto file = desktop_file_path_for(cfg.app_name);
  if (utils::file::file{file}.exists() && not overwrite_existing) {
    return true;
  }

  auto dir = get_autostart_dir();
  if (dir.empty()) {
    return false;
  }

  if (not utils::file::directory{dir}.create_directory()) {
    return false;
  }

  auto exec_line = cfg.exec_path;
  if (not cfg.exec_args.empty()) {
    exec_line += ' ';
    exec_line += join_args_for_exec(cfg.exec_args);
  }

  std::ofstream out(file, std::ios::binary | std::ios::trunc);
  if (not out) {
    return false;
  }

  out << "[Desktop Entry]\n";
  out << "Type=Application\n";
  out << "Version=1.0\n";
  out << "Name=" << cfg.app_name << "\n";
  out << "Exec=" << exec_line << "\n";
  out << "Terminal=" << (cfg.terminal ? "true" : "false") << "\n";

  if (cfg.comment && not cfg.comment->empty()) {
    out << "Comment=" << *cfg.comment << "\n";
  }

  if (cfg.icon_path && not cfg.icon_path->empty()) {
    out << "Icon=" << *cfg.icon_path << "\n";
  }

  if (not cfg.only_show_in.empty()) {
    out << "OnlyShowIn=";
    for (std::size_t idx = 0U; idx < cfg.only_show_in.size(); ++idx) {
      if (idx != 0U) {
        out << ';';
      }
      out << cfg.only_show_in[idx];
    }
    out << ";\n";
  }

  if (not cfg.enabled) {
    out << "X-GNOME-Autostart-enabled=false\n";
    out << "Hidden=true\n";
  }

  out.flush();
  if (not out) {
    return false;
  }

#if defined(__linux__) || defined(__APPLE__)
  chmod(file.c_str(), 0644);
#endif // defined(__linux__) || defined(__APPLE__)

  return true;
}
#endif // defined(__linux__)

auto get_last_error_code() -> int { return errno; }

auto get_thread_id() -> std::uint64_t {
  return convert_to_uint64(pthread_self());
}

auto is_uid_member_of_group(uid_t uid, gid_t gid) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  std::vector<gid_t> groups{};
  auto res = use_getpwuid(
      uid, [&groups](struct passwd *pass) { groups = get_group_list(pass); });
  if (not res) {
    throw utils::error::create_exception(res.function_name,
                                         {"use_getpwuid failed", res.reason});
  }

  return collection::includes(groups, gid);
}

#if defined(__linux__)
auto remove_autostart_entry(std::string_view name) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  auto file = desktop_file_path_for(name);
  if (not utils::file::file{file}.exists()) {
    return true;
  }

  return utils::file::file{file}.remove();
}
#endif // defined(__linux__)

auto use_getpwuid(uid_t uid, passwd_callback_t callback) -> result {
  REPERTORY_USES_FUNCTION_NAME();

  static std::mutex mtx{};
  mutex_lock lock{mtx};

  auto *temp_pw = getpwuid(uid);
  if (temp_pw == nullptr) {
    return {
        .function_name = std::string{function_name},
        .ok = false,
        .reason = "'getpwuid' returned nullptr",
    };
  }

  callback(temp_pw);
  return {
      .function_name = std::string{function_name},
  };
}

#if defined(__APPLE__)
#if defined(PROJECT_ENABLE_PUGIXML)
auto generate_launchd_plist(const plist_cfg &cfg, bool overwrite_existing)
    -> bool {
  auto file = utils::path::combine(cfg.plist_path, {cfg.label + ".plist"});
  if (utils::file::file{file}.exists() && not overwrite_existing) {
    return true;
  }

  pugi::xml_document doc;
  auto decl = doc.append_child(pugi::node_declaration);
  decl.append_attribute("version") = "1.0";
  decl.append_attribute("encoding") = "UTF-8";

  auto plist = doc.append_child("plist");
  plist.append_attribute("version") = "1.0";

  auto dict = plist.append_child("dict");

  dict.append_child("key").text().set("Label");
  dict.append_child("string").text().set(cfg.label.c_str());

  dict.append_child("key").text().set("ProgramArguments");
  auto array = dict.append_child("array");
  for (const auto &arg : cfg.args) {
    array.append_child("string").text().set(arg.c_str());
  }

  dict.append_child("key").text().set("EnvironmentVariables");
  pugi::xml_node env_dict = dict.append_child("dict");
  if (not utils::get_environment_variable("PROJECT_TEST_CONFIG_DIR").empty()) {
    env_dict.append_child("key").text().set("PROJECT_TEST_CONFIG_DIR");
    env_dict.append_child("string").text().set(
        utils::get_environment_variable("PROJECT_TEST_CONFIG_DIR"));
  }
  if (not utils::get_environment_variable("PROJECT_TEST_INPUT_DIR").empty()) {
    env_dict.append_child("key").text().set("PROJECT_TEST_INPUT_DIR");
    env_dict.append_child("string").text().set(
        utils::get_environment_variable("PROJECT_TEST_INPUT_DIR"));
  }

  dict.append_child("key").text().set("WorkingDirectory");
  dict.append_child("string").text().set(cfg.working_dir.c_str());

  dict.append_child("key").text().set("KeepAlive");
  dict.append_child(cfg.keep_alive ? "true" : "false");

  dict.append_child("key").text().set("RunAtLoad");
  dict.append_child(cfg.run_at_load ? "true" : "false");

  dict.append_child("key").text().set("StandardOutPath");
  dict.append_child("string").text().set(cfg.stdout_log.c_str());

  dict.append_child("key").text().set("StandardErrorPath");
  dict.append_child("string").text().set(cfg.stderr_log.c_str());

  return doc.save_file(file.c_str(), "  ",
                       pugi::format_indent | pugi::format_write_bom);
}
#endif // defined(PROJECT_ENABLE_PUGIXML)

#if defined(PROJECT_ENABLE_SPDLOG) || defined(PROJECT_ENABLE_FMT)
auto launchctl_command(std::string_view label, launchctl_type type) -> int {
  switch (type) {
  case launchctl_type::bootout:
    return system(
        fmt::format("launchctl bootout gui/{} '{}' 1>/dev/null 2>&1", getuid(),
                    utils::path::combine("~",
                                         {
                                             "/Library/LaunchAgents",
                                             fmt::format("{}.plist", label),
                                         }))
            .c_str());

  case launchctl_type::bootstrap:
    return system(
        fmt::format("launchctl bootstrap gui/{} '{}' 1>/dev/null 2>&1",
                    getuid(),
                    utils::path::combine("~",
                                         {
                                             "/Library/LaunchAgents",
                                             fmt::format("{}.plist", label),
                                         }))
            .c_str());

  case launchctl_type::kickstart:
    return system(
        fmt::format("launchctl kickstart gui/{}/{}", getuid(), label).c_str());
  }

  return -1;
}

auto remove_launchd_plist(std::string_view plist_path, std::string_view label,
                          bool should_bootout) -> bool {
  auto file = utils::file::file{
      utils::path::combine(plist_path, {std::string{label} + ".plist"}),
  };
  if (not file.exists()) {
    return true;
  }

  auto res =
      should_bootout ? launchctl_command(label, launchctl_type::bootout) : 0;
  if (not file.remove()) {
    return false;
  }

  return res == 0;
}
#endif // defined(PROJECT_ENABLE_SPDLOG) || defined(PROJECT_ENABLE_FMT)
#endif // defined(__APPLE__)
} // namespace repertory::utils

#endif // !defined(_WIN32)
