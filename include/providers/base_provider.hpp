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
#ifndef INCLUDE_PROVIDERS_BASE_PROVIDER_HPP_
#define INCLUDE_PROVIDERS_BASE_PROVIDER_HPP_

#include "common.hpp"
#include "db/meta_db.hpp"
#include "providers/i_provider.hpp"

namespace repertory {
class app_config;
class base_provider : public i_provider {
public:
  explicit base_provider(app_config &config);

  ~base_provider() override = default;

private:
  app_config &config_;

protected:
  api_item_added_callback api_item_added_;
  meta_db meta_db_;
  mutable std::recursive_mutex notify_added_mutex_;
  i_open_file_table *oft_ = nullptr;

protected:
  app_config &get_config() { return config_; }

  app_config &get_config() const { return config_; }

  virtual std::string format_api_path(std::string api_path) const { return api_path; }

  virtual std::string &restore_api_path(std::string &api_path) const { return api_path; }

  void notify_directory_added(const std::string &api_path, const std::string &api_parent) const {
    const_cast<base_provider *>(this)->notify_directory_added(api_path, api_parent);
  }

  virtual void notify_directory_added(const std::string &api_path, const std::string &api_parent);

  api_error notify_file_added(const std::string &api_path, const std::string &api_parent,
                              const std::uint64_t &size) const {
    return const_cast<base_provider *>(this)->notify_file_added(api_path, api_parent, size);
  }

  virtual api_error notify_file_added(const std::string &api_path, const std::string &api_parent,
                                      const std::uint64_t &size) = 0;

  void remove_item_meta(const std::string &api_path) {
    return meta_db_.remove_item_meta(format_api_path(api_path));
  }

  void update_filesystem_item(const bool &directory, const api_error &error,
                              const std::string &api_path, filesystem_item &fsi) const;

public:
  api_error create_directory_clone_source_meta(const std::string &source_api_path,
                                               const std::string &api_path) override;

  api_error create_file(const std::string &api_path, api_meta_map &meta) override;

  api_error get_api_path_from_source(const std::string &source_path,
                                     std::string &api_path) const override;

  api_error get_filesystem_item(const std::string &api_path, const bool &directory,
                                filesystem_item &fsi) const override;

  api_error get_filesystem_item_and_file(const std::string &api_path, api_file &file,
                                         filesystem_item &fsi) const override;

  api_error get_filesystem_item_from_source_path(const std::string &source_path,
                                                 filesystem_item &fsi) const override;

  api_error get_item_meta(const std::string &api_path, api_meta_map &meta) const override;

  api_error get_item_meta(const std::string &api_path, const std::string &key,
                          std::string &value) const override;

  std::vector<std::string> get_pinned_files() const override { return meta_db_.get_pinned_files(); }

  std::uint64_t get_used_drive_space() const override;

  bool is_file_writeable(const std::string &) const override { return true; }

  api_error remove_item_meta(const std::string &api_path, const std::string &key) override {
    return meta_db_.remove_item_meta(format_api_path(api_path), key);
  }

  api_error set_item_meta(const std::string &api_path, const std::string &key,
                          const std::string &value) override {
    return meta_db_.set_item_meta(format_api_path(api_path), key, value);
  }

  api_error set_item_meta(const std::string &api_path, const api_meta_map &meta) override {
    return meta_db_.set_item_meta(format_api_path(api_path), meta);
  }

  api_error set_source_path(const std::string &api_path, const std::string &source_path) override {
    return meta_db_.set_source_path(format_api_path(api_path), source_path);
  }

  bool start(api_item_added_callback api_item_added, i_open_file_table *oft) override;
};
} // namespace repertory

#endif // INCLUDE_PROVIDERS_BASE_PROVIDER_HPP_
