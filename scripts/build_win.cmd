@echo off
setlocal EnableDelayedExpansion

set ROOT=%~dp0%\..
pushd "%ROOT%"

set ROOT=%CD%
set PATH=%ROOT%\bin;%ROOT%\bin\curl\bin;%PATH%
set CMAKE=cmake
set VS_CMAKE_GENERATOR=Visual Studio 16 2019
set TARGET_MODE=%1
set /a ENABLE_PACKAGING=%2
set /a DISABLE_TESTING=%3
set /a DISABLE_PAUSE=%4
shift
set REPOSITORY=repertory
set ERROR_EXIT=0

set OPENSSL_BIN="c:\OpenSSL-%BUILD_ARCH%\bin\openssl.exe"
set REPERTORY_OPENSSL_ROOT="c:\OpenSSL-%BUILD_ARCH%"
set BUILD_ARCH=Win64
set BUILD_ARCH_LOWER=win64
set MSBUILD_ARCH=x64
set WINFSP_BASE_NAME=winfsp-x64
if NOT EXIST %OPENSSL_BIN% (
  set OPENSSL_BIN="c:\Program Files\OpenSSL-!BUILD_ARCH!\bin\openssl.exe"
  set REPERTORY_OPENSSL_ROOT="c:\Program Files\OpenSSL-!BUILD_ARCH!"
)

set PRIVATE_KEY="c:\src\cert\blockstorage_dev_private.pem"
set PUBLIC_KEY="%ROOT%\blockstorage_dev_public.pem"

if "%TARGET_MODE%" NEQ "Debug" (
  if "%TARGET_MODE%" NEQ "Release" (
    call :ERROR "[Debug|Release] not specified"
  )
)

if "%APPVEYOR_BUILD%" == "1" (
  set CMAKE_OPTS=-DOPENSSL_ROOT_DIR=C:\OpenSSL-v111-!BUILD_ARCH!
) else (
  set CMAKE_OPTS=-DOPENSSL_ROOT_DIR=!REPERTORY_OPENSSL_ROOT!
)

if "%DISABLE_SIGNING%" == "1" (
  set CMAKE_OPTS=%CMAKE_OPTS% -DENABLE_SIGNING=OFF
) else (
  if "%USERNAME%" == "sgraves" (
    set CMAKE_OPTS=%CMAKE_OPTS% -DENABLE_SIGNING=ON
  )
  if "%USERNAME%" == "scott" (
    set CMAKE_OPTS=%CMAKE_OPTS% -DENABLE_SIGNING=ON
  )
  if "%USERNAME%" == "Scott Graves" (
    set CMAKE_OPTS=%CMAKE_OPTS% -DENABLE_SIGNING=ON
  )
)

set BUILD_DIR=build%OUTPUT_DIR_EXTRA%
for /F "tokens=*" %%f in ('git rev-parse --short HEAD') do (set GIT_REV=%%f)

