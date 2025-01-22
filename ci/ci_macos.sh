#!/bin/bash
#
# Build script for GitHub Actions.
#

set -e

cmake --version
cmake -S . -B build -GXcode -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --config Release

find ./build

ARTIFACT_DIR="./plugins_macOS"
mkdir -p $ARTIFACT_DIR

function copyArtifact () {
  for artefact in "$1"/**/*_artefacts/ ; do
    rm -rf "$artefact"/Release/*.a
    cp -r "$artefact"/Release/* $ARTIFACT_DIR
  done
}
copyArtifact ./build/experimental

find $ARTIFACT_DIR # debug
