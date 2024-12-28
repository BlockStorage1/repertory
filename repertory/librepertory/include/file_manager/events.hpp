/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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
#ifndef REPERTORY_INCLUDE_FILE_MANAGER_EVENTS_HPP_
#define REPERTORY_INCLUDE_FILE_MANAGER_EVENTS_HPP_

#include "events/events.hpp"
#include "types/repertory.hpp"

namespace repertory {
// clang-format off
E_SIMPLE2(download_begin, info, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, dest_path, dest, E_FROM_STRING
);

E_SIMPLE3(download_end, info, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, dest_path, dest, E_FROM_STRING,
  api_error, result, result, E_FROM_API_FILE_ERROR
);

E_SIMPLE3(download_progress, info, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, dest_path, dest, E_FROM_STRING,
  double, progress, prog, E_FROM_DOUBLE_PRECISE
);

E_SIMPLE2(download_restored, info, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, dest_path, dest, E_FROM_STRING
);

E_SIMPLE3(download_restore_failed, error, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, dest_path, dest, E_FROM_STRING,
  std::string, error, err, E_FROM_STRING
);

E_SIMPLE3(download_resume_add_failed, error, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, dest_path, dest, E_FROM_STRING,
  std::string, error, err, E_FROM_STRING
);

E_SIMPLE2(download_resume_added, debug, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, dest_path, dest, E_FROM_STRING
);

E_SIMPLE2(download_resume_removed, debug, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, dest_path, dest, E_FROM_STRING
);

E_SIMPLE1(item_timeout, trace, true,
  std::string, api_path, ap, E_FROM_STRING
);

E_SIMPLE3(download_type_selected, debug, true,
  std::string, api_path, ap, E_FROM_STRING,
  std::string, source, src, E_FROM_STRING,
  download_type, download_type, type, E_FROM_DOWNLOAD_TYPE
);
// clang-format on
} // namespace repertory

#endif // REPERTORY_INCLUDE_FILE_MANAGER_EVENTS_HPP_
