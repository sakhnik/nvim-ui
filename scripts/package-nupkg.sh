#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

cd BUILD

# Ninja maps prefix directly into Windows paths. For instance, / -> c:/tools/msys64
# We need this path to properly mix in the system dlls.
msys2_root=`python - <<END
import sys
from pathlib import Path
import re

msys2 = str(Path(sys.executable).parent.parent.parent)
print(re.sub(r'[^:]+:', '', msys2))
END`

dist=dist${msys2_root}

rm -rf dist

# Create directories for the distro
mkdir -p $dist/{bin,lib,share/glib-2.0}

# Make chocolatey create a GUI launcher
touch $dist/bin/nvim-ui.exe.gui

# Collect the necessary libraries
dlls=$(ntldd.exe -R ./src/nvim-ui.exe | grep -Po "[^ ]+?msys64[^ ]+" | sort -u | grep -Po '[^\\]+$')
for dll in $dlls; do
  cp /mingw64/bin/$dll $dist/bin
done

# Collect the glib2 stuff
cp -R /mingw64/lib/gdk-pixbuf-2.0 $dist/lib/
cp -R /mingw64/share/glib-2.0/schemas $dist/share/glib-2.0/

# Install the built application and whatever the build system creates
# Do this after the dependencies installed to properly update gschemas
DESTDIR=dist meson install
# Compile the schemas manually until meson install script is fixed
glib-compile-schemas $dist/share/glib-2.0/schemas

# Move everything up to the dist root
mv $dist/* dist
rmdir $dist

# Prepare nuspec
cp -r ../chocolatey/* dist/
if [[ "$NVIM_UI_RELEASE" == "$NVIM_UI_VERSION" ]]; then
  sed -i "s|PACKAGE_SOURCE_URL|https://github.com/sakhnik/nvim-ui/tree/v${NVIM_UI_VERSION}/chocolatey|" dist/*.nuspec
else
  sed -i "s|PACKAGE_SOURCE_URL|https://github.com/sakhnik/nvim-ui/tree/${GITHUB_SHA}/chocolatey|" dist/*.nuspec
fi
sed -i "s|VERSION|${NVIM_UI_RELEASE}|" dist/*.nuspec
