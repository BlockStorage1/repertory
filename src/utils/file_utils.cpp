/*
  Copyright <2018-2023> <scott.e.graves@protonmail.com>

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
#include "utils/file_utils.hpp"

#include "types/repertory.hpp"
#include "utils/error_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace repertory::utils::file {
auto calculate_used_space(std::string path, bool recursive) -> std::uint64_t {
  path = utils::path::absolute(path);
  std::uint64_t ret = 0u;
#ifdef _WIN32
  WIN32_FIND_DATA fd = {0};
  const auto search = utils::path::combine(path, {"*.*"});
  auto find = ::FindFirstFile(search.c_str(), &fd);
  if (find != INVALID_HANDLE_VALUE) {
    do {
      const auto file_name = std::string(fd.cFileName);
      if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (recursive && (file_name != ".") && (file_name != "..")) {
          ret += calculate_used_space(utils::path::combine(path, {file_name}),
                                      recursive);
        }
      } else {
        std::uint64_t file_size{};
        if (get_file_size(utils::path::combine(path, {file_name}), file_size)) {
          ret += file_size;
        }
      }
    } while (::FindNextFile(find, &fd) != 0);
    ::FindClose(find);
  }
#else
  auto *root = opendir(path.c_str());
  if (root) {
    struct dirent *de{};
    while ((de = readdir(root)) != nullptr) {
      if (de->d_type == DT_DIR) {
        if (recursive && (strcmp(de->d_name, ".") != 0) &&
            (strcmp(de->d_name, "..") != 0)) {
          ret += calculate_used_space(utils::path::combine(path, {de->d_name}),
                                      recursive);
        }
      } else {
        std::uint64_t file_size{};
        if (get_file_size(utils::path::combine(path, {de->d_name}),
                          file_size)) {
          ret += file_size;
        }
      }
    }
    closedir(root);
  }
#endif

  return ret;
}

void change_to_process_directory() {
#ifdef _WIN32
  std::string file_name;
  file_name.resize(MAX_PATH);
  ::GetModuleFileNameA(nullptr, &file_name[0u],
                       static_cast<DWORD>(file_name.size()));

  std::string path = file_name.c_str();
  ::PathRemoveFileSpecA(&path[0u]);
  ::SetCurrentDirectoryA(&path[0u]);
#else
  std::string path;
  path.resize(PATH_MAX + 1);
#ifdef __APPLE__
  proc_pidpath(getpid(), &path[0u], path.size());
#else
  readlink("/proc/self/exe", &path[0u], path.size());
#endif
  path = utils::path::get_parent_directory(path);
  chdir(path.c_str());
#endif
}

auto copy_file(std::string from_path, std::string to_path) -> bool {
  from_path = utils::path::absolute(from_path);
  to_path = utils::path::absolute(to_path);
  if (is_file(from_path) && not is_directory(to_path)) {
    return std::filesystem::copy_file(from_path, to_path);
  }

  return false;
}

auto copy_directory_recursively(std::string from_path, std::string to_path)
    -> bool {
  from_path = utils::path::absolute(from_path);
  to_path = utils::path::absolute(to_path);
  auto ret = create_full_directory_path(to_path);
  if (ret) {
#ifdef _WIN32
    WIN32_FIND_DATA fd = {0};
    const auto search = utils::path::combine(from_path, {"*.*"});
    auto find = ::FindFirstFile(search.c_str(), &fd);
    if (find != INVALID_HANDLE_VALUE) {
      ret = true;
      do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          if ((std::string(fd.cFileName) != ".") &&
              (std::string(fd.cFileName) != "..")) {
            ret = copy_directory_recursively(
                utils::path::combine(from_path, {fd.cFileName}),
                utils::path::combine(to_path, {fd.cFileName}));
          }
        } else {
          ret = copy_file(utils::path::combine(from_path, {fd.cFileName}),
                          utils::path::combine(to_path, {fd.cFileName}));
        }
      } while (ret && (::FindNextFile(find, &fd) != 0));

      ::FindClose(find);
    }
#else
    auto *root = opendir(from_path.c_str());
    if (root) {
      struct dirent *de{};
      while (ret && (de = readdir(root))) {
        if (de->d_type == DT_DIR) {
          if ((strcmp(de->d_name, ".") != 0) &&
              (strcmp(de->d_name, "..") != 0)) {
            ret = copy_directory_recursively(
                utils::path::combine(from_path, {de->d_name}),
                utils::path::combine(to_path, {de->d_name}));
          }
        } else {
          ret = copy_file(utils::path::combine(from_path, {de->d_name}),
                          utils::path::combine(to_path, {de->d_name}));
        }
      }

      closedir(root);
    }
#endif
  }

  return ret;
}

auto create_full_directory_path(std::string path) -> bool {
#ifdef _WIN32
  const auto unicode_path =
      utils::string::from_utf8(utils::path::absolute(path));
  return is_directory(path) ||
         (::SHCreateDirectory(nullptr, unicode_path.c_str()) == ERROR_SUCCESS);
#else
  auto ret = true;
  const auto paths = utils::string::split(utils::path::absolute(path),
                                          utils::path::directory_seperator[0u]);
  std::string current_path;
  for (std::size_t i = 0u; ret && (i < paths.size()); i++) {
    if (paths[i].empty()) { // Skip root
      current_path = utils::path::directory_seperator;
    } else {
      current_path = utils::path::combine(current_path, {paths[i]});
      const auto status = mkdir(current_path.c_str(), S_IRWXU);
      ret = ((status == 0) || (errno == EEXIST));
    }
  }

  return ret;
#endif
}

auto delete_directory(std::string path, bool recursive) -> bool {
  if (recursive) {
    return delete_directory_recursively(path);
  }

  path = utils::path::absolute(path);
#ifdef _WIN32
  return (not is_directory(path) || utils::retryable_action([&]() -> bool {
    return !!::RemoveDirectoryA(path.c_str());
  }));
#else
  return not is_directory(path) || (rmdir(path.c_str()) == 0);
#endif
}

auto delete_directory_recursively(std::string path) -> bool {
  path = utils::path::absolute(path);
#ifdef _WIN32

  WIN32_FIND_DATA fd = {0};
  const auto search = utils::path::combine(path, {"*.*"});
  auto find = ::FindFirstFile(search.c_str(), &fd);
  if (find != INVALID_HANDLE_VALUE) {
    auto res = true;
    do {
      if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if ((std::string(fd.cFileName) != ".") &&
            (std::string(fd.cFileName) != "..")) {
          res = delete_directory_recursively(
              utils::path::combine(path, {fd.cFileName}));
        }
      } else {
        res = retry_delete_file(utils::path::combine(path, {fd.cFileName}));
      }
    } while (res && (::FindNextFile(find, &fd) != 0));

    ::FindClose(find);
  }
#else
  auto *root = opendir(path.c_str());
  if (root) {
    auto res = true;
    struct dirent *de{};
    while (res && (de = readdir(root))) {
      if (de->d_type == DT_DIR) {
        if ((strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..") != 0)) {
          res = delete_directory_recursively(
              utils::path::combine(path, {de->d_name}));
        }
      } else {
        res = retry_delete_file(utils::path::combine(path, {de->d_name}));
      }
    }

    closedir(root);
  }
#endif

  return delete_directory(path, false);
}

auto delete_file(std::string path) -> bool {
  path = utils::path::absolute(path);
#ifdef _WIN32
  return (not is_file(path) || utils::retryable_action([&]() -> bool {
    const auto ret = !!::DeleteFileA(path.c_str());
    if (not ret) {
      std::cout << "delete failed:" << path << std::endl;
    }

    return ret;
  }));
#else
  return (not is_file(path) || (unlink(path.c_str()) == 0));
#endif
}

auto generate_sha256(const std::string &file_path) -> std::string {
  crypto_hash_sha256_state state{};
  auto res = crypto_hash_sha256_init(&state);
  if (res != 0) {
    throw std::runtime_error("failed to initialize sha256|" +
                             std::to_string(res));
  }

  native_file_ptr nf;
  if (native_file::open(file_path, nf) != api_error::success) {
    throw std::runtime_error("failed to open file|" + file_path);
  }

  {
    data_buffer buffer(1048576u);
    std::uint64_t read_offset = 0u;
    std::size_t bytes_read = 0u;
    while (
        nf->read_bytes(buffer.data(), buffer.size(), read_offset, bytes_read)) {
      if (not bytes_read) {
        break;
      }

      read_offset += bytes_read;
      res = crypto_hash_sha256_update(
          &state, reinterpret_cast<const unsigned char *>(buffer.data()),
          bytes_read);
      if (res != 0) {
        nf->close();
        throw std::runtime_error("failed to update sha256|" +
                                 std::to_string(res));
      }
    }
    nf->close();
  }

  std::array<unsigned char, crypto_hash_sha256_BYTES> out{};
  res = crypto_hash_sha256_final(&state, out.data());
  if (res != 0) {
    throw std::runtime_error("failed to finalize sha256|" +
                             std::to_string(res));
  }

  return utils::to_hex_string(out);
}

auto get_free_drive_space(const std::string &path) -> std::uint64_t {
#ifdef _WIN32
  ULARGE_INTEGER li{};
  ::GetDiskFreeSpaceEx(path.c_str(), &li, nullptr, nullptr);
  return li.QuadPart;
#endif
#ifdef __linux__
  std::uint64_t ret = 0;
  struct statfs64 st {};
  if (statfs64(path.c_str(), &st) == 0) {
    ret = st.f_bfree * st.f_bsize;
  }
  return ret;
#endif
#if __APPLE__
  struct statvfs st {};
  statvfs(path.c_str(), &st);
  return st.f_bfree * st.f_frsize;
#endif
}

auto get_total_drive_space(const std::string &path) -> std::uint64_t {
#ifdef _WIN32
  ULARGE_INTEGER li{};
  ::GetDiskFreeSpaceEx(path.c_str(), nullptr, &li, nullptr);
  return li.QuadPart;
#endif
#ifdef __linux__
  std::uint64_t ret = 0;
  struct statfs64 st {};
  if (statfs64(path.c_str(), &st) == 0) {
    ret = st.f_blocks * st.f_bsize;
  }
  return ret;
#endif
#if __APPLE__
  struct statvfs st {};
  statvfs(path.c_str(), &st);
  return st.f_blocks * st.f_frsize;
#endif
}

auto get_directory_files(std::string path, bool oldest_first, bool recursive)
    -> std::deque<std::string> {
  path = utils::path::absolute(path);
  std::deque<std::string> ret;
  std::unordered_map<std::string, std::uint64_t> lookup;
#ifdef _WIN32
  WIN32_FIND_DATA fd = {0};
  const auto search = utils::path::combine(path, {"*.*"});
  auto find = ::FindFirstFile(search.c_str(), &fd);
  if (find != INVALID_HANDLE_VALUE) {
    do {
      const auto full_path = utils::path::combine(path, {fd.cFileName});
      if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
          FILE_ATTRIBUTE_DIRECTORY) {
        if (recursive) {
          const auto sub_files =
              get_directory_files(full_path, oldest_first, recursive);
          ret.insert(ret.end(), sub_files.begin(), sub_files.end());
        } else {
          ULARGE_INTEGER li{};
          li.HighPart = fd.ftLastWriteTime.dwHighDateTime;
          li.LowPart = fd.ftLastWriteTime.dwLowDateTime;
          lookup[full_path] = li.QuadPart;
          ret.emplace_back(full_path);
        }
      }
    } while (::FindNextFile(find, &fd) != 0);
    ::FindClose(find);

    std::sort(ret.begin(), ret.end(),
              [&](const auto &p1, const auto &p2) -> bool {
                return (oldest_first != 0) == (lookup[p1] < lookup[p2]);
              });
  }
#else
  auto *root = opendir(path.c_str());
  if (root) {
    struct dirent *de{};
    while ((de = readdir(root)) != nullptr) {
      if (de->d_type == DT_DIR) {
        if (recursive) {
          const auto sub_files =
              get_directory_files(utils::path::combine(path, {de->d_name}),
                                  oldest_first, recursive);
          ret.insert(ret.end(), sub_files.begin(), sub_files.end());
        }
      } else {
        ret.emplace_back(utils::path::combine(path, {de->d_name}));
      }
    }
    closedir(root);

    const auto add_to_lookup = [&](const std::string &lookup_path) {
      if (lookup.find(lookup_path) == lookup.end()) {
        struct stat st {};
        stat(lookup_path.c_str(), &st);
#ifdef __APPLE__
        lookup[lookup_path] = static_cast<std::uint64_t>(
            (st.st_mtimespec.tv_sec * NANOS_PER_SECOND) +
            st.st_mtimespec.tv_nsec);
#else
        lookup[lookup_path] = static_cast<std::uint64_t>(
            (st.st_mtim.tv_sec * NANOS_PER_SECOND) + st.st_mtim.tv_nsec);
#endif
      }
    };

    std::sort(ret.begin(), ret.end(),
              [&](const auto &p1, const auto &p2) -> bool {
                add_to_lookup(p1);
                add_to_lookup(p2);
                return (oldest_first != 0) == (lookup[p1] < lookup[p2]);
              });
  }
#endif
  return ret;
}

auto get_accessed_time(const std::string &path, std::uint64_t &accessed)
    -> bool {
  auto ret = false;
  accessed = 0;
#ifdef _WIN32
  struct _stat64 st {};
  if (_stat64(path.c_str(), &st) != -1) {
    accessed = static_cast<uint64_t>(st.st_atime);
#else
  struct stat st {};
  if (stat(path.c_str(), &st) != -1) {
#ifdef __APPLE__
    accessed = static_cast<uint64_t>(
        st.st_atimespec.tv_nsec + (st.st_atimespec.tv_sec * NANOS_PER_SECOND));
#else
    accessed = static_cast<uint64_t>(st.st_atim.tv_nsec +
                                     (st.st_atim.tv_sec * NANOS_PER_SECOND));
#endif
#endif
    ret = true;
  }

  return ret;
}

auto get_modified_time(const std::string &path, std::uint64_t &modified)
    -> bool {
  auto ret = false;
  modified = 0u;
#ifdef _WIN32
  struct _stat64 st {};
  if (_stat64(path.c_str(), &st) != -1) {
    modified = static_cast<uint64_t>(st.st_mtime);
#else
  struct stat st {};
  if (stat(path.c_str(), &st) != -1) {
#ifdef __APPLE__
    modified = static_cast<uint64_t>(
        st.st_mtimespec.tv_nsec + (st.st_mtimespec.tv_sec * NANOS_PER_SECOND));
#else
    modified = static_cast<uint64_t>(st.st_mtim.tv_nsec +
                                     (st.st_mtim.tv_sec * NANOS_PER_SECOND));
#endif
#endif
    ret = true;
  }

  return ret;
}

auto get_file_size(std::string path, std::uint64_t &file_size) -> bool {
  file_size = 0u;
  path = utils::path::finalize(path);

#ifdef _WIN32
  struct _stat64 st {};
  if (_stat64(path.c_str(), &st) != 0) {
#else
#if __APPLE__
  struct stat st {};
  if (stat(path.c_str(), &st) != 0) {
#else
  struct stat64 st {};
  if (stat64(path.c_str(), &st) != 0) {
#endif
#endif
    return false;
  }

  if (st.st_size >= 0) {
    file_size = static_cast<std::uint64_t>(st.st_size);
  }

  return (st.st_size >= 0);
}

auto is_directory(const std::string &path) -> bool {
#ifdef _WIN32
  return ::PathIsDirectory(path.c_str()) != 0;
#else
  struct stat st {};
  return (not stat(path.c_str(), &st) && S_ISDIR(st.st_mode));
#endif
}

auto is_file(const std::string &path) -> bool {
#ifdef _WIN32
  return (::PathFileExists(path.c_str()) &&
          not ::PathIsDirectory(path.c_str()));
#else
  struct stat st {};
  return (not stat(path.c_str(), &st) && not S_ISDIR(st.st_mode));
#endif
}

auto is_modified_date_older_than(const std::string &path,
                                 const std::chrono::hours &hours) -> bool {
  auto ret = false;
  std::uint64_t modified = 0;
  if (get_modified_time(path, modified)) {
    const auto seconds =
        std::chrono::duration_cast<std::chrono::seconds>(hours);
#ifdef _WIN32
    return (std::chrono::system_clock::from_time_t(modified) + seconds) <
           std::chrono::system_clock::now();
#else
    return (modified +
            static_cast<std::uint64_t>(seconds.count() * NANOS_PER_SECOND)) <
           utils::get_time_now();
#endif
  }
  return ret;
}

auto move_file(std::string from, std::string to) -> bool {
  from = utils::path::finalize(from);
  to = utils::path::finalize(to);

  const auto directory = utils::path::remove_file_name(to);
  if (not create_full_directory_path(directory)) {
    return false;
  }

#ifdef _WIN32
  const bool ret = ::MoveFile(from.c_str(), to.c_str()) != 0;
#else
  const bool ret = (rename(from.c_str(), to.c_str()) == 0);
#endif

  return ret;
}

auto read_file_lines(const std::string &path) -> std::vector<std::string> {
  std::vector<std::string> ret;
  if (is_file(path)) {
    std::ifstream fs(path);
    std::string current_line;
    while (not fs.eof() && std::getline(fs, current_line)) {
      ret.emplace_back(utils::string::right_trim(current_line, '\r'));
    }
    fs.close();
  }

  return ret;
}

auto read_json_file(const std::string &path, json &data) -> bool {
  if (not utils::file::is_file(path)) {
    return true;
  }

  try {
    std::ifstream file_stream(path.c_str());
    if (file_stream.is_open()) {
      try {
        std::stringstream ss;
        ss << file_stream.rdbuf();

        std::string json_text = ss.str();
        if (not json_text.empty()) {
          data = json::parse(json_text.c_str());
        }

        file_stream.close();
        return true;
      } catch (const std::exception &e) {
        file_stream.close();
        throw e;
      }
    }
  } catch (const std::exception &e) {
    utils::error::raise_error(__FUNCTION__, e, path,
                              "failed to read json file");
  }

  return false;
}

auto reset_modified_time(const std::string &path) -> bool {
  auto ret = false;
#ifdef _WIN32
  SYSTEMTIME st{};
  ::GetSystemTime(&st);

  FILETIME ft{};
  if ((ret = !!::SystemTimeToFileTime(&st, &ft))) {
    auto handle = ::CreateFileA(
        path.c_str(), FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
        OPEN_EXISTING, 0, nullptr);
    if ((ret = (handle != INVALID_HANDLE_VALUE))) {
      ret = !!::SetFileTime(handle, nullptr, &ft, &ft);
      ::CloseHandle(handle);
    }
  }
#else
  auto fd = open(path.c_str(), O_RDWR);
  if ((ret = (fd != -1))) {
    ret = not futimens(fd, nullptr);
    close(fd);
  }
#endif
  return ret;
}

auto retry_delete_directory(const std::string &dir) -> bool {
  auto deleted = false;
  for (std::uint8_t i = 0u; not(deleted = delete_directory(dir)) && (i < 200u);
       i++) {
    std::this_thread::sleep_for(10ms);
  }

  return deleted;
}

auto retry_delete_file(const std::string &file) -> bool {
  auto deleted = false;
  for (std::uint8_t i = 0u; not(deleted = delete_file(file)) && (i < 200u);
       i++) {
    std::this_thread::sleep_for(10ms);
  }

  return deleted;
}

auto write_json_file(const std::string &path, const json &j) -> bool {
  std::string data;
  {
    std::stringstream ss;
    ss << std::setw(2) << j;
    data = ss.str();
  }
  native_file_ptr nf;
  auto ret = (native_file::create_or_open(path, nf) == api_error::success);
  if (ret) {
    std::size_t bytes_written = 0u;
    ret = nf->truncate(0) &&
          nf->write_bytes(&data[0u], data.size(), 0u, bytes_written);
    nf->close();
  }

  return ret;
}
} // namespace repertory::utils::file
