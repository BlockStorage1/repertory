# Changelog
## 2.0.0-rc
* \#5 S3 mounts should support encryption
* \#136: Simultaneous Skynet uploads not being tracked in portals
* \#131: Label 'centos7' amd64 build as generic 'linux' amd64
* \#132: Switch to `XChaCha20-Poly1305` for Skynet encryption
* Added replay protection to remote mounts
* Support TUS uploads on Skynet (large upload support)
* Removed 1.1.x and 1.2.x releases
* Refactored build scripts
* Support remote FUSE base64 writes
* Switched to `XChaCha20-Poly1305` for remote mounts
* Updated `cURL` to v7.77.0
* Updated `OpenSSL` to v1.1.1k
* Updated `libmicrohttpd` to v0.9.73
* Fixed encrypted Skynet import
* Implemented chunked read and write
* Removed `repertory-ui` support
* Case refactoring
* Writes for non-cached files are performed in chunks of 8Mib
