#!/bin/bash -e

mkdir BUILD
cd BUILD
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=Yes ..
make -j
cd ..
ln -sf BUILD/compile_commands.json
