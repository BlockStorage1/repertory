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
#include "providers/base_provider.hpp"
#include "app_config.hpp"
#include "types/repertory.hpp"
#include "utils/file_utils.hpp"
#include "utils/global_data.hpp"
#include "utils/native_file.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
base_provider::base_provider(app_config &config) : config_(config), meta_db_(config) {}

void base_provider::notify_directory_added(const std::string &api_path,
                                           const std::string &api_parent) {
  recur_mutex_lock l(notify_added_mutex_);

  const auto now = utils::get_file_time_now();
  api_item_added_(api_path, api_parent, "", true, now, now, now, now);
}

void base_provider::update_filesystem_item(const bool &directory, const api_error &error,
                                           const std::string &api_path,
                                           filesystem_item &fsi) const {
  if (error == api_error::success) {
    fsi.directory = directory;
    fsi.lock = fsi.lock ? fsi.lock : std::make_shared<std::recursive_mutex>();
    fsi.api_path = api_path;
    fsi.api_parent = utils::path::get_parent_api_path(api_path);
  } else {
    event_system::instance().raise<filesystem_item_get_failed>(
        api_path, std::to_string(static_cast<int>(error)));
  }
}

api_error base_provider::create_directory_clone_source_meta(const std::string &source_api_path,
                                                            const std::string &api_path) {
  api_meta_map meta{};
  auto ret = get_item_meta(source_api_path, meta);
  if (ret == api_error::success) {
    ret = create_directory(api_path, meta);
  }
  return ret;
}

api_error base_provider::create_file(const std::string &api_path, api_meta_map &meta) {
  const auto isDir = is_directory(api_path);
  const auto isFile = is_file(api_path);
  auto ret = isDir    ? api_error::directory_exists
             : isFile ? api_error::file_exists
                      : api_error::success;
  if (ret == api_error::success) {
    const auto source =
        utils::path::combine(get_config().get_cache_directory(), {utils::create_uuid_string()});

    native_file_ptr nf;
    if ((ret = native_file::create_or_open(source, nf)) == api_error::success) {
      nf->close();
    }

    if (ret == api_error::success) {
      if (((ret = set_item_meta(api_path, meta)) != api_error::success) ||
          ((ret = set_source_path(api_path, source)) != api_error::success) ||
          ((ret = upload_file(api_path, source, meta[META_ENCRYPTION_TOKEN])) !=
           api_error::success)) {
        meta_db_.remove_item_meta(format_api_path(api_path));
        utils::file::delete_file(source);
      }
    }
  }

  return ret;
}

api_error base_provider::get_api_path_from_source(const std::string &source_path,
                                                  std::string &api_path) const {
  const auto ret = meta_db_.get_api_path_from_source(source_path, api_path);
  restore_api_path(api_path);
  return ret;
}

api_error base_provider::get_filesystem_item(const std::string &api_path, const bool &directory,
                                             filesystem_item &fsi) const {
  auto ret = api_error::error;
  if (directory) {
    ret = is_directory(api_path) ? api_error::success : api_error::item_not_found;
    update_filesystem_item(true, ret, api_path, fsi);
  } else {
    api_file file{};
    ret = get_filesystem_item_and_file(api_path, file, fsi);
  }

  return ret;
}

api_error base_provider::get_filesystem_item_and_file(const std::string &api_path, api_file &file,
                                                      filesystem_item &fsi) const {
  auto ret = get_item_meta(api_path, META_SOURCE, fsi.source_path);
  if (ret == api_error::success) {
    ret = get_file(api_path, file);
    if (ret == api_error::success) {
      fsi.encryption_token = file.encryption_token;
      fsi.size = file.file_size;
    } else if (not is_file(api_path)) {
      ret = api_error::item_not_found;
    }
  }

  update_filesystem_item(false, ret, api_path, fsi);
  return ret;
}

api_error base_provider::get_filesystem_item_from_source_path(const std::string &source_path,
                                                              filesystem_item &fsi) const {
  auto ret = api_error::item_not_found;
  if (not source_path.empty()) {
    std::string api_path;
    if ((ret = get_api_path_from_source(source_path, api_path)) == api_error::success) {
      ret = get_filesystem_item(api_path, false, fsi);
    }
  }

  return ret;
}

api_error base_provider::get_item_meta(const std::string &api_path, api_meta_map &meta) const {
  auto ret = meta_db_.get_item_meta(format_api_path(api_path), meta);
  if (ret == api_error::item_not_found) {
    auto get_meta = false;
    if (is_directory(api_path)) {
      notify_directory_added(api_path, utils::path::get_parent_api_path(api_path));
      get_meta = true;
    } else if (is_file(api_path)) {
      std::uint64_t file_size = 0u;
      if ((ret = get_file_size(api_path, file_size)) == api_error::success) {
        get_meta = ((ret = notify_file_added(api_path, utils::path::get_parent_api_path(api_path),
                                             file_size)) == api_error::success);
      }
    }
    if (get_meta) {
      ret = meta_db_.get_item_meta(format_api_path(api_path), meta);
    }
  }

  return ret;
}

api_error base_provider::get_item_meta(const std::string &api_path, const std::string &key,
                                       std::string &value) const {
  auto ret = meta_db_.get_item_meta(format_api_path(api_path), key, value);
  if (ret == api_error::item_not_found) {
    auto get_meta = false;
    if (is_directory(api_path)) {
      notify_directory_added(api_path, utils::path::get_parent_api_path(api_path));
      get_meta = true;
    } else if (is_file(api_path)) {
      std::uint64_t file_size = 0u;
      if ((ret = get_file_size(api_path, file_size)) == api_error::success) {
        get_meta = ((ret = notify_file_added(api_path, utils::path::get_parent_api_path(api_path),
                                             file_size)) == api_error::success);
      }
    }
    if (get_meta) {
      ret = meta_db_.get_item_meta(format_api_path(api_path), key, value);
    }
  }

  return ret;
}

std::uint64_t base_provider::get_used_drive_space() const {
  return global_data::instance().get_used_drive_space();
}

bool base_provider::start(api_item_added_callback api_item_added, i_open_file_table *oft) {
  api_item_added_ = api_item_added;
  oft_ = oft;

  auto unmount_requested = false;
  {
    repertory::event_consumer ec("unmount_requested",
                                 [&unmount_requested](const event &) { unmount_requested = true; });
    for (std::uint16_t i = 0u; not unmount_requested && not is_online() &&
                               (i < get_config().get_online_check_retry_secs());
         i++) {
      event_system::instance().raise<provider_offline>(
          get_config().get_host_config().host_name_or_ip, get_config().get_host_config().api_port);
      std::this_thread::sleep_for(1s);
    }
  }
  return unmount_requested;
}
} // namespace repertory
