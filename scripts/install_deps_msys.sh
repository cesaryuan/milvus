#!/bin/sh

if [[ "${MSYSTEM}" != "MINGW64" ]] ; then
    echo non MINGW64, exit.
    exit 1
fi

pacman -S --noconfirm --needed \
    git make tar \
    mingw-w64-x86_64-toolchain \
    mingw-w64-x86_64-make \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-go \
    mingw-w64-x86_64-git-lfs \
    mingw-w64-x86_64-arrow \
    mingw-w64-x86_64-intel-tbb \
    mingw-w64-x86_64-rocksdb \
    mingw-w64-x86_64-openblas

rm -fr mman-win32 && \
    git clone https://github.com/alitrack/mman-win32.git mman-win32 && \
    cd mman-win32 && \
    cmake -G "MSYS Makefiles" \
          -DCMAKE_MAKE_PROGRAM=mingw32-make \
          -DCMAKE_INSTALL_PREFIX=/mingw64 \
          -DBUILD_SHARED_LIBS=OFF . && \
    mingw32-make install

cd - && rm -fr mman-win32

touch a.c && \
    gcc -c a.c && \
    ar rc libdl.a a.o && \
    ranlib libdl.a && \
    cp -fr libdl.a /mingw64/lib && \
    rm -fr a.c a.o libdl.a

