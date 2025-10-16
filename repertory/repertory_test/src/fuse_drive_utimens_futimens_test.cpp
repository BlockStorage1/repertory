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

using repertory::utils::time::NANOS_PER_SECOND;

namespace {
void get_times_ns(std::string_view path, long long &at_ns, long long &mt_ns) {
  struct stat st_unix{};
  ASSERT_EQ(0, ::stat(std::string{path}.c_str(), &st_unix));

#if defined(__APPLE__)
  at_ns = static_cast<long long>(st_unix.st_atimespec.tv_sec) *
              static_cast<long long>(NANOS_PER_SECOND) +
          static_cast<long long>(st_unix.st_atimespec.tv_nsec);
  mt_ns = static_cast<long long>(st_unix.st_mtimespec.tv_sec) *
              static_cast<long long>(NANOS_PER_SECOND) +
          static_cast<long long>(st_unix.st_mtimespec.tv_nsec);
#else  // !defined(__APPLE__)
  at_ns = static_cast<long long>(st_unix.st_atim.tv_sec) *
              static_cast<long long>(NANOS_PER_SECOND) +
          static_cast<long long>(st_unix.st_atim.tv_nsec);
  mt_ns = static_cast<long long>(st_unix.st_mtim.tv_sec) *
              static_cast<long long>(NANOS_PER_SECOND) +
          static_cast<long long>(st_unix.st_mtim.tv_nsec);
#endif // defined(__APPLE__)
}

[[nodiscard]] auto ts_make(time_t sec, long long nsec) -> timespec {
  return timespec{
      .tv_sec = sec,
      .tv_nsec = nsec,
  };
}

[[nodiscard]] auto to_ns(const timespec &spec) -> long long {
  return static_cast<long long>(spec.tv_sec) *
             static_cast<long long>(NANOS_PER_SECOND) +
         static_cast<long long>(spec.tv_nsec);
}

[[nodiscard]] auto now_ns() -> long long {
  timespec spec{};
#if defined(CLOCK_REALTIME)
  clock_gettime(CLOCK_REALTIME, &spec);
#else  // defined(CLOCK_REALTIME)
  timeval val{};
  gettimeofday(&val, nullptr);
  spec.tv_sec = val.tv_sec;
  spec.tv_nsec = val.tv_usec * 1000;
#endif // !defined(CLOCK_REALTIME)
  return to_ns(spec);
}

constexpr long long GRANULAR_TOL_NS =
    12LL * static_cast<long long>(NANOS_PER_SECOND);
constexpr long long NOW_TOL_NS =
    15LL * static_cast<long long>(NANOS_PER_SECOND);
} // namespace

