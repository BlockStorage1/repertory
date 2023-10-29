pacman -Sqyuu --noconfirm &&
  pacman -S --noconfirm msys2-keyring &&
  pacman -S --noconfirm --needed --disable-download-timeout \
    mingw64/mingw-w64-x86_64-cmake \
    mingw64/mingw-w64-x86_64-gcc \
    mingw64/mingw-w64-x86_64-gdb \
    mingw64/mingw-w64-x86_64-make \
    mingw64/mingw-w64-x86_64-toolchain \
    msys/git \
    msys/mercurial \
    make
