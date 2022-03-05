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
#ifndef INCLUDE_DOWNLOAD_EVENTS_HPP_
#define INCLUDE_DOWNLOAD_EVENTS_HPP_

#include "common.hpp"
#include "events/events.hpp"
#include "types/repertory.hpp"

namespace repertory {
// clang-format off
E_SIMPLE2(download_begin, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING
);

E_SIMPLE4(download_end, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING,
  std::uint64_t, handle, handle, E_FROM_UINT64,
  api_error, result, result, E_FROM_API_FILE_ERROR
);

E_SIMPLE2(download_paused, normal, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING
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

E_SIMPLE3(download_store_failed, error, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING,
  std::string, error, err, E_STRING
);

E_SIMPLE2(download_timeout, warn, true,
  std::string, api_path, ap, E_STRING,
  std::string, dest_path, dest, E_STRING
);
// clang-format on
} // namespace repertory

#endif // INCLUDE_DOWNLOAD_EVENTS_HPP_
