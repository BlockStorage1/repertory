@echo off

pushd "%~dp0%.."
call build_win.cmd Release 1 1 0 0 0 1 %1 0 1
popd
