#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

meson setup BUILD --buildtype=release --prefix=/usr
ln -sf BUILD/compile_commands.json
meson compile -C BUILD -j0
