#include "rpc/common.hpp"

#include "utils/collection.hpp"
#include "utils/hash.hpp"

namespace repertory::rpc {
auto create_password_hash(std::string_view password) -> std::string {
  return utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_384(password));
}
} // namespace repertory::rpc
