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
#if defined(PROJECT_ENABLE_LIBDSM)

#include "utils/file_smb_directory.hpp"

#include "utils/common.hpp"
#include "utils/error.hpp"
#include "utils/unix.hpp"
#include "utils/windows.hpp"

namespace repertory::utils::file {
auto smb_directory::open(std::string_view host, std::string_view user,
                         std::string_view password, std::string_view path,
                         stop_type *stop_requested) -> smb_directory_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    smb_session_t session{
        smb_session_new(),
        smb_session_deleter,
    };
    netbios_ns_t ns{
        netbios_ns_new(),
        netbios_ns_deleter(),
    };

    sockaddr_in addr{};

    auto res = netbios_ns_resolve(
        ns.get(), std::string{host}.c_str(), NETBIOS_FILESERVER,
        reinterpret_cast<std::uint32_t *>(&addr.sin_addr.s_addr));
    if (res != DSM_SUCCESS) {
      res = inet_pton(AF_INET, std::string{host}.c_str(), &addr.sin_addr);
      if (res != 1) {
        throw utils::error::create_exception(
            function_name, {
                               "failed to resolve host",
                               std::to_string(utils::get_last_error_code()),
                               host,
                           });
      }
    }

    res = smb_session_connect(session.get(), std::string{host}.c_str(),
                              static_cast<std::uint32_t>(addr.sin_addr.s_addr),
                              SMB_TRANSPORT_TCP);
    if (res != DSM_SUCCESS) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to connect to host",
                                               std::to_string(res),
                                               host,
                                           });
    }

    smb_session_set_creds(session.get(), std::string{host}.c_str(),
                          std::string{user}.c_str(),
                          std::string{password}.c_str());
    res = smb_session_login(session.get());
    if (res != DSM_SUCCESS) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to logon to host",
                                               std::to_string(res),
                                               host,
                                               user,
                                           });
    }

    auto share_name = utils::string::split(path, '/', false).at(0U);

    smb_tid tid{};
    res = smb_tree_connect(session.get(), share_name.c_str(), &tid);
    if (res != DSM_SUCCESS) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to connect to share",
                                               std::to_string(res),
                                               share_name,
                                           });
    }

    return smb_directory_t{
        new smb_directory{
            "//" + std::string{host} + "/" + std::string{path},
            session,
            share_name,
            tid,
            stop_requested,
        },
    };
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return nullptr;
}

auto smb_directory::open(std::wstring_view host, std::wstring_view user,
                         std::wstring_view password, std::wstring_view path,
                         stop_type *stop_requested) -> smb_directory_t {
  return open(utils::string::to_utf8(host), utils::string::to_utf8(user),
              utils::string::to_utf8(password), utils::string::to_utf8(path),
              stop_requested);
}

auto smb_directory::copy_to(std::string_view new_path,
                            bool /* overwrite */) const -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    // auto to_path = utils::path::absolute(new_path);

    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to copy directory",
                                             "not implemented",
                                             new_path,
                                             path_,
                                         });
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto smb_directory::count(bool recursive) const -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    smb_stat_list_t list{
        smb_find(session_.get(), tid_, smb_create_search_path(path_).c_str()),
        smb_stat_list_deleter(),
    };
    auto count = smb_stat_list_count(list.get());

    if (not recursive) {
      return count;
    }

    throw utils::error::create_exception(
        function_name, {
                           "failed to get directory count recursively",
                           "not implemented",
                           utils::string::from_bool(recursive),
                           path_,
                       });
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return 0U;
}

auto smb_directory::create_directory(std::string_view path) const
    -> fs_directory_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    auto dir = get_directory(path);
    if (dir) {
      return dir;
    }

    auto res = smb_directory_create(
        session_.get(), tid_,
        smb_create_and_validate_relative_path(path_, path).c_str());
    if (res != DSM_SUCCESS) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to create directory",
                                               std::to_string(res),
                                               path_,
                                           });
    }

    return get_directory(path);
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return nullptr;
}

