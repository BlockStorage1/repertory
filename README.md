# Repertory

Repertory allows you to mount AWS S3 and Sia via FUSE on Linux ~~/OS X~~ or via WinFSP
on Windows.

## Details and Features

* Optimized for [Plex Media Server](https://www.plex.tv/)
* Single application to mount AWS S3 and/or Sia
* Remote mounting of Repertory instances on Linux ~~, OS X~~ and Windows
  * Securely share your mounts over TCP/IP (`XChaCha20-Poly1305` stream cipher)
* Cross-platform support (Linux 64-bit, Linux arm64/aarch64, ~~OS X,~~ Windows 64-bit)

## Minimum Requirements

* [Sia renterd](https://github.com/SiaFoundation/renterd/releases) v0.4.0+ for Sia support
* Only 64-bit operating systems are supported
  * By default, Linux requires `fusermount3`; otherwise, `repertory` must be manually compiled with `libfuse2` support
  * ~~OS X requires the following dependency to be installed:~~
    * ~~[FUSE for macOS v4.5.0](https://github.com/osxfuse/osxfuse/releases/download/macfuse-4.5.0/macfuse-4.5.0.dmg)~~
  * Windows requires the following dependencies to be installed:
    * [WinFSP 2023](https://github.com/winfsp/winfsp/releases/download/v2.0/winfsp-2.0.23075.msi)

## Supported Operating Systems

* Linux `arm64/aarch64`
* Linux `amd64`
* ~~OS X Mojave and above~~
* Windows 64-bit 10, 11

## Usage

### Notable Options

* `-dc`
  * Display mount configuration.
  * For Sia, `--name` is optional
  * For S3, the `-s3` option is required along with `--name`
* `--help`
  * Display all mount utility options.
* `--name, -na [name]`
  * The `--name` option can be set to any valid value allowed as a file name for your filesystem.
  * For Sia, the bucket name will be set to the same value if it is empty in the configuration file.
    * If the `--name` option is not specified, `default` will be used.
  * For S3, the `--name` option is required and does not affect the bucket name.
* `-set SiaConfig.Bucket`
  * Set Sia bucket name for the mount.
  * Can be used in combination with `--name` to target a unique configuration.
* `-set S3Config.Bucket`
  * S3 bucket name for the mount.
  * Must be used in combination with `--name` to target a unique configuration.
  * Must be used in combination with `-s3`.

### Sia

* Linux
  * `repertory /mnt/location`
  * `repertory --name default /mnt/location`
* Windows
  * `repertory.exe t:`
  * `repertory.exe --name default t:`

### S3

* Linux 
  * `repertory --name storj -s3 /mnt/location`
* Windows
  * `repertory.exe --name storj -s3 t:`

## Compiling

* Successful compilation will result in all required files being placed in the `dist/` directory
* Linux
  * Ensure `docker` is installed
    * For x86_64:
      * RelWithDebInfo: `scripts/make_unix.sh`
      * Release: `scripts/make_unix.sh x86_64 Release`
      * Debug: `scripts/make_unix.sh x86_64 Debug`
    * For aarch64:
      * RelWithDebInfo: `scripts/make_unix.sh aarch64`
      * Release: `scripts/make_unix.sh aarch64 Release`
      * Debug: `scripts/make_unix.sh aarch64 Debug`
* Windows
  * OFFICIAL: Cross-compiling on Linux
    * Ensure `docker` is installed
      * RelWithDebInfo: `scripts/make_win32.sh`
      * Release: `scripts/make_win32.sh x86_64 Release`
      * Debug: `scripts/make_win32.sh x86_64 Debug`
  * UNOFFICIAL: Compiling on Windows
    * Ensure latest [MSYS2](https://www.msys2.org/) is installed
      * RelWithDebInfo: `scripts\make_win32.cmd`
      * Release: `scripts\make_win32.cmd x86_64 Release`
      * Debug: `scripts\make_win32.cmd x86_64 Debug`

## Credits

* [boost c++ libraries](https://www.boost.org/)
* [cpp-httplib](https://github.com/yhirose/cpp-httplib)
* [curl](https://curl.haxx.se/)
* ~~[FUSE for macOS](https://osxfuse.github.io/)~~
* [Google Test](https://github.com/google/googletest)
* [JSON for Modern C++](https://github.com/nlohmann/json)
* [libfuse](https://github.com/libfuse/libfuse)
* [libsodium](https://doc.libsodium.org/)
* [OpenSSL](https://www.openssl.org/)
* [ScPrime](https://scpri.me/)
* [Sia Decentralized Cloud Storage](https://sia.tech/)
* [SQLite](https://www.sqlite.org)
* [stduuid](https://github.com/mariusbancila/stduuid)
* [Storj](https://storj.io/)
* [WinFSP - FUSE for Windows](https://github.com/billziss-gh/winfsp)
* [zlib](https://zlib.net/)

## Developer Public Key

```text
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAqXedleDOugdk9sBpgFOA
0+MogIbBF7+iXIIHv8CRBbrrf8nxLSgQvbHQIP0EklebDgLZRgyGI3SSQYj7D957
uNf1//dpkELNzfuezgAyFer9+iH4Svq46HADp5k+ugaK0mMDZM7OLOgo7415/+z4
NIQopv8prMFdxkShr4e4dpR+S6LYMYMVjsi1gnYWaZJMWgzeZouXFSscS1/XDXSE
vr1Jfqme+RmB4Q2QqGcDrY2ijumCJYJzQqlwG6liJ4FNg0U3POTCQDhQmuUoEJe0
/dyiWlo48WQbBu6gUDHbTCCUSZPs2Lc9l65MqOCpX76+VXPYetZgqpMF4GVzb2y9
kETxFNpiMYBlOBZk0I1G33wqVmw46MI5IZMQ2z2F8Mzt1hByUNTgup2IQELCv1a5
a2ACs2TBRuAy1REeHhjLgiA/MpoGX7TpoHCGyo8jBChJVpP9ZHltKoChwDC+bIyx
rgYH3jYDkl2FFuAUJ8zAZl8U1kjqZb9HGq9ootMk34Dbo3IVkc2azB2orEP9F8QV
KxvZZDA9FAFEthSiNf5soJ6mZGLi0es5EWPoKMUEd9tG5bP980DySAWSSRK0AOfE
QShT/z7oG79Orxyomwrb8ZJCi7wEfcCuK1NWgqLVUgXhpi2J9WYS6DAbF3Oh3Hhl
DYSHlcfFBteqNDlR2uFInIECAwEAAQ==
-----END PUBLIC KEY-----
```