mkdir "%BUILD_DIR%" >NUL 2>&1
mkdir "%BUILD_DIR%\%TARGET_MODE%" >NUL 2>&1
pushd "%BUILD_DIR%\%TARGET_MODE%" >NUL 2>&1  
  echo Building [%TARGET_MODE%] [%BUILD_ARCH%]
  ((%CMAKE% "%ROOT%" %CMAKE_OPTS% -A %MSBUILD_ARCH% -DCMAKE_BUILD_TYPE=%TARGET_MODE% -DCMAKE_GENERATOR="%VS_CMAKE_GENERATOR%" -DCMAKE_CONFIGURATION_TYPES="%TARGET_MODE%" && (%CMAKE% --build . --config %TARGET_MODE%)) || (
    popd
    call :ERROR "Compile Error"
  )

  for /f "tokens=*" %%i in ('grep -m1 -a REPERTORY_VERSION repertory.vcxproj ^| sed -e "s/.*REPERTORY_VERSION=\"//g" -e "s/\".*//g"') do (
    set APP_VER=%%i
  )
  
  for /f "tokens=2 delims=-" %%a in ("%APP_VER%") do (
    set APP_RELEASE_TYPE=%%a
  )
  for /f "tokens=1 delims=." %%a in ("%APP_RELEASE_TYPE%") do (
    set APP_RELEASE_TYPE=%%a
  )

  if "%APP_RELEASE_TYPE%"=="alpha" set APP_RELEASE_TYPE=Alpha
  if "%APP_RELEASE_TYPE%"=="beta" set APP_RELEASE_TYPE=Beta
  if "%APP_RELEASE_TYPE%"=="rc" set APP_RELEASE_TYPE=RC
  if "%APP_RELEASE_TYPE%"=="release" set APP_RELEASE_TYPE=Release   

  echo Repertory Version: %APP_VER%
  set TARGET_MODE_LOWER=%TARGET_MODE%
  call :LOWERCASE TARGET_MODE_LOWER

  set OUT_NAME=repertory_%APP_VER%_%GIT_REV%_!TARGET_MODE_LOWER!_%BUILD_ARCH_LOWER%.zip
  set OUT_FILE=..\..\%OUT_NAME%

  pushd %TARGET_MODE%
    if "%DISABLE_TESTING%" NEQ "1" (
      echo Testing [%TARGET_MODE%]
      del /q unittests.err 1>NUL 2>&1
      start "Unittests" /wait /min cmd /v:on /c "unittests.exe>unittests.log 2>&1 || echo ^!errorlevel^!>unittests.err"
      if EXIST unittests.err (
        call :ERROR "Unittests Failed"
      )
    )

    if "%APPVEYOR_BUILD%" NEQ "1" (
      echo Signing Application
      del /q repertory.exe.sha256 1>NUL 2>&1
      del /q repertory.exe.sig 1>NUL 2>&1
      ((certutil -hashfile repertory.exe SHA256 | sed -e "1d" -e "$d" -e "s/\ //g") > repertory.exe.sha256) || (call :ERROR "Create repertory sha-256 failed")
      (%OPENSSL_BIN% dgst -sha256 -sign "%PRIVATE_KEY%" -out repertory.exe.sig repertory.exe) || (call :ERROR "Create repertory signature failed")
      (%OPENSSL_BIN% dgst -sha256 -verify "%PUBLIC_KEY%" -signature repertory.exe.sig repertory.exe) || (call :ERROR "Verify repertory signature failed")
    )

    if "%ENABLE_PACKAGING%"=="1" (
      echo Creating Archive [%OUT_FILE%]
      (7za u "%OUT_FILE%" repertory.exe repertory.exe.sha256 repertory.exe.sig %WINFSP_BASE_NAME%.dll cacert.pem) || (call :ERROR "Create repertory zip failed")

      echo Signing Archive [%OUT_FILE%]
      (certutil -hashfile "%OUT_FILE%" SHA256 | sed -e "1d" -e "$d" -e "s/\ //g" > "%OUT_FILE%.sha256") || (call :ERROR "Create zip sha-256 failed")
      (%OPENSSL_BIN% dgst -sha256 -sign "%PRIVATE_KEY%" -out "%OUT_FILE%.sig" "%OUT_FILE%") || (call :ERROR "Create zip signature failed")
      (%OPENSSL_BIN% dgst -sha256 -verify "%PUBLIC_KEY%" -signature "%OUT_FILE%.sig" "%OUT_FILE%") || (call :ERROR "Verify zip signature failed")
      (b64.exe -e "%OUT_FILE%.sig" "%OUT_FILE%.sig.b64") || (call :ERROR "Create zip base64 signature failed")
      for /f "delims=" %%i in ('type %OUT_FILE%.sig.b64') do set APP_SIG=!APP_SIG!%%i
      for /f "delims=" %%i in ('type %OUT_FILE%.sha256') do set APP_SHA256=!APP_SHA256!%%i
    ) 
  popd
popd

goto :END

:ERROR
  echo %1
  set ERROR_EXIT=1
  if "%DISABLE_PAUSE%" NEQ "1" (
    pause
  )
goto :END

:NO_QUOTES
  set %~1=!%~1:"=!
goto :EOF

:LOWERCASE
  set %~1=!%~1:A=a!
  set %~1=!%~1:B=b!
  set %~1=!%~1:C=c!
  set %~1=!%~1:D=d!
  set %~1=!%~1:E=e!
  set %~1=!%~1:F=f!
  set %~1=!%~1:G=g!
  set %~1=!%~1:H=h!
  set %~1=!%~1:I=i!
  set %~1=!%~1:J=j!
  set %~1=!%~1:K=k!
  set %~1=!%~1:L=l!
  set %~1=!%~1:M=m!
  set %~1=!%~1:N=n!
  set %~1=!%~1:O=o!
  set %~1=!%~1:P=p!
  set %~1=!%~1:Q=q!
  set %~1=!%~1:R=r!
  set %~1=!%~1:S=s!
  set %~1=!%~1:T=t!
  set %~1=!%~1:U=u!
  set %~1=!%~1:V=v!
  set %~1=!%~1:W=w!
  set %~1=!%~1:X=x!
  set %~1=!%~1:Y=y!
  set %~1=!%~1:Z=z!
goto :EOF

:END
popd
if "!ERROR_EXIT!" NEQ "0" (
  exit !ERROR_EXIT!
)
