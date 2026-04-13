---
title: "Usage"
date: 2026-04-13
draft: false
toc: true
weight: 1
gui_mockups: true
section_number: true
layout: "plugin-mockup"
---

If you have troubles related to the plugins, feel free to contact me on [GitHub Discussions page](https://github.com/ryukau/UhhyouPluginsJuce/discussions) or `ryukau@gmail.com.`

## System

### DAW

Tested in the DAWs below on Windows 11:

- FL Studio
- REAPER

### Build Environment

Plugins might not run on older systems.

| Platform | OS Version               | Architecture          |
|:---------|:-------------------------|:----------------------|
| macOS    | macOS 15 (Sequoia)       | Apple Silicon (ARM64) |
| Windows  | Windows 11 / Server 2025 | x64                   |
| Linux    | Ubuntu 24.04 LTS         | x64                   |

## Installation
To download the plugins, visit the [release page (github.com)](https://github.com/ryukau/UhhyouPluginsJuce/releases).

### Plugin Folder

Extract the `.zip` file and move the plugins to these folders.

| OS             | Path                                                                      |
|----------------|---------------------------------------------------------------------------|
| Windows (VST3) | `C:\Program Files\Common Files\VST3\`                                     |
| macOS (VST3)   | `~/Library/Audio/Plug-ins/VST3/`                                          |
| macOS (AU)     | `~/Library/Audio/Plug-ins/Components/` (Must have `.component` extension) |
| Linux (VST3)   | `~/.vst3/`                                                                |

`~/` is your home folder.

### Preset Folder {#preset-folder}

Move the downloaded presets to the `UhhyouPlugins` folder in these folders.

| OS      | Path                                           |
|---------|------------------------------------------------|
| Windows | `%APPDATA%\UhhyouPlugins\`                     |
| macOS   | `~/Library/Application Support/UhhyouPlugins/` |
| Linux   | `~/.config/UhhyouPlugins/`                     |

The plugin will automatically create this folder the first time you open it, or you can create it manually.

### Troubleshooting

#### Windows

If your DAW cannot find the plugin, install Microsoft Visual C++ Redistributable.

- [Download `vc_redist.x64.exe` from Microsoft](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170)

#### Linux (Ubuntu 24.04)

Required libraries: `sudo apt install libxcb-cursor0 libxkbcommon-x11-0`

Note: If REAPER fails to scan the plugin, delete `~/.config/REAPER/reaper-vstplugins64.ini` and restart.

#### macOS

Gatekeeper might flag the plugin as damaged. To fix this:

1. Open Terminal.
2. Run this command (replace `FileName.vst3` with your plugin):
   `xattr -cr ~/Library/Audio/Plug-ins/VST3/FileName.vst3`
3. Rescan plugins in your DAW.

If the plugin still fails, force an ad-hoc signature:
`sudo codesign --force --deep -s - ~/Library/Audio/Plug-ins/VST3/FileName.vst3`

Note: `xattr` and `codesign` are built-in. macOS might prompt you to download Command Line Developer Tools.

## Appearance

The appearance can be changed from [GUI Settings]({{< ref "#gui-settings" >}}) → [Change Appearance]({{< ref "#settings-appearance" >}}).

### Appearance Folder {#appearance-folder}

| OS               | Path                                                                       |
|------------------|----------------------------------------------------------------------------|
| Windows          | `%APPDATA%\UhhyouPlugins\appearance\`                                      |
| macOS            | `~/Library/Preferences/UhhyouPlugins/appearance/`                          |
| Linux (Primary)  | `$XDG_CONFIG_HOME/UhhyouPlugins/appearance/`                               |
| Linux (Fallback) | `$HOME/.config/UhhyouPlugins/appearance/` (If `$XDG_CONFIG_HOME` is empty) |

Avoid editing `_active.json`. Running plugins may overwrite it.

To add an appearance, place a `.json` file in `UhhyouPlugins/appearance/` folder. The appearance will show up in [GUI Settings]({{< ref "#gui-settings" >}}) → [Change Appearance]({{< ref "#settings-appearance" >}}) only if the format is valid.

### Format

The file uses [JSON](https://www.json.org/json-en.html) format. The default is shown below.

```json
{
  "KeyboardFocus"   : true,
  "windowScale"     : 1.0,
  "fontUiName"      : "Ubuntu",
  "fontUiStyle"     : "Regular",
  "fontMonoName"    : "Ubuntu Mono",
  "fontMonoStyle"   : "Regular",
  "foregroundLight" : "#ffffffff",
  "foregroundDark"  : "#000000ff",
  "background"      : "#ffffffff",
  "surface"         : "#f8f8f8ff",
  "border"          : "#888888ff",
  "main"            : "#fcc04fff",
  "accent"          : "#13c136ff",
  "warning"         : "#fc8080ff"
}
```

Invalid formatting overwrites `_active.json` with the default. The default values fill the missing options.

#### Window Options

{{< anchor "windowScale" "opt-window-scale" >}} applies only when launching a new plugin. Individual plugins retain manual size changes.

{{< dl >}}
{{< def terms="KeyboardFocus" id="opt-keyboard-focus" >}}
Toggles keyboard navigation.
{{< /def >}}

{{< def terms="windowScale" id="opt-window-scale" >}}
Sets the default scaling. (e.g., `1.25` is 125%.)
{{< /def >}}
{{< /dl >}}

#### Font Options

`*Name` sets the font family. `*Style` sets the style (e.g., "Regular", "Bold"). Setting `fontMonoName` to a non-monospace font may break text layouts.

{{< dl >}}
{{< def terms="fontUiName|fontUiStyle" id="opt-font-ui" >}}
Main font.
{{< /def >}}

{{< def terms="fontMonoName|fontMonoStyle" id="opt-font-mono" >}}
Must be a monospace font.
{{< /def >}}
{{< /dl >}}

#### Color Options

Use 6-digit (RGB) or 8-digit (RGBA) hex color codes. Plugins ignore the first character, so `#112233` and `?112233` both work. Don't use characters outside of `0-9a-f` in the hex part.

{{< dl >}}
{{< def terms="foregroundLight|foregroundDark" id="opt-color-fg" >}}
Text colors. Automatically switches based on background contrast.
{{< /def >}}

{{< def terms="background" id="opt-color-bg" >}}
Empty space color.
{{< /def >}}

{{< def terms="surface" id="opt-color-surface" >}}
Control background color.
{{< /def >}}

{{< def terms="border" id="opt-color-border" >}}
Control border color.
{{< /def >}}

{{< def terms="main|accent|warning" id="opt-color-highlight" >}}
Highlight colors for mouse hover and keyboard focus. {{< anchor "accent" "opt-color-highlight-1" >}} is rarely used.
{{< /def >}}
{{< /dl >}}

## Controls

Plugin GUI mock-ups on this page have limited shortcuts or functionality.

### Keyboard Navigation

This is a screen reader accessibility feature.

If DAW shortcuts fail on the plugin GUI, disable "Keyboard Navigation" in {{< anchor "GUI Settings" "gui-settings" >}}.

{{< dl >}}
{{< def terms="Navigate" id="global-navigate" >}}
{{< shortcuts >}}<kbd>Tab</kbd> | <kbd>Shift</kbd>+<kbd>Tab</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Interact" id="global-interact" >}}
{{< shortcuts >}}<kbd>Space</kbd> | <kbd>Enter</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Close" id="global-close" >}}
{{< shortcuts >}}<kbd>Esc</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

### GUI Settings {#gui-settings}

The menu items are shown below.

{{< dl >}}
{{< def terms="Reset Window Size" id="settings-reset-window" >}}
Resets window scaling. Default set in `_active.json`.
{{< /def >}}

{{< def terms="Keyboard Navigation" id="settings-keyboard-navigation" >}}
Toggles keyboard navigation. Disable if DAW shortcuts fail. Enabled by default.
{{< /def >}}

{{< def terms="Logging" id="settings-logging" >}}
Toggles logging. Keep disabled unless troubleshooting.
{{< /def >}}

{{< def terms="Open Preset Folder" id="settings-open-preset-folder" >}}
Opens [preset folder]({{< ref "#preset-folder" >}}).
{{< /def >}}

{{< def terms="Open Appearance Folder" id="settings-open-appearance-folder" >}}
Opens [appearance folder]({{< ref "#appearance-folder" >}}).
{{< /def >}}

{{< def terms="Change Appearance" id="settings-appearance" >}}
Loads appearances.
{{< /def >}}
{{< /dl >}}

#### Log File Path

| OS               | Path                                                                 |
|------------------|----------------------------------------------------------------------|
| Windows          | `%APPDATA%\UhhyouPlugins\Logs\`                                      |
| macOS            | `~/Library/UhhyouPlugins/Logs`                                       |
| Linux (Primary)  | `$XDG_CONFIG_HOME/UhhyouPlugins/Logs/`                               |
| Linux (Fallback) | `$HOME/.config/UhhyouPlugins/Logs/` (If `$XDG_CONFIG_HOME` is empty) |

### Knob

{{< div class="gui-section" >}}

<div class="plugin-window" label="Click to activate">
  <div class="knob-row" style="align-items: center;">
    <div class="knob-container">
      <uhhyou-knob id="demo-clamped" value="0.5" data-snaps="0, 0.25, 0.5, 0.75, 1.0"></uhhyou-knob>
      <label class="label">Clamped</label>
    </div>
    <div class="knob-container">
      <uhhyou-rotary-knob id="demo-rotary" value="0.0" data-snaps="0, 0.5, 1.0"></uhhyou-rotary-knob>
      <label class="label">Rotary</label>
    </div>
    <div class="column">
      <div class="column" style="gap: 0.25rem;">
        <label class="label" style="text-align: left;">TextKnob</label>
        <uhhyou-text-knob id="demo-text-clamped" value="0.5" data-min="0" data-max="9" data-step="1"></uhhyou-text-knob>
      </div>
      <div class="column" style="gap: 0.25rem;">
        <label class="label" style="text-align: left;">RotaryTextKnob</label>
        <uhhyou-rotary-text-knob id="demo-text-rotary" value="0.5" data-min="-10" data-max="10" data-step="5"></uhhyou-rotary-text-knob>
      </div>
    </div>
  </div>
</div>

{{< dl >}}
{{< def terms="Adjust" id="knob-adjust" >}}
{{< shortcuts >}}🖱️Vertical Drag | 🖱️Scroll | <kbd>↑</kbd> | <kbd>↓</kbd> | <kbd>←</kbd> | <kbd>→</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Fine-Tune" id="knob-fine-tune" >}}
{{< shortcuts >}}<kbd>Shift</kbd>+{{< anchor "Adjust" "knob-adjust" >}}{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Cycle" id="knob-cycle" >}}
{{< shortcuts >}}🖱️Middle-Click | <kbd>Home</kbd> | <kbd>End</kbd> | {{< modkey >}}+<kbd>↑</kbd> | {{< modkey >}}+<kbd>↓</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Reset" id="knob-reset" >}}
{{< shortcuts >}}{{< modkey >}}+🖱️Click | {{< modkey >}}+<kbd>Backspace</kbd> | {{< modkey >}}+<kbd>Del</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Floor" id="knob-floor" >}}
{{< shortcuts >}}<kbd>Shift</kbd>+🖱️Middle-Click | <kbd>Shift</kbd>+<kbd>Backspace</kbd> | <kbd>Shift</kbd>+<kbd>Del</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Edit Value" id="knob-edit-value" >}}
{{< shortcuts >}}🖱️Double-Click | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Host Menu" id="knob-host-menu" >}}
{{< shortcuts >}}🖱️Right-Click | <kbd>Shift</kbd>+<kbd>F10</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

{{< /div >}}

### XY Pad

{{< div class="gui-section" >}}
<div class='plugin-window'>
  <div class='row'>
    <uhhyou-labeled-xypad label-x='X-Axis' label-y='Y-Axis'>
      <uhhyou-xy-pad
        x='0.5'
        y='0.5'
        data-snaps-x='0.5'
        data-snaps-y='0.5'>
      </uhhyou-xy-pad>
    </uhhyou-labeled-xypad>
  </div>
</div>

<div style='width: 100%;'>

<p>
Toggle snap (top-left button): {{< shortcuts >}}🖱️Click | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
</p>

{{< dl >}}
{{< def terms="Adjust" id="xypad-adjust" >}}
{{< shortcuts >}}🖱️Drag{{< /shortcuts >}}

X: {{< shortcuts >}}<kbd>←</kbd> | <kbd>→</kbd> | 🖱️Scroll{{< /shortcuts >}}

Y: {{< shortcuts >}}<kbd>↑</kbd> | <kbd>↓</kbd> | <kbd>Shift</kbd>+🖱️Scroll{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Fine-Tune" id="xypad-fine-tune" >}}
X: {{< shortcuts >}}<kbd>Shift</kbd>+<kbd>←</kbd> | <kbd>Shift</kbd>+<kbd>→</kbd>{{< /shortcuts >}}

Y: {{< shortcuts >}}<kbd>Shift</kbd>+<kbd>↑</kbd> | <kbd>Shift</kbd>+<kbd>↓</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Cycle" id="xypad-cycle" >}}
X: {{< shortcuts >}}<kbd>PgUp</kbd> | <kbd>PgDn</kbd> | {{< modkey >}}+<kbd>←</kbd> | {{< modkey >}}+<kbd>→</kbd>{{< /shortcuts >}}

Y: {{< shortcuts >}}<kbd>Home</kbd> | <kbd>End</kbd> | {{< modkey >}}+<kbd>↑</kbd> | {{< modkey >}}+<kbd>↓</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Reset" id="xypad-reset" >}}
{{< shortcuts >}}{{< modkey >}}+🖱️Click | {{< modkey >}}+<kbd>Backspace</kbd> | {{< modkey >}}+<kbd>Del</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Lock Axis" id="xypad-lock-axis" >}}
X: {{< shortcuts >}}🖱️Middle-Click+🖱️Drag{{< /shortcuts >}}

Y: {{< shortcuts >}}<kbd>Shift</kbd>+🖱️Middle-Click+🖱️Drag{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Host Menu" id="xypad-host-menu" >}}
X: {{< shortcuts >}}🖱️Right-Click (Left Half) | <kbd>Shift</kbd>+<kbd>F10</kbd>{{< /shortcuts >}}

Y: {{< shortcuts >}}🖱️Right-Click (Right Half) | {{< modkey >}}+<kbd>Shift</kbd>+<kbd>F10</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

</div>

{{< /div >}}

### Button

{{< div class="gui-section" >}}
<div class="plugin-window" label="Click to activate">
  <div class="row">
    <span class="parameter-label">ToggleButton</span>
    <uhhyou-toggle-button label="Switch" type="common"></uhhyou-toggle-button>
    <uhhyou-toggle-button label="Toggle" type="accent"></uhhyou-toggle-button>
    <uhhyou-toggle-button label="Extra Gain" type="warning"></uhhyou-toggle-button>
  </div>
  <div class="row">
    <span class="parameter-label">MomentaryButton</span>
    <uhhyou-momentary-button label="Reset" type="common"></uhhyou-momentary-button>
    <uhhyou-momentary-button label="Tap" type="accent"></uhhyou-momentary-button>
    <uhhyou-momentary-button label="Panic!" type="warning"></uhhyou-momentary-button>
  </div>
</div>

{{< dl >}}
{{< def terms="Trigger" id="button-trigger" >}}
{{< shortcuts >}}🖱️Click | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Host Menu" id="button-host-menu" >}}
{{< shortcuts >}}🖱️Right-Click | <kbd>Shift</kbd>+<kbd>F10</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}
{{< /div >}}

### Combo Box

{{< div class="gui-section" >}}
<div class="plugin-window" label="Click to activate">
  <div class="row">
    <span class="parameter-label">Oscillator Shape</span>
    <uhhyou-combo-box items="Sine,Triangle,Sawtooth,Square,Noise" value="2" type="common"></uhhyou-combo-box>
  </div>
  <div class="row">
    <span class="parameter-label">Filter Type</span>
    <uhhyou-combo-box items="Lowpass 12dB,Lowpass 24dB,Highpass,Bandpass" value="0" type="accent"></uhhyou-combo-box>
  </div>
  <div class="row">
    <span class="parameter-label">Routing</span>
    <uhhyou-combo-box items="Serial,Parallel,Left/Right,Mid/Side" value="1" type="warning"></uhhyou-combo-box>
  </div>
</div>

{{< dl >}}
{{< def terms="Open" id="combobox-open" >}}
{{< shortcuts >}}🖱️Click | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Cycle" id="combobox-cycle" >}}
{{< shortcuts >}}🖱️Scroll | <kbd>↑</kbd> | <kbd>↓</kbd> | <kbd>←</kbd> | <kbd>→</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Reset" id="combobox-reset" >}}
{{< shortcuts >}}{{< modkey >}}+🖱️Click | {{< modkey >}}+<kbd>Backspace</kbd> | {{< modkey >}}+<kbd>Del</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Host Menu" id="combobox-host-menu" >}}
{{< shortcuts >}}🖱️Right-Click | <kbd>Shift</kbd>+<kbd>F10</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}
{{< /div >}}

### Preset Manager

{{< div class="gui-section" >}}
<div class="plugin-window" label="Click to activate">
  <div class="column">
    <span class="label">Preset Manager</span>
    <uhhyou-preset-manager preset="Init Preset"></uhhyou-preset-manager>
  </div>
</div>

<div>

<p>
External file changes require a {{< anchor "Refresh" "preset-refresh" >}}. Slow network drives may pause the UI.
</p>

{{< dl >}}
{{< def terms="Open Menu" id="preset-open-menu" >}}
{{< shortcuts >}}🖱️Click Preset Name | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Cycle" id="preset-cycle" >}}
{{< shortcuts >}}🖱️Click UI Arrows (⮜, ⮞) | <kbd>←</kbd> | <kbd>→</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Refresh" id="preset-refresh" >}}
{{< shortcuts >}}Select in {{< anchor "Menu" "preset-open-menu" >}}{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

</div>

{{< /div >}}

### Parameter Lock

{{< div class="gui-section" >}}
<div class="plugin-window" label="Click to activate">
  <div class="row">
    <div class="column">
      <uhhyou-labeled-widget label="Cutoff Freq">
        <uhhyou-text-knob value="0.5"></uhhyou-text-knob>
      </uhhyou-labeled-widget>
      <uhhyou-labeled-widget label="Resonance" locked="true">
        <uhhyou-text-knob value="0.75"></uhhyou-text-knob>
      </uhhyou-labeled-widget>
      <uhhyou-labeled-widget label="Filter Type">
        <uhhyou-combo-box items="Lowpass,Highpass,Bandpass" value="0"></uhhyou-combo-box>
      </uhhyou-labeled-widget>
    </div>
    <div class="column">
      <uhhyou-momentary-button id="btn-randomize" label="Randomize" type="warning"></uhhyou-momentary-button>
    </div>
  </div>
</div>

<div style='width: 100%;'>

<p>
Locks parameters from "Randomize". Dimmed labels are locked.
</p>

{{< dl >}}
{{< def terms="Toggle" id="lock-toggle" >}}
{{< shortcuts >}}🖱️Click Label | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

</div>

{{< /div >}}

### Horizontal Drawer

{{< div class="gui-section" >}}

<div class="plugin-window" label="Click to activate">
  <uhhyou-horizontal-drawer name="Effects" open="true">
    <div class="row" style="padding: var(--margin); padding-left: calc(2 * var(--margin));">
      <div class="knob-container">
        <uhhyou-knob id="drawer-example-knob" value="0.5" data-snaps="0, 0.5, 1"></uhhyou-knob>
        <label class="label">Example Control</label>
      </div>
    </div>
  </uhhyou-horizontal-drawer>
</div>

<div style='width: 100%;'>

<p>
Hides or shows contents. Focus returns to the toggle button if closed from inside.
</p>

{{< dl >}}
{{< def terms="Toggle" id="drawer-toggle" >}}
{{< shortcuts >}}🖱️Click | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

</div>

{{< /div >}}

### Tab View

<div class="plugin-window" label="Click to activate" style="float=none; margin-bottom: 1em;">
  <uhhyou-tab-view tabs="Tab 1,Tab 2,Tab 3">
    <div slot="tab-0" class="tab-content">
      <h3>Tab View</h3>
      <ul>
        <li>{{< shortcuts >}}<kbd>Mouse Wheel</kbd>{{< /shortcuts >}} or {{< shortcuts >}}<kbd>←</kbd> | <kbd>→</kbd>{{< /shortcuts >}} keys to switch tabs.</li>
        <li>{{< shortcuts >}}<kbd>Tab</kbd> | <kbd>Shift</kbd>+<kbd>Tab</kbd>{{< /shortcuts >}} to navigate into a tab.</li>
      </ul>
    </div>
    <div slot="tab-1" class="tab-content">
      <h3>あざらしサラリーマン</h3>
      <div class="code-block">「あぁぁあ、疲れた、、、。もうだめだぁ、あざらしになりたあぁぁい。。」<br>「その願い、聞き届けた。叶えてやろう。」<br>「あ、あなたはいったい？？」<br>「私は魔法使いだ。さあ、あざらしになれっ。」<br>「きゅううぅぅぅぅぅぅぅぅぅぅ！！！」</div>
    </div>
    <div slot="tab-2" class="tab-content">
      <h3>あざらしサラリーマン in 南極</h3>
      <div class="code-block">ここは南極、ウェッデル海の沿岸。南アメリカ大陸の南端から、やや南東に位置する氷の海の奥の端だ。魔法使いによってウェッデルアザラシとなった私は、岩と氷しかない極寒の大陸で新たな生を受けた。この文章を読んでいる皆は知っていただろうか。ウェッデルアザラシの80%は3歳にもならずに死んでしまうことを。確かに私はあざらしになりたいとは言ったが、今、とてつもない自然の脅威に直面して恐れおののいている。<br>私はあざらし。ウェッデルあざらしだ。</div>
    </div>
  </uhhyou-tab-view>
</div>

{{< dl >}}
{{< def terms="Switch" id="tab-switch" >}}
{{< shortcuts >}}🖱️Click Header | 🖱️Scroll Header | <kbd>←</kbd> | <kbd>→</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

### Info & License

{{< dl >}}
{{< def terms="View" id="info-view" >}}
{{< shortcuts >}}🖱️Click | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Navigate" id="info-navigate" >}}
{{< shortcuts >}}<kbd>Tab</kbd> | <kbd>↑</kbd> | <kbd>↓</kbd> | <kbd>←</kbd> | <kbd>→</kbd> | <kbd>PgUp</kbd> | <kbd>PgDn</kbd> | <kbd>Home</kbd> | <kbd>End</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="Select/Copy" id="info-select-copy" >}}
{{< shortcuts >}}{{< modkey >}}+<kbd>A</kbd> (Select All) | {{< modkey >}}+<kbd>C</kbd> (Copy){{< /shortcuts >}}
{{< /def >}}

{{< def terms="Close" id="info-close" >}}
{{< shortcuts >}}🖱️Click Margins | <kbd>Esc</kbd> | <kbd>Enter</kbd> or <kbd>Space</kbd> when margin has focus{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}
