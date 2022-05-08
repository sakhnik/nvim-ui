#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

cd BUILD

mkdir -p dist
DESTDIR=dist meson install --tags=runtime,i18n
mv dist/tools/msys64/* dist/
mkdir -p dist/{bin,lib,share/glib-2.0}
touch dist/bin/nvim-ui.exe.gui

dlls=$(ntldd.exe -R ./src/nvim-ui.exe | grep -Po "[^ ]+?msys64[^ ]+" | sort -u | grep -Po '[^\\]+$')
for dll in $dlls; do
  cp /mingw64/bin/$dll dist/bin
done

cp -R /mingw64/lib/gdk-pixbuf-2.0 dist/lib/
cp -R /mingw64/share/glib-2.0/schemas dist/share/glib-2.0/

cp -r ../chocolatey/* dist/
if [[ "$NVIM_UI_RELEASE" == "$NVIM_UI_VERSION" ]]; then
  sed -i "s|PACKAGE_SOURCE_URL|https://github.com/sakhnik/nvim-ui/tree/v${NVIM_UI_VERSION}/chocolatey|" dist/*.nuspec
else
  sed -i "s|PACKAGE_SOURCE_URL|https://github.com/sakhnik/nvim-ui/tree/${GITHUB_SHA}/chocolatey|" dist/*.nuspec
fi

sed -i "s|VERSION|${NVIM_UI_RELEASE}|" dist/*.nuspec
