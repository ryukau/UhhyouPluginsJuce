#!/bin/bash
#
# Build script for GitHub Actions.
#

set -e

# Without update, some package will be 404.
sudo apt-get update
sudo apt-get install libasound2-dev libjack-jackd2-dev \
  ladspa-sdk \
  libfreetype6-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
  libglu1-mesa-dev mesa-common-dev

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

ARTIFACT_DIR="./plugins_ubuntu"
mkdir -p $ARTIFACT_DIR

rm -rf ./build/*_artefacts/Release/*.a
mv ./build/*_artefacts/Release/* $ARTIFACT_DIR

find $ARTIFACT_DIR # debug
