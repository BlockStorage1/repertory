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
#ifndef TESTS_MOCKS_MOCK_COMM_HPP_
#define TESTS_MOCKS_MOCK_COMM_HPP_

#include "test_common.hpp"
#include "comm/i_comm.hpp"

namespace repertory {
class mock_comm : public virtual i_comm {
private:
  struct mock_data {
    api_error error;
    json result;
    json json_error;
    bool persist = false;
  };
  std::unordered_map<std::string, std::deque<mock_data>> return_lookup_;

public:
  void push_return(const std::string &op, const std::string &path, const json &result,
                   const json &error, const api_error &apiError, bool persist = false) {
    const auto lookup_path = op + path;
    if (return_lookup_.find(lookup_path) == return_lookup_.end()) {
      return_lookup_[lookup_path] = std::deque<mock_data>();
    };
    mock_data data = {apiError, result, error, persist};
    return_lookup_[lookup_path].emplace_back(data);
  }

  void remove_return(const std::string &op, const std::string &path) {
    const auto lookup_path = op + path;
    return_lookup_.erase(lookup_path);
  }

private:
  api_error process(const std::string &op, const std::string &path, json &data, json &error) {
    const auto lookup_path = op + path;
    if ((return_lookup_.find(lookup_path) == return_lookup_.end()) ||
        return_lookup_[lookup_path].empty()) {
      EXPECT_STREQ("", ("unexpected path: " + lookup_path).c_str());
      return api_error::comm_error;
    }
    error = return_lookup_[lookup_path].front().json_error;
    data = return_lookup_[lookup_path].front().result;
    const auto ret = return_lookup_[lookup_path].front().error;
    if (not return_lookup_[lookup_path].front().persist) {
      return_lookup_[lookup_path].pop_front();
    }
    return ret;
  }

public:
  api_error get(const std::string &path, json &data, json &error) override {
    return process("get", path, data, error);
  }

  api_error get(const host_config &, const std::string &path, json &data, json &error) override {
    return process("get", path, data, error);
  }

  api_error get(const std::string &path, const http_parameters &, json &data,
                json &error) override {
    return process("get_params", path, data, error);
  }

  api_error get(const host_config &, const std::string &path, const http_parameters &, json &data,
                json &error) override {
    return process("get_params", path, data, error);
  }

  api_error get_range(const std::string & /*path*/, const std::uint64_t & /*data_size*/,
                      const http_parameters & /*parameters*/,
                      const std::string & /*encryption_token*/, std::vector<char> & /*data*/,
                      const http_ranges & /*ranges*/, json & /*error*/,
                      const bool & /*stop_requested*/) override {
    return api_error::error;
  }

  api_error get_range(const host_config & /*hc*/, const std::string & /*path*/,
                      const std::uint64_t & /*data_size*/, const http_parameters & /*parameters*/,
                      const std::string & /*encryption_token*/, std::vector<char> & /*data*/,
                      const http_ranges & /*ranges*/, json & /*error*/,
                      const bool & /*stop_requested*/) override {
    return api_error::error;
  }

  api_error get_range_and_headers(const std::string & /*path*/, const std::uint64_t & /*data_size*/,
                                  const http_parameters & /*parameters*/,
                                  const std::string & /*encryption_token*/,
                                  std::vector<char> & /*data*/, const http_ranges & /*ranges*/,
                                  json & /*error*/, http_headers & /*headers*/,
                                  const bool & /*stop_requested*/) override {
    return api_error::error;
  }

  api_error get_range_and_headers(const host_config & /*hc*/, const std::string & /*path*/,
                                  const std::uint64_t & /*data_size*/,
                                  const http_parameters & /*parameters*/,
                                  const std::string & /*encryption_token*/,
                                  std::vector<char> & /*data*/, const http_ranges & /*ranges*/,
                                  json & /*error*/, http_headers & /*headers*/,
                                  const bool & /*stop_requested*/) override {
    return api_error::error;
  }

  api_error get_raw(const std::string &, const http_parameters &, std::vector<char> &, json &,
                    const bool &) override {
    throw std::runtime_error("not implemented: get_raw");
    return api_error::comm_error;
  }

  api_error get_raw(const host_config &, const std::string &, const http_parameters &,
                    std::vector<char> &, json &, const bool &) override {
    throw std::runtime_error("not implemented: get_raw");
    return api_error::comm_error;
  }

  api_error post(const std::string &path, json &data, json &error) override {
    return process("post", path, data, error);
  }

  api_error post(const host_config &, const std::string &path, json &data, json &error) override {
    return process("post", path, data, error);
  }

  api_error post(const std::string &path, const http_parameters &, json &data,
                 json &error) override {
    return process("post_params", path, data, error);
  }

  api_error post(const host_config &, const std::string &path, const http_parameters &, json &data,
                 json &error) override {
    return process("post_params", path, data, error);
  }

  api_error post_file(const std::string &, const std::string &, const http_parameters &, json &,
                      json &, const bool &) override {
    return api_error::error;
  }

  api_error post_file(const host_config &, const std::string &, const std::string &,
                      const http_parameters &, json &, json &, const bool &) override {
    return api_error::error;
  }

  api_error post_multipart_file(const std::string &, const std::string &, const std::string &,
                                const std::string &, json &, json &, const bool &) override {
    return api_error::error;
  }

  api_error post_multipart_file(const host_config &, const std::string &, const std::string &,
                                const std::string &, const std::string &, json &, json &,
                                const bool &) override {
    return api_error::error;
  }
};
} // namespace repertory

#endif // TESTS_MOCKS_MOCK_COMM_HPP_
