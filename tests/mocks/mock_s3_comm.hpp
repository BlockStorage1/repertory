/*
  Copyright <2018-2022> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
  OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef TESTS_MOCKS_MOCK_S3_COMM_HPP_
#define TESTS_MOCKS_MOCK_S3_COMM_HPP_
#ifdef REPERTORY_ENABLE_S3_TESTING

#include "test_common.hpp"
#include "comm/i_s3_comm.hpp"

namespace repertory {
/* class mock_s3_comm final : public i_s3_comm { */
/* private: */
/*   S3Config s3_config_; */
/*  */
/* public: */
/*   MOCK_METHOD1(create_bucket, api_error(const std::string &api_path)); */
/*   MOCK_CONST_METHOD1(Exists, bool(const std::string &api_path)); */
/*  */
/*   MOCK_CONST_METHOD3(get_bucket_name_and_object_name, */
/*                      void(const std::string &api_path, std::string &bucketName, */
/*                           std::string &objectName)); */
/*  */
/*   MOCK_CONST_METHOD2(get_directory_item_count, */
/*                      std::size_t(const std::string &api_path, */
/*                                  const MetaProviderCallback &metaProviderCallback)); */
/*  */
/*   MOCK_CONST_METHOD3(get_directory_items, */
/*                      api_error(const std::string &api_path, */
/*                                   const MetaProviderCallback &metaProviderCallback, */
/*                                   directory_item_list &list)); */
/*  */
/*   S3Config GetS3Config() override { return s3Config_; } */
/*  */
/*   S3Config GetS3Config() const override { return s3Config_; } */
/*  */
/*   MOCK_CONST_METHOD2(GetFile, api_error(const std::string &api_path, ApiFile &apiFile)); */
/*  */
/*   MOCK_CONST_METHOD1(get_file_list, api_error(ApiFileList &apiFileList)); */
/*  */
/*   bool IsOnline() const override { return true; } */
/*  */
/*   MOCK_CONST_METHOD5(read_file_bytes, */
/*                      api_error(const std::string &path, const std::size_t &size, */
/*                                   const std::uint64_t &offset, std::vector<char> &data, */
/*                                   const bool &stop_requested)); */
/*  */
/*   MOCK_METHOD1(remove_bucket, api_error(const std::string &api_path)); */
/*  */
/*   MOCK_METHOD1(RemoveFile, api_error(const std::string &api_path)); */
/*  */
/*   MOCK_METHOD2(RenameFile, */
/*                api_error(const std::string &api_path, const std::string &newApiFilePath));
 */
/*  */
/*   void SetS3Config(S3Config s3Config) { s3Config_ = std::move(s3Config); } */
/*  */
/*   MOCK_METHOD3(upload_file, api_error(const std::string &api_path, */
/*                                         const std::string &sourcePath, const bool
 * &stop_requested)); */
/* }; */
} // namespace repertory

#endif // REPERTORY_ENABLE_S3_TESTING
#endif // TESTS_MOCKS_MOCK_S3_COMM_HPP_
