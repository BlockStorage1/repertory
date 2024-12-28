/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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
#include "utils/file.hpp"

#include "utils/common.hpp"
#include "utils/encryption.hpp"
#include "utils/error.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#include "utils/unix.hpp"
#include "utils/windows.hpp"

namespace repertory::utils::file {
auto change_to_process_directory() -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
#if defined(_WIN32)
    std::string file_name;
    file_name.resize(repertory::max_path_length + 1U);

    ::GetModuleFileNameA(nullptr, file_name.data(),
                         static_cast<DWORD>(file_name.size() - 1U));
    auto path = utils::path::strip_to_file_name(file_name.c_str());
    ::SetCurrentDirectoryA(path.c_str());
#else // !defined(_WIN32)
    std::string path;
    path.resize(PATH_MAX + 1);
#if defined(__APPLE__)
    proc_pidpath(getpid(), path.c_str(), path.size());
#else  // !defined(__APPLE__)
    auto res = readlink("/proc/self/exe", path.data(), path.size());
    if (res == -1) {
      throw utils::error::create_exception(
          function_name, {
                             "failed to readlink",
                             std::to_string(utils::get_last_error_code()),
                             path,
                         });
    }
#endif // defined(__APPLE__)
    path = utils::path::get_parent_path(path);
    res = chdir(path.c_str());
    if (res != 0) {
      throw utils::error::create_exception(
          function_name, {
                             "failed to chdir",
                             std::to_string(utils::get_last_error_code()),
                             path,
                         });
    }
#endif // defined(_WIN32)

