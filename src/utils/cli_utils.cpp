/*
  Copyright <2018-2023> <scott.e.graves@protonmail.com>

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
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace repertory::utils::cli {
void get_api_authentication_data(std::string &user, std::string &password,
                                 std::uint16_t &port, const provider_type &pt,
                                 const std::string &data_directory) {
  const auto cfg_file_path = utils::path::combine(
      data_directory.empty() ? app_config::default_data_directory(pt)
                             : data_directory,
      {"config.json"});

  json data;
  const auto success = utils::retryable_action([&]() -> bool {
    return utils::file::read_json_file(cfg_file_path, data);
  });

  if (success) {
    if (user.empty() && password.empty()) {
      password = data["ApiAuth"].get<std::string>();
      user = data["ApiUser"].get<std::string>();
    }
    port = data["ApiPort"].get<std::uint16_t>();
  }
}

auto get_provider_type_from_args(int argc, char *argv[]) -> provider_type {
#if defined(REPERTORY_ENABLE_S3)
  if (has_option(argc, argv, options::s3_option)) {
    return provider_type::s3;
  }
#endif // defined(REPERTORY_ENABLE_S3)
  if (has_option(argc, argv, options::remote_mount_option)) {
    return provider_type::remote;
  }
  if (has_option(argc, argv, options::encrypt_option)) {
    return provider_type::encrypt;
  }

  return provider_type::sia;
}

auto has_option(int argc, char *argv[], const std::string &option_name)
    -> bool {
  auto ret = false;
  for (int i = 0; not ret && (i < argc); i++) {
    ret = (option_name == argv[i]);
  }
  return ret;
}

auto has_option(int argc, char *argv[], const option &opt) -> bool {
  return has_option(argc, argv, opt[0u]) || has_option(argc, argv, opt[1u]);
}

auto parse_option(int argc, char *argv[], const std::string &option_name,
                  std::uint8_t count) -> std::vector<std::string> {
  std::vector<std::string> ret;
  auto found = false;
  for (auto i = 0; not found && (i < argc); i++) {
    if ((found = (option_name == argv[i]))) {
      if ((++i + count) <= argc) {
        while (count--) {
          ret.emplace_back(argv[i++]);
        }
      }
    }
  }

  return ret;
}

auto parse_string_option(int argc, char **argv, const option &opt,
                         std::string &value) -> exit_code {
  auto ret = exit_code::success;
  if (has_option(argc, argv, opt[0u]) || has_option(argc, argv, opt[1u])) {
    auto data = parse_option(argc, argv, opt[0u], 1u);
    if (data.empty()) {
      data = parse_option(argc, argv, opt[1u], 1u);
      if (data.empty()) {
        std::cerr << "Invalid syntax for '" << opt[0u] << "," << opt[1u] << '\''
                  << std::endl;
        ret = exit_code::invalid_syntax;
      }
    }

    if (not data.empty()) {
      value = data[0u];
    }
  }

  return ret;
}

auto parse_drive_options(int argc, char **argv,
                         [[maybe_unused]] provider_type &pt,
                         [[maybe_unused]] std::string &data_directory)
    -> std::vector<std::string> {
  // Strip out options from command line
  const auto &option_list = options::option_list;
  std::vector<std::string> drive_args;
  for (int i = 0; i < argc; i++) {
    const auto &a = argv[i];
    if (std::find_if(option_list.begin(), option_list.end(),
                     [&a](const auto &pair) -> bool {
                       return ((pair[0] == a) || (pair[1] == a));
                     }) == option_list.end()) {
      drive_args.emplace_back(argv[i]);
    } else if ((std::string(argv[i]) == options::remote_mount_option[0]) ||
               (std::string(argv[i]) == options::remote_mount_option[1])) {
      i++;
    }
#ifdef REPERTORY_ENABLE_S3
    else if ((std::string(argv[i]) == options::name_option[0]) ||
             (std::string(argv[i]) == options::name_option[1])) {
      i++;
    }
#endif // REPERTORY_ENABLE_S3
  }

#ifndef _WIN32
  std::vector<std::string> fuse_flags_list;
  for (std::size_t i = 1; i < drive_args.size(); i++) {
    if (drive_args[i].find("-o") == 0) {
      std::string options = "";
      if (drive_args[i].size() == 2) {
        if ((i + 1) < drive_args.size()) {
          options = drive_args[++i];
        }
      } else {
        options = drive_args[i].substr(2);
      }

      const auto fuse_option_list = utils::string::split(options, ',');
      for (const auto &fuse_option : fuse_option_list) {
#if defined(REPERTORY_ENABLE_S3)
        if (fuse_option.find("s3") == 0) {
          pt = provider_type::s3;
        }
#endif // defined(REPERTORY_ENABLE_S3)
        if ((fuse_option.find("dd") == 0) ||
            (fuse_option.find("data_directory") == 0)) {
          const auto data = utils::string::split(fuse_option, '=');
          if (data.size() == 2) {
            data_directory = data[1];
          } else {
            std::cerr << "Invalid syntax for '-dd,--data_directory'"
                      << std::endl;
            exit(3);
          }
        } else {
          fuse_flags_list.push_back(fuse_option);
        }
      }
    }
  }

  {
    std::vector<std::string> new_drive_args(drive_args);
    for (std::size_t i = 1; i < new_drive_args.size(); i++) {
      if (new_drive_args[i].find("-o") == 0) {
        if (new_drive_args[i].size() == 2) {
          if ((i + 1) < drive_args.size()) {
            utils::remove_element_from(new_drive_args, new_drive_args[i]);
            utils::remove_element_from(new_drive_args, new_drive_args[i]);
            i--;
          }
        } else {
          utils::remove_element_from(new_drive_args, new_drive_args[i]);
          i--;
        }
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

#ifdef __APPLE__
    {
      auto it = std::remove_if(
          fuse_flags_list.begin(), fuse_flags_list.end(),
          [](const auto &opt) -> bool { return opt.find("volname") == 0; });
      if (it != fuse_flags_list.end()) {
        fuse_flags_list.erase(it, fuse_flags_list.end());
      }
      fuse_flags_list.emplace_back("volname=Repertory" +
                                   app_config::get_provider_display_name(pt));

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
