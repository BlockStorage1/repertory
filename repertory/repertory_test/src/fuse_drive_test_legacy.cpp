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
