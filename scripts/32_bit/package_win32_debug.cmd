@echo off

pushd "%~dp0%.."
call build_win.cmd Debug 1 1 0 0 0 0
popd
