@echo off

call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" 

md ..\build2\debug
pushd "..\build2\debug"
  (cmake ..\.. -G "CodeBlocks - NMake Makefiles" -DREPERTORY_ENABLE_SKYNET_PREMIUM_TESTS=ON -DREPERTORY_ENABLE_S3_TESTING=ON -DREPERTORY_ENABLE_S3=ON -DCMAKE_BUILD_TYPE=Debug -DREPERTORY_WRITE_SUPPORT=ON && nmake) || exit 1
  copy /y compile_commands.json ..
popd