auto smb_directory::create_file(std::string_view file_name,
                                bool read_only) const -> fs_file_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    auto fs_file = get_file(file_name);
    if (fs_file) {
      if (not dynamic_cast<smb_file *>(fs_file.get())->open(read_only)) {
        throw utils::error::create_exception(function_name,
                                             {
                                                 "failed to open existing file",
                                                 file_name,
                                             });
      }

      return fs_file;
    }

    auto rel_path = smb_create_and_validate_relative_path(path_, file_name);

    smb_fd fd{};
    auto res =
        smb_fopen(session_.get(), tid_, rel_path.c_str(), SMB_MOD_RW, &fd);
    if (res != DSM_SUCCESS) {
      return nullptr;
    }
    smb_fclose(session_.get(), fd);

    res = smb_fopen(session_.get(), tid_, rel_path.c_str(),
                    read_only ? SMB_MOD_RO : SMB_MOD_RW2, &fd);
    if (res != DSM_SUCCESS) {
      return nullptr;
    }

    return std::make_unique<smb_file>(
        fd, smb_create_smb_path(path_, std::string{rel_path}), session_,
        share_name_, tid_);
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return nullptr;
}
auto smb_directory::exists() const -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    smb_stat_t st{
        smb_fstat(session_.get(), tid_,
                  smb_create_relative_path(path_).c_str()),
        smb_stat_deleter(),
    };
    if (not st) {
      return false;
    }

    return smb_stat_get(st.get(), SMB_STAT_ISDIR) != 0U;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto smb_directory::get_directory(std::string_view path) const
    -> fs_directory_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    auto rel_path = smb_create_and_validate_relative_path(path_, path);
    smb_stat_t st{
        smb_fstat(session_.get(), tid_, rel_path.c_str()),
        smb_stat_deleter(),
    };
    if (not st) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to stat directory",
                                               rel_path,
                                               path_,
                                           });
    }

    bool is_dir{smb_stat_get(st.get(), SMB_STAT_ISDIR) != 0U};
    if (not is_dir) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "path is not a directory",
                                               rel_path,
                                               path_,
                                           });
    }

    return smb_directory_t{
        new smb_directory{
            smb_create_smb_path(path_, rel_path),
            session_,
            share_name_,
            tid_,
            stop_requested_,
        },
    };
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return nullptr;
}

auto smb_directory::get_directories() const -> std::vector<fs_directory_t> {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }
    smb_stat_list_t list{
        smb_find(session_.get(), tid_, smb_create_search_path(path_).c_str()),

        smb_stat_list_deleter(),
    };
    if (not list) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to get directory list",
                                               path_,
                                           });
    }

    std::vector<fs_directory_t> ret{};

    auto count = smb_stat_list_count(list.get());
    for (std::size_t idx = 0U; !is_stop_requested() && idx < count; ++idx) {
      auto *item_st = smb_stat_list_at(list.get(), idx);

      bool is_dir{smb_stat_get(item_st, SMB_STAT_ISDIR) != 0U};
      if (not is_dir) {
        continue;
      }

      std::string name{smb_stat_name(item_st)};
      if (name == "." || name == "..") {
        continue;
      }

      try {
        ret.emplace_back(smb_directory_t{
            new smb_directory{
                smb_create_smb_path(path_, name),
                session_,
                share_name_,
                tid_,
                stop_requested_,
            },
        });
      } catch (const std::exception &e) {
        utils::error::handle_exception(function_name, e);
      } catch (...) {
        utils::error::handle_exception(function_name);
      }
    }

    return ret;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return {};
}

auto smb_directory::get_file(std::string_view path) const -> fs_file_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    auto rel_path = smb_create_and_validate_relative_path(path_, path);
    smb_stat_t st{
        smb_fstat(session_.get(), tid_, rel_path.c_str()),
        smb_stat_deleter(),
    };
    if (not st) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to stat file",
                                               rel_path,
                                               path_,
                                           });
    }

    bool is_dir{smb_stat_get(st.get(), SMB_STAT_ISDIR) != 0U};
    if (is_dir) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "path is not a file",
                                               rel_path,
                                               path_,
                                           });
    }

    return std::make_unique<smb_file>(
        std::nullopt, smb_create_smb_path(path_, std::string{rel_path}),
        session_, share_name_, tid_);
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return nullptr;
}

