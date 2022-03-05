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
#include "rpc/server/full_server.hpp"
#include "app_config.hpp"
#include "drives/directory_iterator.hpp"
#include "drives/i_open_file_table.hpp"
#include "providers/i_provider.hpp"
#include "providers/skynet/skynet_provider.hpp"
#include "types/repertory.hpp"
#include "types/rpc.hpp"
#include "types/skynet.hpp"
#include "utils/global_data.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
full_server::full_server(app_config &config, i_provider &provider, i_open_file_table &oft)
    : server(config), provider_(provider), oft_(oft) {}

bool full_server::handle_request(jsonrpcpp::request_ptr &request,
                                 std::unique_ptr<jsonrpcpp::Response> &response) {
  auto handled = true;
  if (request->method == rpc_method::get_drive_information) {
    if (request->params.param_array.empty()) {
      response = std::make_unique<jsonrpcpp::Response>(
          *request, json({{"cache_space_used", global_data::instance().get_used_cache_space()},
                          {"drive_space_total", provider_.get_total_drive_space()},
                          {"drive_space_used", global_data::instance().get_used_drive_space()},
                          {"item_count", provider_.get_total_item_count()}}));
    } else {
      throw jsonrpcpp::InvalidParamsException(*request);
    }
  } else if (request->method == rpc_method::get_pinned_files) {
    if (request->params.param_array.empty()) {
      response = std::make_unique<jsonrpcpp::Response>(*request, provider_.get_pinned_files());
    } else {
      throw jsonrpcpp::InvalidParamsException(*request);
    }
  } else if (request->method == rpc_method::get_directory_items) {
    if (request->params.param_array.size() == 1u) {
      const auto api_path = utils::path::create_api_path(request->params.param_array[0]);
      auto directoryItems = oft_.get_directory_items(api_path);
      std::vector<json> items;
      for (const auto &item : directoryItems) {
        items.emplace_back(item.to_json());
      }

      response = std::make_unique<jsonrpcpp::Response>(*request, json({{"items", items}}));
    } else {
      throw jsonrpcpp::InvalidParamsException(*request);
    }
  } else if (request->method == rpc_method::pin_file) {
    if (request->params.param_array.size() == 1u) {
      const auto api_path = utils::path::create_api_path(request->params.param_array[0]);
      auto success = provider_.is_file(api_path);
      if (success) {
        success = api_error::success ==
                  provider_.set_item_meta(api_path, META_PINNED, utils::string::from_bool(true));
      }

      if (success) {
        event_system::instance().raise<file_pinned>(api_path);
      }

      response = std::make_unique<jsonrpcpp::Response>(*request, json({{"success", success}}));
    } else {
      throw jsonrpcpp::InvalidParamsException(*request);
    }
  } else if (request->method == rpc_method::pinned_status) {
    if (request->params.param_array.size() == 1u) {
      const auto api_path = utils::path::create_api_path(request->params.param_array[0]);
      std::string pinned;
      const auto success =
          api_error::success == provider_.get_item_meta(api_path, META_PINNED, pinned);

      response = std::make_unique<jsonrpcpp::Response>(
          *request, json({{"success", success},
                          {"pinned", pinned.empty() ? false : utils::string::to_bool(pinned)}}));
    } else {
      throw jsonrpcpp::InvalidParamsException(*request);
    }
  } else if (request->method == rpc_method::unpin_file) {
    if (request->params.param_array.size() == 1u) {
      const auto api_path = utils::path::create_api_path(request->params.param_array[0]);
      auto success = provider_.is_file(api_path);
      if (success) {
        success = api_error::success ==
                  provider_.set_item_meta(api_path, META_PINNED, utils::string::from_bool(false));
      }

      if (success) {
        event_system::instance().raise<file_unpinned>(api_path);
      }

      response = std::make_unique<jsonrpcpp::Response>(*request, json({{"success", success}}));
    } else {
      throw jsonrpcpp::InvalidParamsException(*request);
    }
  } else if (request->method == rpc_method::get_open_files) {
    if (request->params.param_array.empty()) {
      const auto list = oft_.get_open_files();
      json open_files = {{"file_list", std::vector<json>()}};
      for (const auto &kv : list) {
        open_files["file_list"].emplace_back(json({{"path", kv.first}, {"count", kv.second}}));
      }
      response = std::make_unique<jsonrpcpp::Response>(*request, open_files);
    } else {
      throw jsonrpcpp::InvalidParamsException(*request);
    }
#if defined(REPERTORY_ENABLE_SKYNET)
  } else if (get_config().get_provider_type() == provider_type::skynet) {
    if (request->method == rpc_method::import) {
      if (request->params.param_array.size() == 1) {
        json results = {{"success", std::vector<json>()}, {"failed", std::vector<std::string>()}};
        for (const auto &link : request->params.param_array[0]) {
          const auto si = skylink_import::from_json(link);
          auto &provider = dynamic_cast<skynet_provider &>(provider_);
          const auto res = provider.import_skylink(si);
          if (res == api_error::success) {
            results["success"].emplace_back(link);
          } else {
            results["failed"].emplace_back(link["skylink"].get<std::string>());
          }
        }
        response = std::make_unique<jsonrpcpp::Response>(*request, results);
      } else {
        throw jsonrpcpp::InvalidParamsException(*request);
      }
    } else if (request->method == rpc_method::export_links) {
      if (request->params.param_array.empty()) {
        auto &provider = dynamic_cast<skynet_provider &>(provider_);
        response = std::make_unique<jsonrpcpp::Response>(*request, provider.export_all());
      } else if (request->params.param_array.size() == 1) {
        auto &provider = dynamic_cast<skynet_provider &>(provider_);
        response = std::make_unique<jsonrpcpp::Response>(
            *request,
            provider.export_list(request->params.param_array[0].get<std::vector<std::string>>()));
      } else {
        throw jsonrpcpp::InvalidParamsException(*request);
      }
    } else {
      handled = server::handle_request(request, response);
    }
#endif
  } else {
    handled = server::handle_request(request, response);
  }
  return handled;
}
} // namespace repertory
