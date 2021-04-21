#!/bin/bash -e

version=$(./scripts/version.sh)
if git describe --tags 2>/dev/null | grep -F "$version" >/dev/null 2>&1; then
  echo "${version}"
else
  echo "${version}-dev${GITHUB_RUN_NUMBER}-${GITHUB_SHA:0:7}"
fi
