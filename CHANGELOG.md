# Changelog

## 2.0.1-rc

### Issues

* \#10 Address compiler warnings
* \#11 Switch to SQLite over RocksDB

### Changes from v2.0.0-rc

* Refactored Sia, S3 and base provider
* Fixed intermittent deadlock on file close
* Removed MSVC compilation support (MinGW-64 should be used)
* Require `c++20`
* Switched to Storj over Filebase for hosting binaries
* Updated `boost` to v1.83.0
* Updated `cpp-httplib` to v0.14.2
* Updated `curl` to v8.4.0
* Updated `libsodium` to v1.0.19
* Updated `OpenSSL` to v3.2.0

## 2.0.0-rc

<!-- markdownlint-disable-next-line -->
### Issues

* \#1 \[bug\] Unable to mount S3 due to 'item_not_found' exception
* \#2 Require bucket name for S3 mounts
* \#3 \[bug\] File size is not being updated in S3 mount
* \#4 Upgrade to libfuse-3.x.x
* \#5 Switch to renterd for Sia support
* \#6 Switch to cpp-httplib to further reduce dependencies
* \#7 Remove global_data and calculate used disk space per provider
* \#8 Switch to libcurl for S3 mount support

### Changes from v1.x.x

* Added read-only encrypt provider
  * Pass-through mount point that transparently encrypts source data using `XChaCha20-Poly1305`
* Added S3 encryption support via `XChaCha20-Poly1305`
* Added replay protection to remote mounts
* Added support base64 writes in remote FUSE
* Created static linked Linux binaries for `amd64` and `aarch64` using `musl-libc`
* Removed legacy Sia renter support
* Removed Skynet support
* Fixed multiple remote mount WinFSP API issues on \*NIX servers
* Implemented chunked read and write
  * Writes for non-cached files are performed in chunks of 8Mib
* Removed `repertory-ui` support
* Removed `FreeBSD` support
* Switched to `libsodium` over `CryptoPP`
* Switched to `XChaCha20-Poly1305` for remote mounts
* Updated `GoogleTest` to v1.14.0
* Updated `JSON for Modern C++` to v3.11.2
* Updated `OpenSSL` to v1.1.1w
* Updated `RocksDB` to v8.5.3
* Updated `WinFSP` to 2023
* Updated `boost` to v1.78.0
* Updated `cURL` to v8.3.0
* Updated `zlib` to v1.3
* Use `upload_manager` for all providers
  * Adds a delay to uploads to prevent excessive API calls
  * Supports re-upload after mount restart for incomplete uploads
  * NOTE: Uploads for all providers are full file (no resume support)
    * Multipart upload support is planned for S3
