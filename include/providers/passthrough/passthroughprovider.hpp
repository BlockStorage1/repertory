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
#ifndef INCLUDE_PROVIDERS_PASSTHROUGH_PASSTHROUGHPROVIDER_HPP_
#define INCLUDE_PROVIDERS_PASSTHROUGH_PASSTHROUGHPROVIDER_HPP_
#if defined(REPERTORY_TESTING_NEW)

#include "common.hpp"
#include "providers/basebase_provider.hpp"

namespace repertory {
class app_config;
class CPassthroughProvider final : public base_provider {
public:
  explicit CPassthroughProvider(app_config &config)
      : base_provider(config), passthroughLocation_("" /*config.GetPassthroughConfig().Location*/) {
  }
  ~CPassthroughProvider() override = default;

private:
  const std::string passthroughLocation_;

private:
  std::string ConstructFullPath(const std::string &api_path) const;

protected:
  api_error notify_file_added(const std::string &api_path, const std::string &api_parent,
                              const std::uint64_t &size) override;

public:
  api_error CreateDirectory(const std::string &api_path, const api_meta_map &metaMap) override;

  api_error get_file_list(ApiFileList &fileList) const override;

  std::uint64_t get_directory_item_count(const std::string &api_path) const override;

  api_error get_directory_items(const std::string &api_path,
                                directory_item_list &list) const override;

  api_error GetFile(const std::string &api_path, ApiFile &file) const override;

  api_error GetFileSize(const std::string &api_path, std::uint64_t &fileSize) const override;

  api_error get_filesystem_item(const std::string &api_path, const bool &directory,
                                FileSystemItem &fileSystemItem) const override;

  api_error get_filesystem_item_from_source_path(const std::string &sourceFilePath,
                                                 FileSystemItem &fileSystemItem) const override;

  api_error get_item_meta(const std::string &api_path, api_meta_map &meta) const override;

  api_error get_item_meta(const std::string &api_path, const std::string &key,
                          std::string &value) const override;

  provider_type get_provider_type() const override { return provider_type::passthrough; }

  std::uint64_t get_total_drive_space() const override;

  std::uint64_t get_total_item_count() const override;

  std::uint64_t get_used_drive_space() const override;

  bool IsDirectory(const std::string &api_path) const override;

  bool IsFile(const std::string &api_path) const override;

  bool IsOnline() const override { return true; }

  bool is_rename_supported() const override { return true; }

  api_error read_file_bytes(const std::string &apiFilepath, const std::size_t &size,
                            const std::uint64_t &offset, std::vector<char> &data,
                            const bool &stop_requested) override;

  api_error RemoveDirectory(const std::string &api_path) override;

  api_error RemoveFile(const std::string &api_path) override;

  api_error RenameFile(const std::string &fromApiPath, const std::string &toApiPath) override;

  api_error remove_item_meta(const std::string &api_path, const std::string &key) override;

  api_error set_item_meta(const std::string &api_path, const std::string &key,
                          const std::string &value) override;

  api_error set_item_meta(const std::string &api_path, const api_meta_map &meta) override;

  api_error set_source_path(const std::string &api_path, const std::string &sourcePath) override;

  bool Start(api_item_added_callback apiItemAdded, i_open_file_table *openFileTable) override;

  void Stop() override;

  api_error upload_file(const std::string &api_path, const std::string &sourcePath,
                        const std::string &encryptionToken) override;
};
} // namespace repertory

#endif // REPERTORY_TESTING_NEW
#endif // INCLUDE_PROVIDERS_PASSTHROUGH_PASSTHROUGHPROVIDER_HPP_
