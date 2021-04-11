#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

mkdir -p dist/{bin,lib,share}
cp build/nvim-sdl2.exe dist/bin

dlls=$(ntldd.exe -R build/nvim-sdl2.exe | grep -Po "[^ ]+?msys64[^ ]+" | sort -u | grep -Po '[^\\]+$')
for dll in $dlls; do
  cp /mingw64/bin/$dll dist/bin
done

cp -r chocolatey/* dist/
# TODO: adjust the version
