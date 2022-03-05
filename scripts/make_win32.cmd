@echo off

set BUILD_TYPE=%1

pushd "%~dp0"
  make_%BUILD_TYPE%.cmd || exit 1 
popd
