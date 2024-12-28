@echo off

setlocal

set DEST=%~1
set DIST=%~2
set ARG1=%~3
set ARG2=%~4
set ARG3=%~5

pushd "%~dp0%"
  call mingw64 -no-start ./deliver.sh "%DEST%" "%DIST%" "%ARG1%" "%ARG2%" "%ARG3%" 1 0 || exit 1
popd
