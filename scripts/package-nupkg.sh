#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

mkdir -p dist/{bin,lib,share}
cp build/nvim-ui.exe dist/bin
touch dist/bin/nvim-ui.exe.gui

dlls=$(ntldd.exe -R build/nvim-ui.exe | grep -Po "[^ ]+?msys64[^ ]+" | sort -u | grep -Po '[^\\]+$')
for dll in $dlls; do
  cp /mingw64/bin/$dll dist/bin
done

cp -r chocolatey/* dist/
if [[ "$NVIM_UI_RELEASE" == "$NVIM_UI_VERSION" ]]; then
  sed -i "s|PACKAGE_SOURCE_URL|https://github.com/sakhnik/nvim-ui/tree/v${NVIM_UI_VERSION}/chocolatey|" dist/*.nuspec
else
  sed -i "s|PACKAGE_SOURCE_URL|https://github.com/sakhnik/nvim-ui/tree/${GITHUB_SHA}/chocolatey|" dist/*.nuspec
fi

sed -i "s|VERSION|${NVIM_UI_RELEASE}|" dist/*.nuspec
