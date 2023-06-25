#
# Build script for GitHub Actions.
#

$ErrorActionPreference = "Stop"

mkdir build
$SRC_ROOT = (Get-Item .).FullName

cmake --version

cmake `
  -S . `
  -B build `
  -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_BUILD_TYPE=Release `

cmake --build build -j --config Release

# https://gitlab.com/gitlab-org/gitlab-runner/issues/3194#note_196458158
if (!$?) { Exit $LASTEXITCODE }

$ARTIFACT_DIR = "$SRC_ROOT\plugins_windows"
New-Item -Path "$ARTIFACT_DIR" -ItemType "directory"

Get-ChildItem -Path "$SRC_ROOT\build" -Filter "*_artefacts" |
ForEach-Object {
  $PLUGIN_DIR = "$($_.FullName)\Release"
  Get-ChildItem -Path $PLUGIN_DIR -Include @("*.lib", "*.exp") -Recurse | Remove-Item -Force
  Move-Item -Path "$PLUGIN_DIR\*" -Destination $ARTIFACT_DIR -Force
}

tree /A /F $ARTIFACT_DIR # debug
