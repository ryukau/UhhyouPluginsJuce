#
# Build script for GitHub Actions.
#

$ErrorActionPreference = "Stop"

New-Item -Path build -ItemType "directory" -Force

cmake --version # debug

cmake `
  -S . `
  -B build `
  -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_BUILD_TYPE=Release `

cmake --build build -j --config Release

# https://gitlab.com/gitlab-org/gitlab-runner/issues/3194#note_196458158
if (!$?) { Exit $LASTEXITCODE }

$ARTIFACT_DIR = ".\plugins_windows"
New-Item -Path "$ARTIFACT_DIR" -ItemType "directory" -Force

Get-ChildItem -Path ".\build" -Filter "*_artefacts" |
ForEach-Object {
  $PLUGIN_DIR = "$($_.FullName)\Release\VST3"
  tree /A /F $PLUGIN_DIR # debug
  Copy-Item -Recurse -Path "$PLUGIN_DIR\*.vst3" -Destination $ARTIFACT_DIR -Force
}
tree /A /F $ARTIFACT_DIR # debug
