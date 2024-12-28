// static void test_rename_file(const std::string &from_file_path,
//                              const std::string &to_file_path,
//                              bool is_rename_supported) {
//   std::cout << __FUNCTION__ << std::endl;
//   auto fd = open(from_file_path.c_str(), O_RDWR, S_IRUSR | S_IWUSR |
//   S_IRGRP); EXPECT_LE(1, fd); close(fd);
//
//   std::this_thread::sleep_for(SLEEP_SECONDS);
//
//   if (is_rename_supported) {
//     EXPECT_EQ(0, rename(from_file_path.c_str(), to_file_path.c_str()));
//     EXPECT_FALSE(utils::file::is_file(from_file_path));
//     EXPECT_TRUE(utils::file::is_file(to_file_path));
//   } else {
//     EXPECT_EQ(-1, rename(from_file_path.c_str(), to_file_path.c_str()));
//     EXPECT_TRUE(utils::file::is_file(from_file_path));
//     EXPECT_FALSE(utils::file::is_file(to_file_path));
//   }
// }
//
// static void test_rename_directory(const std::string &from_dir_path,
//                                   const std::string &to_dir_path,
//                                   bool is_rename_supported) {
//   std::cout << __FUNCTION__ << std::endl;
//   EXPECT_EQ(0, mkdir(from_dir_path.c_str(),
//                      S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP));
//   std::this_thread::sleep_for(SLEEP_SECONDS);
//
//   EXPECT_TRUE(utils::file::is_directory(from_dir_path));
//   if (is_rename_supported) {
//     EXPECT_EQ(0, rename(from_dir_path.c_str(), to_dir_path.c_str()));
//     EXPECT_FALSE(utils::file::is_directory(from_dir_path));
//     EXPECT_TRUE(utils::file::is_directory(to_dir_path));
//   } else {
//     EXPECT_EQ(-1, rename(from_dir_path.c_str(), to_dir_path.c_str()));
//     EXPECT_TRUE(utils::file::is_directory(from_dir_path));
//     EXPECT_FALSE(utils::file::is_directory(to_dir_path));
//   }
// }
//
// static void test_truncate(const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   EXPECT_EQ(0, truncate(file_path.c_str(), 10u));
//
//   std::uint64_t file_size{};
//   EXPECT_TRUE(utils::file::get_file_size(file_path, file_size));
//
//   EXPECT_EQ(std::uint64_t(10u), file_size);
// }
//
// static void test_ftruncate(const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   auto fd = open(file_path.c_str(), O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
//   EXPECT_LE(1, fd);
//
//   EXPECT_EQ(0, ftruncate(fd, 10u));
//
//   std::uint64_t file_size{};
//   EXPECT_TRUE(utils::file::get_file_size(file_path, file_size));
//
//   EXPECT_EQ(std::uint64_t(10u), file_size);
//
//   close(fd);
// }
//
// #if !defined(__APPLE__)
// static void test_fallocate(const std::string & /* api_path */,
//                            const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   auto file =
//       open(file_path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
//   EXPECT_LE(1, file);
//   EXPECT_EQ(0, fallocate(file, 0, 0, 16));
//
//   std::uint64_t file_size{};
//   EXPECT_TRUE(utils::file::get_file_size(file_path, file_size));
//   EXPECT_EQ(16U, file_size);
//
//   EXPECT_EQ(0, close(file));
//
//   file_size = 0U;
//   EXPECT_TRUE(utils::file::get_file_size(file_path, file_size));
//   EXPECT_EQ(16U, file_size);
// }
// #endif
//
// static void test_file_getattr(const std::string & /* api_path */,
//                               const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   auto fd =
//       open(file_path.c_str(), O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR |
//       S_IRGRP);
//   EXPECT_LE(1, fd);
//
//   EXPECT_EQ(0, close(fd));
//
//   struct stat64 unix_st {};
//   EXPECT_EQ(0, stat64(file_path.c_str(), &unix_st));
//   EXPECT_EQ(static_cast<std::uint32_t>(S_IRUSR | S_IWUSR | S_IRGRP),
//             ACCESSPERMS & unix_st.st_mode);
//   EXPECT_FALSE(S_ISDIR(unix_st.st_mode));
//   EXPECT_TRUE(S_ISREG(unix_st.st_mode));
// }
//
// static void test_directory_getattr(const std::string & /* api_path */,
//                                    const std::string &directory_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   EXPECT_EQ(0, mkdir(directory_path.c_str(),
//                      S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP));
//
//   struct stat64 unix_st {};
//   EXPECT_EQ(0, stat64(directory_path.c_str(), &unix_st));
//   EXPECT_EQ(static_cast<std::uint32_t>(S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP
//   |
//                                        S_IXGRP),
//             ACCESSPERMS & unix_st.st_mode);
//   EXPECT_TRUE(S_ISDIR(unix_st.st_mode));
//   EXPECT_FALSE(S_ISREG(unix_st.st_mode));
// }
//
// static void
// test_write_operations_fail_if_read_only(const std::string & /* api_path */,
//                                         const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   auto fd =
//       open(file_path.c_str(), O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR |
//       S_IRGRP);
//   EXPECT_LE(1, fd);
//
//   std::string data = "TestData";
//   EXPECT_EQ(-1, write(fd, data.data(), data.size()));
//
//   EXPECT_EQ(-1, ftruncate(fd, 9u));
//
// #if !defined(__APPLE__)
//   EXPECT_EQ(-1, fallocate(fd, 0, 0, 16));
// #endif
//
//   EXPECT_EQ(0, close(fd));
//
//   std::this_thread::sleep_for(SLEEP_SECONDS);
//
//   std::uint64_t file_size{};
//   EXPECT_TRUE(utils::file::get_file_size(file_path, file_size));
//   EXPECT_EQ(std::size_t(0u), file_size);
//
//   // filesystem_item fsi{};
//   // EXPECT_EQ(api_error::success,
//   //           provider.get_filesystem_item(api_path, false, fsi));
//   // EXPECT_TRUE(utils::file::get_file_size(fsi.source_path, file_size));
//   // EXPECT_EQ(std::size_t(0u), file_size);
// }
//
// #if !__APPLE__ && HAS_SETXATTR
// static void test_xattr_invalid_parameters(const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   std::string attr = "moose";
//   EXPECT_EQ(-1, setxattr(nullptr, "user.test_attr", attr.data(), attr.size(),
//                          XATTR_CREATE));
//   EXPECT_EQ(errno, EFAULT);
//   EXPECT_EQ(-1, setxattr(file_path.c_str(), nullptr, attr.data(),
//   attr.size(),
//                          XATTR_CREATE));
//   EXPECT_EQ(errno, EFAULT);
//   EXPECT_EQ(-1, setxattr(file_path.c_str(), "user.test_attr", nullptr,
//                          attr.size(), XATTR_CREATE));
//   EXPECT_EQ(errno, EFAULT);
//   EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", nullptr, 0,
//                         XATTR_CREATE));
// }
//
// static void test_xattr_create_and_get(const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   std::string attr = "moose";
//   EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
//                         attr.size(), XATTR_CREATE));
//
//   std::string val;
//   val.resize(attr.size());
//   EXPECT_EQ(attr.size(),
//             static_cast<std::size_t>(getxattr(
//                 file_path.c_str(), "user.test_attr", val.data(),
//                 val.size())));
//   EXPECT_STREQ(attr.c_str(), val.c_str());
// }
//
// static void test_xattr_listxattr(const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   std::string attr = "moose";
//   EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
//                         attr.size(), XATTR_CREATE));
//   EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr2", attr.data(),
//                         attr.size(), XATTR_CREATE));
//
//   std::string val;
//   val.resize(attr.size());
//   EXPECT_EQ(attr.size(),
//             static_cast<std::size_t>(getxattr(
//                 file_path.c_str(), "user.test_attr", val.data(),
//                 val.size())));
//   EXPECT_STREQ(attr.c_str(), val.c_str());
//
//   std::string data;
//   auto size = listxattr(file_path.c_str(), data.data(), 0U);
//   EXPECT_EQ(31, size);
//
//   data.resize(static_cast<std::size_t>(size));
//   EXPECT_EQ(size, listxattr(file_path.c_str(), data.data(),
//                             static_cast<std::size_t>(size)));
//
//   auto *ptr = data.data();
//   EXPECT_STREQ("user.test_attr", ptr);
//
//   ptr += strlen(ptr) + 1;
//   EXPECT_STREQ("user.test_attr2", ptr);
// }
//
// static void test_xattr_replace(const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   std::string attr = "moose";
//   EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
//                         attr.size(), XATTR_CREATE));
//
//   attr = "cow";
//   EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
//                         attr.size(), XATTR_REPLACE));
//
//   std::string val;
//   val.resize(attr.size());
//   EXPECT_EQ(attr.size(),
//             static_cast<std::size_t>(getxattr(
//                 file_path.c_str(), "user.test_attr", val.data(),
//                 val.size())));
//   EXPECT_STREQ(attr.c_str(), val.c_str());
// }
//
// static void test_xattr_default_create(const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   std::string attr = "moose";
//   EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
//                         attr.size(), 0));
//
//   std::string val;
//   val.resize(attr.size());
//   EXPECT_EQ(attr.size(),
//             static_cast<std::size_t>(getxattr(
//                 file_path.c_str(), "user.test_attr", val.data(),
//                 val.size())));
//   EXPECT_STREQ(attr.c_str(), val.c_str());
// }
//
// static void test_xattr_default_replace(const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   std::string attr = "moose";
//   EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
//                         attr.size(), 0));
//
//   attr = "cow";
//   EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
//                         attr.size(), 0));
//
//   std::string val;
//   val.resize(attr.size());
//   EXPECT_EQ(attr.size(),
//             static_cast<std::size_t>(getxattr(
//                 file_path.c_str(), "user.test_attr", val.data(),
//                 val.size())));
//   EXPECT_STREQ(attr.c_str(), val.c_str());
// }
//
// static void test_xattr_create_fails_if_exists(const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   std::string attr = "moose";
//   EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
//                         attr.size(), 0));
//   EXPECT_EQ(-1, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
//                          attr.size(), XATTR_CREATE));
//   EXPECT_EQ(EEXIST, errno);
// }
//
// static void
// test_xattr_create_fails_if_not_exists(const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   std::string attr = "moose";
//   EXPECT_EQ(-1, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
//                          attr.size(), XATTR_REPLACE));
//   EXPECT_EQ(ENODATA, errno);
// }
//
// static void test_xattr_removexattr(const std::string &file_path) {
//   std::cout << __FUNCTION__ << std::endl;
//   std::string attr = "moose";
//   EXPECT_EQ(0, setxattr(file_path.c_str(), "user.test_attr", attr.data(),
//                         attr.size(), XATTR_CREATE));
//
//   EXPECT_EQ(0, removexattr(file_path.c_str(), "user.test_attr"));
//
//   std::string val;
//   val.resize(attr.size());
//   EXPECT_EQ(-1, getxattr(file_path.c_str(), "user.test_attr", val.data(),
//                          val.size()));
//   EXPECT_EQ(ENODATA, errno);
// }
// #endif
//
// //         file_path = create_file_and_test(mount_location,
// //         "write_read_test");
// // test_write_and_read(utils::path::create_api_path("write_read_test"),
// //                             file_path);
// //         unlink_file_and_test(file_path);
// //
// //         file_path =
// //             create_file_and_test(mount_location, "from_rename_file_test");
// //         auto to_file_path =
// //             utils::path::combine(mount_location, {"to_rename_file_test"});
// //         test_rename_file(file_path, to_file_path,
// //                          provider_ptr->is_rename_supported());
// //         EXPECT_TRUE(utils::file::file(file_path).remove());
// //         EXPECT_TRUE(utils::file::file(to_file_path).remove());
// //
// //         file_path =
// //             utils::path::combine(mount_location,
// {"from_rename_dir_test"});
// //         to_file_path =
// //             utils::path::combine(mount_location, {"to_rename_dir_test"});
// //         test_rename_directory(file_path, to_file_path,
// //                               provider_ptr->is_rename_supported());
// // EXPECT_TRUE(utils::file::retry_delete_directory(file_path.c_str()));
// // EXPECT_TRUE(utils::file::retry_delete_directory(to_file_path.c_str()));
// //
// //         file_path = create_file_and_test(mount_location,
// //         "truncate_file_test"); test_truncate(file_path);
// //         unlink_file_and_test(file_path);
// //
// //         file_path = create_file_and_test(mount_location,
// //         "ftruncate_file_test"); test_ftruncate(file_path);
// //         unlink_file_and_test(file_path);
// //
// // #if !defined(__APPLE__)
// //         file_path = create_file_and_test(mount_location,
// //         "fallocate_file_test");
// // test_fallocate(utils::path::create_api_path("fallocate_file_test"),
// //                        file_path);
// //         unlink_file_and_test(file_path);
// // #endif
// //
// //         file_path = create_file_and_test(mount_location,
// //         "write_fails_ro_test"); test_write_operations_fail_if_read_only(
// //             utils::path::create_api_path("write_fails_ro_test"),
// //             file_path);
// //         unlink_file_and_test(file_path);
// //
// //         file_path = create_file_and_test(mount_location, "getattr.txt");
// //         test_file_getattr(utils::path::create_api_path("getattr.txt"),
// //                           file_path);
// //         unlink_file_and_test(file_path);
// //
// //         file_path = utils::path::combine(mount_location, {"getattr_dir"});
// // test_directory_getattr(utils::path::create_api_path("getattr_dir"),
// //                                file_path);
// //         rmdir_and_test(file_path);
// //
// // #if !__APPLE__ && HAS_SETXATTR
// //         file_path =
// //             create_file_and_test(mount_location,
// //             "xattr_invalid_names_test");
// //         test_xattr_invalid_parameters(file_path);
// //         unlink_file_and_test(file_path);
// //
// //         file_path =
// //             create_file_and_test(mount_location, "xattr_create_get_test");
// //         test_xattr_create_and_get(file_path);
// //         unlink_file_and_test(file_path);
// //
// //         file_path =
// //             create_file_and_test(mount_location, "xattr_listxattr_test");
// //         test_xattr_listxattr(file_path);
// //         unlink_file_and_test(file_path);
// //
// //         file_path = create_file_and_test(mount_location,
// //         "xattr_replace_test"); test_xattr_replace(file_path);
// //         unlink_file_and_test(file_path);
// //
// //         file_path =
// //             create_file_and_test(mount_location,
// //             "xattr_default_create_test");
// //         test_xattr_default_create(file_path);
// //         unlink_file_and_test(file_path);
// //
// //         file_path =
// //             create_file_and_test(mount_location,
// //             "xattr_default_replace_test");
// //         test_xattr_default_replace(file_path);
// //         unlink_file_and_test(file_path);
// //
// //         file_path = create_file_and_test(mount_location,
// // "xattr_create_fails_exists_test");
// //         test_xattr_create_fails_if_exists(file_path);
// //         unlink_file_and_test(file_path);
// //
// //         file_path = create_file_and_test(mount_location,
// // "xattr_create_fails_not_exists_test");
// //         test_xattr_create_fails_if_not_exists(file_path);
// //         unlink_file_and_test(file_path);
// //
// //         file_path =
// //             create_file_and_test(mount_location,
// "xattr_removexattr_test");
// //         test_xattr_removexattr(file_path);
// //         unlink_file_and_test(file_path);
// // #endif
