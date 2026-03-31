---
title: "GUI Configuration"
date: 3000-01-01
draft: false
toc: true
weight: 3
---

**TODO**: Fix link.

Change colors via the ["Theme / Style"]({{< ref "controls.md#settings-theme" >}}) menu in ["GUI Settings"]({{< ref "controls.md#gui-settings" >}}).

## Configuration File Path

- Windows: `%APPDATA%\UhhyouPlugins\style\style.json`
- macOS: `~/Library/Preferences/UhhyouPlugins/style/style.json`
- Linux (Primary): `$XDG_CONFIG_HOME/UhhyouPlugins/style/style.json`
- Linux (Fallback): `$HOME/.config/UhhyouPlugins/style/style.json` if `$XDG_CONFIG_HOME` is empty.

`style.json` is the active configuration. Running plugins may overwrite it.

Place any valid `.json` configuration file in the `UhhyouPlugins/style/` directory to add it to the theme menu. Selecting a new theme updates the UI immediately and overwrites `style.json`.

## Format

The file uses standard JSON format. Invalid formatting will trigger the default configuration shown below.

```json
{
  "keyboardFocusEnabled": true,
  "windowScale"         : 1.0,
  "fontUiName"          : "Ubuntu",
  "fontUiStyle"         : "Regular",
  "fontMonoName"        : "Ubuntu Mono",
  "fontMonoStyle"       : "Regular",
  "foregroundLight"     : "#ffffffff",
  "foregroundDark"      : "#000000ff",
  "background"          : "#ffffffff",
  "surface"             : "#f8f8f8ff",
  "border"              : "#888888ff",
  "main"                : "#fcc04fff",
  "accent"              : "#13c136ff",
  "warning"             : "#fc8080ff"
}
```

### Window Options

`windowScale` applies only when launching a new plugin (e.g., `0.75` is 75%, `2.0` is 200%). Individual plugins retain manual size changes.

{{< dl >}}
{{< def terms="keyboardFocusEnabled" id="opt-keyboard-focus" >}}
Toggles keyboard navigation.
{{< /def >}}

{{< def terms="windowScale" id="opt-window-scale" >}}
Sets the default scaling.
{{< /def >}}
{{< /dl >}}

### Font Options

`*Name` sets the font family. `*Style` sets the style (e.g., "Regular", "Bold"). Setting `fontMonoName` to a non-monospace font may break text layouts.

{{< dl >}}
{{< def terms="fontUiName|fontUiStyle" id="opt-font-ui" >}}
Main font.
{{< /def >}}

{{< def terms="fontMonoName|fontMonoStyle" id="opt-font-mono" >}}
Must be a monospace font.
{{< /def >}}
{{< /dl >}}

### Color Options

Use 6-digit (RGB) or 8-digit (RGBA) hex color codes. Plugins ignore the first character, so `?112233` and `\n112233` both work. Don't use characters outside `0-9a-f`.

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
Highlight colors for mouse hover and keyboard focus. `accent` is rarely used.
{{< /def >}}
{{< /dl >}}
