/*
  Copyright <2018-2022> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
  OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "utils/path_utils.hpp"
#include "types/repertory.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace repertory::utils::path {
static std::string &format_path(std::string &path, const std::string &sep,
                                const std::string &not_sep);

std::string absolute(std::string path) {
#ifdef _WIN32
  if (not path.empty() && ::PathIsRelative(&path[0u])) {
    std::string temp;
    temp.resize(MAX_PATH + 1);
    path = _fullpath(&temp[0u], &path[0u], MAX_PATH);
  }
#else
  if (not path.empty() && (path[0u] != '/')) {
    auto found = false;
    auto tmp = path;
    do {
      auto *res = realpath(&tmp[0u], nullptr);
      if (res) {
        path = combine(res, {path.substr(tmp.size())});
        free(res);
        found = true;
      } else if (tmp == ".") {
        found = true;
      } else {
        tmp = dirname(&tmp[0u]);
      }
    } while (not found);
  }
#endif

  return path;
}

std::string combine(std::string path, const std::vector<std::string> &paths) {
  return finalize(
      std::accumulate(paths.begin(), paths.end(), path, [](std::string path, const auto &pathPart) {
        path += (directory_seperator + pathPart);
        return path;
      }));
}

std::string create_api_path(std::string path) {
  if (path.empty() || (path == ".")) {
    path = "/";
  } else {
    format_path(path, "/", "\\");
    if (path.find("./") == 0) {
      path = path.substr(1);
    }
    if (path[0u] != '/') {
      path = "/" + path;
    }
  }

  return path;
}

static std::string &format_path(std::string &path, const std::string &sep,
                                const std::string &not_sep) {
  std::replace(path.begin(), path.end(), not_sep[0u], sep[0u]);

  while (utils::string::contains(path, sep + sep)) {
    utils::string::replace(path, sep + sep, sep);
  }

  return path;
}

std::string finalize(std::string path) {
  format_path(path, directory_seperator, not_directory_seperator);
  if ((path.size() > 1) && (path[path.size() - 1] == directory_seperator[0u])) {
    path = path.substr(0, path.size() - 1);
  }

  return path;
}

std::string get_parent_api_path(const std::string &path) {
  std::string ret;
  if (path != "/") {
    ret = path.substr(0, path.rfind('/') + 1);
    if (ret != "/") {
      ret = utils::string::right_trim(ret, '/');
    }
  }

  return ret;
}

#ifndef _WIN32
std::string get_parent_directory(std::string path) {
  auto ret = std::string(dirname(&path[0u]));
  if (ret == ".") {
    ret = "/";
  }

  return ret;
}
#endif

bool is_ads_file_path(const std::string &path) {
#ifdef _WIN32
  return utils::string::contains(path, ":");
#else
  return false;
#endif
}

bool is_trash_directory(std::string path) {
  path = create_api_path(utils::string::to_lower(path));
  if (utils::string::begins_with(path, "/.trash-") ||
      utils::string::begins_with(path, "/.trashes") ||
      utils::string::begins_with(path, "/$recycle.bin")) {
    return true;
  }
  return false;
}

std::string remove_file_name(std::string path) {
  path = finalize(path);

#ifdef _WIN32
  ::PathRemoveFileSpec(&path[0u]);
  path = path.c_str();
#else
  if (path != "/") {
    auto i = path.size() - 1;
    while ((i != 0) && (path[i] != '/')) {
      i--;
    }

    path = (i > 0) ? finalize(path.substr(0, i)) : "/";
  }
#endif

  return path;
}

#ifndef _WIN32
std::string resolve(std::string path) {
  struct passwd *pw = getpwuid(getuid());
  std::string home = (pw->pw_dir ? pw->pw_dir : "");
  if (home.empty() || ((home == "/") && (getuid() != 0))) {
    home = combine("/home", {pw->pw_name});
  }

  return finalize(utils::string::replace(path, "~", home));
}
#endif

std::string strip_to_file_name(std::string path) {
#ifdef _WIN32
  return ::PathFindFileName(&path[0u]);
#else
  return utils::string::contains(path, "/") ? basename(&path[0u]) : path;
#endif
}
} // namespace repertory::utils::path
