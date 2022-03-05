# Repertory
Repertory allows you to mount AWS S3, Sia, and Skynet \[EXPERIMENTAL\] via FUSE on Linux/OS X or via WinFSP on Windows.

# IMPORTANT
Skynet support is considered EXPERIMENTAL. Files added to Skynet should not be considered permanent. For a better understanding of how Skynet currently functions, please visit [siasky.net](https://siasky.net/).

# Details and Features
* Optimized for [Plex Media Server](https://www.plex.tv/)
* Single application to mount Sia, and/or Skynet
* Only 1 Sia, and 1 Skynet mount per user is supported.
* Remote mounting of Repertory instances (Sia, and Skynet) on Linux, OS X and Windows
    * Securely share your mounts over TCP/IP (`XChaCha20-Poly1305` stream cipher)
* Cross-platform support (Linux 64-bit, Linux arm64/aarch64, OS X, Windows 64-bit)

# Minimum Requirements
* Sia 1.4.1+
* NOTE: Windows standalone version requires the following dependencies to be installed:
    * [WinFSP 2020.2](https://github.com/billziss-gh/winfsp/releases/download/v1.8/winfsp-1.8.20304.msi)
    * [Microsoft Visual C++ Redistributable for Visual Studio 2015, 2017 and 2019](https://aka.ms/vs/16/release/vc_redist.x64.exe)
* NOTE: OS X standalone version requires the following dependency to be installed:
      * [FUSE for macOS v4.1.2](https://github.com/osxfuse/osxfuse/releases/download/macfuse-4.1.2/macfuse-4.1.2.dmg)

# Current Version
* Linux `arm64/aarch64` distribution support:
    * Debian 9
    * Debian 10
* Linux 64-bit distribution support:
    * Antergos (uses `CentOS 7`)
    * Arch Linux (uses `CentOS 7`)
    * Bodhi 5.0.0 (uses `CentOS 7`)
    * CentOS 7
    * CentOS 8 (uses `CentOS 7`)
    * Debian 9 (uses `CentOS 7`)
    * Debian 10 (uses `CentOS 7`)
    * Elementary OS 5.0, 5.1 (uses `CentOS 7`)
    * Fedora 28 (uses `CentOS 7`)
    * Fedora 29, 30, 31, 32, 33, 34 (uses `CentOS 7`)
    * Linux Mint 19, 19.1, 19.2, 19.3, 20, 20.1 (uses `CentOS 7`)
    * Manjaro (uses `CentOS 7`)
    * OpenSUSE Leap 15.0, 15.1, 15.2 (uses `CentOS 7`)
    * OpenSUSE Tumbleweed (uses `CentOS 7`)
    * Solus
    * Ubuntu 18.04 (uses `CentOS 7`)
    * Ubuntu 18.10, 19.04, 19.10, 20.04, 20.10, 21.04 (uses `CentOS 7`)

# Upcoming Features
* S3-compatible mount support

# Planned Features
* Random-write/partial upload support
* NTFS ADS support
* NTFS permissions
* Windows mount-at-boot support

# Skynet Limitations
* Aside from directory paths included in `skylink`'s, directories are local-only. A local RocksDB is used to manage directories.
* Modifications to files result in a full file re-upload and a new `skylink` will be created. There is no way to modify a file once it has been uploaded to Skynet.
* Renaming files and directories is supported under the following scenarios:
    * File has never been uploaded to Skynet
    * Parent directory path is renamed, excluding any directory paths included in the `skylink`. Some `skylink`'s include multiple files in a nested directory structure.
    * PLANNED ~~File has been previously uploaded and `EnableUploadOnRename` is set to `true` (by default this is `false`).~~
        * ~~NOTE: This may cause noticeable performance issues for files that do not reside in local cache. Repertory will need to fully download the file before it can be renamed. It will then be re-uploaded to Skynet, similar to a file modification.~~

# Usage

## Mac OS X

### Sia Mount
```repertory -o big_writes ~/sia```

### Skynet Mount
```repertory -o big_writes,sk ~/skynet```

### Remote Mount
```repertory -o big_writes -rm <IP/Hostname>:<Port> ~/remote```

* Sia remote mounts use port 20000 by default
* Skynet remote mounts use port 20003 by default

## Windows

### Sia Mount
```repertory.exe T:```

### Skynet Mount
```repertory.exe -sk T:```

### Remote Mount
```repertory -rm <IP/Hostname>:<Port> R:```

* Sia remote mounts use port 20000 by default
* Skynet remote mounts use port 20000 by default

# Sharing Mounts
All supported OS's have the ability to share local mounts with other systems. To enable sharing, make sure your drive is not mounted and execute the following commands:

## Sia Sharing
* `repertory -set RemoteMount.EnableRemoteMount true`
    * By default, port 20000 is used. To configure a different port, execute the following:
        * `repertory -set RemoteMount.RemotePort <Your Port>`
* `repertory.exe -set RemoteMount.RemoteToken <My Password>`
    * `RemoteToken` must be set to the same value on client systems.

## Skynet Sharing
* `repertory -sk -set RemoteMount.EnableRemoteMount true`
    * By default, port 20003 is used. To configure a different port, execute the following:
        * `repertory -sk -set RemoteMount.RemotePort <Your Port>`
* `repertory.exe -sk -set RemoteMount.RemoteToken <My Password>`
    * `RemoteToken` must be set to the same value on client systems.

# Mounting a Shared Remote Instance

## Configuring Connection
Before mounting the first time, you must set the client's `RemoteToken`.

* `repertory -rm <IP/Hostname>:<Port> -set RemoteMount.RemoteToken <My Password>`
    * `RemoteToken` must be set to the same value as the host system.

## Remote Mounting

### Windows
* ```repertory -rm <IP/Hostname>:<Port> R:```

# Plex Media Server on Linux
Most Linux distributions will create a service user account `plex`. Repertory will need to be mounted as the `plex` user for media discovery and playback. The following is an example of how to configure and mount Repertory for use in Plex:

```bash
sudo mkdir /home/plex
sudo chown plex.plex /home/plex
sudo -u plex repertory -o uid=<plex uid>,gid=<plex gid>,big_writes /mnt/location
```

Alternatively, you can mount as root and allow default permissions. This will create a FUSE mount that mimics a normal disk. Once mounted, you will need to set filesystem permissions according to your requirements. The following is an example of how to setup a Repertory mount in `/etc/fstab`:

```bash
sudo ln -s <path to 'repertory'> /sbin/mount.repertory
```
Example `/etc/fstab` entries for Sia:
```
none /mnt/sia repertory defaults,_netdev,default_permissions,allow_other 0 0
```
```
repertory /mnt/sia fuse defaults,_netdev,default_permissions,allow_other 0 0
```
Please note that the Sia daemon must also be launched at boot time for `fstab` mounts to work.

# Data Locations
You will find all relevant configuration and logging files in the root data directory of repertory. Consult the list below to determine this location for your operating system.

* Mac OS X
    * Sia
        * `~/Library/Application Support/repertory2/sia`
    * Skynet
        * `~/Library/Application Support/repertory2/skynet`
    * Remote Mounts
        * `~/Library/Application Support/repertory2/remote`
* Windows
    * Sia
        * `%LOCALAPPDATA%\repertory\sia`
    * Skynet
        * `%LOCALAPPDATA%\repertory\skynet`
    * Remote Mounts
        * `%LOCALAPPDATA%\repertory\remote`

# Configuration Details
If you have enabled an API password in Sia, it is recommended to pre-create repertory configuration prior to mounting.

## Mac OS X

### Generate Sia Configuration
* ```repertory -gc```

### Setting API Password
* For Sia edit:
    * ```~/Library/Application Support/repertory2/sia/config.json```

## Windows

### Generate Sia Configuration
* ```repertory.exe -gc```

### Setting API Password
* For Sia edit:
    * ```%LOCALAPPDATA%\repertory\sia\config.json```

## Sample Sia Configuration
```json
{
  "ApiAuth": "",
  "ApiPort": 11101,
  "ApiUser": "repertory",
  "ChunkDownloaderTimeoutSeconds": 30,
  "ChunkSize": 4096,
  "EnableChunkDownloaderTimeout": true,
  "EnableCommDurationEvents": false,
  "EnableDriveEvents": false,
  "EnableMaxCacheSize": true,
  "EventLevel": "Normal",
  "EvictionDelayMinutes": 30,
  "HighFreqIntervalSeconds": 30,
  "HostConfig": {
    "AgentString": "Sia-Agent",
    "ApiPassword": "",
    "ApiPort": 9980,
    "HostNameOrIp": "localhost",
    "TimeoutMs": 60000
  },
  "LowFreqIntervalSeconds": 900,
  "MaxCacheSizeBytes": 21474836480,
  "MinimumRedundancy": 2.5,
  "OnlineCheckRetrySeconds": 60,
  "OrphanedFileRetentionDays": 15,
  "PreferredDownloadType": "Fallback",
  "ReadAheadCount": 4,
  "RemoteMount": {
    "EnableRemoteMount": false,
    "IsRemoteMount": false,
    "RemoteClientPoolSize": 10,
    "RemoteHostNameOrIp": "",
    "RemoteMaxConnections": 20,
    "RemotePort": 20000,
    "RemoteReceiveTimeoutSeconds": 120,
    "RemoteSendTimeoutSeconds": 30,
    "RemoteToken": ""
  },
  "RingBufferFileSize": 512,
  "StorageByteMonth": "0",
  "Version": 5
}
```

## Sample Skynet Configuration
```json
{
  "ApiAuth": "",
  "ApiPort": 11104,
  "ApiUser": "repertory",
  "ChunkDownloaderTimeoutSeconds": 30,
  "ChunkSize": 4096,
  "EnableChunkDownloaderTimeout": true,
  "EnableCommDurationEvents": false,
  "EnableDriveEvents": false,
  "EnableMaxCacheSize": true,
  "EventLevel": "Normal",
  "EvictionDelayMinutes": 30,
  "HighFreqIntervalSeconds": 30,
  "LowFreqIntervalSeconds": 3600,
  "MaxCacheSizeBytes": 21474836480,
  "OnlineCheckRetrySeconds": 60,
  "OrphanedFileRetentionDays": 15,
  "PreferredDownloadType": "Fallback",
  "ReadAheadCount": 4,
  "RemoteMount": {
    "EnableRemoteMount": false,
    "IsRemoteMount": false,
    "RemoteClientPoolSize": 10,
    "RemoteHostNameOrIp": "",
    "RemoteMaxConnections": 20,
    "RemotePort": 20003,
    "RemoteReceiveTimeoutSeconds": 120,
    "RemoteSendTimeoutSeconds": 30,
    "RemoteToken": ""
  },
  "RingBufferFileSize": 512,
  "SkynetConfig": {
    "EncryptionToken": "",
    "PortalList": [
      "https://siasky.net",
      "https://sialoop.net",
      "https://skydrain.net",
      "https://skynet.luxor.tech",
      "https://skynet.tutemwesi.com",
      "https://siacdn.com",
      "https://vault.lightspeedhosting.com"
    ]
  },
  "StorageByteMonth": "0",
  "Version": 5
}
```

## Configuration Settings
Configuration settings that are not documented below should be ignored for now as they are used internally.

`StorageByteMonth` should never be altered. On start-up, this value is used to calculate estimated storage space.

You can change or retrieve these values using the `repertory` executable:


`repertory -get EnableDriveEvents`

`repertory -set EnableDriveEvents false`

### ApiAuth
Password used to connect to Repertory API. Auto-generated by default.

### ApiPort
Repertory API port to use for JSON-RPC requests.

* Sia default: `11101`
* Skynet default: `11104`

### ApiUser
Username used to connect to Repertory API. Default is 'repertory'.

### ChunkDownloaderTimeoutSeconds
Files that are not cached locally will download data in `ChunkSize` chunks when a read or write operation occurs. This timeout value specifies the amount of time chunks should continue downloading after the last file handle has been closed.

### ChunkSize
This is the minimum data size (converted to KiB - value of 8 means 8KiB) used for downloads. This value cannot be less than 8 and should also be a multiple of 8. Default is 2048.

### EnableDriveEvents
When set to `true`, additional logging for FUSE on UNIX or WinFSP on Windows will occur. It's best to leave this value set to 'false' unless troubleshooting an issue as enabling it may have an adverse affect on performance.

### EnableMaxCacheSize
If set to `true`, files will begin to be removed from the local cache as soon as `MaxCacheSizeBytes` and `MinimumRedundancy` have been met. This does not mean further attempts to write will fail when `MaxCacheSizeBytes` is reached. Writes will continue as long as there is enough local drive space to accommodate the operation.

If set to `false`, files will begin to be removed from the local cache as soon as `MinimumRedundancy` has been met.

In both cases, files that do not have any open handles will be chosen by oldest modification date for removal.

### EventLevel
Internally, events are fired during certain operations. This setting determines which events should be logged to `repertory.log`. Valid values are `Error`, `Warn`, `Normal`, `Debug`, and `Verbose`.

### EvictionDelayMinutes
Number of minutes to wait after all file handles are closed before allowing file to be evicted from cache.

### HostConfig.AgentString
'User-Agent' used when communicating with Sia's API.

### HostConfig.ApiPassword
Password used when communicating with Sia's API.

### HostConfig.ApiPort
API port used to connect to Sia daemon.

This is not the same as your wallet's password, so please do not enter it here. Sia typically auto-generate this value. It is exclusively used for API authentication purposes.

### HostConfig.HostNameOrIp
IP address or host name of Sia daemon.

### HostConfig.TimeoutMs
Number of milliseconds to wait for Sia API responses before timing out.

### MaxCacheSizeBytes
This value specifies the maximum amount of local space to consume before files are removed from cache. `EnableMaxCacheSize` must also be set to `true` for this value to take affect.

### MinimumRedundancy
Files are elected for removal once this value has been reached. Be aware that this value cannot be set to  less than 1.5x.

### OnlineCheckRetrySeconds
Number of seconds to wait for Sia daemon to become available/connectable.

### OrphanedFileRetentionDays
Repertory attempts to keep modifications between Sia-UI and the mounted location in sync as much as possible. In the event a file is removed from Sia-UI, it will be marked as orphaned in Repertory and moved into an 'orphaned' directory within Repertory's data directory. This setting specifies the number of days this file is retained before final deletion.

### PreferredDownloadType
Repertory supports 3 download modes for reading files that are not cached locally: full file allocation, ring buffer mode and direct mode.

Full file allocation mode pre-allocates the entire file prior to downloading. This mode is required for writes but also ensures the best performance when reading data.

Ring buffer mode utilizes a fixed size file buffer to enable a reasonable amount of seeking. This alleviates the need to fully allocate a file. By default, it is 512MiB. When the buffer is full, it attempts to maintain the ability to seek 50% ahead or behind the current read location without the need to re-download data from the provider.

Direct mode utilizes no disk space. All data is read directly from the provider.

Preferred download type modes are:

* _Fallback_ - If there isn't enough local space to allocate the full file, ring buffer mode is used. If there isn't enough local space to create the ring buffer's file, then direct mode is used.
* _RingBuffer_ - Full file allocation is always bypassed; however, if there isn't enough space to create the ring buffer's file, then direct mode will be chosen.
* _Direct_ - All files will be read directly from the provider.

### ReadAheadCount
This value specifies the number of read-ahead chunks to use when downloading a file. This is a per-open file setting and will result in the creation of an equal number of threads.

### RemoteMount.EnableRemoteMount
Allow mounting this location over TCP.

### RemoteMount.IsRemoteMount
Used internally. Should not be modified manually and cannot be `true` if `EnableRemoteMount` is also `true`.

This value is used on client systems to indicate the mount is not local.

### RemoteMount.RemoteClientPoolSize
Number of threads to use for each unique client. Only available when `EnableRemoteMount` is `true`.

### RemoteMount.RemoteHostNameOrIp
Host name or IP of host to connect to for remote mounting.

Only available when `IsRemoteMount` is `true`.

### RemoteMount.RemoteMaxConnections
Maximum number of TCP connections to use when communicating with remote instances.

### RemoteMount.RemotePort
TCP port used for remote mounting.

### RemoteMount.RemoteToken
Encryption token used for remote mounts. This value must be the same on local and remote systems.

### RingBufferFileSize
The size of the ring buffer file in MiB. Default is 512. Valid values are: 64, 128, 256, 512, 1024.

### Version
Configuration file version. This is used internally and should not be modified.

# Compiling

## Linux 64-bit
* Requires [CMake](https://cmake.org) 3.1 or above
* Requires ```fuse``` 2.9.x binary/kernel module
* Requires ```libfuse``` 2.9.x binary and development package
* Requires ```openssl``` binary and development package
* Requires ```zlib``` binary and development package
* Execute the following from the ```repertory``` source directory:
    * ```mkdir build && cd build```
    * Debug build:
        * ```cmake -DCMAKE_BUILD_TYPE=Debug ..```
    * Release build:
        * ```cmake -DCMAKE_BUILD_TYPE=Release ..```
    * ```make```
    * Optionally:
        * ```sudo make install```

## Mac OS X
* Requires XCode from Apple Store
* Requires [CMake](https://cmake.org) 3.1 or above
* Requires [FUSE for macOS v4.1.2](https://github.com/osxfuse/osxfuse/releases/download/macfuse-4.1.2/macfuse-4.1.2.dmg)
* Execute the following from the ```repertory``` source directory:
    * ```mkdir build && cd build```
    * Debug build:
        * ```cmake -DCMAKE_BUILD_TYPE=Debug ..```
    * Release build:
        * ```cmake -DCMAKE_BUILD_TYPE=Release ..```
    * ```make```

## Windows 7 or above 64-bit
* Install [cmake-3.15.2](https://github.com/Kitware/CMake/releases/download/v3.15.2/cmake-3.15.2-win64-x64.msi)
* Install [Win32 OpenSSL](https://slproweb.com/products/Win32OpenSSL.html) 64-bit version
* Install [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/)
    * During installation, make sure to install:
        * C++ development support
        * Windows 10 SDK
* Install [WinFSP 2020.2](https://github.com/billziss-gh/winfsp/releases/download/v1.8/winfsp-1.8.20304.msi)
* Execute Windows build script
    * Debug build (64-bit)
        * `scripts\64_bit\build_win64_debug.cmd`
    * Release build (64-bit)
        * `scripts\64_bit\build_win64_release.cmd`
    * NOTE: `cmake.exe` must be in Windows search path. If not, modify the following line in `build_win64.cmd`:
        * `set CMAKE="<Full path to cmake.exe>"`

# Credits
* [AWS SDK for C++](https://aws.amazon.com/sdk-for-cpp/)
* [boost c++ libraries](https://www.boost.org/)
* [curl](https://curl.haxx.se/)
* [FUSE for macOS](https://osxfuse.github.io/)
* [Google Test](https://github.com/google/googletest)
* [JSON for Modern C++](https://github.com/nlohmann/json)
* [jsonrpcpp - C++ JSON-RPC 2.0 library](https://github.com/badaix/jsonrpcpp)
* [libfuse](https://github.com/libfuse/libfuse)
* [libhttpserver](https://github.com/etr/libhttpserver)
* [libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/)
* [OpenSSL](https://www.openssl.org/)
* [OSSP uuid](http://www.ossp.org/pkg/lib/uuid/)
* [RocksDB](https://rocksdb.org/)
* [Sia Decentralized Cloud Storage](https://sia.tech/)
* [ttmath - Bignum C++ library](https://www.ttmath.org/)
* [WinFSP - FUSE for Windows](https://github.com/billziss-gh/winfsp)
* [zlib](https://zlib.net/)

# Developer Public Key
```
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
