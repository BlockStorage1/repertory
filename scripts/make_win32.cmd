@echo off

setlocal

set ARG1=%~1
set ARG2=%~2
set ARG3=%~3

pushd "%~dp0%"
  call setup_msys2.cmd "%ARG1%" "%ARG2%" "%ARG3%"
  if exist "cleanup.cmd" (
    call cleanup.cmd "%ARG1%" "%ARG2%" "%ARG3%" 1 0
    del cleanup.*
  )

  call mingw64 -no-start ./make_flutter.sh "%ARG1%" "%ARG2%" "%ARG3%" 1 0 || exit 1
  call mingw64 -no-start ./make_common.sh "%ARG1%" "%ARG2%" "%ARG3%" 1 0 || exit 1
popd
