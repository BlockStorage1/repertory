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

#include "utils/file_smb_file.hpp"

#include "utils/common.hpp"
#include "utils/error.hpp"
#include "utils/string.hpp"

namespace repertory::utils::file {
void smb_file::close() {
  if (fd_.has_value()) {
    smb_fclose(session_.get(), *fd_);
    fd_.reset();
  }
}

auto smb_file::copy_to(std::string_view new_path,
                       bool overwrite) const -> bool {
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
                                             "failed to copy file",
                                             "not implemented",
                                             std::to_string(overwrite),
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

auto smb_file::exists() const -> bool {
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

    return smb_stat_get(st.get(), SMB_STAT_ISDIR) == 0U;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

void smb_file::flush() const {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to flush file",
                                             "not implemented",
                                             path_,
                                         });
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }
}

auto smb_file::get_time(smb_session *session, smb_tid tid, std::string path,
                        time_type type) -> std::optional<std::uint64_t> {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (session == nullptr) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path,
                                           });
    }

    auto rel_path = smb_create_relative_path(path);
    smb_stat_t st{
        smb_fstat(session, tid, rel_path.c_str()),
        smb_stat_deleter(),
    };
    if (not st) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to stat file",
                                               "not implemented",
                                               rel_path,
                                               path,
                                           });
    }

    switch (type) {
    case time_type::accessed:
      return smb_stat_get(st.get(), SMB_STAT_ATIME);

    case time_type::created:
      return smb_stat_get(st.get(), SMB_STAT_CTIME);

    case time_type::modified:
      return smb_stat_get(st.get(), SMB_STAT_MTIME);

    case time_type::written:
      return smb_stat_get(st.get(), SMB_STAT_WTIME);
    }
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return std::nullopt;
}

