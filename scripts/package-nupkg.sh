#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

mkdir -p dist/{bin,lib,share/{glib-2.0,icons/hicolor/scalable/apps}}
cp build/nvim-ui.exe dist/bin
touch dist/bin/nvim-ui.exe.gui

dlls=$(ntldd.exe -R build/nvim-ui.exe | grep -Po "[^ ]+?msys64[^ ]+" | sort -u | grep -Po '[^\\]+$')
for dll in $dlls; do
  cp /mingw64/bin/$dll dist/bin
done

cp -R /mingw64/lib/gdk-pixbuf-2.0 dist/lib/
cp -R /mingw64/share/glib-2.0/schemas dist/share/glib-2.0/
cp -R /mingw64/share/icons/hicolor/index.theme dist/share/icons/hicolor/
cp -R res/nvim-ui.svg dist/share/icons/hicolor/scalable/apps/
gtk-update-icon-cache dist/share/icons/hicolor

cp -r chocolatey/* dist/
if [[ "$NVIM_UI_RELEASE" == "$NVIM_UI_VERSION" ]]; then
  sed -i "s|PACKAGE_SOURCE_URL|https://github.com/sakhnik/nvim-ui/tree/v${NVIM_UI_VERSION}/chocolatey|" dist/*.nuspec
else
  sed -i "s|PACKAGE_SOURCE_URL|https://github.com/sakhnik/nvim-ui/tree/${GITHUB_SHA}/chocolatey|" dist/*.nuspec
fi

sed -i "s|VERSION|${NVIM_UI_RELEASE}|" dist/*.nuspec
