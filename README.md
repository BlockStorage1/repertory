# Repertory

Repertory allows you to mount AWS S3 and Sia via FUSE on Linux/OS X or via WinFSP
on Windows.

## Details and Features

* Optimized for [Plex Media Server](https://www.plex.tv/)
* Single application to mount AWS S3 and/or Sia
* Only 1 Sia mount and 1 S3 mount (per bucket) per user is supported.
* Remote mounting of Repertory instances on Linux, OS X and Windows
  * Securely share your mounts over TCP/IP (`XChaCha20-Poly1305` stream cipher)
* Cross-platform support (Linux 64-bit, Linux arm64/aarch64, OS X, Windows 64-bit)

## Minimum Requirements

* [Sia renterd](https://github.com/SiaFoundation/renterd/releases) v0.4.0+ for Sia support
* Only 64-bit operating systems are supported
  * Linux requires the following dependencies:
    * `libfuse3`
  * OS X requires the following dependency to be installed:
    * [FUSE for macOS v4.5.0](https://github.com/osxfuse/osxfuse/releases/download/macfuse-4.5.0/macfuse-4.5.0.dmg)
  * Windows requires the following dependencies to be installed:
    * [WinFSP 2023](https://github.com/winfsp/winfsp/releases/download/v2.0/winfsp-2.0.23075.msi)
    * [Microsoft Visual C++ Redistributable for Visual Studio 2015, 2017 and 2019](https://aka.ms/vs/16/release/vc_redist.x64.exe)

## Supported Operating Systems

* Linux `arm64/aarch64`
* Linux `amd64`
* OS X Mojave and above
* Windows 64-bit 10, 11

## Credits

* [boost c++ libraries](https://www.boost.org/)
* [cpp-httplib](https://github.com/yhirose/cpp-httplib)
* [curl](https://curl.haxx.se/)
* [Filebase](https://filebase.com/)
* [FUSE for macOS](https://osxfuse.github.io/)
* [Google Test](https://github.com/google/googletest)
* [JSON for Modern C++](https://github.com/nlohmann/json)
* [libfuse](https://github.com/libfuse/libfuse)
* [libsodium](https://doc.libsodium.org/)
* [OpenSSL](https://www.openssl.org/)
* [OSSP uuid](http://www.ossp.org/pkg/lib/uuid/)
* [RocksDB](https://rocksdb.org/)
* [ScPrime](https://scpri.me/)
* [Sia Decentralized Cloud Storage](https://sia.tech/)
* [WinFSP - FUSE for Windows](https://github.com/billziss-gh/winfsp)
* [zlib](https://zlib.net/)

## Developer Public Key

```text
-----BEGIN PUBLIC KEY-----
MIIEIjANBgkqhkiG9w0BAQEFAAOCBA8AMIIECgKCBAEKfZmq5mMAtD4kSt2Gc/5J
H+HHTYtUZE6YYvsvz8TNG/bNL67ZtNRyaoMyhLTfIN4rPBNLUfD+owNS+u5Yk+lS
ZLYyOuhoCZIFefayYqKLr42G8EeuRbx0IMzXmJtN0a4rqxlWhkYufJubpdQ+V4DF
oeupcPdIATaadCKVeZC7A0G0uaSwoiAVMG5dZqjQW7F2LoQm3PhNkPvAybIJ6vBy
LqdBegS1JrDn43x/pvQHzLO+l+FIG23D1F7iF+yZm3DkzBdcmi/mOMYs/rXZpBym
2/kTuSGh5buuJCeyOwR8N3WdvXw6+KHMU/wWU8qTCTT87mYbzH4YR8HgkjkLHxAO
5waHK6vMu0TxugCdJmVV6BSbiarJsh66VRosn7+6hlq6AdgksxqCeNELZBS+LBki
tb5hKyL+jNZnaHiR0U7USWtmnqZG6FVVRzlCnxP7tZo5O5Ex9AAFGz5JzOzsFNbv
xwQ0zqaTQOze+MJbkda7JfRoC6TncD0+3hoXsiaF4mCn8PqUCn0DwhglcRucZlST
ZvDNDo1WAtxPJebb3aS6uymNhBIquQbVAWxVO4eTrOYEgutxwkHE3yO3is+ogp8d
xot7f/+vzlbsbIDyuZBDe0fFkbTIMTU48QuUUVZpRKmKZTHQloz4EHqminbfX1sh
M7wvDkpJEtqbc0VnG/BukUzP6e7Skvgc7eF1sI3+8jH8du2rivZeZAl7Q2f+L9JA
BY9pjaxttxsud7V5jeFi4tKuDHi21/XhSjlJK2c2C4AiUEK5/WhtGbQ5JjmcOjRq
yXFRqLlerzOcop2kbtU3Ar230wOx3Dj23Wg8++lV3LU4U9vMR/t0qnSbCSGJys7m
ax2JpFlTwj/0wYuTlVFoNQHZJ1cdfyRiRBY4Ou7XO0W5hcBBKiYsC+neEeMMHdCe
iTDIW/ojcVTdFovl+sq3n1u4SBknE90JC/3H+TPE1s2iB+fwORVg0KPosQSNDS0A
7iK6AZCDC3YooFo+OzHkYMt9uLkXiXMSLx70az+qlIwOzVHKxCo7W/QpeKCXUCRZ
MMdlYEUs1PC8x2qIRUEVHuJ0XMTKNyOHmzVLuLK93wUWbToh+rdDxnbhX+emuESn
XH6aKiUwX4olEVKSylRUQw8nVckZGVWXzLDlgpzDrLHC8J8qHzFt7eCqOdiqsxhZ
x1U5LtugxwSWncTZ7vlKl0DuC/AWB7SuDi7bGRMSVp2n+MnD1VLKlsCclHXjIciE
W29n3G3lJ/sOta2sxqLd0j1XBQddrFXl5b609sIY81ocHqu8P2hRu5CpqJ/sGZC5
mMH3segHBkRj0xJcfOxceRLj1a+ULIIR3xL/3f8s5Id25TDo/nqBoCvu5PeCpo6L
9wIDAQAB
-----END PUBLIC KEY-----
```

