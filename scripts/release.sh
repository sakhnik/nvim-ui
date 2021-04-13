#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

gh release create "${GITHUB_REF#refs/tags/}" dist/*.nupkg
