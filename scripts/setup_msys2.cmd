@echo off

setlocal

set ARG1=%~1
set ARG2=%~2
set ARG3=%~3

pushd "%~dp0%"
  call mingw64 -no-start ./setup_msys2.sh "%ARG1%" "%ARG2%" "%ARG3%" 1 0
  call mingw64 -no-start ./setup_msys2.sh "%ARG1%" "%ARG2%" "%ARG3%" 1 0 
popd
