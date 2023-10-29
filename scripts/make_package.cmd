@echo off
setlocal
setlocal enabledelayedexpansion

set SIGNING_FOLDER=%1
set BINARY_FOLDER=%2
set SOURCE_FOLDER=%3
set OUTPUT_FOLDER=%4

set PATH=%~dp0%..\bin;!PATH!

if "%SIGNING_FOLDER%" == "" (
  call :EXIT_SCRIPT "'SIGNING_FOLDER' is not set (arg1)"
)

if "%BINARY_FOLDER%" == "" (
  call :EXIT_SCRIPT "'BINARY_FOLDER' is not set (arg2)"
)

if "%SOURCE_FOLDER%" == "" (
  call :EXIT_SCRIPT "'SOURCE_FOLDER' is not set (arg3)"
)

if "%OUTPUT_FOLDER%" == "" (
  call :EXIT_SCRIPT "'OUTPUT_FOLDER' is not set (arg4)"
)

call :NORMALIZE "%SIGNING_FOLDER%"
set SIGNING_FOLDER=%RETVAL%

call :NORMALIZE "%BINARY_FOLDER%"
set BINARY_FOLDER=%RETVAL%

call :NORMALIZE "%SOURCE_FOLDER%"
set SOURCE_FOLDER=%RETVAL%

set BUILD_ARCH=Win64
set OPENSSL_BIN="c:\OpenSSL-%BUILD_ARCH%\bin\openssl.exe"
if NOT EXIST %OPENSSL_BIN% (
  set OPENSSL_BIN="c:\Program Files\OpenSSL-%BUILD_ARCH%\bin\openssl.exe"
  set REPERTORY_OPENSSL_ROOT="c:\Program Files\OpenSSL-%BUILD_ARCH%"
)

pushd "%SOURCE_FOLDER%"
  for /F "tokens=*" %%f in ('git rev-parse --short HEAD') do (set GIT_REV=%%f)
  for /F "tokens=*" %%f in ('git branch --show-current') do (set GIT_BRANCH=%%f)
  for /F "tokens=*" %%f in ('grep set(REPERTORY_MAJOR .\CMakeLists.txt ^| sed "s/)//g" ^| awk "{print $2}"') do (set REPERTORY_VERSION=%%f)
  for /F "tokens=*" %%f in ('grep set(REPERTORY_MINOR .\CMakeLists.txt ^| sed "s/)//g" ^| awk "{print $2}"') do (set REPERTORY_VERSION=%REPERTORY_VERSION%.%%f)
  for /F "tokens=*" %%f in ('grep set(REPERTORY_REV .\CMakeLists.txt ^| sed "s/)//g" ^| awk "{print $2}"') do (set REPERTORY_VERSION=%REPERTORY_VERSION%.%%f)
  for /F "tokens=*" %%f in ('grep set(REPERTORY_RELEASE_ITER .\CMakeLists.txt ^| sed "s/)//g" ^| awk "{print $2}"') do (set REPERTORY_VERSION=%REPERTORY_VERSION%-%%f)
popd

if "%GIT_BRANCH%" == "development" (
  set RELEASE_FOLDER=nightly
) else (
  set RELEASE_FOLDER=%REPERTORY_RELEASE_ITER%
)

call :NORMALIZE "%OUTPUT_FOLDER%\!RELEASE_FOLDER!"
set OUTPUT_FOLDER=%RETVAL%

set OUT_FILE=repertory_%REPERTORY_VERSION%_%GIT_REV%_windows_amd64.zip
set OUT_ZIP=%BINARY_FOLDER%\%OUT_FILE%
set FILE_LIST=repertory.exe repertory.exe.sha256 repertory.exe.sig winfsp-x64.dll winfsp-x64.dll.sha256 winfsp-x64.dll.sig cacert.pem cacert.pem.sha256 cacert.pem.sig

pushd "%BINARY_FOLDER%"
  call :CLEANUP

  call :CREATE_HASH "%BINARY_FOLDER%\repertory.exe"
  call :CREATE_HASH "%BINARY_FOLDER%\winfsp-x64.dll"
  call :CREATE_HASH "%BINARY_FOLDER%\cacert.pem"

  (7za u "%OUT_FILE%" %FILE_LIST%) || (7z u "%OUT_FILE%" %FILE_LIST%) || (call :EXIT_SCRIPT "Create repertory zip failed")

  call :CREATE_HASH "%OUT_FILE%"

  copy /y "%OUT_ZIP%" "%OUTPUT_FOLDER%" || call :EXIT_SCRIPT "Copy %OUT_ZIP% to %OUTPUT_FOLDER% failed"
  copy /y "%OUT_ZIP%.sha256" "%OUTPUT_FOLDER%" || call :EXIT_SCRIPT "Copy %OUT_ZIP%.sha256 to %OUTPUT_FOLDER% failed"
  copy /y "%OUT_ZIP%.sig" "%OUTPUT_FOLDER%" || call :EXIT_SCRIPT "Copy %OUT_ZIP%.sig to %OUTPUT_FOLDER% failed"

  call :CLEANUP
popd
goto :END

:CREATE_HASH
  call :NORMALIZE %1
  set HASH_FILE=%RETVAL%

  (%OPENSSL_BIN% dgst -sha256 -sign "%SIGNING_FOLDER%\blockstorage_dev_private.pem" -out "%HASH_FILE%.sig" "%HASH_FILE%") || (call :EXIT_SCRIPT "Create %HASH_FILE% signature failed")
  (%OPENSSL_BIN% dgst -sha256 -verify "%SIGNING_FOLDER%\blockstorage_dev_public.pem" -signature "%HASH_FILE%.sig" "%HASH_FILE%") || (call :EXIT_SCRIPT "Verify %HASH_FILE% signature failed")
  ((certutil -hashfile "%HASH_FILE%" SHA256 | sed -e "1d" -e "$d" -e "s/\ //g") > "%HASH_FILE%.sha256") || (call :EXIT_SCRIPT "Create %HASH_FILE% sha-256 failed")
  EXIT /B

:CLEANUP
  del /q "%OUT_ZIP%" 1>NUL 2>&1 
  del /q "%OUT_ZIP%.sha256" 1>NUL 2>&1 
  del /q "%OUT_ZIP%.sig" 1>NUL 2>&1 
  del /q "%BINARY_FOLDER%\cacert.pem.sha256" 1>NUL 2>&1 
  del /q "%BINARY_FOLDER%\cacert.pem.sig" 1>NUL 2>&1 
  del /q "%BINARY_FOLDER%\repertory.exe.sha256" 1>NUL 2>&1 
  del /q "%BINARY_FOLDER%\repertory.exe.sig" 1>NUL 2>&1 
  del /q "%BINARY_FOLDER%\winfsp-x64.dll.sha256" 1>NUL 2>&1 
  del /q "%BINARY_FOLDER%\winfsp-x64.dll.sig" 1>NUL 2>&1 
  EXIT /B

:NORMALIZE
  SET RETVAL=%~f1
  exit /B

:EXIT_SCRIPT
  echo %1
  exit 1

:END
  echo Done
