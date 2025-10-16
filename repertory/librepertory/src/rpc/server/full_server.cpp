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
#include "rpc/server/full_server.hpp"

#include "app_config.hpp"
#include "events/event_system.hpp"
#include "events/types/file_pinned.hpp"
#include "events/types/file_unpinned.hpp"
#include "file_manager/cache_size_mgr.hpp"
#include "file_manager/i_file_manager.hpp"
#include "providers/i_provider.hpp"
#include "types/repertory.hpp"
#include "types/rpc.hpp"
#include "utils/error_utils.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"

namespace repertory {
full_server::full_server(app_config &config, i_provider &provider,
                         i_file_manager &fm)
    : server(config), provider_(provider), fm_(fm) {}

void full_server::handle_get_directory_items(const httplib::Request &req,
                                             httplib::Response &res) {
  auto api_path = utils::path::create_api_path(req.get_param_value("api_path"));
  res.set_content(json({
                           {"items", fm_.get_directory_items(api_path)},
                       })
                      .dump(),
                  "application/json");
  res.status = http_error_codes::ok;
}

void full_server::handle_get_item_info(const httplib::Request &req,
                                       httplib::Response &res) {
  auto api_path = utils::path::create_api_path(req.get_param_value("api_path"));
  directory_item item;
  auto ret = fm_.get_directory_item(api_path, item);
  if (ret == api_error::success) {
    res.set_content(json(item).dump(), "application/json");
    res.status = http_error_codes::ok;
    return;
  }

  res.status = http_error_codes::not_found;
}

void full_server::handle_get_drive_information(const httplib::Request & /*req*/,
                                               httplib::Response &res) {
  res.set_content(
      json({
               {"cache_space_used", cache_size_mgr::instance().size()},
               {"drive_space_total", provider_.get_total_drive_space()},
               {"drive_space_used", provider_.get_used_drive_space()},
               {"item_count", provider_.get_total_item_count()},
           })
          .dump(),
      "application/json");
  res.status = http_error_codes::ok;
}

void full_server::handle_get_open_files(const httplib::Request & /*req*/,
                                        httplib::Response &res) {
  auto list = fm_.get_open_files();

  json open_files;
  for (const auto &kv : list) {
    open_files["items"].emplace_back(json({
        {"path", kv.first},
        {"count", kv.second},
    }));
  }
  res.set_content(open_files.dump(), "application/json");
  res.status = http_error_codes::ok;
}

void full_server::handle_get_pinned_files(const httplib::Request & /*req*/,
                                          httplib::Response &res) {
  res.set_content(json({
                           {"items", provider_.get_pinned_files()},
                       })
                      .dump(),
                  "application/json");
  res.status = http_error_codes::ok;
}

void full_server::handle_get_pinned_status(const httplib::Request &req,
                                           httplib::Response &res) {
  REPERTORY_USES_FUNCTION_NAME();

  auto api_path = utils::path::create_api_path(req.get_param_value("api_path"));

  std::string pinned;
  auto result = provider_.get_item_meta(api_path, META_PINNED, pinned);
  if (result != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, result,
                                       "failed to get pinned status");
    res.status = http_error_codes::internal_error;
    return;
  }

  res.set_content(
      json({
               {"pinned",
                pinned.empty() ? false : utils::string::to_bool(pinned)},
           })
          .dump(),
      "application/json");
  res.status = http_error_codes::ok;
}

void full_server::handle_pin_file(const httplib::Request &req,
                                  httplib::Response &res) {
  REPERTORY_USES_FUNCTION_NAME();

  auto api_path = utils::path::create_api_path(req.get_param_value("api_path"));

  bool exists{};
  auto result = provider_.is_file(api_path, exists);
  if (result != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, result,
                                       "failed to pin file");
    res.status = http_error_codes::internal_error;
    return;
  }

  if (exists) {
    exists = api_error::success ==
             provider_.set_item_meta(api_path, META_PINNED,
                                     utils::string::from_bool(true));
  }

  if (exists) {
    event_system::instance().raise<file_pinned>(api_path, function_name);
  }

  res.status = exists ? http_error_codes::ok : http_error_codes::not_found;
}

void full_server::handle_unpin_file(const httplib::Request &req,
                                    httplib::Response &res) {
  REPERTORY_USES_FUNCTION_NAME();

  auto api_path = utils::path::create_api_path(req.get_param_value("api_path"));

  bool exists{};
  auto result = provider_.is_file(api_path, exists);
  if (result != api_error::success) {
    utils::error::raise_api_path_error(function_name, api_path, result,
                                       "failed to unpin file");
    res.status = http_error_codes::internal_error;
    return;
  }

  if (exists) {
    exists = api_error::success ==
             provider_.set_item_meta(api_path, META_PINNED,
                                     utils::string::from_bool(false));
  }

  if (exists) {
    event_system::instance().raise<file_unpinned>(api_path, function_name);
  }

  res.status = exists ? http_error_codes::ok : http_error_codes::not_found;
}

void full_server::initialize(httplib::Server &inst) {
  server::initialize(inst);

  inst.Get("/api/v1/" + rpc_method::get_directory_items,
           [this](auto &&req, auto &&res) {
             handle_get_directory_items(std::forward<decltype(req)>(req),
                                        std::forward<decltype(res)>(res));
           });
  inst.Get("/api/v1/" + rpc_method::get_item_info,
           [this](auto &&req, auto &&res) {
             handle_get_item_info(std::forward<decltype(req)>(req),
                                  std::forward<decltype(res)>(res));
           });
  inst.Get("/api/v1/" + rpc_method::get_drive_information,
           [this](auto &&req, auto &&res) {
             handle_get_drive_information(std::forward<decltype(req)>(req),
                                          std::forward<decltype(res)>(res));
           });
  inst.Get("/api/v1/" + rpc_method::get_open_files,
           [this](auto &&req, auto &&res) {
             handle_get_open_files(std::forward<decltype(req)>(req),
                                   std::forward<decltype(res)>(res));
           });
  inst.Get("/api/v1/" + rpc_method::get_pinned_files,
           [this](auto &&req, auto &&res) {
             handle_get_pinned_files(std::forward<decltype(req)>(req),
                                     std::forward<decltype(res)>(res));
           });
  inst.Get("/api/v1/" + rpc_method::pinned_status,
           [this](auto &&req, auto &&res) {
             handle_get_pinned_status(std::forward<decltype(req)>(req),
                                      std::forward<decltype(res)>(res));
           });
  inst.Post("/api/v1/" + rpc_method::pin_file, [this](auto &&req, auto &&res) {
    handle_pin_file(std::forward<decltype(req)>(req),
                    std::forward<decltype(res)>(res));
  });
  inst.Post("/api/v1/" + rpc_method::unpin_file,
            [this](auto &&req, auto &&res) {
              handle_unpin_file(std::forward<decltype(req)>(req),
                                std::forward<decltype(res)>(res));
            });
}
} // namespace repertory
