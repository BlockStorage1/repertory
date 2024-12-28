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
#ifndef REPERTORY_INCLUDE_UTILS_FILE_SMB_FILE_HPP_
#define REPERTORY_INCLUDE_UTILS_FILE_SMB_FILE_HPP_
#if defined(PROJECT_ENABLE_LIBDSM)

#include "utils/file.hpp"

namespace repertory::utils::file {
class smb_file final : public i_file {
public:
  smb_file() = default;

  smb_file(std::optional<smb_fd> fd, std::string path, smb_session_t session,
           std::string_view share_name, smb_tid tid)
      : fd_(std::move(fd)),
        path_(std::move(path)),
        session_(std::move(session)),
        share_name_(share_name),
        tid_(tid) {}

  smb_file(const smb_file &) = delete;

  smb_file(smb_file &&f) noexcept
      : fd_(std::move(f.fd_)),
        path_(std::move(f.path_)),
        read_buffer_size(f.get_read_buffer_size()),
        read_only_(f.read_only_),
        session_(std::move(f.session_)),
        share_name_(std::move(f.share_name_)),
        tid_(f.tid_) {}

  ~smb_file() override { close(); }

private:
  std::optional<smb_fd> fd_;
  std::string path_;
  std::atomic_uint32_t read_buffer_size{65536U};
  bool read_only_;
  smb_session_t session_;
  std::string share_name_;
  smb_tid tid_;

public:
  void close() override;

  [[nodiscard]] auto copy_to(std::string_view new_path,
                             bool overwrite) const -> bool override;

  [[nodiscard]] auto exists() const -> bool override;

  void flush() const override;

  [[nodiscard]] auto get_handle() const -> native_handle override {
    return INVALID_HANDLE_VALUE;
  }

  [[nodiscard]] auto get_path() const -> std::string override { return path_; }

  [[nodiscard]] auto get_read_buffer_size() const -> std::uint32_t override {
    return read_buffer_size;
  }

  [[nodiscard]] static auto
  get_time(smb_session *session, smb_tid tid, std::string path,
           time_type type) -> std::optional<std::uint64_t>;

  [[nodiscard]] auto
  get_time(time_type type) const -> std::optional<std::uint64_t> override {
    return get_time(session_.get(), tid_, path_, type);
  }

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

  [[nodiscard]] auto is_read_only() const -> bool override {
    return read_only_;
  }

  [[nodiscard]] auto is_symlink() const -> bool override;

  [[nodiscard]] auto move_to(std::string_view new_path) -> bool override;

  [[nodiscard]] auto open(bool read_only) -> bool;

  [[nodiscard]] auto read(unsigned char *data, std::size_t to_read,
                          std::uint64_t offset,
                          std::size_t *total_read = nullptr) -> bool override;

  [[nodiscard]] auto remove() -> bool override;

  auto set_read_buffer_size(std::uint32_t size) -> std::uint32_t override {
    read_buffer_size = size;
    return read_buffer_size;
  }

  [[nodiscard]] auto size() const -> std::optional<std::uint64_t> override;

  [[nodiscard]] auto truncate(std::size_t size) -> bool override;

  [[nodiscard]] auto
  write(const unsigned char *data, std::size_t to_write, std::size_t offset,
        std::size_t *total_written = nullptr) -> bool override;

public:
  auto operator=(const smb_file &) noexcept -> smb_file & = delete;

  auto operator=(smb_file &&move_file) noexcept -> smb_file & {
    if (this != &move_file) {
      fd_ = std::move(move_file.fd_);
      path_ = std::move(move_file.path_);
      read_buffer_size = move_file.get_read_buffer_size();
      read_only_ = move_file.read_only_;
      session_ = std::move(move_file.session_);
      share_name_ = std::move(move_file.share_name_);
      tid_ = move_file.tid_;
    }

    return *this;
  }

  [[nodiscard]] operator bool() const override { return fd_.has_value(); }
};
} // namespace repertory::utils::file

#endif // defined(PROJECT_ENABLE_LIBDSM)
#endif // REPERTORY_INCLUDE_UTILS_FILE_SMB_FILE_HPP_
