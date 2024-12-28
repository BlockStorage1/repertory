@echo off

setlocal

set ARG1=%~1
set ARG2=%~2
set ARG3=%~3

pushd "%~dp0%"
  call mingw64 -no-start ./make_package.sh "%ARG1%" "%ARG2%" "%ARG3%" 1 0 || exit 1
popd
