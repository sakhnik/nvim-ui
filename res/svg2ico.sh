#!/bin/bash -e

convert nvim-ui.svg -bordercolor white -border 0 \
      \( -clone 0 -resize 16x16 \) \
      \( -clone 0 -resize 32x32 \) \
      \( -clone 0 -resize 48x48 \) \
      \( -clone 0 -resize 64x64 \) \
      -alpha off -colors 256 nvim-ui.ico
