#!/bin/bash -e

meson setup BUILD --buildtype=debugoptimized
ln -sf BUILD/compile_commands.json
meson compile -C BUILD -j0
