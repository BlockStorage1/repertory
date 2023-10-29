@echo off

set MSVC_BUILD_TYPE=%1
set BUILD_CLEAN=%2

if "%MSVC_BUILD_TYPE%" == "" (
  echo "Build type not set"
  exit 1
)

if "%MSVC_BUILD_TYPE%" == "Debug" (
  set BUILD_FOLDER=debug
) else (
  if "%MSVC_BUILD_TYPE%" == "Release" (
    set BUILD_FOLDER=release
  ) else (
    set BUILD_FOLDER=%MSVC_BUILD_TYPE%
  )
  set MSVC_BUILD_TYPE=Release
)

if EXIST "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
  call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" 
) else (
  if EXIST "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" 
  )
)

pushd "%~dp0%"
  md ..\build2\%BUILD_FOLDER%
  del /q ..\build2\%BUILD_FOLDER%\librepertory.lib
  del /q ..\build2\%BUILD_FOLDER%\repertory.exe
  del /q ..\build2\%BUILD_FOLDER%\unittests.exe

  pushd "..\build2\%BUILD_FOLDER%"
    cmake ..\.. -G "NMake Makefiles" -DREPERTORY_ENABLE_S3_TESTING=ON  -DREPERTORY_ENABLE_S3=ON -DCMAKE_BUILD_TYPE=%MSVC_BUILD_TYPE% || exit 1
    copy /y compile_commands.json ..
    if "%BUILD_CLEAN%" == "clean" (
      nmake clean || exit 1
    )
    nmake || exit 1
  popd
popd
