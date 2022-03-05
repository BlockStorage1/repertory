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
#include "types/skynet.hpp"
#include "utils/string_utils.hpp"

namespace repertory {
std::vector<host_config> skynet_config::from_string(const std::string &list) {
  const auto obj = json::parse(list);
  std::vector<host_config> ret;
  std::transform(obj.begin(), obj.end(), std::back_inserter(ret),
                 [](const json &portal) -> host_config {
                   host_config hc{};
                   hc.agent_string = portal["AgentString"].get<std::string>();
                   hc.api_password = portal["ApiPassword"].get<std::string>();
                   hc.api_port = portal["ApiPort"].get<std::uint16_t>();
                   hc.host_name_or_ip = portal["HostNameOrIp"].get<std::string>();
                   hc.path = portal["Path"].get<std::string>();
                   hc.protocol = portal["Protocol"].get<std::string>();
                   hc.timeout_ms = portal["TimeoutMs"].get<std::uint32_t>();
                   hc.auth_url = portal["AuthURL"].get<std::string>();
                   hc.auth_user = portal["AuthUser"].get<std::string>();
                   hc.auth_password = portal["AuthPassword"].get<std::string>();
                   return hc;
                 });

  return ret;
}

std::string skynet_config::to_string(const std::vector<host_config> &list) {
  json j;
  std::transform(list.begin(), list.end(), std::back_inserter(j), [](const auto &hc) -> json {
    return {
        {"AgentString", hc.agent_string},
        {"ApiPassword", hc.api_password},
        {"ApiPort", hc.api_port},
        {"HostNameOrIp", hc.host_name_or_ip},
        {"Path", hc.path},
        {"Protocol", hc.protocol},
        {"TimeoutMs", hc.timeout_ms},
        {"AuthURL", hc.auth_url},
        {"AuthUser", hc.auth_user},
        {"AuthPassword", hc.auth_password},
    };
  });

  return j.dump();
}

skylink_import skylink_import::from_json(const json &j) {
  return skylink_import{
      j.contains("directory") ? j["directory"].get<std::string>() : "/",
      j.contains("filename") ? j["filename"].get<std::string>() : "",
      j["skylink"].get<std::string>(),
      j.contains("token") ? j["token"].get<std::string>() : "",
  };
}

skylink_import skylink_import::from_string(const std::string &str) {
  auto parts = utils::string::split(str, ',');
  if (parts.empty() || (parts.size() > 4)) {
    throw std::runtime_error("unable to parse skylink from string: " + str);
  }

  std::unordered_map<std::string, std::string> linkData;
  for (auto &part : parts) {
    utils::string::replace(part, "@comma@", ",");
    auto parts2 = utils::string::split(part, '=', false);
    if (parts2.size() != 2) {
      throw std::runtime_error("unable to parse skylink from string: " + str);
    }

    for (auto &part2 : parts2) {
      utils::string::replace(part2, "@equal@", "=");
    }

    linkData[parts2[0]] = parts2[1];
  }

  return skylink_import{
      linkData["directory"],
      linkData["filename"],
      linkData["skylink"],
      linkData["token"],
  };
}

json skylink_import::to_json() const {
  return {
      {"directory", directory},
      {"filename", file_name},
      {"skylink", skylink},
      {"token", token},
  };
}
} // namespace repertory
