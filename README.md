# Repertory

Repertory allows you to mount S3 and Sia via FUSE on Linux or via WinFSP
on Windows.

## Contents

1. [Details and Features](#details-and-features)
2. [Minimum Requirements](#minimum-requirements)
   1. [Supported Operating Systems](#supported-operating-systems)
3. [GUI](#gui)
4. [Usage](#usage)
   1. [Important Options](#important-options)
   2. [Sia](#sia)
      * [Sia Initial Configuration](#sia-initial-configuration)
      * [Sia Mounting](#sia-mounting)
      * [Sia Configuration File](#sia-configuration-file)
   3. [S3](#s3)
      * [S3 Initial Configuration](#s3-initial-configuration)
      * [S3 Mounting](#s3-mounting)
      * [S3 Configuration File](#s3-configuration-file)
5. [Data Directories](#data-directories)
   1. [Linux Directories](#linux-directories)
   2. [Windows Directories](#windows-directories)
6. [Remote Mounting](#remote-mounting)
   1. [Server Setup](#server-setup)
      * [Remote Mount Configuration File Section](#remote-mount-configuration-file-section)
   2. [Client Setup](#client-setup)
      * [Client Remote Mounting](#client-remote-mounting)
      * [Remote Mount Configuration File](#remote-mount-configuration-file)
7. [Compiling](#compiling)
   1. [Linux Compilation](#linux-compilation)
   2. [Windows Setup](#windows-compilation)
8. [Credits](#credits)
9. [Developer Public Key](#developer-public-key)
10. [Consult the Wiki for additional information](https://git.fifthgrid.com/BlockStorage/repertory/wiki)

## Details and Features

* Optimized for [Plex Media Server](https://www.plex.tv/)
* Remote mounting of `repertory` instances on Linux and Windows
  * Securely share your mounts over TCP/IP via `XChaCha20-Poly1305` with other systems on your network or over the internet.
* Cross-platform support (Linux 64-bit, Linux arm64/aarch64, Windows 64-bit)
* Optionally encrypt file names and file data via `XChaCha20-Poly1305` in S3 mounts

## Minimum Requirements

* [Sia renterd](https://github.com/SiaFoundation/renterd/releases) v2.0.0+ for Sia support
* Linux requires `fusermount3`; otherwise, `repertory` must be manually compiled with `libfuse2` support
* Windows requires the following dependencies to be installed:
  * [WinFSP 2025](https://github.com/winfsp/winfsp/releases/download/v2.1/winfsp-2.1.25156.msi)

### Supported Operating Systems

Only 64-bit operating systems are supported

* Linux `arm64/aarch64`
* Linux `amd64`
* Windows 64-bit 10, 11

## GUI

As of `v2.0.5-rc`, mounts can be managed using the `Repertory Management Portal`.
To launch the portal, execute the following command:

* `repertory -ui`
  * The default username is `repertory`
  * The default password is `repertory`

After first launch, `ui.json` will be created in the appropriate data directory.
See [Data Directories](#data-directories).
You should modify this file directly or use the portal to change the default
username and password.

### Screenshot
 
<a href="https://ibb.co/fVyJqnbF"><img src="https://i.ibb.co/fVyJqnbF/repertory-portal.png" alt="repertory-portal" border="0"></a>

## Usage

### Important Options

* `--help`
  * Display all mount utility options

* `-f`
  * Keep process in foreground on Linux.

* `--name, -na [name]`
  * Identifies a unique configuration name to support multiple mounts.
  * The `--name` option can be set to any valid value allowed as a file name for your filesystem.
  * For Sia, the bucket name will be set to the same value if it is empty in the configuration file.
    * If the `--name` option is not specified, `default` will be used.
  * For S3, the `--name` option is required and does not affect the bucket name.

* `-dc`
  * Display mount configuration
  * For Sia, `--name` is optional
  * For S3, the `-s3` option is required along with `--name`

### Sia

#### Sia Initial Configuration

* Required steps:
  * Set the appropriate bucket name and `renterd` API password in `repertory` configuration:
    * To use `default` as the bucket name and configuration name, you only need to set the `renterd` API password:
      * `repertory -set HostConfig.ApiPassword '<my password>'`
    * To specify a different bucket name while using `default` as the configuration name:
      * `repertory -set HostConfig.ApiPassword '<my password>'`
      * `repertory -set SiaConfig.Bucket '<my bucket>'`
    * For all other configurations:
      * `repertory --name '<my config name>' -set HostConfig.ApiPassword '<my password>'`
      * `repertory --name '<my config name>' -set SiaConfig.Bucket '<my bucket name>'`

* Optional steps:
  * Set a user name used during `renterd` basic authentication:
    * `repertory -set HostConfig.ApiUser '<my user>'`
    * `repertory --name '<my config name>' -set HostConfig.ApiUser '<my user>'`
  * Set a custom agent string (default `Sia-Agent`):
    * `repertory -set HostConfig.AgentString '<my agent>'`
    * `repertory --name '<my config name>' -set HostConfig.AgentString '<my agent>'`
  * Set the host name or IP of the `renterd` instance (default `localhost`):
    * `repertory -set HostConfig.HostNameOrIp '<my host name>'`
    * `repertory --name '<my config name>' -set HostConfig.HostNameOrIp '<my host name>'`
  * Set the `renterd` API port (default `9980`):
    * `repertory -set HostConfig.ApiPort 9981`
    * `repertory --name '<my config name>' -set HostConfig.ApiPort 9981`

* To verify/view all configuration options:
  * `repertory -dc`
  * `repertory --name '<my config name>' -dc`
    * Example:
      * `repertory --name default -dc`

#### Sia Mounting

* Linux:
  * `repertory /mnt/location`
  * `repertory --name '<my config name>' /mnt/location`
    * Example:
      * `repertory --name default /mnt/location`

* Windows:
  * `repertory t:`
  * `repertory --name '<my config name>' t:`
    * Example:
      * `repertory --name default t:`

#### Sia Configuration File

```json
{
  "ApiPassword": "<random generated rpc password>",
  "ApiPort": 10000,
  "ApiUser": "repertory",
  "DatabaseType": "rocksdb",
  "DownloadTimeoutSeconds": 30,
  "EnableDownloadTimeout": true,
  "EnableDriveEvents": false,
  "EventLevel": "info",
  "EvictionDelayMinutes": 1,
  "EvictionUseAccessedTime": false,
  "HighFreqIntervalSeconds": 30,
  "HostConfig": {
    "AgentString": "Sia-Agent",
    "ApiPassword": "<renterd api password>",
    "ApiPort": 9980,
    "ApiUser": "",
    "HostNameOrIp": "localhost",
    "Path": "",
    "Protocol": "http",
    "TimeoutMs": 60000
  },
  "LowFreqIntervalSeconds": 3600,
  "MaxCacheSizeBytes": 21474836480,
  "MaxUploadCount": 5,
  "MedFreqIntervalSeconds": 120,
  "OnlineCheckRetrySeconds": 60,
  "PreferredDownloadType": "default",
  "RemoteMount": {
    "ApiPort": 20000,
    "ClientPoolSize": 20,
    "Enable": false,
    "EncryptionToken": ""
  },
  "RetryReadCount": 6,
  "RingBufferFileSize": 512,
  "SiaConfig": {
    "Bucket": "default"
  },
  "TaskWaitMs": 100,
  "Version": 1
}
```

### S3

#### S3 Initial Configuration

* Required steps:
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

#### S3 Mounting

* Linux:
  * `repertory -s3 --name '<my config name>' /mnt/location`
    * Example:
      * `repertory -s3 --name minio /mnt/location`

* Windows:
  * `repertory -s3 --name '<my config name>' t:`
    * Example:
      * `repertory -s3 --name minio t:`

#### S3 Configuration File

```json
{
  "ApiPassword": "<random generated rpc password>",
  "ApiPort": 10100,
  "ApiUser": "repertory",
  "DatabaseType": "rocksdb",
  "DownloadTimeoutSeconds": 30,
  "EnableDownloadTimeout": true,
  "EnableDriveEvents": false,
  "EventLevel": "info",
  "EvictionDelayMinutes": 1,
  "EvictionUseAccessedTime": false,
  "HighFreqIntervalSeconds": 30,
  "LowFreqIntervalSeconds": 3600,
  "MaxCacheSizeBytes": 21474836480,
  "MaxUploadCount": 5,
  "MedFreqIntervalSeconds": 120,
  "OnlineCheckRetrySeconds": 60,
  "PreferredDownloadType": "default",
  "RemoteMount": {
    "ApiPort": 20100,
    "ClientPoolSize": 20,
    "Enable": false,
    "EncryptionToken": ""
  },
  "RetryReadCount": 6,
  "RingBufferFileSize": 512,
  "S3Config": {
    "AccessKey": "<my access key>",
    "Bucket": "<my bucket name>",
    "EncryptionToken": "",
    "Region": "any",
    "SecretKey": "<my secret key>",
    "TimeoutMs": 60000,
    "URL": "http://localhost:9000",
    "UsePathStyle": true,
    "UseRegionInURL": false
  },
  "TaskWaitMs": 100,
  "Version": 1
}
```

### Data Directories

#### Linux Directories

* `~/.local/repertory2/s3`
* `~/.local/repertory2/sia`

#### Windows Directories

* `%LOCALAPPDATA%\repertory2\s3`
* `%LOCALAPPDATA%\repertory2\sia`
  * Examples:
    * `C:\Users\Tom\AppData\Local\repertory2\s3`
    * `C:\Users\Tom\AppData\Local\repertory2\sia`
* IMPORTANT:
  * It is highly recommended to exclude this folder from any anti-virus/anti-malware applications as severe performance issues may arise.
  * Excluding the mounted drive letter is also highly recommended.

## Remote Mounting

`repertory` allows local mounts to be shared with other computers on your network
or over the internet. This option is referred to as remote mounting.

### Server Setup

The following steps must be performed on the mount you wish to share with
other systems. Changes to configuration will not take affect while a mount is
active, so it is recommended to unmount beforehand.

* Required steps:
  * Enable remote mount:
    * Sia
      * `repertory -set RemoteMount.Enable true`
      * `repertory --name '<my config name>' -set RemoteMount.Enable true`
    * S3:
      * `repertory -s3 --name '<my config name>' -set RemoteMount.Enable true`
  * Set a secure encryption token:
    * Sia:
      * `repertory -set RemoteMount.EncryptionToken '<my secure password>'`
      * `repertory --name '<my config name>' -set RemoteMount.EncryptionToken '<my secure password>'`
    * S3:
      * `repertory -s3 --name '<my config name>' -set RemoteMount.EncryptionToken '<my secure password>'`

* Optional steps:
  * Change the port clients will use to connect to your mount:
    * Sia:
      * `repertory -set RemoteMount.ApiPort 20000`
      * `repertory --name '<my config name>' -set RemoteMount.ApiPort 20000`
    * S3:
      * `repertory -s3 --name '<my config name>' -set RemoteMount.ApiPort 20000`

* IMPORTANT:
  * Be sure to configure your firewall to allow incoming TCP connections on the port configured in `RemoteMount.ApiPort`.

#### Remote Mount Configuration File Section

```json
{
  ...
  "RemoteMount": {
    "ApiPort": 20000,
    "ClientPoolSize": 20,
    "Enable": true,
    "EncryptionToken": "<my secure password>"
  },
  ...
}
```

### Client Setup

Client configuration is provider agnostic, so there's no need to specify `-s3`
for S3 providers.

* Required steps:
  * Set the encryption token to the same value configured during server setup:
    * `repertory -rm <host name or IP>:<port> -set RemoteConfig.EncryptionToken '<my secure password>'`
      * Replace `<host name or IP>` with the host name or IP of the server
      * Replace `<port>` with the value of `RemoteMount.ApiPort` used in the server configuration
    * Example:
      * `repertory -rm 192.168.1.10:20000 -set RemoteConfig.EncryptionToken '<my secure password>'`
      * `repertory -rm my.host.com:20000 -set RemoteConfig.EncryptionToken '<my secure password>'`

#### Client Remote Mounting

* Linux:
  * `repertory -rm <host name or IP>:<port> /mnt/location`
    * Example:
      * `repertory -rm 192.168.1.10:20000 /mnt/location`

* Windows:
  * `repertory -rm <host name or IP>:<port> t:`
    * Example:
      * `repertory -rm 192.168.1.10:20000 t:`

#### Remote Mount Configuration File

```json
{
  "ApiPassword": "<random generated rpc password>",
  "ApiPort": 10010,
  "ApiUser": "repertory",
  "EnableDriveEvents": false,
  "EventLevel": "info",
  "RemoteConfig": {
    "ApiPort": 20000,
    "EncryptionToken": "<my secure password>",
    "HostNameOrIp": "192.168.1.10",
    "MaxConnections": 20,
    "ReceiveTimeoutMs": 120000,
    "SendTimeoutMs": 30000
  },
  "TaskWaitMs": 100,
  "Version": 1
}
```

## Compiling

Successful compilation will result in all files required for execution to be placed
in the `dist/` directory

### Linux Compilation

* Ensure `docker` is installed
  * For x86_64:
    * RelWithDebInfo: `scripts/make_unix.sh`
    * Release: `scripts/make_unix.sh x86_64 Release`
    * Debug: `scripts/make_unix.sh x86_64 Debug`

  * For aarch64:
    * RelWithDebInfo: `scripts/make_unix.sh aarch64`
    * Release: `scripts/make_unix.sh aarch64 Release`
    * Debug: `scripts/make_unix.sh aarch64 Debug`

### Windows Compilation

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
