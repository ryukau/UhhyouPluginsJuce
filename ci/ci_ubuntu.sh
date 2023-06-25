#!/bin/bash
#
# Build script for GitHub Actions.
#

set -e

# Without update, some package will be 404.
sudo apt-get update
sudo apt-get install cmake build-essential

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

ARTIFACT_DIR="./plugins_ubuntu"
mkdir -p $ARTIFACT_DIR

rm -rf ./build/*_artefacts/Release/*.a
mv ./build/*_artefacts/Release/* $ARTIFACT_DIR

find $ARTIFACT_DIR # debug
