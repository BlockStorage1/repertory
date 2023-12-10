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
#ifndef TESTS_MOCKS_MOCK_UPLOAD_MANAGER_HPP_
#define TESTS_MOCKS_MOCK_UPLOAD_MANAGER_HPP_

#include "test_common.hpp"

#include "file_manager/i_upload_manager.hpp"

namespace repertory {
class mock_upload_manager : public i_upload_manager {
public:
  MOCK_METHOD(void, queue_upload, (const i_open_file &o), (override));

  MOCK_METHOD(void, remove_resume,
              (const std::string &api_path, const std::string &source_path),
              (override));

  MOCK_METHOD(void, remove_upload, (const std::string &api_path), (override));

  MOCK_METHOD(void, store_resume, (const i_open_file &o), (override));
};
} // namespace repertory

#endif // TESTS_MOCKS_MOCK_UPLOAD_MANAGER_HPP_