auto smb_directory::get_files() const -> std::vector<fs_file_t> {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    smb_stat_list_t list{
        smb_find(session_.get(), tid_, smb_create_search_path(path_).c_str()),
        smb_stat_list_deleter(),
    };
    if (not list) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to get file list",
                                               path_,
                                           });
    }

    std::vector<fs_file_t> ret{};

    auto count = smb_stat_list_count(list.get());
    for (std::size_t idx = 0U; !is_stop_requested() && idx < count; ++idx) {
      auto *item_st = smb_stat_list_at(list.get(), idx);

      bool is_dir{smb_stat_get(item_st, SMB_STAT_ISDIR) != 0U};
      if (is_dir) {
        continue;
      }

      try {
        std::string name{smb_stat_name(item_st)};
        ret.emplace_back(std::make_unique<smb_file>(
            std::nullopt, smb_create_smb_path(path_, name), session_,
            share_name_, tid_));
      } catch (const std::exception &e) {
        utils::error::handle_exception(function_name, e);
      } catch (...) {
        utils::error::handle_exception(function_name);
      }
    }

    return ret;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return {};
}

auto smb_directory::get_items() const -> std::vector<fs_item_t> {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    smb_stat_list_t list{
        smb_find(session_.get(), tid_, smb_create_search_path(path_).c_str()),
        smb_stat_list_deleter(),
    };
    if (not list) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to get item list",
                                               path_,
                                           });
    }
    std::vector<fs_item_t> ret{};

    auto count = smb_stat_list_count(list.get());
    for (std::size_t idx = 0U; !is_stop_requested() && idx < count; ++idx) {
      auto *item_st = smb_stat_list_at(list.get(), idx);

      bool is_dir{smb_stat_get(item_st, SMB_STAT_ISDIR) != 0U};
      std::string name{smb_stat_name(item_st)};

      if (is_dir) {
        if (name == "." || name == "..") {
          continue;
        }

        try {
          ret.emplace_back(smb_directory_t{
              new smb_directory{
                  path_ + '/' + name,
                  session_,
                  share_name_,
                  tid_,
                  stop_requested_,
              },
          });
        } catch (const std::exception &e) {
          utils::error::handle_exception(function_name, e);
        } catch (...) {
          utils::error::handle_exception(function_name);
        }
        continue;
      }

      try {
        ret.emplace_back(std::make_unique<smb_file>(
            std::nullopt, smb_create_smb_path(path_, name), session_,
            share_name_, tid_));
      } catch (const std::exception &e) {
        utils::error::handle_exception(function_name, e);
      } catch (...) {
        utils::error::handle_exception(function_name);
      }
    }

    return ret;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return {};
}

auto smb_directory::get_time(time_type type) const
    -> std::optional<std::uint64_t> {
  return smb_file::get_time(session_.get(), tid_, path_, type);
}

auto smb_directory::is_stop_requested() const -> bool {
  return (stop_requested_ != nullptr) && *stop_requested_;
}

auto smb_directory::is_symlink() const -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto smb_directory::move_to(std::string_view new_path) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to move directory",
                                             "not implemented",
                                             new_path,
                                             path_,
                                         });
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto smb_directory::remove() -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    return utils::retry_action([this]() -> bool {
      if (not exists()) {
        return true;
      }

      try {
        auto res = smb_directory_rm(session_.get(), tid_,
                                    smb_create_relative_path(path_).c_str());
        if (res != DSM_SUCCESS) {
          throw utils::error::create_exception(function_name,
                                               {
                                                   "failed to remove directory",
                                                   std::to_string(res),
                                                   path_,
                                               });
        }

        return true;
      } catch (const std::exception &e) {
        utils::error::handle_exception(function_name, e);
      } catch (...) {
        utils::error::handle_exception(function_name);
      }

      return false;
    });
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto smb_directory::remove_recursively() -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    if (not exists()) {
      return true;
    }

    throw utils::error::create_exception(
        function_name, {
                           "failed to remove directory recursively",
                           "not implemented",
                           path_,
                       });
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto smb_directory::size(bool /* recursive */) const -> std::uint64_t {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to get directory size",
                                             "not implemented",
                                             path_,
                                         });
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}
} // namespace repertory::utils::file

#endif // defined(PROJECT_ENABLE_LIBDSM)