    return true;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto create_temp_name(std::string_view file_part) -> std::string {
  std::array<std::uint8_t, 8U> data{
      utils::generate_random_between<std::uint8_t>(0U, 9U),
      utils::generate_random_between<std::uint8_t>(0U, 9U),
      utils::generate_random_between<std::uint8_t>(0U, 9U),
      utils::generate_random_between<std::uint8_t>(0U, 9U),
      utils::generate_random_between<std::uint8_t>(0U, 9U),
      utils::generate_random_between<std::uint8_t>(0U, 9U),
      utils::generate_random_between<std::uint8_t>(0U, 9U),
      utils::generate_random_between<std::uint8_t>(0U, 9U),
  };

  return std::accumulate(data.begin(), data.end(), std::string{file_part} + '_',
                         [](auto &&name, auto &&val) -> auto {
                           return name + std::to_string(val);
                         });
}

auto create_temp_name(std::wstring_view file_part) -> std::wstring {
  return utils::string::from_utf8(
      create_temp_name(utils::string::to_utf8(file_part)));
}

auto get_free_drive_space(std::string_view path)
    -> std::optional<std::uint64_t> {
  REPERTORY_USES_FUNCTION_NAME();

  try {
#if defined(_WIN32)
    ULARGE_INTEGER li{};
    if (not::GetDiskFreeSpaceEx(std::string{path}.c_str(), &li, nullptr,
                                nullptr)) {
      throw utils::error::create_exception(
          function_name, {
                             "failed to get free disk space",
                             std::to_string(utils::get_last_error_code()),
                             path,
                         });
    }

    return li.QuadPart;
#endif // defined(_WIN32)

#if defined(__linux__)
    struct statfs64 st{};
    if (statfs64(std::string{path}.c_str(), &st) != 0) {
      throw utils::error::create_exception(
          function_name, {
                             "failed to get free disk space",
                             std::to_string(utils::get_last_error_code()),
                             path,
                         });
    }

    return st.f_bfree * static_cast<std::uint64_t>(st.f_bsize);
#endif // defined(__linux__)

#if defined(__APPLE__)
    struct statvfs st{};
    if (statvfs(path.c_str(), &st) != 0) {
      throw utils::error::create_exception(
          function_name, {
                             "failed to get free disk space",
                             std::to_string(utils::get_last_error_code()),
                             path,
                         });
    }

    return st.f_bfree * static_cast<std::uint64_t>(st.f_frsize);
#endif // defined(__APPLE__)
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return std::nullopt;
}

auto get_free_drive_space(std::wstring_view path)
    -> std::optional<std::uint64_t> {
  return get_free_drive_space(utils::string::to_utf8(path));
}

auto get_time(std::string_view path, time_type type)
    -> std::optional<std::uint64_t> {
  auto times = get_times(path);
  if (times.has_value()) {
    return times->get(type);
  }

  return std::nullopt;
}

auto get_time(std::wstring_view path, time_type type)
    -> std::optional<std::uint64_t> {
  return get_time(utils::string::to_utf8(path), type);
}

auto get_times(std::string_view path) -> std::optional<file_times> {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    file_times ret{};

#if defined(_WIN32)
    auto file_handle = ::CreateFileA(
        std::string{path}.c_str(), GENERIC_READ,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (file_handle != INVALID_HANDLE_VALUE) {
      std::array<FILETIME, 3U> times{};
      auto res = ::GetFileTime(file_handle, &times.at(0U), &times.at(1U),
                               &times.at(2U));
      ::CloseHandle(file_handle);
      if (res) {
        ret.accessed =
            utils::time::windows_file_time_to_unix_time(times.at(1U));
        ret.created = utils::time::windows_file_time_to_unix_time(times.at(0U));
        ret.modified =
            utils::time::windows_file_time_to_unix_time(times.at(2U));
        ret.written = utils::time::windows_file_time_to_unix_time(times.at(2U));
        return ret;
      }
    }

    struct _stat64 st{};
    if (_stat64(std::string{path}.c_str(), &st) != 0) {
      throw utils::error::create_exception(
          function_name, {
                             "failed to get file times",
                             std::to_string(utils::get_last_error_code()),
                             path,
                         });
    }

    ret.accessed = utils::time::windows_time_t_to_unix_time(st.st_atime);
    ret.created = utils::time::windows_time_t_to_unix_time(st.st_ctime);
    ret.modified = utils::time::windows_time_t_to_unix_time(st.st_mtime);
    ret.written = utils::time::windows_time_t_to_unix_time(st.st_mtime);
#else // !defined(_WIN32)
    struct stat64 st{};
    if (stat64(std::string{path}.c_str(), &st) != 0) {
      throw utils::error::create_exception(
          function_name, {
                             "failed to get file times",
                             std::to_string(utils::get_last_error_code()),
                             path,
                         });
    }

    ret.accessed = static_cast<std::uint64_t>(st.st_atim.tv_nsec) +
                   static_cast<std::uint64_t>(st.st_atim.tv_sec) *
                       utils::time::NANOS_PER_SECOND;
    ret.created = static_cast<std::uint64_t>(st.st_ctim.tv_nsec) +
                  static_cast<std::uint64_t>(st.st_ctim.tv_sec) *
                      utils::time::NANOS_PER_SECOND;
    ret.modified = static_cast<std::uint64_t>(st.st_mtim.tv_nsec) +
                   static_cast<std::uint64_t>(st.st_mtim.tv_sec) *
                       utils::time::NANOS_PER_SECOND;
    ret.written = static_cast<std::uint64_t>(st.st_mtim.tv_nsec) +
                  static_cast<std::uint64_t>(st.st_mtim.tv_sec) *
                      utils::time::NANOS_PER_SECOND;

#endif // defined(_WIN32)

    return ret;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return std::nullopt;
}

auto get_times(std::wstring_view path) -> std::optional<file_times> {
  return get_times(utils::string::to_utf8(path));
}

auto get_total_drive_space(std::string_view path)
    -> std::optional<std::uint64_t> {
  REPERTORY_USES_FUNCTION_NAME();

  try {
#if defined(_WIN32)
    ULARGE_INTEGER li{};
    if (not::GetDiskFreeSpaceEx(std::string{path}.c_str(), nullptr, &li,
                                nullptr)) {
      throw utils::error::create_exception(
          function_name, {
                             "failed to get total disk space",
                             std::to_string(utils::get_last_error_code()),
                             path,
                         });
    }

    return li.QuadPart;
#endif // defined(_WIN32)

#if defined(__linux__)
    struct statfs64 st{};
    if (statfs64(std::string{path}.c_str(), &st) != 0) {
      throw utils::error::create_exception(
          function_name, {
                             "failed to get total disk space",
                             std::to_string(utils::get_last_error_code()),
                             path,
                         });
    }

    return st.f_blocks * static_cast<std::uint64_t>(st.f_bsize);
#endif // defined(__linux__)

#if defined(__APPLE__)
    struct statvfs st{};
    if (statvfs(path.c_str(), &st) != 0) {
      throw utils::error::create_exception(
          function_name, {
                             "failed to get total disk space",
                             std::to_string(utils::get_last_error_code()),
                             path,
                         });
    }

    return st.f_blocks * static_cast<std::uint64_t>(st.f_frsize);
#endif // defined(__APPLE__)
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return std::nullopt;
}

auto get_total_drive_space(std::wstring_view path)
    -> std::optional<std::uint64_t> {
  return get_total_drive_space(utils::string::to_utf8(path));
}

auto i_fs_item::get_time(time_type type) const -> std::optional<std::uint64_t> {
  return utils::file::get_time(get_path(), type);
}

auto i_file::read_all(data_buffer &data, std::uint64_t offset,
                      std::size_t *total_read) -> bool {
  data_buffer buffer;
  buffer.resize(get_read_buffer_size());

  std::size_t current_read{};
  while (read(reinterpret_cast<unsigned char *>(buffer.data()),
              buffer.size() * sizeof(data_buffer::value_type), offset,
              &current_read)) {
    if (total_read != nullptr) {
      *total_read += current_read;
    }

    if (current_read != 0U) {
      offset += current_read;

      data.insert(
          data.end(), buffer.begin(),
          std::next(buffer.begin(),
                    static_cast<std::int64_t>(
                        current_read / sizeof(data_buffer::value_type))));
      continue;
    }

    return true;
  }

  return false;
}

#if defined(PROJECT_ENABLE_JSON)
#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
auto read_json_file(std::string_view path, nlohmann::json &data,
                    std::optional<std::string_view> password) -> bool {
#else  // !defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
auto read_json_file(std::string_view path, nlohmann::json &data) -> bool {
#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
  REPERTORY_USES_FUNCTION_NAME();

  try {
    auto abs_path = utils::path::absolute(path);
    auto file = file::open_file(abs_path);
    if (not*file) {
      return false;
    }

    try {
      data_buffer buffer{};
      if (not file->read_all(buffer, 0U)) {
        return false;
      }

#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
      if (password.has_value()) {
        data_buffer decrypted_data{};
        if (not utils::encryption::decrypt_data(*password, buffer,
                                                decrypted_data)) {
          return false;
        }

        buffer = decrypted_data;
      }
#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)

      std::string json_str(buffer.begin(), buffer.end());
      if (not json_str.empty()) {
        data = nlohmann::json::parse(json_str);
      }
    } catch (const std::exception &e) {
      utils::error::handle_exception(function_name, e);
      return false;
    } catch (...) {
      utils::error::handle_exception(function_name);
      return false;
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
auto write_json_file(std::string_view path, const nlohmann::json &data,
                     std::optional<std::string_view> password) -> bool {
#else  // !defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
auto write_json_file(std::string_view path, const nlohmann::json &data)
    -> bool {
#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
  REPERTORY_USES_FUNCTION_NAME();

  try {
    auto file = file::open_or_create_file(path);
    if (not file->truncate()) {
      throw utils::error::create_exception(
          function_name, {
                             "failed to truncate file",
                             std::to_string(utils::get_last_error_code()),
                             path,
                         });
    }

#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
    if (password.has_value()) {
      const auto str_data = data.dump(2);

      data_buffer encrypted_data{};
      utils::encryption::encrypt_data(
          *password, reinterpret_cast<const unsigned char *>(str_data.c_str()),
          str_data.size(), encrypted_data);
      return file->write(encrypted_data, 0U);
    }
#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)

    auto json_str = data.dump(2);
    return file->write(
        reinterpret_cast<const unsigned char *>(json_str.c_str()),
        json_str.size(), 0U);
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
auto read_json_file(std::wstring_view path, nlohmann::json &data,
                    std::optional<std::wstring_view> password) -> bool {
  if (password.has_value()) {
    auto password_a = utils::string::to_utf8(*password);
    return read_json_file(utils::string::to_utf8(path), data, password_a);
  }

  return read_json_file(utils::string::to_utf8(path), data, std::nullopt);
}

auto write_json_file(std::wstring_view path, const nlohmann::json &data,
                     std::optional<std::wstring_view> password) -> bool {
  if (password.has_value()) {
    auto password_a = utils::string::to_utf8(*password);
    return write_json_file(utils::string::to_utf8(path), data, password_a);
  }

  return write_json_file(utils::string::to_utf8(path), data, std::nullopt);
}
#else  // !defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
auto read_json_file(std::wstring_view path, nlohmann::json &data) -> bool {
  return read_json_file(utils::string::to_utf8(path), data);
}

auto write_json_file(std::wstring_view path, const nlohmann::json &data)
    -> bool {
  return write_json_file(utils::string::to_utf8(path), data);
}
#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
#endif // defined(PROJECT_ENABLE_JSON)

#if defined(PROJECT_ENABLE_LIBDSM)
static constexpr const auto validate_smb_path =
    [](std::string_view path) -> bool {
  return (not utils::string::begins_with(path, "///") &&
          utils::string::begins_with(path, "//") &&
          // not utils::string::contains(path, " ") &&
          std::count(path.begin(), path.end(), '/') >= 3U);
};

auto smb_create_smb_path(std::string_view smb_path, std::string_view rel_path)
    -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

  if (not validate_smb_path(smb_path)) {
    throw utils::error::create_exception(function_name, {
                                                            "invalid smb path",
                                                            smb_path,
                                                        });
  }

  std::string path{rel_path};
  utils::path::format_path(path, "/", "\\");
  utils::string::left_trim(path, '/');

  auto old_parts =
      repertory::utils::string::split(smb_path.substr(2U), '/', false);

  auto new_parts = repertory::utils::string::split(path, '/', false);
  old_parts.insert(old_parts.end(), new_parts.begin(), new_parts.end());

  path = utils::string::join(old_parts, '/');
  path = "//" + utils::path::format_path(path, "/", "\\");

  if (not validate_smb_path(path)) {
    throw utils::error::create_exception(function_name, {
                                                            "invalid smb path",
                                                            path,
                                                        });
  }

  return path;
}

auto smb_create_and_validate_relative_path(std::string_view smb_path,
                                           std::string_view path)
    -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

  if (not validate_smb_path(smb_path)) {
    throw utils::error::create_exception(function_name, {
                                                            "invalid smb path",
                                                            smb_path,
                                                        });
  }

  std::string dir_path;
  if (utils::string::begins_with(path, "//")) {
    if (not utils::file::smb_parent_is_same(smb_path, path)) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to validate path",
                                               "parent paths are not the same",
                                               smb_path,
                                               path,
                                           });
    }

    return utils::file::smb_create_relative_path(path);
  }

  return utils::file::smb_create_relative_path(std::string{smb_path} + '/' +
                                               std::string{path});
}

auto smb_create_relative_path(std::string_view smb_path) -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

  if (not validate_smb_path(smb_path)) {
    throw utils::error::create_exception(function_name, {
                                                            "invalid smb path",
                                                            smb_path,
                                                        });
  }

  std::string path{smb_path};
  utils::path::format_path(path, "\\", "/");
  utils::string::left_trim(path, '\\');

  auto parts = repertory::utils::string::split(path, '\\', false);
  parts.erase(parts.begin(), std::next(parts.begin(), 2U));
  return "\\" + utils::string::join(parts, '\\');
}

auto smb_create_search_path(std::string_view smb_path) -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

  if (not validate_smb_path(smb_path)) {
    throw utils::error::create_exception(function_name, {
                                                            "invalid smb path",
                                                            smb_path,
                                                        });
  }

  std::string path{smb_path};
  utils::string::left_trim(path, '/');

  auto parts = repertory::utils::string::split(path, '/', false);
  parts.erase(parts.begin(), std::next(parts.begin(), 2U));

  auto search_path = repertory::utils::string::join(parts, '\\');
  return search_path.empty() ? "\\*" : "\\" + search_path + "\\*";
}

auto smb_get_parent_path(std::string_view smb_path) -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

  if (not validate_smb_path(smb_path)) {
    throw utils::error::create_exception(function_name, {
                                                            "invalid smb path",
                                                            smb_path,
                                                        });
  }

  auto parts = repertory::utils::string::split(smb_path.substr(2U), '/', false);
  if (parts.size() > 2U) {
    parts.erase(std::prev(parts.end()), parts.end());
  }

  auto parent_smb_path = "//" + utils::string::join(parts, '/');
  if (not validate_smb_path(parent_smb_path)) {
    throw utils::error::create_exception(function_name,
                                         {
                                             "invalid parent smb path",
                                             parent_smb_path,
                                         });
  }

  return parent_smb_path;
}

auto smb_get_root_path(std::string_view smb_path) -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

  if (not validate_smb_path(smb_path)) {
    throw utils::error::create_exception(function_name, {
                                                            "invalid smb path",
                                                            smb_path,
                                                        });
  }

  auto parts = repertory::utils::string::split(smb_path.substr(2U), '/', false);
  if (parts.size() > 2U) {
    parts.erase(std::next(parts.begin(), 2U), parts.end());
  }

  return "//" + utils::string::join(parts, '/');
}

auto smb_get_unc_path(std::string_view smb_path) -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

  if (not validate_smb_path(smb_path)) {
    throw utils::error::create_exception(function_name, {
                                                            "invalid smb path",
                                                            smb_path,
                                                        });
  }

  std::string unc_path{smb_path};
  utils::path::format_path(unc_path, "\\", "/");
  return '\\' + unc_path;
}

auto smb_get_uri_path(std::string_view smb_path) -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

