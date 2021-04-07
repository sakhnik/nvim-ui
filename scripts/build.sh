#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
