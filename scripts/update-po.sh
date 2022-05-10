#!/bin/bash -e

this_dir=$(dirname ${BASH_SOURCE[0]})
cd "$this_dir/../BUILD"

meson compile nvim-ui-update-po
meson compile nvim-ui-pot
