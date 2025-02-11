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
#include "utils/cli_utils.hpp"

#include "app_config.hpp"
#include "utils/collection.hpp"
#include "utils/common.hpp"
#include "utils/file.hpp"
#include "utils/path.hpp"
#include "utils/string.hpp"
#include "utils/utils.hpp"

namespace repertory::utils::cli {
void get_api_authentication_data(std::string &user, std::string &password,
                                 std::uint16_t &port, const provider_type &prov,
                                 const std::string &data_directory) {
  const auto cfg_file_path = utils::path::combine(
      data_directory.empty() ? app_config::default_data_directory(prov)
                             : data_directory,
      {"config.json"});

  json data;
  const auto success = utils::retry_action([&]() -> bool {
    return utils::file::read_json_file(cfg_file_path, data);
  });

  if (success) {
    if (user.empty() && password.empty()) {
      password = data[JSON_API_AUTH].get<std::string>();
      user = data[JSON_API_USER].get<std::string>();
    }
    port = data[JSON_API_PORT].get<std::uint16_t>();
  }
}

[[nodiscard]] auto get_provider_type_from_args(std::vector<const char *> args)
    -> provider_type {
  if (has_option(args, options::s3_option)) {
    return provider_type::s3;
  }
  if (has_option(args, options::remote_mount_option)) {
    return provider_type::remote;
  }
  if (has_option(args, options::encrypt_option)) {
    return provider_type::encrypt;
  }

  return provider_type::sia;
}

auto has_option(std::vector<const char *> args, const std::string &option_name)
    -> bool {
  return std::ranges::find_if(args, [&option_name](auto &&value) -> bool {
           return option_name == value;
         }) != args.end();
}

auto has_option(std::vector<const char *> args, const option &opt) -> bool {
  return has_option(args, opt.at(0U)) || has_option(args, opt.at(1U));
}

auto parse_option(std::vector<const char *> args,
                  const std::string &option_name, std::uint8_t count)
    -> std::vector<std::string> {
  std::vector<std::string> ret;
  auto found{false};
  for (std::size_t i = 0U; not found && (i < args.size()); i++) {
    if ((found = (option_name == args.at(i)))) {
      if ((++i + count) <= args.size()) {
        while ((count--) != 0U) {
          ret.emplace_back(args.at(i++));
        }
      }
    }
  }

  return ret;
}

auto parse_string_option(std::vector<const char *> args, const option &opt,
                         std::string &value) -> exit_code {
  exit_code ret{exit_code::success};
  if (has_option(args, opt.at(0U)) || has_option(args, opt.at(1U))) {
    auto data = parse_option(args, opt.at(0U), 1U);
    if (data.empty()) {
      data = parse_option(args, opt.at(1U), 1U);
      if (data.empty()) {
        std::cerr << "Invalid syntax for '" << opt.at(0U) << "," << opt.at(1U)
                  << '\'' << std::endl;
        ret = exit_code::invalid_syntax;
      }
    }

    if (not data.empty()) {
      value = data.at(0U);
    }
  }

  return ret;
}

auto parse_drive_options(std::vector<const char *> args,
                         [[maybe_unused]] provider_type &prov,
                         [[maybe_unused]] std::string &data_directory)
    -> std::vector<std::string> {
  // Strip out options from command line
  const auto &option_list = options::option_list;
  std::vector<std::string> drive_args;
  for (std::size_t i = 0U; i < args.size(); i++) {
    const auto &arg = args.at(i);
    if (std::ranges::find_if(option_list, [&arg](auto &&pair) -> bool {
          return ((pair.at(0U) == arg) || (pair.at(1U) == arg));
        }) == option_list.end()) {
      drive_args.emplace_back(args.at(i));
      continue;
    }

    if ((std::string(args.at(i)) == options::remote_mount_option.at(0U)) ||
        (std::string(args.at(i)) == options::remote_mount_option.at(1U)) ||
        (std::string(args.at(i)) == options::data_directory_option.at(0U)) ||
        (std::string(args.at(i)) == options::data_directory_option.at(1U)) ||
        (std::string(args.at(i)) == options::name_option.at(0U)) ||
        (std::string(args.at(i)) == options::name_option.at(1U))) {
      i++;
      continue;
    }
  }

#if !defined(_WIN32)
  std::vector<std::string> fuse_flags_list;
  for (std::size_t i = 1; i < drive_args.size(); i++) {
    if (drive_args.at(i).find("-o") == 0) {
      std::string options;
      if (drive_args[i].size() == 2) {
        if ((i + 1) < drive_args.size()) {
          options = drive_args.at(++i);
        }
      } else {
        options = drive_args.at(i).substr(2);
      }

      const auto fuse_option_list = utils::string::split(options, ',', true);
      for (const auto &fuse_option : fuse_option_list) {
        if (fuse_option.find("s3") == 0) {
          prov = provider_type::s3;
          continue;
        }
        if ((fuse_option.find("dd") == 0) ||
            (fuse_option.find("data_directory") == 0)) {
          const auto data = utils::string::split(fuse_option, '=', true);
          if (data.size() == 2U) {
            data_directory = data.at(1U);
          } else {
            std::cerr << "Invalid syntax for '-dd,--data_directory'"
                      << std::endl;
            exit(3);
          }
          continue;
        }

        fuse_flags_list.push_back(fuse_option);
      }
    }
  }

  {
    std::vector<std::string> new_drive_args(drive_args);
    for (std::size_t i = 1; i < new_drive_args.size(); i++) {
      if (new_drive_args[i].find("-o") == 0) {
        if (new_drive_args[i].size() == 2) {
          if ((i + 1) < drive_args.size()) {
            utils::collection::remove_element(new_drive_args,
                                              new_drive_args[i]);
            utils::collection::remove_element(new_drive_args,
                                              new_drive_args[i]);
            i--;
          }
          continue;
        }

        utils::collection::remove_element(new_drive_args, new_drive_args[i]);
        i--;
        continue;
      }
    }

#if FUSE_USE_VERSION < 30
    {
      auto it = std::remove_if(
          fuse_flags_list.begin(), fuse_flags_list.end(),
          [](const auto &opt) -> bool { return opt.find("hard_remove") == 0; });
      if (it == fuse_flags_list.end()) {
        fuse_flags_list.emplace_back("hard_remove");
      }
    }
#endif

#if defined(__APPLE__)
    {
      auto it = std::remove_if(
          fuse_flags_list.begin(), fuse_flags_list.end(),
          [](const auto &opt) -> bool { return opt.find("volname") == 0; });
      if (it != fuse_flags_list.end()) {
        fuse_flags_list.erase(it, fuse_flags_list.end());
      }
      fuse_flags_list.emplace_back("volname=Repertory" +
                                   app_config::get_provider_display_name(prov));

      it = std::remove_if(fuse_flags_list.begin(), fuse_flags_list.end(),
                          [](const auto &opt) -> bool {
                            return opt.find("daemon_timeout") == 0;
                          });
      if (it != fuse_flags_list.end()) {
        fuse_flags_list.erase(it, fuse_flags_list.end());
      }
      fuse_flags_list.emplace_back("daemon_timeout=300");

      it = std::remove_if(fuse_flags_list.begin(), fuse_flags_list.end(),
                          [](const auto &opt) -> bool {
                            return opt.find("noappledouble") == 0;
                          });
      if (it == fuse_flags_list.end()) {
        fuse_flags_list.emplace_back("noappledouble");
      }
    }
#endif // __APPLE__

    if (not fuse_flags_list.empty()) {
      std::string fuse_options;
      for (const auto &flag : fuse_flags_list) {
        fuse_options += (fuse_options.empty()) ? flag : (',' + flag);
      }
      new_drive_args.emplace_back("-o");
      new_drive_args.emplace_back(fuse_options);
    }
    drive_args = std::move(new_drive_args);
  }
#endif // _WIN32

  return drive_args;
}
} // namespace repertory::utils::cli
