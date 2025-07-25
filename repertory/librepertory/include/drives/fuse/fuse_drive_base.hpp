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
#ifndef REPERTORY_INCLUDE_DRIVES_FUSE_FUSE_DRIVE_BASE_HPP_
#define REPERTORY_INCLUDE_DRIVES_FUSE_FUSE_DRIVE_BASE_HPP_
#if !defined(_WIN32)

#include "drives/fuse/fuse_base.hpp"
#include "drives/fuse/i_fuse_drive.hpp"

namespace repertory {
class app_config;
class i_provider;

class fuse_drive_base : public fuse_base, public i_fuse_drive {
public:
  explicit fuse_drive_base(app_config &config) : fuse_base(config) {}

  ~fuse_drive_base() override = default;

public:
  fuse_drive_base(const fuse_drive_base &) = delete;
  fuse_drive_base(fuse_drive_base &&) = delete;
  auto operator=(const fuse_drive_base &) -> fuse_drive_base & = delete;
  auto operator=(fuse_drive_base &&) -> fuse_drive_base & = delete;

protected:
  [[nodiscard]] auto access_impl(std::string api_path, int mask)
      -> api_error override;

protected:
  [[nodiscard]] auto check_access(const std::string &api_path, int mask) const
      -> api_error;

  [[nodiscard]] auto
  check_and_perform(const std::string &api_path, int parent_mask,
                    const std::function<api_error(api_meta_map &meta)> &action)
      -> api_error;

  [[nodiscard]] auto get_current_gid() const -> gid_t;

  [[nodiscard]] auto get_current_uid() const -> uid_t;

  [[nodiscard]] auto get_effective_gid() const -> gid_t;

  [[nodiscard]] auto get_effective_uid() const -> uid_t;

  [[nodiscard]] static auto check_open_flags(int flags, int mask,
                                             api_error fail_error) -> api_error;

  [[nodiscard]] auto check_owner(const api_meta_map &meta) const -> api_error;

  [[nodiscard]] static auto check_readable(int flags, api_error fail_error)
      -> api_error;

  [[nodiscard]] static auto check_writeable(int flags, api_error fail_error)
      -> api_error;

#if defined(__APPLE__)
  [[nodiscard]] static auto get_flags_from_meta(const api_meta_map &meta)
      -> __uint32_t;
#endif // defined(__APPLE__)

  [[nodiscard]] static auto get_gid_from_meta(const api_meta_map &meta)
      -> gid_t;

  [[nodiscard]] static auto get_mode_from_meta(const api_meta_map &meta)
      -> mode_t;

  static void get_timespec_from_meta(const api_meta_map &meta,
                                     const std::string &name,
                                     struct timespec &ts);

  [[nodiscard]] static auto get_uid_from_meta(const api_meta_map &meta)
      -> uid_t;

#if defined(__APPLE__)
  [[nodiscard]] auto parse_xattr_parameters(const char *name,
                                            const uint32_t &position,
                                            std::string &attribute_name,
                                            const std::string &api_path)
      -> api_error;
#else  // !defined(__APPLE__)
  [[nodiscard]] auto parse_xattr_parameters(const char *name,
                                            std::string &attribute_name,
                                            const std::string &api_path)
      -> api_error;
#endif // defined(__APPLE__)

#if defined(__APPLE__)
  [[nodiscard]] auto
  parse_xattr_parameters(const char *name, const char *value, size_t size,
                         const uint32_t &position, std::string &attribute_name,
                         const std::string &api_path) -> api_error;
#else  // !defined(__APPLE__)
  [[nodiscard]] auto parse_xattr_parameters(const char *name, const char *value,
                                            size_t size,
                                            std::string &attribute_name,
                                            const std::string &api_path)
      -> api_error;
#endif // defined(__APPLE__)

  static void populate_stat(const std::string &api_path,
                            std::uint64_t size_or_count,
                            const api_meta_map &meta, bool directory,
                            i_provider &provider, struct stat *st);

  static void set_timespec_from_meta(const api_meta_map &meta,
                                     const std::string &name,
                                     struct timespec &ts);

public:
  [[nodiscard]] auto check_owner(const std::string &api_path) const
      -> api_error override;

  [[nodiscard]] auto check_parent_access(const std::string &api_path,
                                         int mask) const -> api_error override;
};
} // namespace repertory

#endif // !defined(_WIN32)
#endif // REPERTORY_INCLUDE_DRIVES_FUSE_FUSE_DRIVE_BASE_HPP_
