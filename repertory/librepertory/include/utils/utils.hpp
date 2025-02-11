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
#ifndef REPERTORY_INCLUDE_UTILS_UTILS_HPP_
#define REPERTORY_INCLUDE_UTILS_UTILS_HPP_

#include "types/repertory.hpp"

namespace repertory {
class app_config;

namespace utils {
void calculate_allocation_size(bool directory, std::uint64_t file_size,
                               UINT64 allocation_size,
                               std::string &allocation_meta_size);

[[nodiscard]] auto
create_rocksdb(const app_config &cfg, const std::string &name,
               const std::vector<rocksdb::ColumnFamilyDescriptor> &families,
               std::vector<rocksdb::ColumnFamilyHandle *> &handles, bool clear)
    -> std::unique_ptr<rocksdb::TransactionDB>;

[[nodiscard]] auto create_volume_label(const provider_type &prov)
    -> std::string;

[[nodiscard]] auto get_attributes_from_meta(const api_meta_map &meta) -> DWORD;
} // namespace utils
} // namespace repertory

#endif // REPERTORY_INCLUDE_UTILS_UTILS_HPP_
