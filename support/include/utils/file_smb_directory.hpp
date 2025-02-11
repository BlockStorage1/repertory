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
#ifndef REPERTORY_INCLUDE_UTILS_FILE_SMB_DIRECTORY_HPP_
#define REPERTORY_INCLUDE_UTILS_FILE_SMB_DIRECTORY_HPP_
#if defined(PROJECT_ENABLE_LIBDSM)

#include "utils/file.hpp"

namespace repertory::utils::file {
class smb_directory final : public i_directory {
public:
  using smb_directory_t = std::unique_ptr<smb_directory>;

  [[nodiscard]] static auto
  open(std::string_view host, std::string_view user, std::string_view password,
       std::string_view path,
       stop_type *stop_requested = nullptr) -> smb_directory_t;

  [[nodiscard]] static auto
  open(std::wstring_view host, std::wstring_view user,
       std::wstring_view password, std::wstring_view path,
       stop_type *stop_requested = nullptr) -> smb_directory_t;

public:
  smb_directory() noexcept = default;

  smb_directory(const smb_directory &) noexcept = delete;

  smb_directory(smb_directory &&) noexcept = default;

  ~smb_directory() override = default;

private:
  smb_directory(std::string path, smb_session_t session,
                std::string_view share_name, smb_tid tid,
                stop_type *stop_requested)
      : path_(std::move(path)),
        session_(std::move(session)),
        share_name_(share_name),
        tid_(tid),
        stop_requested_(stop_requested) {}

private:
  std::string path_{};
  smb_session_t session_{};
  std::string share_name_{};
  smb_tid tid_{};
  stop_type *stop_requested_{nullptr};

public:
  [[nodiscard]] auto
  count(bool recursive = false) const -> std::uint64_t override;

  [[nodiscard]] auto copy_to(std::string_view new_path,
                             bool overwrite) const -> bool override;

  [[nodiscard]] auto
  create_directory(std::string_view path = "") const -> fs_directory_t override;

  [[nodiscard]] auto create_file(std::string_view file_name,
                                 bool read_only) const -> fs_file_t override;

  [[nodiscard]] auto exists() const -> bool override;

  [[nodiscard]] auto
  get_directory(std::string_view path) const -> fs_directory_t override;

  [[nodiscard]] auto
  get_directories() const -> std::vector<fs_directory_t> override;

  [[nodiscard]] auto
  get_file(std::string_view path) const -> fs_file_t override;

  [[nodiscard]] auto get_files() const -> std::vector<fs_file_t> override;

  [[nodiscard]] auto get_items() const -> std::vector<fs_item_t> override;

  [[nodiscard]] auto get_path() const -> std::string override { return path_; }

  [[nodiscard]] auto
  get_time(time_type type) const -> std::optional<std::uint64_t> override;

  [[nodiscard]] auto get_unc_path() const -> std::string {
    return smb_get_unc_path(path_);
  }

  [[nodiscard]] auto get_uri_path() const -> std::string {
    return smb_get_uri_path(path_);
  }

  [[nodiscard]] auto
  get_uri_path(std::string_view user,
               std::string_view password) const -> std::string {
    return smb_get_uri_path(path_, user, password);
  }

  [[nodiscard]] auto is_stop_requested() const -> bool override;

  [[nodiscard]] auto is_symlink() const -> bool override;

  [[nodiscard]] auto move_to(std::string_view new_path) -> bool override;

  [[nodiscard]] auto remove() -> bool override;

  [[nodiscard]] auto remove_recursively() -> bool override;

  [[nodiscard]] auto
  size(bool recursive = false) const -> std::uint64_t override;

public:
  auto operator=(const smb_directory &) noexcept -> smb_directory & = delete;

  auto
  operator=(smb_directory &&move_dir) noexcept -> smb_directory & = default;

  [[nodiscard]] operator bool() const override { return session_ != nullptr; }
};
} // namespace repertory::utils::file

#endif // defined(PROJECT_ENABLE_LIBDSM)
#endif // REPERTORY_INCLUDE_UTILS_FILE_SMB_DIRECTORY_HPP_
