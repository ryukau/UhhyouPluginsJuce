#!/bin/bash
#
# Build script for GitHub Actions.
#

set -e
shopt -s globstar

# Dependencies are based on `JUCE/docs/Linux Dependencies.md`.
sudo apt update
sudo apt install libasound2-dev libjack-jackd2-dev \
  ladspa-sdk \
  libcurl4-openssl-dev  \
  libfreetype-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
  libwebkit2gtk-4.1-dev \
  libglu1-mesa-dev mesa-common-dev

cmake --version
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

echo "UhhyouDebug: Printing ./build"
tree ./build

ARTIFACT_DIR="./plugins_ubuntu"
mkdir -p $ARTIFACT_DIR

function copyArtifact () {
  for artefact in "$1"/**/*_artefacts/ ; do
    rm -rf "$artefact"/Release/*.a
    cp -r "$artefact"/Release/* $ARTIFACT_DIR
  done
}
copyArtifact ./build/experimental

echo "UhhyouDebug: Printing $ARTIFACT_DIR"
tree $ARTIFACT_DIR # debug