auto smb_file::is_symlink() const -> bool {
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

auto smb_file::move_to(std::string_view new_path) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (utils::string::begins_with(new_path, "//")) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to move file",
                                               "new path must be in same share",
                                               new_path,
                                               path_,
                                           });
    }

    auto from_path = smb_create_relative_path(path_);
    auto to_path = smb_create_and_validate_relative_path(
        utils::string::begins_with(new_path, "/") ? smb_get_root_path(path_)
                                                  : smb_get_parent_path(path_),
        new_path);

    auto was_open{false};
    if (fd_.has_value()) {
      close();
      was_open = true;
    }

    auto res = smb_tree_connect(session_.get(), share_name_.c_str(), &tid_);
    if (res != DSM_SUCCESS) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to connect to share",
                                               std::to_string(res),
                                               share_name_,
                                               path_,
                                           });
    }

    res = smb_file_mv(session_.get(), tid_, from_path.c_str(), to_path.c_str());
    if (res != DSM_SUCCESS) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to move file",
                                               std::to_string(res),
                                               from_path,
                                               to_path,
                                           });
    }

    path_ = smb_create_smb_path(path_, to_path);
    if (was_open) {
      return open(read_only_);
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto smb_file::open(bool read_only) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (fd_.has_value()) {
      if (read_only == read_only_) {
        return true;
      }

      close();
    }

    auto rel_path = smb_create_relative_path(path_);

    auto res = smb_tree_connect(session_.get(), share_name_.c_str(), &tid_);
    if (res != DSM_SUCCESS) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to connect to share",
                                               std::to_string(res),
                                               share_name_,
                                               path_,
                                           });
    }

    smb_fd fd{};
    res = smb_fopen(session_.get(), tid_, rel_path.c_str(),
                    read_only ? SMB_MOD_RO : SMB_MOD_RW2, &fd);
    if (res != DSM_SUCCESS) {
      throw utils::error::create_exception(
          function_name, {
                             "failed to open file",
                             std::to_string(res),
                             utils::string::from_bool(read_only),
                             rel_path,
                             path_,
                         });
    }

    fd_ = fd;
    read_only_ = read_only;

    return true;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto smb_file::read(unsigned char *data, std::size_t to_read,
                    std::uint64_t offset, std::size_t *total_read) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (total_read != nullptr) {
      (*total_read) = 0U;
    }

    if (not fd_.has_value()) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to read file",
                                               "file not open",
                                               path_,
                                           });
    }

    auto res = smb_fseek(session_.get(), *fd_, static_cast<off_t>(offset),
                         SMB_SEEK_SET);
    if (res == -1) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to seek file",
                                               std::to_string(res),
                                               std::to_string(offset),
                                               path_,
                                           });
    }

    std::size_t bytes_read{0U};
    while (bytes_read != to_read) {
      res = smb_fread(session_.get(), *fd_, &data[bytes_read],
                      to_read - bytes_read);
      if (res == -1) {
        throw utils::error::create_exception(function_name,
                                             {
                                                 "failed to read file",
                                                 std::to_string(res),
                                                 std::to_string(offset),
                                                 std::to_string(to_read),
                                                 path_,
                                             });
      }

      if (res == 0) {
        break;
      }

      bytes_read += static_cast<std::size_t>(res);
    }

    if (total_read != nullptr) {
      (*total_read) = bytes_read;
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto smb_file::remove() -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    close();

    return utils::retry_action([this]() -> bool {
      if (not exists()) {
        return true;
      }

      try {
        auto res = smb_tree_connect(session_.get(), share_name_.c_str(), &tid_);
        if (res != DSM_SUCCESS) {
          throw utils::error::create_exception(function_name,
                                               {
                                                   "failed to connect to share",
                                                   std::to_string(res),
                                                   share_name_,
                                                   path_,
                                               });
        }

        auto rel_path = smb_create_relative_path(path_);
        res = smb_file_rm(session_.get(), tid_, rel_path.c_str());
        if (res != DSM_SUCCESS) {
          throw utils::error::create_exception(
              function_name,
              {
                  "failed to remove file",
                  std::to_string(res),
                  std::to_string(smb_session_get_nt_status(session_.get())),
                  rel_path,
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

auto smb_file::size() const -> std::optional<std::uint64_t> {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (not session_) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "session not found",
                                               path_,
                                           });
    }

    auto rel_path = smb_create_relative_path(path_);
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

    return smb_stat_get(st.get(), SMB_STAT_SIZE);
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return std::nullopt;
}

auto smb_file::truncate(std::size_t size) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    throw utils::error::create_exception(function_name,
                                         {
                                             "failed to truncate file",
                                             "not implemented",
                                             std::to_string(size),
                                             path_,
                                         });
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}

auto smb_file::write(const unsigned char *data, std::size_t to_write,
                     std::size_t offset, std::size_t *total_written) -> bool {
  REPERTORY_USES_FUNCTION_NAME();

  try {
    if (total_written != nullptr) {
      (*total_written) = 0U;
    }

    if (not fd_.has_value()) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to write file",
                                               "file not open",
                                               path_,
                                           });
    }

    auto res = smb_fseek(session_.get(), *fd_, static_cast<off_t>(offset),
                         SMB_SEEK_SET);
    if (res == -1) {
      throw utils::error::create_exception(function_name,
                                           {
                                               "failed to seek file",
                                               std::to_string(res),
                                               std::to_string(offset),
                                               path_,
                                           });
    }

    std::size_t bytes_written{0U};
    while (bytes_written != to_write) {
      res = smb_fwrite(session_.get(), *fd_,
                       const_cast<unsigned char *>(&data[bytes_written]),
                       to_write - bytes_written);
      if (res == -1) {
        throw utils::error::create_exception(function_name,
                                             {
                                                 "failed to write file",
                                                 std::to_string(res),
                                                 std::to_string(offset),
                                                 std::to_string(to_write),
                                                 path_,
                                             });
      }

      if (res == 0) {
        break;
      }

      bytes_written += static_cast<std::size_t>(res);
    }

    if (total_written != nullptr) {
      (*total_written) = bytes_written;
    }

    return true;
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  return false;
}
} // namespace repertory::utils::file

#endif // defined(PROJECT_ENABLE_LIBDSM)
