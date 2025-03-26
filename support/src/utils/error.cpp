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
#include "utils/error.hpp"

namespace repertory::utils::error {
auto create_error_message(std::vector<std::string_view> items) -> std::string {
  std::stringstream stream{};
  for (std::size_t idx = 0U; idx < items.size(); ++idx) {
    if (idx > 0) {
      stream << '|';
    }

    stream << items.at(idx);
  }

  return stream.str();
}

auto create_error_message(std::string_view function_name,
                          std::vector<std::string_view> items) -> std::string {
  items.insert(items.begin(), function_name);
  return create_error_message(items);
}

auto create_exception(std::string_view function_name,
                      std::vector<std::string_view> items)
    -> std::runtime_error {
  return std::runtime_error(create_error_message(function_name, items));
}

} // namespace repertory::utils::error
