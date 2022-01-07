#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

this_dir=$(dirname ${BASH_SOURCE[0]})
$this_dir/prepare-gtkpp.sh

meson setup BUILD --buildtype=release --prefix=/usr
meson compile -C BUILD -j0