namespace repertory {
using std::strerror;

TYPED_TEST_SUITE(fuse_test, platform_provider_types);

TYPED_TEST(fuse_test, utimens_set_both_times_specific_values) {
  std::string name{"utimens"};
  auto src = this->create_file_and_test(name);

  long long at0{};
  long long mt0{};
  get_times_ns(src, at0, mt0);

  auto now = now_ns();
  auto target_at =
      now - 3600LL * static_cast<long long>(NANOS_PER_SECOND) + 111111111LL;
  auto target_mt =
      now - 1800LL * static_cast<long long>(NANOS_PER_SECOND) + 222222222LL;

  auto spec = std::array<timespec, 2U>{
      ts_make(static_cast<time_t>(target_at /
                                  static_cast<long long>(NANOS_PER_SECOND)),
              static_cast<long>(target_at %
                                static_cast<long long>(NANOS_PER_SECOND))),
      ts_make(static_cast<time_t>(target_mt /
                                  static_cast<long long>(NANOS_PER_SECOND)),
              static_cast<long>(target_mt %
                                static_cast<long long>(NANOS_PER_SECOND))),
  };

  errno = 0;
  ASSERT_EQ(0, ::utimensat(AT_FDCWD, src.c_str(), spec.data(), 0));

  long long at1{};
  long long mt1{};
  get_times_ns(src, at1, mt1);

  EXPECT_LE(std::abs(at1 - target_at), GRANULAR_TOL_NS);
  EXPECT_LE(std::abs(mt1 - target_mt), GRANULAR_TOL_NS);

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, utimens_set_atime_only_omit_mtime) {
  std::string name{"utimens"};
  auto src = this->create_file_and_test(name);

  long long at_before{};
  long long mt_before{};
  get_times_ns(src, at_before, mt_before);

  long long target_at =
      now_ns() - 10LL * static_cast<long long>(NANOS_PER_SECOND);

  auto spec = std::array<timespec, 2U>{
      ts_make(static_cast<time_t>(target_at /
                                  static_cast<long long>(NANOS_PER_SECOND)),
              static_cast<long>(target_at %
                                static_cast<long long>(NANOS_PER_SECOND))),
      ts_make(0, UTIME_OMIT),
  };

  errno = 0;
  ASSERT_EQ(0, ::utimensat(AT_FDCWD, src.c_str(), spec.data(), 0));

  long long at_after{};
  long long mt_after{};
  get_times_ns(src, at_after, mt_after);

  EXPECT_LE(std::abs(at_after - target_at), GRANULAR_TOL_NS);
  EXPECT_LE(std::abs(mt_after - mt_before), GRANULAR_TOL_NS);

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, utimens_set_mtime_only_omit_atime) {
  std::string name{"utimens"};
  auto src = this->create_file_and_test(name);

  long long at_before{};
  long long mt_before{};
  get_times_ns(src, at_before, mt_before);

  auto target_mt = now_ns() - 30LL * static_cast<long long>(NANOS_PER_SECOND);

  auto spec = std::array<timespec, 2U>{
      ts_make(0, UTIME_OMIT),
      ts_make(static_cast<time_t>(target_mt /
                                  static_cast<long long>(NANOS_PER_SECOND)),
              static_cast<long>(target_mt %
                                static_cast<long long>(NANOS_PER_SECOND))),
  };

  errno = 0;
  ASSERT_EQ(0, ::utimensat(AT_FDCWD, src.c_str(), spec.data(), 0));

  long long at_after{};
  long long mt_after{};
  get_times_ns(src, at_after, mt_after);

  EXPECT_LE(std::abs(mt_after - target_mt), GRANULAR_TOL_NS);
  EXPECT_LE(std::abs(at_after - at_before), GRANULAR_TOL_NS);

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, utimens_set_now_for_both) {
  std::string name{"utimens"};
  auto src = this->create_file_and_test(name);

  auto spec = std::array<timespec, 2U>{
      ts_make(0, UTIME_NOW),
      ts_make(0, UTIME_NOW),
  };

  errno = 0;
  ASSERT_EQ(0, ::utimensat(AT_FDCWD, src.c_str(), spec.data(), 0));

  auto now_after = now_ns();
  long long access_time{};
  long long modified_time{};
  get_times_ns(src, access_time, modified_time);

  EXPECT_LE(std::abs(access_time - now_after), NOW_TOL_NS);
  EXPECT_LE(std::abs(modified_time - now_after), NOW_TOL_NS);

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, utimens_nonexistent_path_returns_enoent) {
  std::string file_name{"utimens"};
  auto missing = this->create_file_path(file_name);

  auto spec = std::array<timespec, 2U>{
      ts_make(123, 0),
      ts_make(456, 0),
  };

  errno = 0;
  EXPECT_EQ(-1, ::utimensat(AT_FDCWD, missing.c_str(), spec.data(), 0));
  EXPECT_EQ(ENOENT, errno);
}

TYPED_TEST(fuse_test, utimens_invalid_nsec_returns_einval) {
  std::string name{"utimens"};
  auto src = this->create_file_and_test(name);

  auto spec = std::array<timespec, 2U>{
      ts_make(0, static_cast<long long>(NANOS_PER_SECOND)),
      ts_make(0, 0),
  };

  errno = 0;
  EXPECT_EQ(-1, ::utimensat(AT_FDCWD, src.c_str(), spec.data(), 0));
  EXPECT_EQ(EINVAL, errno);

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, futimens_set_both_times_specific_values) {
  std::string name{"futimens"};
  auto src = this->create_file_and_test(name);

  auto desc = ::open(src.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  auto now = now_ns();
  auto target_at =
      now - 7200LL * static_cast<long long>(NANOS_PER_SECOND) + 333333333LL;
  auto target_mt =
      now - 600LL * static_cast<long long>(NANOS_PER_SECOND) + 444444444LL;

  auto spec = std::array<timespec, 2U>{
      ts_make(static_cast<time_t>(target_at /
                                  static_cast<long long>(NANOS_PER_SECOND)),
              static_cast<long>(target_at %
                                static_cast<long long>(NANOS_PER_SECOND))),
      ts_make(static_cast<time_t>(target_mt /
                                  static_cast<long long>(NANOS_PER_SECOND)),
              static_cast<long>(target_mt %
                                static_cast<long long>(NANOS_PER_SECOND))),
  };

  errno = 0;
  ASSERT_EQ(0, ::futimens(desc, spec.data()));
  ::close(desc);

  long long access_time{};
  long long modified_time{};
  get_times_ns(src, access_time, modified_time);
  EXPECT_LE(std::abs(access_time - target_at), GRANULAR_TOL_NS);
  EXPECT_LE(std::abs(modified_time - target_mt), GRANULAR_TOL_NS);

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, futimens_set_mtime_only_omit_atime) {
  std::string name{"futimens"};
  auto src = this->create_file_and_test(name);

  long long at_before{};
  long long mt_before{};
  get_times_ns(src, at_before, mt_before);

  auto desc = ::open(src.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  auto target_mt = now_ns() - 20LL * static_cast<long long>(NANOS_PER_SECOND);

  auto spec = std::array<timespec, 2U>{
      ts_make(0, UTIME_OMIT),
      ts_make(static_cast<time_t>(target_mt /
                                  static_cast<long long>(NANOS_PER_SECOND)),
              static_cast<long>(target_mt %
                                static_cast<long long>(NANOS_PER_SECOND))),
  };

  errno = 0;
  ASSERT_EQ(0, ::futimens(desc, spec.data()));
  ::close(desc);

  long long at_after{};
  long long mt_after{};
  get_times_ns(src, at_after, mt_after);

  EXPECT_LE(std::abs(mt_after - target_mt), GRANULAR_TOL_NS);
  EXPECT_LE(std::abs(at_after - at_before), GRANULAR_TOL_NS);

  this->unlink_file_and_test(src);
}

TYPED_TEST(fuse_test, futimens_invalid_nsec_returns_einval) {
  std::string name{"futimens"};
  auto src = this->create_file_and_test(name);

  auto desc = ::open(src.c_str(), O_RDWR);
  ASSERT_NE(desc, -1);

  auto spec = std::array<timespec, 2U>{
      ts_make(0, 0),
      ts_make(0, static_cast<long long>(NANOS_PER_SECOND)),
  };

  errno = 0;
  EXPECT_EQ(-1, ::futimens(desc, spec.data()));
  EXPECT_EQ(EINVAL, errno);

  ::close(desc);
  this->unlink_file_and_test(src);
}
} // namespace repertory

#endif // !defined(_WIN32)
