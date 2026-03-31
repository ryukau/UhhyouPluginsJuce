---
title: "GUI Controls"
date: 3000-01-01
draft: false
toc: true
weight: 2
gui_mockups: true
layout: "plugin-mockup"
---

If DAW shortcuts fail, disable "Keyboard Navigation" in GUI Settings.

Mock-ups have limited functionality.

## Keyboard Navigation

Restores focus to the last control after {{< shortcuts >}}<kbd>Alt</kbd>+<kbd>Tab</kbd>{{< /shortcuts >}}.

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

## GUI Settings

{{< dl >}}
{{< def terms="Keyboard Navigation" id="settings-keyboard-nav" >}}
Toggles keyboard navigation. Disable if DAW shortcuts fail. Enabled by default.
{{< /def >}}

{{< def terms="Reset Window Size" id="settings-reset-window" >}}
Resets window scaling. Default set in `style.json`.
{{< /def >}}

{{< def terms="Logging" id="settings-logging" >}}
Toggles logging. Keep disabled unless troubleshooting.
{{< /def >}}

{{< def terms="Theme / Style" id="settings-theme" >}}
Loads custom color and font themes.
{{< /def >}}
{{< /dl >}}

Log file path:

- Windows: `%APPDATA%\UhhyouPlugins\Logs\`
- macOS: `~/Library/UhhyouPlugins/Logs`
- Linux (Primary): `$XDG_CONFIG_HOME/UhhyouPlugins/Logs/`
- Linux (Fallback): `$HOME/.config/UhhyouPlugins/Logs/` if `$XDG_CONFIG_HOME` is empty.

## Knob

{{< div class="gui-section" >}}

<div class="plugin-window">
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

## XY Pad

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

## Button

{{< div class="gui-section" >}}
<div class="plugin-window">
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

## Combo Box

{{< div class="gui-section" >}}
<div class="plugin-window">
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

## Preset Manager

{{< div class="gui-section" >}}
<div class="plugin-window">
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

## Parameter Lock

{{< div class="gui-section" >}}
<div class="plugin-window">
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

## Horizontal Drawer

{{< div class="gui-section" >}}

<div class="plugin-window">
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

## Tab View

<div class="plugin-window" style="float=none; margin-bottom: 1em;">
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
      <div class="code-block">ここは南極、ウェッデル海の沿岸。南アメリカ大陸の南端から、やや南東に位置する氷の海の奥の端だ。魔法使いによってウェッデルアザラシとなった私は、岩と氷しかない極寒の大陸で新たな生を受けた。この文章を読んでいる皆は知っていただろうか。ウェッデルアザラシの80%は3歳にもならずに死んでしまうことを。確かに私はあざらしになりたいとは言ったが、今、とてつもない自然の脅威に直面して恐れおののいている。私はあざらし。ウェッデルあざらしだ。</div>
    </div>
  </uhhyou-tab-view>
</div>

{{< dl >}}
{{< def terms="Switch" id="tab-switch" >}}
{{< shortcuts >}}🖱️Click Header | 🖱️Scroll Header | <kbd>←</kbd> | <kbd>→</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

## Info & License

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
