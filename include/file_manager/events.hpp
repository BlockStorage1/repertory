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
#ifndef INCLUDE_FILE_MANAGER_EVENTS_HPP_
#define INCLUDE_FILE_MANAGER_EVENTS_HPP_

#include "events/events.hpp"
#include "types/repertory.hpp"

namespace repertory {
// clang-format off
E_SIMPLE2(download_begin, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING
);

E_SIMPLE5(download_chunk_begin, debug, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING,
  std::size_t, chunk, chunk, E_FROM_SIZE_T,
  std::size_t, total, total, E_FROM_SIZE_T,
  std::size_t, complete, complete, E_FROM_SIZE_T
);

E_SIMPLE6(download_chunk_end, debug, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING,
  std::size_t, chunk, chunk, E_FROM_SIZE_T,
  std::size_t, total, total, E_FROM_SIZE_T,
  std::size_t, complete, complete, E_FROM_SIZE_T,
  api_error, result, result, E_FROM_API_FILE_ERROR
);

E_SIMPLE3(download_end, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING,
  api_error, result, result, E_FROM_API_FILE_ERROR
);

E_SIMPLE3(download_progress, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING,
  double, progress, prog, E_DOUBLE_PRECISE
);

E_SIMPLE2(download_restored, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING
);

E_SIMPLE3(download_restore_failed, error, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING,
  std::string, error, err, E_STRING
);

E_SIMPLE2(download_resumed, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING
);

E_SIMPLE2(download_stored, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING
);

E_SIMPLE2(download_stored_removed, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING
);

E_SIMPLE3(download_stored_failed, error, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING,
  std::string, error, err, E_STRING
);

E_SIMPLE1(item_timeout, normal, true,
  std::string, api_path, ap, E_STRING
);
// clang-format on
} // namespace repertory

#endif // INCLUDE_FILE_MANAGER_EVENTS_HPP_
