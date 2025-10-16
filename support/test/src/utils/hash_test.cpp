/*
 Copyright <2018-2025> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

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
#include "test.hpp"

#if defined(PROJECT_ENABLE_LIBSODIUM)

namespace repertory {
TEST(utils_hash, hash_type_sizes) {
  EXPECT_EQ(4U, utils::hash::hash_32_t{}.size());
  EXPECT_EQ(8U, utils::hash::hash_64_t{}.size());
  EXPECT_EQ(16U, utils::hash::hash_128_t{}.size());
  EXPECT_EQ(32U, utils::hash::hash_256_t{}.size());
  EXPECT_EQ(48U, utils::hash::hash_384_t{}.size());
  EXPECT_EQ(64U, utils::hash::hash_512_t{}.size());
}

TEST(utils_hash, default_hasher_is_blake2b) {
  EXPECT_EQ(&utils::hash::blake2b_32_hasher,
            &utils::hash::default_create_hash<utils::hash::hash_32_t>());

  EXPECT_EQ(&utils::hash::blake2b_64_hasher,
            &utils::hash::default_create_hash<utils::hash::hash_64_t>());

  EXPECT_EQ(&utils::hash::blake2b_128_hasher,
            &utils::hash::default_create_hash<utils::hash::hash_128_t>());

  EXPECT_EQ(&utils::hash::blake2b_256_hasher,
            &utils::hash::default_create_hash<utils::hash::hash_256_t>());

  EXPECT_EQ(&utils::hash::blake2b_384_hasher,
            &utils::hash::default_create_hash<utils::hash::hash_384_t>());

  EXPECT_EQ(&utils::hash::blake2b_512_hasher,
            &utils::hash::default_create_hash<utils::hash::hash_512_t>());
}

TEST(utils_hash, blake2b_32) {
  auto hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_32("a"));
  EXPECT_STREQ("ca234c55", hash.c_str());

  hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_32(L"a"));
#if defined(_WIN32)
  EXPECT_STREQ("4c368117", hash.c_str());
#else // !defined(_WIN32)
  EXPECT_STREQ("02a631b8", hash.c_str());
#endif

  hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_32({1U}));
  EXPECT_STREQ("593bda73", hash.c_str());
}

TEST(utils_hash, blake2b_64) {
  auto hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_64("a"));
  EXPECT_STREQ("40f89e395b66422f", hash.c_str());

  hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_64(L"a"));
#if defined(_WIN32)
  EXPECT_STREQ("4dd0bb1c45b748c1", hash.c_str());
#else // !defined(_WIN32)
  EXPECT_STREQ("85ff8cc55b79d38a", hash.c_str());
#endif

  hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_64({1U}));
  EXPECT_STREQ("00e83d0a3f7519ad", hash.c_str());
}

TEST(utils_hash, blake2b_128) {
  auto hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_128("a"));
  EXPECT_STREQ("27c35e6e9373877f29e562464e46497e", hash.c_str());

  hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_128(L"a"));
#if defined(_WIN32)
  EXPECT_STREQ("396660e76c84bb7786f979f10b58fa79", hash.c_str());
#else // !defined(_WIN32)
  EXPECT_STREQ("dae64afb310a3426ad84f0739fde5cef", hash.c_str());
#endif

  hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_128({1U}));
  EXPECT_STREQ("4a9e6f9b8d43f6ad008f8c291929dee2", hash.c_str());
}

TEST(utils_hash, blake2b_256) {
  auto hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_256("a"));
  EXPECT_STREQ(
      "8928aae63c84d87ea098564d1e03ad813f107add474e56aedd286349c0c03ea4",
      hash.c_str());

  hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_256(L"a"));
#if defined(_WIN32)
  EXPECT_STREQ(
      "d2373b17cd8a8e19e39f52fa4905a274f93805fbb8bb4c7f3cb4b2cd6708ec8a",
      hash.c_str());
#else // !defined(_WIN32)
  EXPECT_STREQ(
      "9fdf5757d7eea386f0d34d2c0e202527986febf1ebb4315fcf7fff40776fa41d",
      hash.c_str());
#endif

  hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_256({1U}));
  EXPECT_STREQ(
      "ee155ace9c40292074cb6aff8c9ccdd273c81648ff1149ef36bcea6ebb8a3e25",
      hash.c_str());
}

TEST(utils_hash, blake2b_384) {
  auto hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_384("a"));
  EXPECT_STREQ("7d40de16ff771d4595bf70cbda0c4ea0a066a6046fa73d34471cd4d93d827d7"
               "c94c29399c50de86983af1ec61d5dcef0",
               hash.c_str());

  hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_384(L"a"));
#if defined(_WIN32)
  EXPECT_STREQ("637fe31d1e955760ef31043d525d9321826a778ddbe82fcde45a98394241380"
               "96675e2f87e36b53ab223a7fd254198fd",
               hash.c_str());
#else // !defined(_WIN32)
  EXPECT_STREQ("9d469bd9dab9d4b48b8688de7c22704a8de1b081294f9be294100dfa9f05c92"
               "e8d3616476e46cd14f9e613fed80fd157",
               hash.c_str());
#endif

  hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_384({1U}));
  EXPECT_STREQ("42cfe875d08d816538103b906bb0b05202e0b09c4e981680c1110684fc7845b"
               "c91c178fa167afcc445490644b2bf5f5b",
               hash.c_str());
}

TEST(utils_hash, blake2b_512) {
  auto hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_512("a"));
  EXPECT_STREQ(
      "333fcb4ee1aa7c115355ec66ceac917c8bfd815bf7587d325aec1864edd24e34d5abe2c6"
      "b1b5ee3face62fed78dbef802f2a85cb91d455a8f5249d330853cb3c",
      hash.c_str());

  hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_512(L"a"));
#if defined(_WIN32)
  EXPECT_STREQ(
      "05970b95468b0b1941066ff189091493e73859ce41cde5ad08118e93ea1d81a57a144296"
      "a26a9fe7781481bde97b886725e36e30b305d8bd5cce1ae36bf1564a",
      hash.c_str());
#else // !defined(_WIN32)
  EXPECT_STREQ(
      "bbc187c6e4d8525655d0ada62d16eed59f3db3ab07e04fb0483fd4ae21d88b984774add9"
      "b3fbcff56f9638091013994f8e2d4646fdbbcb4879e2b5160bbb755d",
      hash.c_str());
#endif

  hash = utils::collection::to_hex_string(
      utils::hash::create_hash_blake2b_512({1U}));
  EXPECT_STREQ(
      "9545ba37b230d8a2e716c4707586542780815b7c4088edcb9af6a9452d50f32474d5ba9a"
      "ab52a67aca864ef2696981c2eadf49020416136afd838fb048d21653",
      hash.c_str());
}

TEST(utils_hash, sha256) {
  auto hash =
      utils::collection::to_hex_string(utils::hash::create_hash_sha256("a"));
  EXPECT_STREQ(
      "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb",
      hash.c_str());

  hash =
      utils::collection::to_hex_string(utils::hash::create_hash_sha256(L"a"));
#if defined(_WIN32)
  EXPECT_STREQ(
      "ffe9aaeaa2a2d5048174df0b80599ef0197ec024c4b051bc9860cff58ef7f9f3",
      hash.c_str());
#else // !defined(_WIN32)
  EXPECT_STREQ(
      "a2d398922901344d08180dc41d3e9d73d8c148c7f6e092835bbb28e02dbcf184",
      hash.c_str());
#endif

  hash =
      utils::collection::to_hex_string(utils::hash::create_hash_sha256({1U}));
  EXPECT_STREQ(
      "4bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a",
      hash.c_str());
}

TEST(utils_hash, sha512) {
  auto hash =
      utils::collection::to_hex_string(utils::hash::create_hash_sha512("a"));
  EXPECT_STREQ(
      "1f40fc92da241694750979ee6cf582f2d5d7d28e18335de05abc54d0560e0f5302860c65"
      "2bf08d560252aa5e74210546f369fbbbce8c12cfc7957b2652fe9a75",
      hash.c_str());

  hash =
      utils::collection::to_hex_string(utils::hash::create_hash_sha512(L"a"));
#if defined(_WIN32)
  EXPECT_STREQ(
      "5c2ca3d50f46ece6066c53bd1a490cbe5f72d2738ae9417332e91e5c3f75205c639d71a9"
      "a41d67d965fa137dddf439e0ab9443a6ea44915e90d8b5b566d1c076",
      hash.c_str());
#else // !defined(_WIN32)
  EXPECT_STREQ(
      "a93498d992e81915075144cb304d2bdf040b336283f888252244882d8366dd3a6e2d9749"
      "077114dda1a9aa1a7b69d33f7a781f003ccd12e599a6341014f29aaf",
      hash.c_str());
#endif

  hash =
      utils::collection::to_hex_string(utils::hash::create_hash_sha512({1U}));
  EXPECT_STREQ(
      "7b54b66836c1fbdd13d2441d9e1434dc62ca677fb68f5fe66a464baadecdbd00576f8d6b"
      "5ac3bcc80844b7d50b1cc6603444bbe7cfcf8fc0aa1ee3c636d9e339",
      hash.c_str());
}
} // namespace repertory

#endif // defined(PROJECT_ENABLE_LIBSODIUM)
