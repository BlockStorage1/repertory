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
#ifndef INCLUDE_DRIVES_FUSE_EVENTS_HPP_
#define INCLUDE_DRIVES_FUSE_EVENTS_HPP_

#include "events/event_system.hpp"

namespace repertory {
// clang-format off
E_SIMPLE3(fuse_event, debug, true,
  std::string, function, func, E_STRING,
  std::string, api_path, ap, E_STRING,
  int, result, res, E_FROM_INT32
);

E_SIMPLE1(fuse_args_parsed, normal, true,
  std::string, arguments, args, E_STRING
);
// clang-format on
} // namespace repertory

#endif // INCLUDE_DRIVES_FUSE_EVENTS_HPP_
