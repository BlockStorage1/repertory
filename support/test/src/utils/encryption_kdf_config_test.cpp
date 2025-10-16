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

#if defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)

namespace repertory {
TEST(utils_encryption_kdf_config, can_construct_using_default_constructor) {
  utils::encryption::kdf_config cfg;

  EXPECT_EQ(utils::encryption::kdf_version::v1, cfg.version);
  EXPECT_EQ(utils::encryption::kdf_type::argon2id, cfg.kdf);
  EXPECT_EQ(utils::encryption::memlimit_level::level3, cfg.memlimit);
  EXPECT_EQ(utils::encryption::opslimit_level::level2, cfg.opslimit);
  EXPECT_EQ(utils::encryption::kdf_config::salt_t{}, cfg.salt);
  EXPECT_EQ(0U, cfg.unique_id);
  EXPECT_EQ(0U, cfg.checksum);
}

TEST(utils_encryption_kdf_config, can_seal) {
  utils::encryption::kdf_config cfg;
  cfg.seal();
  EXPECT_NE(utils::encryption::kdf_config::salt_t{}, cfg.salt);

  auto orig_salt = cfg.salt;
  cfg.seal();
  EXPECT_NE(orig_salt, cfg.salt);
}

TEST(utils_encryption_kdf_config, can_generate_checksum) {
  utils::encryption::kdf_config cfg;
  EXPECT_EQ(13087047540462255120ULL, cfg.generate_checksum());
}

TEST(utils_encryption_kdf_config, seal_calculates_checksum) {
  utils::encryption::kdf_config cfg;
  cfg.seal();

  EXPECT_NE(0U, cfg.checksum);
}

TEST(utils_encryption_kdf_config, can_create_header_and_restore) {
  utils::encryption::kdf_config cfg;
  cfg.unique_id = 2U;
  cfg.seal();
  auto hdr = cfg.to_header();

  EXPECT_EQ(utils::encryption::kdf_config::size(), hdr.size());

  utils::encryption::kdf_config restored_cfg;
  EXPECT_TRUE(utils::encryption::kdf_config::from_header(hdr, restored_cfg));
  auto restored_hdr = restored_cfg.to_header();

  EXPECT_EQ(hdr, restored_hdr);
  EXPECT_EQ(cfg.version, restored_cfg.version);
  EXPECT_EQ(cfg.kdf, restored_cfg.kdf);
  EXPECT_EQ(cfg.memlimit, restored_cfg.memlimit);
  EXPECT_EQ(cfg.opslimit, restored_cfg.opslimit);
  EXPECT_EQ(cfg.salt, restored_cfg.salt);
  EXPECT_EQ(cfg.checksum, restored_cfg.checksum);
  EXPECT_EQ(cfg.unique_id, restored_cfg.unique_id);
  EXPECT_EQ(cfg, restored_cfg);
}

TEST(utils_encryption_kdf_config, header_restore_fails_if_version_is_invalid) {
  utils::encryption::kdf_config cfg;
  cfg.version = static_cast<utils::encryption::kdf_version>(0x11);
  cfg.seal();

  auto hdr = cfg.to_header();
  utils::encryption::kdf_config restored_cfg;
  EXPECT_FALSE(utils::encryption::kdf_config::from_header(hdr, restored_cfg));
}

TEST(utils_encryption_kdf_config, header_restore_fails_if_kdf_is_invalid) {
  utils::encryption::kdf_config cfg;
  cfg.kdf = static_cast<utils::encryption::kdf_type>(0x11);
  cfg.seal();

  auto hdr = cfg.to_header();
  utils::encryption::kdf_config restored_cfg;
  EXPECT_FALSE(utils::encryption::kdf_config::from_header(hdr, restored_cfg));
}

TEST(utils_encryption_kdf_config, header_restore_fails_if_memlimit_is_invalid) {
  utils::encryption::kdf_config cfg;
  cfg.memlimit = static_cast<utils::encryption::memlimit_level>(0x11);
  cfg.seal();

  auto hdr = cfg.to_header();
  utils::encryption::kdf_config restored_cfg;
  EXPECT_FALSE(utils::encryption::kdf_config::from_header(hdr, restored_cfg));
}

TEST(utils_encryption_kdf_config, header_restore_fails_if_opslimit_is_invalid) {
  utils::encryption::kdf_config cfg;
  cfg.opslimit = static_cast<utils::encryption::opslimit_level>(0x11);
  cfg.seal();

  auto hdr = cfg.to_header();
  utils::encryption::kdf_config restored_cfg;
  EXPECT_FALSE(utils::encryption::kdf_config::from_header(hdr, restored_cfg));
}

TEST(utils_encryption_kdf_config, header_restore_fails_if_salt_is_invalid) {
  utils::encryption::kdf_config cfg;
  cfg.seal();
  cfg.salt = utils::encryption::kdf_config::salt_t{};

  auto hdr = cfg.to_header();
  utils::encryption::kdf_config restored_cfg;
  EXPECT_FALSE(utils::encryption::kdf_config::from_header(hdr, restored_cfg));
}

TEST(utils_encryption_kdf_config, header_restore_fails_if_id_is_invalid) {
  utils::encryption::kdf_config cfg;
  cfg.seal();
  cfg.unique_id = 22U;

  auto hdr = cfg.to_header();
  utils::encryption::kdf_config restored_cfg;
  EXPECT_FALSE(utils::encryption::kdf_config::from_header(hdr, restored_cfg));
}

TEST(utils_encryption_kdf_config, create_subkey_sets_id_and_updates_checksum) {
  using hash_t = utils::hash::hash_256_t;

  utils::encryption::kdf_config cfg;
  cfg.seal();

  hash_t master_key =
      utils::encryption::generate_key<hash_t>("root-master-key");

  constexpr std::size_t sub_id = 42;
  auto [subkey, out_cfg] = cfg.create_subkey<hash_t>(
      utils::encryption::kdf_context::path, sub_id, master_key);

  EXPECT_NE(subkey, hash_t{});
  EXPECT_NE(subkey, master_key);

  EXPECT_EQ(out_cfg.unique_id, static_cast<std::uint64_t>(sub_id));
  EXPECT_EQ(out_cfg.checksum, out_cfg.generate_checksum());

  EXPECT_EQ(out_cfg.version, cfg.version);
  EXPECT_EQ(out_cfg.kdf, cfg.kdf);
  EXPECT_EQ(out_cfg.memlimit, cfg.memlimit);
  EXPECT_EQ(out_cfg.opslimit, cfg.opslimit);
  EXPECT_EQ(out_cfg.salt, cfg.salt);
}

TEST(utils_encryption_kdf_config,
     create_subkey_is_deterministic_for_same_inputs) {
  using hash_t = utils::hash::hash_256_t;

  utils::encryption::kdf_config cfg;
  cfg.seal();

  hash_t master_key =
      utils::encryption::generate_key<hash_t>("root-master-key");

  constexpr auto ctx = utils::encryption::kdf_context::data;
  constexpr std::size_t sub_id = 7;

  auto [k1, c1] = cfg.create_subkey<hash_t>(ctx, sub_id, master_key);
  auto [k2, c2] = cfg.create_subkey<hash_t>(ctx, sub_id, master_key);

  EXPECT_EQ(k1, k2);
  EXPECT_EQ(c1.unique_id, c2.unique_id);
  EXPECT_EQ(c1.checksum, c2.checksum);
  EXPECT_EQ(c1, c2);
}

TEST(utils_encryption_kdf_config, create_subkey_varies_with_different_id) {
  using hash_t = utils::hash::hash_256_t;

  utils::encryption::kdf_config cfg;
  cfg.seal();

  hash_t master_key =
      utils::encryption::generate_key<hash_t>("root-master-key");

  constexpr auto ctx = utils::encryption::kdf_context::data;

  auto [k1, c1] = cfg.create_subkey<hash_t>(ctx, 1, master_key);
  auto [k2, c2] = cfg.create_subkey<hash_t>(ctx, 2, master_key);

  EXPECT_NE(k1, k2);
  EXPECT_NE(c1.unique_id, c2.unique_id);
  EXPECT_NE(c1.checksum, c2.checksum);

  EXPECT_EQ(c1.version, c2.version);
  EXPECT_EQ(c1.kdf, c2.kdf);
  EXPECT_EQ(c1.memlimit, c2.memlimit);
  EXPECT_EQ(c1.opslimit, c2.opslimit);
  EXPECT_EQ(c1.salt, c2.salt);
}

TEST(utils_encryption_kdf_config, create_subkey_varies_with_different_context) {
  using hash_t = utils::hash::hash_256_t;

  utils::encryption::kdf_config cfg;
  cfg.seal();

  hash_t master_key =
      utils::encryption::generate_key<hash_t>("root-master-key");

  constexpr std::size_t sub_id = 123;

  auto [ka, ca] = cfg.create_subkey<hash_t>(
      utils::encryption::kdf_context::data, sub_id, master_key);
  auto [kb, cb] = cfg.create_subkey<hash_t>(
      utils::encryption::kdf_context::path, sub_id, master_key);

  EXPECT_NE(ka, kb);
  EXPECT_EQ(ca.unique_id, cb.unique_id);
  EXPECT_EQ(ca.checksum, cb.checksum);
  EXPECT_EQ(ca, cb);
}

TEST(utils_encryption_kdf_config,
     create_subkey_with_undefined_context_uses_fallback) {
  using hash_t = utils::hash::hash_256_t;

  utils::encryption::kdf_config cfg;
  cfg.seal();

  hash_t master_key =
      utils::encryption::generate_key<hash_t>("root-master-key");

  constexpr std::size_t sub_id = 55;

  auto [k_def, c_def] = cfg.create_subkey<hash_t>(
      utils::encryption::kdf_context::undefined, sub_id, master_key);
  auto [k_dat, c_dat] = cfg.create_subkey<hash_t>(
      utils::encryption::kdf_context::data, sub_id, master_key);

  EXPECT_NE(k_def, hash_t{});
  EXPECT_NE(k_dat, hash_t{});
  EXPECT_NE(k_def, k_dat);

  EXPECT_EQ(c_def, c_dat);
}

#if defined(PROJECT_ENABLE_JSON)
TEST(utils_encryption_kdf_config, can_convert_kdf_config_to_and_from_json) {
  utils::encryption::kdf_config cfg;
  cfg.unique_id = 2U;
  cfg.seal();

  nlohmann::json json_kdf(cfg);

  auto cfg2 = json_kdf.get<utils::encryption::kdf_config>();
  EXPECT_EQ(cfg, cfg2);
}
#endif // defined(PROJECT_ENABLE_JSON)

TEST(utils_encryption_kdf_config, equality) {
  {
    utils::encryption::kdf_config cfg;
    utils::encryption::kdf_config cfg2;

    EXPECT_EQ(cfg, cfg2);
  }

  {
    utils::encryption::kdf_config cfg;
    utils::encryption::kdf_config cfg2{cfg};

    EXPECT_EQ(cfg, cfg2);
  }

  {
    utils::encryption::kdf_config cfg;
    cfg.seal();

    utils::encryption::kdf_config cfg2{cfg};

    EXPECT_EQ(cfg, cfg2);
  }

  {
    utils::encryption::kdf_config cfg;
    utils::encryption::kdf_config cfg2;
    cfg2 = cfg;

    EXPECT_EQ(cfg, cfg2);
  }

  {
    utils::encryption::kdf_config cfg;
    cfg.seal();

    utils::encryption::kdf_config cfg2;
    cfg2 = cfg;

    EXPECT_EQ(cfg, cfg2);
  }
}

TEST(utils_encryption_kdf_config, sealed_is_not_equal_to_unsealed) {
  utils::encryption::kdf_config cfg;
  cfg.seal();

  utils::encryption::kdf_config cfg2;

  EXPECT_NE(cfg, cfg2);
}

TEST(utils_encryption_kdf_config, sealed_is_not_equal_to_sealed) {
  utils::encryption::kdf_config cfg;
  cfg.seal();

  utils::encryption::kdf_config cfg2;
  cfg2.seal();

  EXPECT_NE(cfg, cfg2);
}

TEST(utils_encryption_kdf_config, is_not_equal_to_different_id) {
  utils::encryption::kdf_config cfg;
  cfg.unique_id = 2UL;

  utils::encryption::kdf_config cfg2;

  EXPECT_NE(cfg, cfg2);
}

TEST(utils_encryption_kdf_config, is_not_equal_to_different_version) {
  utils::encryption::kdf_config cfg;
  cfg.version = static_cast<utils::encryption::kdf_version>(0x11);

  utils::encryption::kdf_config cfg2;

  EXPECT_NE(cfg, cfg2);
}

TEST(utils_encryption_kdf_config, is_not_equal_to_different_kdf) {
  utils::encryption::kdf_config cfg;
  cfg.kdf = static_cast<utils::encryption::kdf_type>(0x11);

  utils::encryption::kdf_config cfg2;

  EXPECT_NE(cfg, cfg2);
}

TEST(utils_encryption_kdf_config, is_not_equal_to_different_memlimit) {
  utils::encryption::kdf_config cfg;
  cfg.memlimit = static_cast<utils::encryption::memlimit_level>(0x11);

  utils::encryption::kdf_config cfg2;

  EXPECT_NE(cfg, cfg2);
}

TEST(utils_encryption_kdf_config, is_not_equal_to_different_opslimit) {
  utils::encryption::kdf_config cfg;
  cfg.opslimit = static_cast<utils::encryption::opslimit_level>(0x11);

  utils::encryption::kdf_config cfg2;

  EXPECT_NE(cfg, cfg2);
}

TEST(utils_encryption_kdf_config, is_not_equal_to_different_salt) {
  utils::encryption::kdf_config cfg;
  cfg.salt[0U] = 1U;

  utils::encryption::kdf_config cfg2;

  EXPECT_NE(cfg, cfg2);
}

TEST(utils_encryption_kdf_config, is_not_equal_to_different_checksum) {
  utils::encryption::kdf_config cfg;
  cfg.checksum = 2U;

  utils::encryption::kdf_config cfg2;

  EXPECT_NE(cfg, cfg2);
}
} // namespace repertory

#endif // defined(PROJECT_ENABLE_LIBSODIUM) && defined(PROJECT_ENABLE_BOOST)
