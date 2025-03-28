/*
 Copyright <2018-2025> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

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
#include "test.hpp"

namespace repertory {
template <typename T, typename U>
constexpr bool is_decay_equ = std::is_same_v<std::decay_t<T>, U>;

TEST(utils_error, check_default_exception_handler) {
  EXPECT_TRUE(utils::error::get_exception_handler() != nullptr);
  if (&utils::error::default_exception_handler ==
      utils::error::get_exception_handler()) {
#if defined(PROJECT_ENABLE_SPDLOG) && defined(PROJECT_ENABLE_V2_ERRORS)
    auto default_handler_is_spdlog =
        is_decay_equ<decltype(utils::error::default_exception_handler),
                     utils::error::spdlog_exception_handler>;
    EXPECT_TRUE(default_handler_is_spdlog);
#else // !defined(PROJECT_ENABLE_SPDLOG) || !defined(PROJECT_ENABLE_V2_ERRORS)
    auto default_handler_is_iostream =
        is_decay_equ<decltype(utils::error::default_exception_handler),
                     utils::error::iostream_exception_handler>;
    EXPECT_TRUE(default_handler_is_iostream);
#endif
  }
}

TEST(utils_error, can_override_exception_handler) {
  struct my_exc_handler final : public utils::error::i_exception_handler {
#if defined(PROJECT_ENABLE_V2_ERRORS)
    MOCK_METHOD(void, handle_debug,
                (std::string_view function_name, std::string_view msg),
                (const, override));
#endif // defined(PROJECT_ENABLE_V2_ERRORS)

    MOCK_METHOD(void, handle_error,
                (std::string_view function_name, std::string_view msg),
                (const, override));

    MOCK_METHOD(void, handle_exception, (std::string_view function_name),
                (const, override));

    MOCK_METHOD(void, handle_exception,
                (std::string_view function_name, const std::exception &ex),
                (const, override));

#if defined(PROJECT_ENABLE_V2_ERRORS)
    MOCK_METHOD(void, handle_info,
                (std::string_view function_name, std::string_view msg),
                (const, override));

    MOCK_METHOD(void, handle_trace,
                (std::string_view function_name, std::string_view msg),
                (const, override));

    MOCK_METHOD(void, handle_warn,
                (std::string_view function_name, std::string_view msg),
                (const, override));
#endif // defined(PROJECT_ENABLE_V2_ERRORS)
  };

  my_exc_handler handler{};
  utils::error::set_exception_handler(&handler);

#if defined(PROJECT_ENABLE_V2_ERRORS)
  EXPECT_CALL(handler, handle_debug("test_func", "debug")).WillOnce(Return());
  utils::error::handle_debug("test_func", "debug");
#endif // defined(PROJECT_ENABLE_V2_ERRORS)

  EXPECT_CALL(handler, handle_error("test_func", "error")).WillOnce(Return());
  utils::error::handle_error("test_func", "error");

  EXPECT_CALL(handler, handle_exception("test_func")).WillOnce(Return());
  utils::error::handle_exception("test_func");

  auto ex = std::runtime_error("moose");
  EXPECT_CALL(handler, handle_exception(_, _))
      .WillOnce(
          [&ex](std::string_view function_name, const std::exception &ex2) {
            EXPECT_EQ("test_func_ex", function_name);
            EXPECT_STREQ(ex.what(), ex2.what());
          });
  utils::error::handle_exception("test_func_ex", ex);

#if defined(PROJECT_ENABLE_V2_ERRORS)
  EXPECT_CALL(handler, handle_info("test_func", "info")).WillOnce(Return());
  utils::error::handle_info("test_func", "info");

  EXPECT_CALL(handler, handle_trace("test_func", "trace")).WillOnce(Return());
  utils::error::handle_trace("test_func", "trace");

  EXPECT_CALL(handler, handle_warn("test_func", "warn")).WillOnce(Return());
  utils::error::handle_warn("test_func", "warn");
#endif // defined(PROJECT_ENABLE_V2_ERRORS)

  utils::error::set_exception_handler(&utils::error::default_exception_handler);
}
} // namespace repertory
