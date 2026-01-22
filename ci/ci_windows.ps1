#
# Build script for GitHub Actions.
#

$ErrorActionPreference = "Stop"

New-Item -Path build -ItemType "directory" -Force

cmake --version
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --config Release

# https://gitlab.com/gitlab-org/gitlab-runner/issues/3194#note_196458158
if (!$?) { Exit $LASTEXITCODE }

Write-Output "UhhyouDebug: Printing ./build"
tree /A /F ./build # debug

$ARTIFACT_DIR = ".\plugins_windows"
New-Item -Path "$ARTIFACT_DIR" -ItemType "directory" -Force

function CopyArtifacts {
  param ([string]$SearchRootPath)
  Get-ChildItem -Path $SearchRootPath -Recurse -Directory -Filter "*_artefacts" |
  ForEach-Object {
    $ArtifactFolder = $_.FullName
    $ReleaseDir = "$ArtifactFolder\Release"
    if (Test-Path $ReleaseDir) {
      Remove-Item "$ReleaseDir\*.lib" -Force -ErrorAction SilentlyContinue
      $FilesToCopy = Get-ChildItem -Path "$ReleaseDir\*"
      if ($FilesToCopy) {
        Copy-Item -Recurse -Path $FilesToCopy.FullName -Destination $ARTIFACT_DIR -Force
        Write-Output "UhhyouDebug: Copied artifacts from: $ReleaseDir"
      }
    }
  }
}
CopyArtifacts ".\build"

Write-Output "UhhyouDebug: Printing $ARTIFACT_DIR"
tree /A /F $ARTIFACT_DIR # debug
