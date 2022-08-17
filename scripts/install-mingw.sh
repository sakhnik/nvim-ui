#!/bin/bash -e

pacman -S --noconfirm \
  mingw-w64-x86_64-{gcc,msgpack-c,libuv,gtk4,pkg-config,gobject-introspection,python-virtualenv,fmt} \
  mingw-w64-x86_64-{cmake,ninja,meson,ntldd-git,python-markdown} \
  zip
