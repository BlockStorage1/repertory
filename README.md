# Repertory

Repertory allows you to mount S3 and Sia via FUSE on Linux or via WinFSP
on Windows.

## Details and Features

* Optimized for [Plex Media Server](https://www.plex.tv/)
* Single application to mount S3 and/or Sia
* Remote mounting of Repertory instances on Linux and Windows
  * Securely share your mounts over TCP/IP via `XChaCha20-Poly1305` with other systems on your network or over the internet.
* Cross-platform support (Linux 64-bit, Linux arm64/aarch64, Windows 64-bit)
* Optionally encrypt file names and file data via `XChaCha20-Poly1305` in S3 mounts

## Minimum Requirements

* [Sia renterd](https://github.com/SiaFoundation/renterd/releases) v2.0.0+ for Sia support
* Only 64-bit operating systems are supported
  * By default, Linux requires `fusermount3`; otherwise, `repertory` must be manually compiled with `libfuse2` support
  * Windows requires the following dependencies to be installed:
    * [WinFSP 2023](https://github.com/winfsp/winfsp/releases/download/v2.0/winfsp-2.0.23075.msi)

## Supported Operating Systems

* Linux `arm64/aarch64`
* Linux `amd64`
* Windows 64-bit 10, 11

## Usage

### Sia

* Initial Configuration
  * Sia steps:
    * Set the appropriate bucket name and `renterd` API password in `repertory` configuration:
      * To use `default` as the bucket name and configuration name:
        * `repertory -set HostConfig.ApiPassword '<my password>'`
      * To use a different bucket name with `default` as the configuration name:
        * `repertory -set HostConfig.ApiPassword '<my password>'`
        * `repertory -set SiaConfig.Bucket '<my bucket>'`
      * For all other configurations:
        * `repertory --name '<my config name>' -set HostConfig.ApiPassword '<my password>'`
        * `repertory --name '<my config name>' -set SiaConfig.Bucket '<my bucket name>'`
  * To verify/view all configuration options:
    * `repertory -dc`
    * `repertory --name '<my config name>' -dc`
      * Example:
        * `repertory --name default -dc`
* Mounting on Linux:
  * `repertory /mnt/location`
  * `repertory --name '<my config name>' /mnt/location`
    * Example:
      * `repertory --name default /mnt/location`
* Mounting on Windows:
  * `repertory t:`
  * `repertory --name '<my config name>' t:`
    * Example:
      * `repertory --name default t:`

### S3

* Initial Configuration
  * S3 steps:
    * Set the appropriate base URL:
      * `repertory -s3 --name '<my config name>' -set S3Config.URL '<my url>'`
        * Example:
          * `repertory -s3 --name minio -set S3Config.URL 'http://localhost:9000'`
    * Set the appropriate bucket name:
      * `repertory -s3 --name '<my config name>' -set S3Config.Bucket '<my bucket name>'`
    * Set the appropriate access key:
      * `repertory -s3 --name '<my config name>' -set S3Config.AccessKey '<my access key>'`
    * Set the appropriate secret key:
      * `repertory -s3 --name '<my config name>' -set S3Config.SecretKey '<my secret key>'`
    * For Sia and most local S3 gateway instances, enable path style URL's:
      * `repertory -s3 --name '<my config name>' -set S3Config.UsePathStyle true`
    * Optional steps:
      * Set an appropriate region. Default is set to `any`:
        * `repertory -s3 --name '<my config name>' -set S3Config.Region '<my region>'`
      * Enable encrypted file names and file data. Set a strong, random encryption token and be sure to store it in a secure backup location:
        * `repertory -s3 --name '<my config name>' -set S3Config.EncryptionToken '<my strong password>'`
  * To verify/view all configuration options:
    * `repertory -s3 --name '<my config name>' -dc`
      * Example:
        * `repertory -s3 --name minio -dc`
* Mounting on Linux:
  * `repertory -s3 --name '<my config name>' /mnt/location`
    * Example:
      * `repertory -s3 --name minio /mnt/location`
* Mounting on Windows:
  * `repertory -s3 --name '<my config name>' t:`
    * Example:
      * `repertory -s3 --name minio t:`

### Notable Options

* `--help`
  * Display all mount utility options
* `--name, -na [name]`
  * The `--name` option can be set to any valid value allowed as a file name for your filesystem.
  * For Sia, the bucket name will be set to the same value if it is empty in the configuration file.
    * If the `--name` option is not specified, `default` will be used.
  * For S3, the `--name` option is required and does not affect the bucket name.
* `-dc`
  * Display mount configuration
  * For Sia, `--name` is optional
  * For S3, the `-s3` option is required along with `--name`

### Data Directories

* Linux
  * `~/.local/repertory2`
* Windows
  * `%LOCALAPPDATA%\repertory2`
    * Example:
      * `C:\Users\Tom\AppData\Local\repertory2`
    * IMPORTANT:
      * It is highly recommended to exclude this folder from any anti-virus/anti-malware applications as severe performance issues may arise.
      * Excluding the mounted drive letter is also highly recommended.

## Remote Mounting

`Repertory` allows local mounts to be shared with other computers on your network.
This option is referred to as remote mounting. Instructions TBD.

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

* [binutils](https://www.gnu.org/software/binutils/)
* [boost c++ libraries](https://www.boost.org/)
* [cpp-httplib](https://github.com/yhirose/cpp-httplib)
* [curl](https://curl.haxx.se/)
* [docker](https://www.docker.com/)
* [Google Test](https://github.com/google/googletest)
* [ICU](https://icu.unicode.org/)
* [JSON for Modern C++](https://github.com/nlohmann/json)
* [libexpat](https://github.com/libexpat/libexpat)
* [libfuse](https://github.com/libfuse/libfuse)
* [libsodium](https://doc.libsodium.org/)
* [mingw-w64](https://www.mingw-w64.org/)
* [MSYS2](https://www.msys2.org)
* [OpenSSL](https://www.openssl.org/)
* [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)
* [pugixml](https://pugixml.org/)
* [RocksDB](https://rocksdb.org)
* [ScPrime](https://scpri.me/)
* [Sia Decentralized Cloud Storage](https://sia.tech/)
* [spdlog](https://github.com/gabime/spdlog)
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
