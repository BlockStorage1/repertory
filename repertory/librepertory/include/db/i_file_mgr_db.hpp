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
#ifndef REPERTORY_INCLUDE_DB_I_FILE_MGR_DB_HPP_
#define REPERTORY_INCLUDE_DB_I_FILE_MGR_DB_HPP_

#include "types/repertory.hpp"

namespace repertory {
class i_file_mgr_db {
  INTERFACE_SETUP(i_file_mgr_db);

public:
  struct resume_entry final {
    std::string api_path;
    std::uint64_t chunk_size{};
    boost::dynamic_bitset<> read_state;
    std::string source_path;
  };

  struct upload_active_entry final {
    std::string api_path;
    std::string source_path;
  };

  using upload_entry = upload_active_entry;

public:
  [[nodiscard]] virtual auto add_resume(const resume_entry &entry) -> bool = 0;

  [[nodiscard]] virtual auto add_upload(const upload_entry &entry) -> bool = 0;

  [[nodiscard]] virtual auto add_upload_active(const upload_active_entry &entry)
      -> bool = 0;

  virtual void clear() = 0;

  [[nodiscard]] virtual auto get_next_upload() const
      -> std::optional<upload_entry> = 0;

  [[nodiscard]] virtual auto get_resume_list() const
      -> std::vector<resume_entry> = 0;

  [[nodiscard]] virtual auto get_upload(const std::string &api_path) const
      -> std::optional<upload_entry> = 0;

  [[nodiscard]] virtual auto get_upload_active_list() const
      -> std::vector<upload_active_entry> = 0;

  [[nodiscard]] virtual auto remove_resume(const std::string &api_path)
      -> bool = 0;

  [[nodiscard]] virtual auto remove_upload(const std::string &api_path)
      -> bool = 0;

  [[nodiscard]] virtual auto remove_upload_active(const std::string &api_path)
      -> bool = 0;

  [[nodiscard]] virtual auto rename_resume(const std::string &from_api_path,
                                           const std::string &to_api_path)
      -> bool = 0;
};
} // namespace repertory

#endif // REPERTORY_INCLUDE_DB_I_FILE_MGR_DB_HPP_
