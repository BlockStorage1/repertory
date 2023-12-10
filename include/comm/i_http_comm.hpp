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
#ifndef INCLUDE_COMM_I_HTTP_COMM_HPP_
#define INCLUDE_COMM_I_HTTP_COMM_HPP_

#include "comm/curl/requests/http_delete.hpp"
#include "comm/curl/requests/http_get.hpp"
#include "comm/curl/requests/http_head.hpp"
#include "comm/curl/requests/http_post.hpp"
#include "comm/curl/requests/http_put_file.hpp"
#include "types/repertory.hpp"

namespace repertory {
struct i_http_comm {
  INTERFACE_SETUP(i_http_comm);

public:
  virtual void enable_s3_path_style(bool enable) = 0;

  [[nodiscard]] virtual auto
  make_request(const curl::requests::http_delete &del, long &response_code,
               stop_type &stop_requested) const -> bool = 0;

  [[nodiscard]] virtual auto make_request(const curl::requests::http_get &get,
                                          long &response_code,
                                          stop_type &stop_requested) const
      -> bool = 0;

  [[nodiscard]] virtual auto make_request(const curl::requests::http_head &head,
                                          long &response_code,
                                          stop_type &stop_requested) const
      -> bool = 0;

  [[nodiscard]] virtual auto make_request(const curl::requests::http_post &post,
                                          long &response_code,
                                          stop_type &stop_requested) const
      -> bool = 0;

  [[nodiscard]] virtual auto
  make_request(const curl::requests::http_put_file &put_file,
               long &response_code, stop_type &stop_requested) const
      -> bool = 0;
};
} // namespace repertory

#endif // INCLUDE_COMM_I_HTTP_COMM_HPP_
