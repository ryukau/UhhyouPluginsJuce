---
title: "Installation & Setup"
date: 2024-03-16
draft: false
---

TODO: Add a link to GitHub discussions when the plugin is not working.

## DAW
The plugins are mostly tested in the following DAWs on Windows 11.

- FL Studio
- REAPER

## Build Environment
Following environments are used to build. The plugins may not run on the older systems.

| Platform | OS Version               | Architecture          |
|:---------|:-------------------------|:----------------------|
| macOS    | macOS 15 (Sequoia)       | Apple Silicon (ARM64) |
| Windows  | Windows 11 / Server 2025 | x64                   |
| Linux    | Ubuntu 24.04 LTS         | x64                   |

## Installation

## Plugin Location
Extract the downloaded `.zip` file and move the plugin files to your operating system's standard directory.

- Windows (VST3): `C:\Program Files\Common Files\VST3\`
- macOS (VST3): `~/Library/Audio/Plug-ins/VST3/`
- macOS (AU): `~/Library/Audio/Plug-ins/Components/` (Must have the `.component` extension)
- Linux (VST3): `~/.vst3/`

Note: The `~/` symbol represents your user home folder. You can paste these exact paths into your OS file browser's "Go to Folder" or address bar.

## Presets
TODO: Update paths.

If your downloaded package includes a `presets` folder, you must install them manually. Move the `Uhhyou` folder into the following directory:

- Windows: `%USERPROFILE%\Documents\VST3 Presets\`
- macOS: `~/Library/Audio/Presets/`
- Linux: `~/.vst3/presets/`

## Troubleshooting & OS-Specific Notes

### Windows
If the plugin is not recognized by DAW, you may need to install the latest Microsoft C++ Redistributable.

- [Download `vc_redist.x64.exe` from Microsoft](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170)

### Linux (Ubuntu 24.04)
The plugin requires following libraries: `sudo apt install libxcb-cursor0 libxkbcommon-x11-0`

Note on REAPER: If the plugin fails to scan, try deleting `~/.config/REAPER/reaper-vstplugins64.ini` and restarting the DAW.

### macOS
Because the plugin is released without an Apple Developer signature, Gatekeeper will likely flag it as damaged.

To fix the "damaged" or "cannot be opened" error:

1. Open the Terminal app.
2. Run the following command (be sure to replace `FileName.vst3` with the actual name of the plugin file):
   `xattr -cr ~/Library/Audio/Plug-ins/VST3/FileName.vst3`
3. Rescan your plugins in your DAW.

If the plugin still fails to load, you may need to force an ad-hoc signature. Run this command:
`sudo codesign --force --deep -s - ~/Library/Audio/Plug-ins/VST3/FileName.vst3`

Note: `xattr` and `codesign` are built into macOS. If this is your first time using `codesign`, your Mac may prompt you to download the "Command Line Developer Tools." Simply click "Install" to proceed.
