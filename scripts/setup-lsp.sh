#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

rm -rf BUILD-clang
# Need to specify full path because ninja/clang get confused in Arch Linux.
# clang++ is found at /sbin/clang++ getting system headers considered dirty all the time.
CXX=/usr/bin/clang++ meson setup BUILD-clang --buildtype=debug --prefix=/usr
ln -sf BUILD-clang/compile_commands.json
