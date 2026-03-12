#!/bin/bash
#
# Build script for GitHub Actions.
#

set -e
shopt -s globstar
shopt -s nullglob

PLUGIN=${1:-"ALL"}

# Dependencies are based on `JUCE/docs/Linux Dependencies.md`.
sudo apt update
sudo apt install libasound2-dev libjack-jackd2-dev \
  ladspa-sdk \
  libcurl4-openssl-dev  \
  libfreetype-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
  libwebkit2gtk-4.1-dev \
  libglu1-mesa-dev mesa-common-dev \
  ninja-build

cmake --version
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

if [ "$PLUGIN" = "ALL" ] ||[ -z "$PLUGIN" ]; then
  cmake --build build
else
  cmake --build build --target "${PLUGIN}_All"
fi

echo "UhhyouDebug: Printing ./build"
tree ./build

ARTIFACT_DIR="./plugins_ubuntu"
mkdir -p $ARTIFACT_DIR

function copyArtifact () {
  for artefact in "$1"/**/*_artefacts/ ; do
    release_dir="$artefact/Release"
    if [ -d "$release_dir" ]; then
      find "$release_dir" -maxdepth 1 -name "*.a" -delete
      files_to_copy=("$release_dir"/*)
      if [ ${#files_to_copy[@]} -gt 0 ]; then
        cp -r "${files_to_copy[@]}" "$ARTIFACT_DIR"
        echo "UhhyouDebug: Copied artifacts from: $release_dir"
      fi
    fi
  done
}
copyArtifact ./build

echo "UhhyouDebug: Printing $ARTIFACT_DIR"
tree $ARTIFACT_DIR # debug
