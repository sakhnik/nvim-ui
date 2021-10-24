#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

meson setup BUILD --buildtype=release --prefix=/usr
meson compile -C BUILD -j0
