#!/bin/bash -e

if [[ "$WORKSPACE_MSYS2" ]]; then
  cd "$WORKSPACE_MSYS2"
fi

git tag -d latest || true
git tag latest
git push --tags -f
gh release delete -y latest
gh release create --prerelease latest dist/*.nupkg
