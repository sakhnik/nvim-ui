#!/bin/bash -e

########################################################################
# Package the binaries built as an AppImage
# By Simon Peter 2016
# For more information, see http://appimage.org/
########################################################################

# App arch, used by generate_appimage.
if [ -z "$ARCH" ]; then
  export ARCH="$(arch)"
fi

TAG=$1

# App name, used by generate_appimage.
APP=nvim-ui

ROOT_DIR="$(git rev-parse --show-toplevel)"
APP_BUILD_DIR="$ROOT_DIR/build"
APP_DIR="$APP.AppDir"

########################################################################
# Compile nvim-ui and install it into AppDir
########################################################################

meson setup BUILD --buildtype=release --prefix=/usr
meson compile -C BUILD -j0
ninja -C $APP_BUILD_DIR test
DESTDIR=${APP_DIR} ninja -C $APP_BUILD_DIR install

########################################################################
# Get helper functions and move to AppDir
########################################################################

# App version, used by generate_appimage.
VERSION="$NVIM_UI_RELEASE"

cd "$APP_BUILD_DIR"

# Only downloads linuxdeploy if the remote file is different from local
if [ -e "$APP_BUILD_DIR"/linuxdeploy-x86_64.AppImage ]; then
  curl -Lo "$APP_BUILD_DIR"/linuxdeploy-x86_64.AppImage \
    -z "$APP_BUILD_DIR"/linuxdeploy-x86_64.AppImage \
    https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage  
else
  curl -Lo "$APP_BUILD_DIR"/linuxdeploy-x86_64.AppImage \
    https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
fi

chmod +x "$APP_BUILD_DIR"/linuxdeploy-x86_64.AppImage

# metainfo is not packaged automatically by linuxdeploy
mkdir -p "$APP_DIR/usr/share/metainfo/"
# TODO
#cp "$ROOT_DIR/res/nvim-ui.appdata.xml" "$APP_DIR/usr/share/metainfo/"

cd "$APP_DIR"

########################################################################
# AppDir complete. Now package it as an AppImage.
########################################################################

# Appimage set the ARGV0 environment variable. This causes problems in zsh.
# To prevent this, we use wrapper script to unset ARGV0 as AppRun.
# See https://github.com/AppImage/AppImageKit/issues/852
#
cat << 'EOF' > AppRun
#!/bin/bash

unset ARGV0
exec "$(dirname "$(readlink  -f "${0}")")/usr/bin/nvim-ui" ${@+"$@"}
EOF
chmod 755 AppRun

cd "$APP_BUILD_DIR" # Get out of AppImage directory.

# Set the name of the file generated by appimage
export OUTPUT=nvim-ui.appimage

# If it's a release generate the zsync file
if [ -n "$TAG" ]; then
  export UPDATE_INFORMATION="gh-releases-zsync|neovim-ui|neovim-ui|$TAG|nvim-ui.appimage.zsync"
fi

# Generate AppImage.
#   - Expects: $ARCH, $APP, $VERSION env vars
#   - Expects: ./$APP.AppDir/ directory
#   - Produces: ./nvim-ui.appimage
./linuxdeploy-x86_64.AppImage --appdir $APP.AppDir -d $ROOT_DIR/res/nvim-ui.desktop -i \
"$ROOT_DIR/res/nvim-ui.svg" --output appimage

echo 'genappimage.sh: finished'
