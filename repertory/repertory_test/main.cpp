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
#if defined(PROJECT_ENABLE_BACKWARD_CPP)
#include "backward.hpp"
#endif // defined(PROJECT_ENABLE_BACKWARD_CPP)

#include "initialize.hpp"
#include "test_common.hpp"
#include "utils/error.hpp"

using namespace repertory;

int PROJECT_TEST_RESULT{0};

auto main(int argc, char **argv) -> int {
  REPERTORY_USES_FUNCTION_NAME();

#if defined(PROJECT_ENABLE_BACKWARD_CPP)
  static backward::SignalHandling sh;
#endif // defined(PROJECT_ENABLE_BACKWARD_CPP)

  if (not repertory::project_initialize()) {
    repertory::project_cleanup();
    return -1;
  }

  try {
    ::testing::InitGoogleTest(&argc, argv);
    PROJECT_TEST_RESULT = RUN_ALL_TESTS();
  } catch (const std::exception &e) {
    utils::error::handle_exception(function_name, e);
  } catch (...) {
    utils::error::handle_exception(function_name);
  }

  repertory::project_cleanup();

  return PROJECT_TEST_RESULT;
}
