#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

mkdir build
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja -v
