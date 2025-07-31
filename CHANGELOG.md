# Changelog

## v2.0.7-release

### Issues
* \#55 [bug] UI is unable to launch `repertory.exe` on Windows when absolute path contains spaces
* \#57 [bug] Directory entries . and .. are incorrectly being reported as files in Linux remote mounts

## v2.0.6-release

### Issues
* \#42 [bug] Remote mount directory listing on Windows connected to Linux is failing
* \#43 [bug] Directories are not importing properly for Sia
* \#44 [bug] Windows-to-Linux remote mount ignores `CREATE_NEW`
* \#45 [bug] Windows-to-Linux remote mount is not handling attempts to remove a non-empty directory properly
* \#46 [bug] Changes to maximum cache size should be updated live
* \#47 [bug] Windows-to-Linux remote mount is allowing directory rename when directory is not empty
* \#48 [bug] Windows-to-Linux remote mount overlapped I/O is not detecting EOF for read operations
* \#49 [ui] Implement provider test button

### Changes from v2.0.5-rc

* Added request retry on `libcurl` error code `CURLE_COULDNT_RESOLVE_HOST`
* Added `libcurl` DNS caching
* Drive letters in UI should always be lowercase
* Fixed WinFSP directory rename for non-empty directories
* Fixed segfault in UI due to incorrect `SIGCHLD` handling
* Migrated to v2 error handling
* Upgraded WinFSP to v2.1 (2025)

## v2.0.5-rc

<!-- markdownlint-disable-next-line -->
### Issues

* \#39 Create management portal in Flutter

### Changes from v2.0.4-rc

* Continue documentation updates
* Fixed `-status` command erasing active mount information
* Fixed overlapping HTTP REST API port's
* Refactored/fixed instance locking
* Removed passwords and secret key values from API calls
* Renamed setting `ApiAuth` to `ApiPassword`
* Require `--name,-na` option for encryption provider

## v2.0.4-rc

### BREAKING CHANGES

* `renterd` v2.0.0+ is now required. Prior versions will fail to mount.

<!-- markdownlint-disable-next-line -->
### Issues

* \#35 [bug] Low frequency check is set to '0' instead of 1 hour by default
* \#36 [bug] Max cache size bytes is set to '0' by default

### Changes from v2.0.3-rc

* Added Sia API version check prior to mounting
* Added back `-cv` (check version) CLI option
* Continue documentation updates
* Fixed setting `ApiAuth` via `set_value_by_name`
* Fixed setting `HostConfig.ApiUser` via `set_value_by_name`
* Fixed setting `HostConfig.Path` via `set_value_by_name`
* Fixed setting `HostConfig.Protocol` via `set_value_by_name`
* Improved ring buffer read-ahead
* Integrated `renterd` version 2.0.0
* Prefer using local cache file when opening files
* Refactored `app_config` unit tests
* Refactored polling to be more accurate on scheduling tasks

## v2.0.3-rc

<!-- markdownlint-disable-next-line -->
### Issues

* \#28 \[bug\] Address slow directory responses in S3 mounts for deeply nested directories
* \#29 \[bug\] S3 error responses are not being logged
* \#30 \[bug\] Sia provider error responses are not logged
* \#31 \[bug\] S3 provider should limit max key size to 1024

### Changes from v2.0.2-rc

* Always use direct for read-only providers
* Fixed externally removed files not being processed during cleanup
* Fixed http headers not being added for requests
* Fixed incorrect `stat` values for remote mounts
* Fixed invalid directory nullptr error on remote mounts
* Fixed memory leak in event system
* Refactored application shutdown
* Refactored event system
* Updated build system to Alpine 3.21.0
* Updated build system to MinGW-w64 12.0.0
* Updated copyright to 2018-2025

## v2.0.2-rc

<!-- markdownlint-disable-next-line -->
### BREAKING CHANGES

* Refactored `config.json` - will need to verify configuration settings prior to mounting

<!-- markdownlint-disable-next-line -->
### Issues

* \#14 \[Unit Test\] SQLite mini-ORM unit tests and cleanup
* \#16 Add support for bucket name in Sia provider
* \#17 Update to common c++ build system
  * A single 64-bit Linux Jenkins server is used to build all Linux and Windows versions
  * All dependency sources are now included
  * MSVC is no longer supported
  * MSYS2 is required for building Windows binaries on Windows
  * OS X support is temporarily disabled
* \#19 \[bug\] Rename file is broken for files that are existing
* \#23 \[bug\] Incorrect file size displayed while upload is pending
* \#24 RocksDB implementations should be transactional
* \#25 Writes should block when maximum cache size is reached
* \#26 Complete ring buffer and direct download support

### Changes from v2.0.1-rc

* Ability to choose between RocksDB and SQLite databases
* Added direct reads and implemented download fallback
* Corrected file times on S3 and Sia providers
* Corrected handling of `chown()` and `chmod()`
* Fixed erroneous download of chunks after resize

## v2.0.1-rc

<!-- markdownlint-disable-next-line -->
### Issues

* \#10 Address compiler warnings
* \#11 Switch to SQLite over RocksDB

### Changes from v2.0.0-rc

* Fixed intermittent deadlock on file close
* Refactored Sia, S3 and base provider
* Removed MSVC compilation support (MinGW-64 should be used)
* Removed legacy win32 build binaries
* Require `c++20`
* Switched to Storj over Filebase for hosting binaries
* Updated `OpenSSL` to v3.2.0
* Updated `boost` to v1.83.0
* Updated `cpp-httplib` to v0.14.2
* Updated `curl` to v8.4.0
* Updated `libsodium` to v1.0.19

## v2.0.0-rc

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
