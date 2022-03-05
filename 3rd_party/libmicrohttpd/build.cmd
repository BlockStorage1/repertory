pushd "%1"
  cd w32\VS2019
  if "%3" == "ON" (set BUILD_TYPE=Win32) else (set BUILD_TYPE=x64)
  msbuild libmicrohttpd.sln /p:Configuration=%2-static /p:Platform="%BUILD_TYPE%" /t:libmicrohttpd || exit 1
exit 0