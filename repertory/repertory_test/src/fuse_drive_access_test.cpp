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
#if !defined(_WIN32)

#include "fixtures/drive_fixture.hpp"

namespace {
constexpr const auto access_permutations = {
    // clang-format off
    std::make_tuple(0000, R_OK, -1, EACCES),  // No permissions, R_OK
    std::make_tuple(0000, W_OK, -1, EACCES),  // No permissions, W_OK
    std::make_tuple(0000, X_OK, -1, EACCES),  // No permissions, X_OK
    std::make_tuple(0444, R_OK, 0, 0),        // Read-only, R_OK
    std::make_tuple(0444, W_OK, -1, EACCES),  // Read-only, W_OK
    std::make_tuple(0444, X_OK, -1, EACCES),  // Read-only, X_OK
    std::make_tuple(0222, R_OK, -1, EACCES),  // Write-only, R_OK
    std::make_tuple(0222, W_OK, 0, 0),        // Write-only, W_OK
    std::make_tuple(0222, X_OK, -1, EACCES),  // Write-only, X_OK
    std::make_tuple(0111, R_OK, -1, EACCES),  // Execute-only, R_OK
    std::make_tuple(0111, W_OK, -1, EACCES),  // Execute-only, W_OK
    std::make_tuple(0111, X_OK, 0, 0),        // Execute-only, X_OK
    std::make_tuple(0666, R_OK | W_OK, 0, 0), // Read and write, R_OK | W_OK
    std::make_tuple(0666, R_OK | X_OK, -1, EACCES), // Read and write, R_OK | X_OK
    std::make_tuple(0777, R_OK | W_OK | X_OK, 0, 0), // All permissions
    std::make_tuple(0555, R_OK | X_OK, 0, 0), // Read and execute, R_OK | X_OK
    std::make_tuple(0555, W_OK, -1, EACCES)   // Read and execute, W_OK
    // clang-format on
};

void perform_access_test(auto &&permutation, auto &&item_path) {
  mode_t permissions{};
  int access_mode{};
  int expected_result{};
  int expected_errno{};
  std::tie(permissions, access_mode, expected_result, expected_errno) =
      permutation;

  EXPECT_EQ(0, chmod(item_path.c_str(), permissions));

  auto result = access(item_path.c_str(), access_mode);

  EXPECT_EQ(result, expected_result)
      << "Expected access result to be " << expected_result
      << " for permissions " << std::oct << permissions << " and access mode "
      << access_mode;
  if (result == -1) {
    EXPECT_EQ(errno, expected_errno)
        << "Expected errno to be " << expected_errno << " for permissions "
        << std::oct << permissions;
  }
}
} // namespace

namespace repertory {
TYPED_TEST_SUITE(fuse_test, platform_provider_types);

TYPED_TEST(fuse_test, access_can_check_if_item_does_not_exist) {
  EXPECT_EQ(
      -1,
      access(utils::path::combine(this->mount_location, {"test_dir"}).c_str(),
             F_OK));
  EXPECT_EQ(ENOENT, utils::get_last_error_code());
}

TYPED_TEST(fuse_test, access_can_check_if_directory_exists) {
  std::string dir_name{"access_test"};
  auto dir_path = this->create_directory_and_test(dir_name);

  EXPECT_EQ(0, access(dir_path.c_str(), F_OK));

  this->rmdir_and_test(dir_path);
}

TYPED_TEST(fuse_test, access_can_check_if_file_exists) {
  std::string file_name{"access_test"};
  auto file_path = this->create_file_and_test(file_name);

  EXPECT_EQ(0, access(file_path.c_str(), F_OK));

  this->unlink_file_and_test(file_path);
}

TYPED_TEST(fuse_test, access_directory_permutations_test) {
  std::string dir_name{"access_test"};
  auto dir_path = this->create_directory_and_test(dir_name);

  for (const auto &permutation : access_permutations) {
    perform_access_test(permutation, dir_path);
  }

  this->rmdir_and_test(dir_path);
}

TYPED_TEST(fuse_test, access_file_permutations_test) {
  std::string file_name{"access_test"};
  auto file_path = this->create_file_and_test(file_name);

  for (const auto &permutation : access_permutations) {
    perform_access_test(permutation, file_path);
  }

  this->unlink_file_and_test(file_path);
}
} // namespace repertory

#endif // !defined(_WIN32)
