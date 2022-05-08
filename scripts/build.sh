#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

prefix=/usr
case "$(uname)" in
  MINGW*) prefix=/ ;;
esac

this_dir=$(dirname ${BASH_SOURCE[0]})
$this_dir/prepare-gtkpp.sh

meson setup BUILD --buildtype=release --prefix=$prefix
meson compile -C BUILD -j0
