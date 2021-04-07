#!/bin/bash -e

pacman -S --noconfirm \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-msgpack-c \
  mingw-w64-x86_64-libuv \
  mingw-w64-x86_64-SDL2 \
  mingw-w64-x86_64-SDL2_ttf \
  mingw-w64-x86_64-pkg-config \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ntldd-git \
  zip
