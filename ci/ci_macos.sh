#!/bin/bash
#
# Build script for GitHub Actions.
#

set -e

cmake --version

cmake \
  -S . \
  -B build \
  -GXcode \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build -j --config Release

ARTIFACT_DIR="./plugins_macOS"
mkdir -p $ARTIFACT_DIR

rm -rf ./build/*_artefacts/Release/*.a
mv ./build/*_artefacts/Release/* $ARTIFACT_DIR

find $ARTIFACT_DIR # debug
