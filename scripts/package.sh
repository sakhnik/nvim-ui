#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

package=nvim-sdl2

mkdir -p dist/$package/{bin,lib,share}
cp build/$package.exe dist/$package/bin

dlls=$(ntldd.exe -R build/$package.exe | grep -Po "[^ ]+?msys64[^ ]+" | sort -u | grep -Po '[^\\]+$')
for dll in $dlls; do
  cp /mingw64/bin/$dll dist/$package/bin
done
cp /mingw64/bin/gspawn* dist/$package/bin

cp -R /mingw64/lib/gdk-pixbuf-2.0 dist/$package/lib/
cp -R /mingw64/share/{glib-2.0,icons} dist/$package/share/

pushd dist
zip -r $package-win64.zip *
popd
