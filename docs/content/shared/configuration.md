---
title: "Color/Window Configuration"
date: 3000-01-01
draft: false
---

**TODO**: Fix link.

Colors can be changed from {{< ref "\"Theme / Style\" menu in \"GUI Settings\"" "settings-theme" >}}.

## Path
Configuration files are located in the following paths.

| OS      | Configuration File Path                                                                                                                                     |
|---------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Windows | `%APPDATA%\UhhyouPlugins\style\style.json`                                                                                                                  |
| macOS   | `~/Library/Preferences/UhhyouPlugins/style/style.json`                                                                                                      |
| Linux   | Primary is `$XDG_CONFIG_HOME/UhhyouPlugins/style/style.json`<br>Fallbacks to `$HOME/.config/UhhyouPlugins/style/style.json` if `$XDG_CONFIG_HOME` is empty. |

`style.json` is the active configuration which may be overwritten by the running plugins.

Any valid `.json` configuration file placed inside the `UhhyouPlugins/style/` directory will automatically populate in this sub-menu. Selecting a new theme immediately updates the user interface and overwrites your active `style.json`.

## Format
JSON is used. Commas `,` are required for every line except the last one. If the configuration format is invalid, the default configuration shown below is used.

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
`windowScale` is only applied at the launch of a new plugin. Each plugin can hold a different window scaling by manually changing the window size. `windowScale` sets the default scaling in "GUI Settings".

{{< dl >}}
{{< def terms="keyboardFocusEnabled" id="opt-keyboard-focus" >}}
Toggle keyboard navigation. This is an accessibility feature.
{{< /def >}}

{{< def terms="windowScale" id="opt-window-scale" >}}
Window scaling. For example, `0.75` is 75%, and `2.0` is 200%.
{{< /def >}}
{{< /dl >}}

### Font Options
`*Name` is a font family name. `*Style` is a font style such as "Regular", "Bold" and so on. Available font styles depend on the font family. If `fontMonoName` is set to a non-monospace font, it may break text layout.

{{< dl >}}
{{< def terms="fontUiName|fontUiStyle" id="opt-font-ui" >}}
Main font.
{{< /def >}}

{{< def terms="fontMonoName|fontMonoStyle" id="opt-font-mono" >}}
Must be monospace font.
{{< /def >}}
{{< /dl >}}

### Color Options
Hex color codes are used.

- 6 digit color is RGB. For example, `#112233`.
- 8 digit color is RGBA. For example, `#445566ff`.

First letter `#` is conventional. Plugins ignore the first letter of color code, thus `?102938`, `\n11335577` are valid. Do not use characters outside of `0-9a-f` for color value.

{{< dl >}}
{{< def terms="foregroundLight|foregroundDark" id="opt-color-fg" >}}
Text colors. Automatically set to light or dark depending on the color behind the text.
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
