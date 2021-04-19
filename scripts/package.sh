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
version=$(grep -Po '(?<=set\(VERSION )[^) ]+' CMakeLists.txt)
if git describe --tags 2>/dev/null | grep -F "$version" >/dev/null 2>&1; then
  sed -i "s|PACKAGE_SOURCE_URL|https://github.com/sakhnik/nvim-ui/tree/v${version}/chocolatey|" dist/*.nuspec
  echo "Release version is ${version}"
else
  sed -i "s|PACKAGE_SOURCE_URL|https://github.com/sakhnik/nvim-ui/tree/${GITHUB_SHA}/chocolatey|" dist/*.nuspec
  version="${version}-dev${GITHUB_RUN_NUMBER}-${GITHUB_SHA:0:7}"
  echo "Development version is ${version}"
fi

sed -i "s|VERSION|${version}|" dist/*.nuspec