  if (not validate_smb_path(smb_path)) {
    throw utils::error::create_exception(function_name, {
                                                            "invalid smb path",
                                                            smb_path,
                                                        });
  }

  return "smb:" + std::string{smb_path};
}

auto smb_get_uri_path(std::string_view smb_path, std::string_view user,
                      std::string_view password) -> std::string {
  REPERTORY_USES_FUNCTION_NAME();

  if (not validate_smb_path(smb_path)) {
    throw utils::error::create_exception(function_name, {
                                                            "invalid smb path",
                                                            smb_path,
                                                        });
  }

  return "smb://" + std::string{user} + ':' + std::string{password} + '@' +
         std::string{smb_path.substr(2U)};
}

auto smb_parent_is_same(std::string_view smb_path1, std::string_view smb_path2)
    -> bool {
  if (not(validate_smb_path(smb_path1) && validate_smb_path(smb_path2))) {
    return false;
  }

  auto parts1 = utils::string::split(smb_path1.substr(2U), "/", false);
  auto parts2 = utils::string::split(smb_path2.substr(2U), "/", false);
  if (parts1.size() < 2U || parts2.size() < 2U) {
    return false;
  }

  if (parts2.at(1U).empty() || parts1.at(1U).empty()) {
    return false;
  }

  return std::equal(parts1.begin(), std::next(parts1.begin(), 2U),
                    parts2.begin());
}
#endif // defined(PROJECT_ENABLE_LIBDSM)
} // namespace repertory::utils::file
