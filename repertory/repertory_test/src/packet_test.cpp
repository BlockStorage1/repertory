/* Copyright <2018-2025>
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/
#include "test_common.hpp"

#include "comm/packet/packet.hpp"
#include "types/remote.hpp"

namespace repertory {
TEST(packet_test, encrypt_and_decrypt) {
  packet test_packet;
  test_packet.encode("test");
  test_packet.encrypt("moose");

  std::uint32_t size{};
  EXPECT_EQ(0, test_packet.decode(size));
  EXPECT_EQ(0, test_packet.decrypt("moose"));

  std::string data;
  EXPECT_EQ(0, test_packet.decode(data));
  EXPECT_STREQ("test", data.c_str());
}

TEST(packet_test, encode_decode_primitives_and_strings) {
  packet pkt;

  const std::int8_t i8{-12};
  const std::uint8_t u8{250};
  const std::int16_t i16{-12345};
  const std::uint16_t u16{54321};
  const std::int32_t i32{-123456789};
  const std::uint32_t u32{3141592653U};
  const std::int64_t i64{-1234567890123456789LL};
  const std::uint64_t u64{12345678901234567890ULL};
  const std::string s{"hello world"};
  const std::wstring ws{L"wide 🌟"};

  pkt.encode(i8);
  pkt.encode(u8);
  pkt.encode(i16);
  pkt.encode(u16);
  pkt.encode(i32);
  pkt.encode(u32);
  pkt.encode(i64);
  pkt.encode(u64);
  pkt.encode(s);
  pkt.encode(ws);

  std::int8_t i8_r{};
  std::uint8_t u8_r{};
  std::int16_t i16_r{};
  std::uint16_t u16_r{};
  std::int32_t i32_r{};
  std::uint32_t u32_r{};
  std::int64_t i64_r{};
  std::uint64_t u64_r{};
  std::string s_r;
  std::wstring ws_r;

  EXPECT_EQ(0, pkt.decode(i8_r));
  EXPECT_EQ(0, pkt.decode(u8_r));
  EXPECT_EQ(0, pkt.decode(i16_r));
  EXPECT_EQ(0, pkt.decode(u16_r));
  EXPECT_EQ(0, pkt.decode(i32_r));
  EXPECT_EQ(0, pkt.decode(u32_r));
  EXPECT_EQ(0, pkt.decode(i64_r));
  EXPECT_EQ(0, pkt.decode(u64_r));
  EXPECT_EQ(0, pkt.decode(s_r));
  EXPECT_EQ(0, pkt.decode(ws_r));

  EXPECT_EQ(i8, i8_r);
  EXPECT_EQ(u8, u8_r);
  EXPECT_EQ(i16, i16_r);
  EXPECT_EQ(u16, u16_r);
  EXPECT_EQ(i32, i32_r);
  EXPECT_EQ(u32, u32_r);
  EXPECT_EQ(i64, i64_r);
  EXPECT_EQ(u64, u64_r);
  EXPECT_EQ(s, s_r);
  EXPECT_EQ(ws, ws_r);
}

TEST(packet_test, encode_decode_null_c_string_is_empty) {
  packet pkt;
  const char *p_null{nullptr};
  pkt.encode(p_null);

  std::string out;
  EXPECT_EQ(0, pkt.decode(out));
  EXPECT_TRUE(out.empty());
}

TEST(packet_test, encode_decode_raw_buffer) {
  packet pkt;

  std::array<unsigned char, 32> src{};
  for (std::size_t i = 0; i < src.size(); ++i) {
    src[i] = static_cast<unsigned char>(i);
  }

  pkt.encode(src.data(), src.size());

  std::array<unsigned char, 32> dst{};
  EXPECT_EQ(0, pkt.decode(dst.data(), dst.size()));
  EXPECT_EQ(0, std::memcmp(src.data(), dst.data(), dst.size()));
}

TEST(packet_test, encode_decode_pointer_round_trip) {
  packet pkt;

  int val{42};
  void *in_ptr = &val;

  pkt.encode(in_ptr);

  void *out_ptr = nullptr;
  EXPECT_EQ(0, pkt.decode(out_ptr));
  EXPECT_EQ(in_ptr, out_ptr);
}

TEST(packet_test, encode_top_affects_decode_order) {
  packet pkt;

  pkt.encode(static_cast<std::uint32_t>(1));
  pkt.encode(static_cast<std::uint32_t>(2));
  pkt.encode_top(static_cast<std::uint32_t>(99));

  std::uint32_t a{}, b{}, c{};
  EXPECT_EQ(0, pkt.decode(a));
  EXPECT_EQ(0, pkt.decode(b));
  EXPECT_EQ(0, pkt.decode(c));

  EXPECT_EQ(99U, a);
  EXPECT_EQ(1U, b);
  EXPECT_EQ(2U, c);
}

TEST(packet_test, copy_ctor_preserves_decode_offset) {
  packet pkt;
  pkt.encode(static_cast<std::uint32_t>(10));
  pkt.encode(static_cast<std::uint32_t>(20));
  pkt.encode(static_cast<std::uint32_t>(30));

  std::uint32_t first{};
  ASSERT_EQ(0, pkt.decode(first));
  ASSERT_EQ(10U, first);

  packet pkt_copy{pkt};

  std::uint32_t from_copy{};
  EXPECT_EQ(0, pkt_copy.decode(from_copy));
  EXPECT_EQ(20U, from_copy);

  std::uint32_t from_src{};

  EXPECT_EQ(0, pkt.decode(from_src));
  EXPECT_EQ(20U, from_src);
}

TEST(packet_test, copy_assign_preserves_decode_offset) {
  packet src;
  src.encode(static_cast<std::uint32_t>(1));
  src.encode(static_cast<std::uint32_t>(2));
  src.encode(static_cast<std::uint32_t>(3));

  std::uint32_t first{};
  ASSERT_EQ(0, src.decode(first));
  ASSERT_EQ(1U, first);

  packet dst;
  dst = src;

  std::uint32_t from_dst{};
  EXPECT_EQ(0, dst.decode(from_dst));
  EXPECT_EQ(2U, from_dst);

  std::uint32_t from_src{};
  EXPECT_EQ(0, src.decode(from_src));
  EXPECT_EQ(2U, from_src);
}

TEST(packet_test, move_ctor_preserves_decode_offset) {
  packet pkt;
  pkt.encode(static_cast<std::uint32_t>(100));
  pkt.encode(static_cast<std::uint32_t>(200));

  std::uint32_t x{};
  ASSERT_EQ(0, pkt.decode(x));
  ASSERT_EQ(100U, x);

  packet moved{std::move(pkt)};

  std::uint32_t y{};
  EXPECT_EQ(0, moved.decode(y));
  EXPECT_EQ(200U, y);
}

TEST(packet_test, move_assign_preserves_decode_offset) {
  packet src;
  src.encode(static_cast<std::uint32_t>(7));
  src.encode(static_cast<std::uint32_t>(8));
  src.encode(static_cast<std::uint32_t>(9));

  std::uint32_t first{};
  ASSERT_EQ(0, src.decode(first));
  ASSERT_EQ(7U, first);

  packet dst;
  dst.encode(
      static_cast<std::uint32_t>(999)); // give dst some state to overwrite
  dst = std::move(src);

  std::uint32_t next{};
  EXPECT_EQ(0, dst.decode(next));
  EXPECT_EQ(8U, next);
}

TEST(packet_test, operator_index_read_write_byte) {
  packet pkt;
  pkt.encode(static_cast<std::uint8_t>(7));

  EXPECT_EQ(pkt[0], static_cast<unsigned char>(7));

  pkt[0] = static_cast<unsigned char>(8);

  std::uint8_t out{};
  EXPECT_EQ(0, pkt.decode(out));
  EXPECT_EQ(8U, out);
}

TEST(packet_test, current_pointer_tracks_decode_offset) {
  packet pkt;
  pkt.encode(static_cast<std::uint8_t>(1));
  pkt.encode(static_cast<std::uint8_t>(2));

  std::uint8_t first{};
  EXPECT_EQ(0, pkt.decode(first));
  EXPECT_EQ(1U, first);

  auto *ptr = pkt.current_pointer();
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, static_cast<unsigned char>(2));

  const auto &const_pkt = pkt;
  const auto *cptr = const_pkt.current_pointer();
  ASSERT_NE(cptr, nullptr);
  EXPECT_EQ(*cptr, static_cast<unsigned char>(2));
}

TEST(packet_test, open_flags_round_trip) {
  packet pkt;

  remote::open_flags flags = remote::open_flags::read_only |
                             remote::open_flags::create |
                             remote::open_flags::truncate;

  pkt.encode(flags);

  remote::open_flags decoded{};
  EXPECT_EQ(0, pkt.decode(decoded));

  EXPECT_EQ(static_cast<std::uint32_t>(flags),
            static_cast<std::uint32_t>(decoded));
}

TEST(packet_test, encrypt_and_decrypt_without_size_prefix) {
  packet pkt;
  pkt.encode("opaque");
  pkt.encrypt("moose", false);

  EXPECT_EQ(0, pkt.decrypt("moose"));

  std::string out;
  EXPECT_EQ(0, pkt.decode(out));
  EXPECT_EQ("opaque", out);
}

TEST(packet_test, decode_fails_when_empty) {
  packet pkt;
  std::uint32_t val{};
  EXPECT_NE(0, pkt.decode(val));
}

TEST(packet_test, decode_json_round_trip) {
  packet pkt;

  const json src = {{"x", 1}, {"y", "z"}, {"ok", true}};
  pkt.encode(src.dump());

  json got{};
  EXPECT_EQ(0, packet::decode_json(pkt, got));
  EXPECT_EQ(src, got);
}

TEST(packet_test, remote_setattr_x_round_trip) {
  packet pkt;

  remote::setattr_x sa{};
  sa.valid = 0x7;
  sa.mode = static_cast<remote::file_mode>(0644);
  sa.uid = 1001U;
  sa.gid = 1002U;
  sa.size = 1234567ULL;
  sa.acctime = 111ULL;
  sa.modtime = 222ULL;
  sa.crtime = 333ULL;
  sa.chgtime = 444ULL;
  sa.bkuptime = 555ULL;
  sa.flags = 0xA5A5A5A5U;

  pkt.encode(sa);

  remote::setattr_x out{};
  EXPECT_EQ(0, pkt.decode(out));

  EXPECT_EQ(sa.valid, out.valid);
  EXPECT_EQ(sa.mode, out.mode);
  EXPECT_EQ(sa.uid, out.uid);
  EXPECT_EQ(sa.gid, out.gid);
  EXPECT_EQ(sa.size, out.size);
  EXPECT_EQ(sa.acctime, out.acctime);
  EXPECT_EQ(sa.modtime, out.modtime);
  EXPECT_EQ(sa.crtime, out.crtime);
  EXPECT_EQ(sa.chgtime, out.chgtime);
  EXPECT_EQ(sa.bkuptime, out.bkuptime);
  EXPECT_EQ(sa.flags, out.flags);
}

TEST(packet_test, remote_stat_round_trip) {
  packet pkt;

  remote::stat st{};
  st.st_mode = static_cast<remote::file_mode>(0755);
  st.st_nlink = static_cast<remote::file_nlink>(2);
  st.st_uid = 2001U;
  st.st_gid = 2002U;
  st.st_atimespec = 101ULL;
  st.st_mtimespec = 202ULL;
  st.st_ctimespec = 303ULL;
  st.st_birthtimespec = 404ULL;
  st.st_size = 987654321ULL;
  st.st_blocks = 4096ULL;
  st.st_blksize = 8192U;
  st.st_flags = 0xDEADBEEFU;

  pkt.encode(st);

  remote::stat out{};
  EXPECT_EQ(0, pkt.decode(out));

  EXPECT_EQ(st.st_mode, out.st_mode);
  EXPECT_EQ(st.st_nlink, out.st_nlink);
  EXPECT_EQ(st.st_uid, out.st_uid);
  EXPECT_EQ(st.st_gid, out.st_gid);
  EXPECT_EQ(st.st_atimespec, out.st_atimespec);

  EXPECT_EQ(st.st_mtimespec, out.st_mtimespec);
  EXPECT_EQ(st.st_ctimespec, out.st_ctimespec);
  EXPECT_EQ(st.st_birthtimespec, out.st_birthtimespec);
  EXPECT_EQ(st.st_size, out.st_size);
  EXPECT_EQ(st.st_blocks, out.st_blocks);
  EXPECT_EQ(st.st_blksize, out.st_blksize);
  EXPECT_EQ(st.st_flags, out.st_flags);
}

TEST(packet_test, remote_statfs_round_trip) {
  packet pkt;

  remote::statfs sfs{};
  sfs.f_bavail = 1'000'000ULL;
  sfs.f_bfree = 2'000'000ULL;
  sfs.f_blocks = 3'000'000ULL;
  sfs.f_favail = 4'000'000ULL;
  sfs.f_ffree = 5'000'000ULL;
  sfs.f_files = 6'000'000ULL;

  pkt.encode(sfs);

  remote::statfs out{};
  EXPECT_EQ(0, pkt.decode(out));

  EXPECT_EQ(sfs.f_bavail, out.f_bavail);
  EXPECT_EQ(sfs.f_bfree, out.f_bfree);
  EXPECT_EQ(sfs.f_blocks, out.f_blocks);
  EXPECT_EQ(sfs.f_favail, out.f_favail);
  EXPECT_EQ(sfs.f_ffree, out.f_ffree);
  EXPECT_EQ(sfs.f_files, out.f_files);
}

TEST(packet_test, remote_statfs_x_round_trip) {
  packet pkt;

  remote::statfs_x sfsx{};
  sfsx.f_bavail = 7'000'000ULL;
  sfsx.f_bfree = 8'000'000ULL;
  sfsx.f_blocks = 9'000'000ULL;
  sfsx.f_favail = 10'000'000ULL;
  sfsx.f_ffree = 11'000'000ULL;
  sfsx.f_files = 12'000'000ULL;

  const char *mnt = "test_mnt";
  std::memcpy(sfsx.f_mntfromname.data(), mnt, std::strlen(mnt));

  pkt.encode(sfsx);

  remote::statfs_x out{};
  EXPECT_EQ(0, pkt.decode(out));

  EXPECT_EQ(sfsx.f_bavail, out.f_bavail);
  EXPECT_EQ(sfsx.f_bfree, out.f_bfree);
  EXPECT_EQ(sfsx.f_blocks, out.f_blocks);
  EXPECT_EQ(sfsx.f_favail, out.f_favail);
  EXPECT_EQ(sfsx.f_ffree, out.f_ffree);
  EXPECT_EQ(sfsx.f_files, out.f_files);
  EXPECT_EQ(0, std::memcmp(sfsx.f_mntfromname.data(), out.f_mntfromname.data(),
                           sfsx.f_mntfromname.size()));
}

TEST(packet_test, remote_file_info_round_trip) {
  packet pkt;

  remote::file_info fi{};
  fi.FileAttributes = 0x1234U;
  fi.ReparseTag = 0x5678U;
  fi.AllocationSize = 1111ULL;
  fi.FileSize = 2222ULL;
  fi.CreationTime = 3333ULL;
  fi.LastAccessTime = 4444ULL;
  fi.LastWriteTime = 5555ULL;
  fi.ChangeTime = 6666ULL;
  fi.IndexNumber = 7777ULL;
  fi.HardLinks = 3U;
  fi.EaSize = 0U;

  pkt.encode(fi);

  remote::file_info out{};
  EXPECT_EQ(0, pkt.decode(out));

  EXPECT_EQ(fi.FileAttributes, fi.FileAttributes);
  EXPECT_EQ(fi.ReparseTag, out.ReparseTag);
  EXPECT_EQ(fi.AllocationSize, out.AllocationSize);
  EXPECT_EQ(fi.FileSize, out.FileSize);
  EXPECT_EQ(fi.CreationTime, out.CreationTime);
  EXPECT_EQ(fi.LastAccessTime, out.LastAccessTime);
  EXPECT_EQ(fi.LastWriteTime, out.LastWriteTime);
  EXPECT_EQ(fi.ChangeTime, out.ChangeTime);
  EXPECT_EQ(fi.IndexNumber, out.IndexNumber);
  EXPECT_EQ(fi.HardLinks, out.HardLinks);
  EXPECT_EQ(fi.EaSize, out.EaSize);
}
} // namespace repertory
