#!/bin/zsh
#
# Build script for GitHub Actions (macOS).
#

set -e
setopt NULL_GLOB

PLUGIN=${1:-"ALL"}

cmake --version
cmake -S . -B build -GXcode -DCMAKE_BUILD_TYPE=Release

if [[ "$PLUGIN" == "ALL" || -z "$PLUGIN" ]]; then
  cmake --build build -j --config Release
else
  cmake --build build -j --config Release --target "${PLUGIN}_All"
fi

echo "UhhyouDebug: Printing ./build structure"
find ./build

ARTIFACT_DIR="./plugins_macOS"
mkdir -p "$ARTIFACT_DIR"

function copyArtifact () {
  for artefact in $1/**/*_artefacts(/); do
    release_dir="$artefact/Release"
    if [[ -d "$release_dir" ]]; then
      rm -f "$release_dir"/*.a
      files=("$release_dir"/*)
      if (( ${#files} > 0 )); then
        cp -r $files "$ARTIFACT_DIR"
        echo "UhhyouDebug: Copied artifacts from: $release_dir"
      fi
    fi
  done
}
copyArtifact ./build

echo "UhhyouDebug: Printing $ARTIFACT_DIR"
find "$ARTIFACT_DIR"
