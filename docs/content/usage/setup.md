---
title: "Setup"
date: 3000-01-01
draft: false
toc: true
weight: 1
---

TODO: Add a link to GitHub discussions when the plugin is not working.

## DAW
Tested in (Windows 11):

- FL Studio
- REAPER

## Build Environment
Built in these environments. Plugins might not run on older systems:

| Platform | OS Version               | Architecture          |
|:---------|:-------------------------|:----------------------|
| macOS    | macOS 15 (Sequoia)       | Apple Silicon (ARM64) |
| Windows  | Windows 11 / Server 2025 | x64                   |
| Linux    | Ubuntu 24.04 LTS         | x64                   |

## Installation

### Plugin Location
Extract the `.zip` file and move the plugins to:

- Windows (VST3): `C:\Program Files\Common Files\VST3\`
- macOS (VST3): `~/Library/Audio/Plug-ins/VST3/`
- macOS (AU): `~/Library/Audio/Plug-ins/Components/` (Must have `.component` extension)
- Linux (VST3): `~/.vst3/`

Note: `~/` is your home folder.

### Presets
Move the downloaded presets into the `UhhyouPlugins` folder at these locations:

- Windows: `%APPDATA%\UhhyouPlugins\`
- macOS: `~/Library/Application Support/UhhyouPlugins/`
- Linux: `~/.config/UhhyouPlugins/`

The plugin will automatically create this folder the first time you open it, or you can create it manually.

## Troubleshooting

### Windows
If your DAW cannot find the plugin, install Microsoft Visual C++ Redistributable.

- [Download `vc_redist.x64.exe` from Microsoft](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170)

### Linux (Ubuntu 24.04)
Required libraries: `sudo apt install libxcb-cursor0 libxkbcommon-x11-0`

Note: If REAPER fails to scan the plugin, delete `~/.config/REAPER/reaper-vstplugins64.ini` and restart.

### macOS
Gatekeeper might flag the plugin as damaged. To fix this:

1. Open Terminal.
2. Run this command (replace `FileName.vst3` with your plugin):
   `xattr -cr ~/Library/Audio/Plug-ins/VST3/FileName.vst3`
3. Rescan plugins in your DAW.

If the plugin still fails, force an ad-hoc signature:
`sudo codesign --force --deep -s - ~/Library/Audio/Plug-ins/VST3/FileName.vst3`

Note: `xattr` and `codesign` are built-in. macOS might prompt you to download Command Line Developer Tools.
