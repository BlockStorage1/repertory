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
#if defined(REPERTORY_TESTING_NEW)
#include "providers/passthrough/passthroughprovider.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"

namespace repertory {
std::string CPassthroughProvider::ConstructFullPath(const std::string &api_path) const {
  return utils::path::combine(passthroughLocation_, {api_path});
}

api_error CPassthroughProvider::CreateDirectory(const std::string &api_path,
                                                const api_meta_map &meta) {
  const auto fullPath = ConstructFullPath(api_path);
  auto ret = utils::file::create_full_directory_path(fullPath) ? api_error::OSErrorCode
                                                               : api_error::Success;
  if (ret == api_error::Success) {
    ret = set_item_meta(api_path, meta);
  }

  return ret;
}

api_error CPassthroughProvider::get_file_list(ApiFileList &fileList) const {
  const auto fullPath = ConstructFullPath("/");
  const auto fl = utils::file::get_directory_files(fullPath, false, true);
  for (const auto &file : fl) {
    const auto api_path = utils::path::create_api_path(file.substr(fullPath.length()));
    /*
      struct ApiFile {
        std::string ApiFilePath{};
        std::string ApiParent{};
        std::uint64_t AccessedDate = 0;
        std::uint64_t ChangedDate = 0;
        std::uint64_t CreationDate = 0;
        std::string EncryptionToken{};
        std::uint64_t FileSize = 0;
        std::uint64_t ModifiedDate = 0;
        bool Recoverable = false;
        double Redundancy = 0.0;
        std::string SourceFilePath{};
      };
    */
    ApiFile apiFile{
        api_path,
        utils::path::get_parent_api_path(api_path),
    };
    // apiFile.Recoverable = not IsProcessing(api_path);
    apiFile.Redundancy = 3.0;
    apiFile.SourceFilePath = file;
    // utils::file::UpdateApiFileInfo(apiFile);
    fileList.emplace_back(apiFile);
  }

  return api_error::Success;
}

std::uint64_t CPassthroughProvider::get_directory_item_count(const std::string &api_path) const {
  const auto fullPath = ConstructFullPath(api_path);
  return 0;
}

api_error CPassthroughProvider::get_directory_items(const std::string &api_path,
                                                    directory_item_list &list) const {
  return api_error::NotImplemented;
}

api_error CPassthroughProvider::GetFile(const std::string &api_path, ApiFile &file) const {
  return api_error::Error;
}

api_error CPassthroughProvider::GetFileSize(const std::string &api_path,
                                            std::uint64_t &fileSize) const {
  return api_error::Error;
}

api_error CPassthroughProvider::get_filesystem_item(const std::string &api_path,
                                                    const bool &directory,
                                                    FileSystemItem &fileSystemItem) const {
  return api_error::NotImplemented;
}

api_error
CPassthroughProvider::get_filesystem_item_from_source_path(const std::string &sourceFilePath,
                                                           FileSystemItem &fileSystemItem) const {
  return api_error::NotImplemented;
}

api_error CPassthroughProvider::get_item_meta(const std::string &api_path,
                                              api_meta_map &meta) const {
  return api_error::NotImplemented;
}

api_error CPassthroughProvider::get_item_meta(const std::string &api_path,
                                              const std::string &key, std::string &value) const {
  return api_error::NotImplemented;
}

std::uint64_t CPassthroughProvider::get_total_drive_space() const { return 0; }

std::uint64_t CPassthroughProvider::get_total_item_count() const { return 0; }

std::uint64_t CPassthroughProvider::get_used_drive_space() const { return 0; }

bool CPassthroughProvider::IsDirectory(const std::string &api_path) const {
  const auto fullPath = ConstructFullPath(api_path);
  return utils::file::is_directory(fullPath);
}

bool CPassthroughProvider::IsFile(const std::string &api_path) const {
  const auto fullPath = ConstructFullPath(api_path);
  return utils::file::is_file(fullPath);
}

api_error CPassthroughProvider::notify_file_added(const std::string &api_path,
                                                  const std::string &api_parent,
                                                  const std::uint64_t &size) {
  return api_error::NotImplemented;
}

api_error CPassthroughProvider::read_file_bytes(const std::string &apiFilepath,
                                                const std::size_t &size,
                                                const std::uint64_t &offset,
                                                std::vector<char> &data,
                                                const bool &stop_requested) {
  return api_error::NotImplemented;
}

api_error CPassthroughProvider::RemoveDirectory(const std::string &api_path) {
  return api_error::NotImplemented;
}

api_error CPassthroughProvider::RemoveFile(const std::string &api_path) {
  return api_error::NotImplemented;
}

api_error CPassthroughProvider::RenameFile(const std::string &fromApiPath,
                                           const std::string &toApiPath) {
  return api_error::NotImplemented;
}

api_error CPassthroughProvider::remove_item_meta(const std::string &api_path,
                                                 const std::string &key) {
  return api_error::NotImplemented;
}

api_error CPassthroughProvider::set_item_meta(const std::string &api_path,
                                              const std::string &key, const std::string &value) {
  return api_error::NotImplemented;
}

api_error CPassthroughProvider::set_item_meta(const std::string &api_path,
                                              const api_meta_map &meta) {
  return api_error::NotImplemented;
}

api_error CPassthroughProvider::set_source_path(const std::string &api_path,
                                                const std::string &sourcePath) {
  return api_error::NotImplemented;
}

bool CPassthroughProvider::Start(ApiItemAdded apiItemAdded, i_open_file_table *openFileTable) {
  return false;
}

void CPassthroughProvider::Stop() {}

api_error CPassthroughProvider::upload_file(const std::string &api_path,
                                            const std::string &sourcePath,
                                            const std::string &encryptionToken) {
  return api_error::NotImplemented;
}
} // namespace repertory

#endif // defined(REPERTORY_TESTING_NEW)
