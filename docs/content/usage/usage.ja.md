---
title: "使い方"
date: 2026-04-13
draft: false
toc: true
weight: 1
gui_mockups: true
section_number: true
layout: "plugin-mockup"
---

使い方で何かあれば [GitHub のリポジトリ](https://github.com/ryukau/UhhyouPluginsJuce/discussions)に discussion を開くか `ryukau@gmail.com` にメールしてください。お気軽にどうぞ。

## システム要件

### DAW

Windows 11 環境において、以下の DAW で動作確認しています。

- FL Studio
- REAPER

### ビルド環境

古いシステムではプラグインが動作しないことがあります。

| プラットフォーム | OS                       | CPU                   |
|:---------|:-------------------------|:----------------------|
| macOS    | macOS 15 (Sequoia)       | Apple Silicon (ARM64) |
| Windows  | Windows 11 / Server 2025 | x64                   |
| Linux    | Ubuntu 24.04 LTS         | x64                   |

## インストール
プラグインは [リリースページ (github.com)](https://github.com/ryukau/UhhyouPluginsJuce/releases) からダウンロードできます。

### プラグインフォルダ

`.zip`ファイルを展開し、プラグインを以下のフォルダに移動してください。

| OS             | パス                                                                  |
|----------------|---------------------------------------------------------------------|
| Windows (VST3) | `C:\Program Files\Common Files\VST3\`                               |
| macOS (VST3)   | `~/Library/Audio/Plug-ins/VST3/`                                    |
| macOS (AU)     | `~/Library/Audio/Plug-ins/Components/` (拡張子 `.component` のプラグイン) |
| Linux (VST3)   | `~/.vst3/`                                                          |

`~/` はホームフォルダです。

### プリセットフォルダ {#preset-folder}

ダウンロードしたプリセットを、以下のいずれかのフォルダに移動してください。

| OS      | パス                                             |
|---------|------------------------------------------------|
| Windows | `%APPDATA%\UhhyouPlugins\`                     |
| macOS   | `~/Library/Application Support/UhhyouPlugins/` |
| Linux   | `~/.config/UhhyouPlugins/`                     |

このフォルダはプラグインを初めて開いたときに自動的に作成されます。手動で作成しても動作します。

### トラブルシューティング

#### Windows

DAW がプラグインを認識しないときは Microsoft Visual C++ Redistributable をインストールしてください。

- [Microsoftから `vc_redist.x64.exe` をダウンロードする](https://learn.microsoft.com/ja-jp/cpp/windows/latest-supported-vc-redist?view=msvc-170)

#### Linux (Ubuntu 24.04)

必要なライブラリ: `sudo apt install libxcb-cursor0 libxkbcommon-x11-0`

注意: REAPER がプラグインのスキャンに失敗するときは、`~/.config/REAPER/reaper-vstplugins64.ini` を削除して再起動してください。

#### macOS

Gatekeeper はプラグインが破損していると判定することがあります。修正するには以下の手順を試してください。

1. ターミナルを開く。
2. 次のコマンドを実行 (`FileName.vst3` は実際のプラグインファイル名に置換)
   - `xattr -cr ~/Library/Audio/Plug-ins/VST3/FileName.vst3`
3. DAW でプラグインを再スキャン。

上の手順で解決しないときは、次のコマンドで ad-hoc signature を付与してください。

```
sudo codesign --force --deep -s - ~/Library/Audio/Plug-ins/VST3/FileName.vst3
```

注意: `xattr` と `codesign` は macOS の組み込みコマンドです。実行時に Command Line Developer Tools のダウンロードを求められることがあります。

## 外観 (Appearance)

外観は [GUI Settings]({{< ref "#gui-settings" >}}) → [Change Appearance]({{< ref "#settings-appearance" >}}) から変更できます。

### Appearance フォルダ {#appearance-folder}

| OS               | パス                                                                    |
|------------------|-----------------------------------------------------------------------|
| Windows          | `%APPDATA%\UhhyouPlugins\appearance\`                                 |
| macOS            | `~/Library/Preferences/UhhyouPlugins/appearance/`                     |
| Linux (Primary)  | `$XDG_CONFIG_HOME/UhhyouPlugins/appearance/`                          |
| Linux (Fallback) | `$HOME/.config/UhhyouPlugins/appearance/` (`$XDG_CONFIG_HOME` が空のとき) |

`_active.json` の編集は避けてください。実行中のプラグインによって上書きされる可能性があります。

外観を追加するには、`.json` ファイルを `UhhyouPlugins/appearance/` フォルダに配置します。 JSON の書式が正しければ [GUI Settings]({{< ref "#gui-settings" >}}) → [Change Appearance]({{< ref "#settings-appearance" >}}) に表示されます。

### フォーマット

ファイルは [JSON](https://www.json.org/json-ja.html) 形式です。以下はデフォルトです。

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

要素の順番を入れ替えても動きます。最後の要素の後にコンマ `,` を置くと不正な書式となるので注意してください。書式に誤りがあると `_active.json` は上記のデフォルトで上書きされます。不足している要素はデフォルト値に置換されます。

#### ウィンドウオプション

{{< anchor "windowScale" "opt-window-scale" >}} はプラグインを新規起動した時にのみ適用されます。起動後のサイズ変更は各プラグインごとに保持されます。

{{< dl >}}
{{< def terms="KeyboardFocus" id="opt-keyboard-focus" >}}
キーボードナビゲーションの有効/無効を切り替え。
{{< /def >}}

{{< def terms="windowScale" id="opt-window-scale" >}}
デフォルトの拡大率。(例: 1.25 は 125% )
{{< /def >}}
{{< /dl >}}

#### フォントオプション

`*Name` はフォントファミリーを設定します。`*Style` はスタイル（例: "Regular", "Bold"）を設定します。`fontMonoName` に等幅でないフォントを設定するとレイアウトが崩れることがあります。

{{< dl >}}
{{< def terms="fontUiName|fontUiStyle" id="opt-font-ui" >}}
メインのフォント。
{{< /def >}}

{{< def terms="fontMonoName|fontMonoStyle" id="opt-font-mono" >}}
等幅フォント。
{{< /def >}}
{{< /dl >}}

#### カラーオプション

6 桁 (RGB) または 8 桁 (RGBA) の 16 進数カラーコードを使用します。プラグインは最初の文字を無視するため、`#112233` と `?112233` のどちらでも機能します。 16 進数の部分には `0-9a-f` 以外の文字を使用しないでください。

{{< dl >}}
{{< def terms="foregroundLight|foregroundDark" id="opt-color-fg" >}}
テキストの色。背景のコントラストに応じて自動的に切り替わる。
{{< /def >}}

{{< def terms="background" id="opt-color-bg" >}}
余白の色。
{{< /def >}}

{{< def terms="surface" id="opt-color-surface" >}}
コントロールの背景色。
{{< /def >}}

{{< def terms="border" id="opt-color-border" >}}
コントロールの枠の色。
{{< /def >}}

{{< def terms="main|accent|warning" id="opt-color-highlight" >}}
マウスホバーやキーボードフォーカスを受けたときのハイライト色。{{< anchor "accent" "opt-color-highlight-1" >}} は、ほぼ使われない。
{{< /def >}}
{{< /dl >}}

## コントロール

このページのプラグイン GUI のモックアップは、ショートカットや機能が制限されています。

### キーボード操作

スクリーンリーダーに対応するための機能です。

プラグイン GUI 上で DAW のショートカットが効かないときは、 {{< anchor "GUI Settings" "gui-settings" >}} → {{< anchor "Keyboard Navigation" "settings-keyboard-navigation" >}} を無効にしてください。

{{< dl >}}
{{< def terms="移動" id="global-navigate" >}}
{{< shortcuts >}}<kbd>Tab</kbd> | <kbd>Shift</kbd>+<kbd>Tab</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="操作" id="global-interact" >}}
{{< shortcuts >}}<kbd>Space</kbd> | <kbd>Enter</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="閉じる" id="global-close" >}}
{{< shortcuts >}}<kbd>Esc</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

### GUI Settings (GUI 設定) {#gui-settings}

メニュー項目は以下の通りです。

{{< dl >}}
{{< def terms="Reset Window Size" id="settings-reset-window" >}}
ウィンドウの拡大率をリセット。デフォルトは [外観の設定]({{< ref "#gui-settings" >}})による。
{{< /def >}}

{{< def terms="Keyboard Navigation" id="settings-keyboard-navigation" >}}
キーボードナビゲーションの有効、無効を切り替え。デフォルトで有効。 DAW のショートカットが効かないときは無効に変更。
{{< /def >}}

{{< def terms="Logging" id="settings-logging" >}}
ログの有効、無効を切り替え。トラブルシューティングを行うのでなければ無効のままを推奨。
{{< /def >}}

{{< def terms="Open Preset Folder" id="settings-open-preset-folder" >}}
[プリセットフォルダ]({{< ref "#preset-folder" >}}) を開く。
{{< /def >}}

{{< def terms="Open Appearance Folder" id="settings-open-appearance-folder" >}}
[Appearance フォルダ]({{< ref "#appearance-folder" >}}) を開く。
{{< /def >}}

{{< def terms="Change Appearance" id="settings-appearance" >}}
外観の変更。
{{< /def >}}
{{< /dl >}}

#### ログファイルのパス

| OS               | パス                                                              |
|------------------|-----------------------------------------------------------------|
| Windows          | `%APPDATA%\UhhyouPlugins\Logs\`                                 |
| macOS            | `~/Library/UhhyouPlugins/Logs`                                  |
| Linux (Primary)  | `$XDG_CONFIG_HOME/UhhyouPlugins/Logs/`                          |
| Linux (Fallback) | `$HOME/.config/UhhyouPlugins/Logs/` (`$XDG_CONFIG_HOME` が空のとき) |

### つまみ

{{< div class="gui-section" >}}

<div class="plugin-window" label='クリックして起動'>
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
{{< def terms="調整" id="knob-adjust" >}}
{{< shortcuts >}}🖱️上下ドラッグ | 🖱️スクロール | <kbd>↑</kbd> | <kbd>↓</kbd> | <kbd>←</kbd> | <kbd>→</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="微調整" id="knob-fine-tune" >}}
{{< shortcuts >}}<kbd>Shift</kbd>+{{< anchor "調整" "knob-adjust" >}}{{< /shortcuts >}}
{{< /def >}}

{{< def terms="サイクル" id="knob-cycle" >}}
{{< shortcuts >}}🖱️中クリック | <kbd>Home</kbd> | <kbd>End</kbd> | {{< modkey >}}+<kbd>↑</kbd> | {{< modkey >}}+<kbd>↓</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="リセット" id="knob-reset" >}}
{{< shortcuts >}}{{< modkey >}}+🖱️クリック | {{< modkey >}}+<kbd>Backspace</kbd> | {{< modkey >}}+<kbd>Del</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="小数点以下の切り捨て" id="knob-floor" >}}
{{< shortcuts >}}<kbd>Shift</kbd>+🖱️中クリック | <kbd>Shift</kbd>+<kbd>Backspace</kbd> | <kbd>Shift</kbd>+<kbd>Del</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="値の編集" id="knob-edit-value" >}}
{{< shortcuts >}}🖱️ダブルクリック | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="ホストメニュー" id="knob-host-menu" >}}
{{< shortcuts >}}🖱️右クリック | <kbd>Shift</kbd>+<kbd>F10</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

{{< /div >}}

### XY パッド

{{< div class="gui-section" >}}
<div class='plugin-window' label='クリックして起動'>
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
スナップの切り替え (左上のボタン): {{< shortcuts >}}🖱️クリック | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
</p>

{{< dl >}}
{{< def terms="調整" id="xypad-adjust" >}}
{{< shortcuts >}}🖱️ドラッグ{{< /shortcuts >}}

X: {{< shortcuts >}}<kbd>←</kbd> | <kbd>→</kbd> | 🖱️スクロール{{< /shortcuts >}}

Y: {{< shortcuts >}}<kbd>↑</kbd> | <kbd>↓</kbd> | <kbd>Shift</kbd>+🖱️スクロール{{< /shortcuts >}}
{{< /def >}}

{{< def terms="微調整" id="xypad-fine-tune" >}}
X: {{< shortcuts >}}<kbd>Shift</kbd>+<kbd>←</kbd> | <kbd>Shift</kbd>+<kbd>→</kbd>{{< /shortcuts >}}

Y: {{< shortcuts >}}<kbd>Shift</kbd>+<kbd>↑</kbd> | <kbd>Shift</kbd>+<kbd>↓</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="サイクル" id="xypad-cycle" >}}
X: {{< shortcuts >}}<kbd>PgUp</kbd> | <kbd>PgDn</kbd> | {{< modkey >}}+<kbd>←</kbd> | {{< modkey >}}+<kbd>→</kbd>{{< /shortcuts >}}

Y: {{< shortcuts >}}<kbd>Home</kbd> | <kbd>End</kbd> | {{< modkey >}}+<kbd>↑</kbd> | {{< modkey >}}+<kbd>↓</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="リセット" id="xypad-reset" >}}
{{< shortcuts >}}{{< modkey >}}+🖱️クリック | {{< modkey >}}+<kbd>Backspace</kbd> | {{< modkey >}}+<kbd>Del</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="軸を固定 (Lock Axis)" id="xypad-lock-axis" >}}
X: {{< shortcuts >}}🖱️中クリック+🖱️ドラッグ{{< /shortcuts >}}

Y: {{< shortcuts >}}<kbd>Shift</kbd>+🖱️中クリック+🖱️ドラッグ{{< /shortcuts >}}
{{< /def >}}

{{< def terms="ホストメニュー" id="xypad-host-menu" >}}
X: {{< shortcuts >}}🖱️右クリック (左半分) | <kbd>Shift</kbd>+<kbd>F10</kbd>{{< /shortcuts >}}

Y: {{< shortcuts >}}🖱️右クリック (右半分) | {{< modkey >}}+<kbd>Shift</kbd>+<kbd>F10</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

</div>

{{< /div >}}

### ボタン

{{< div class="gui-section" >}}
<div class="plugin-window" label='クリックして起動'>
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
{{< def terms="押す" id="button-trigger" >}}
{{< shortcuts >}}🖱️クリック | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="ホストメニュー" id="button-host-menu" >}}
{{< shortcuts >}}🖱️右クリック | <kbd>Shift</kbd>+<kbd>F10</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}
{{< /div >}}

### コンボボックス

{{< div class="gui-section" >}}
<div class="plugin-window" label='クリックして起動'>
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
{{< def terms="開く" id="combobox-open" >}}
{{< shortcuts >}}🖱️クリック | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="サイクル" id="combobox-cycle" >}}
{{< shortcuts >}}🖱️スクロール | <kbd>↑</kbd> | <kbd>↓</kbd> | <kbd>←</kbd> | <kbd>→</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="リセット" id="combobox-reset" >}}
{{< shortcuts >}}{{< modkey >}}+🖱️クリック | {{< modkey >}}+<kbd>Backspace</kbd> | {{< modkey >}}+<kbd>Del</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="ホストメニュー" id="combobox-host-menu" >}}
{{< shortcuts >}}🖱️右クリック | <kbd>Shift</kbd>+<kbd>F10</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}
{{< /div >}}

### プリセットマネージャー

{{< div class="gui-section" >}}
<div class="plugin-window" label='クリックして起動'>
  <div class="column">
    <span class="label">Preset Manager</span>
    <uhhyou-preset-manager preset="Init Preset"></uhhyou-preset-manager>
  </div>
</div>

<div>

<p>
外部ファイルの変更を反映するには {{< anchor "更新 (Refresh)" "preset-refresh" >}} が必要です。低速なネットワークドライブでは UI が一時停止する場合があります。
</p>

{{< dl >}}
{{< def terms="プリセットメニューを開く" id="preset-open-menu" >}}
{{< shortcuts >}}🖱️プリセット名 (中央) をクリック | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="サイクル" id="preset-cycle" >}}
{{< shortcuts >}}🖱️UIの矢印 (⮜, ⮞) をクリック | <kbd>←</kbd> | <kbd>→</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="更新 (Refresh)" id="preset-refresh" >}}
{{< shortcuts >}}{{< anchor "プリセットメニュー" "preset-open-menu" >}} から選択{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

</div>

{{< /div >}}

### パラメーターロック

{{< div class="gui-section" >}}
<div class="plugin-window" label='クリックして起動'>
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
Randomize (ランダム化) によるパラメーターの変更をロックします。暗く表示されているラベルはロックされています。
</p>

{{< dl >}}
{{< def terms="切り替え" id="lock-toggle" >}}
{{< shortcuts >}}🖱️ラベルをクリック | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

</div>

{{< /div >}}

### 水平引き出し

{{< div class="gui-section" >}}

<div class="plugin-window" label='クリックして起動'>
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
コンテンツの表示、非表示を切り替えます。
</p>

{{< dl >}}
{{< def terms="切り替え" id="drawer-toggle" >}}
{{< shortcuts >}}🖱️クリック | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

</div>

{{< /div >}}

### タブビュー

<div class="plugin-window" label='クリックして起動' style="float=none; margin-bottom: 1em;">
  <uhhyou-tab-view tabs="Tab 1,Tab 2,Tab 3">
    <div slot="tab-0" class="tab-content">
      <h3>Tab View</h3>
      <ul>
        <li>タブを切り替えるには {{< shortcuts >}}🖱️スクロール{{< /shortcuts >}} または {{< shortcuts >}}<kbd>←</kbd> | <kbd>→</kbd>{{< /shortcuts >}} キーを使用します。</li>
        <li>タブ内に移動するには {{< shortcuts >}}<kbd>Tab</kbd>{{< /shortcuts >}} または {{< shortcuts >}}<kbd>Shift</kbd>+<kbd>Tab</kbd>{{< /shortcuts >}} を使用します。</li>
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
{{< def terms="切り替え" id="tab-switch" >}}
{{< shortcuts >}}🖱️ヘッダーをクリック | 🖱️ヘッダーをスクロール | <kbd>←</kbd> | <kbd>→</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}

### 情報 & ライセンス

{{< dl >}}
{{< def terms="表示" id="info-view" >}}
{{< shortcuts >}}🖱️クリック | <kbd>Enter</kbd> | <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="カーソル移動" id="info-navigate" >}}
{{< shortcuts >}}<kbd>Tab</kbd> | <kbd>↑</kbd> | <kbd>↓</kbd> | <kbd>←</kbd> | <kbd>→</kbd> | <kbd>PgUp</kbd> | <kbd>PgDn</kbd> | <kbd>Home</kbd> | <kbd>End</kbd>{{< /shortcuts >}}
{{< /def >}}

{{< def terms="選択 / コピー" id="info-select-copy" >}}
{{< shortcuts >}}{{< modkey >}}+<kbd>A</kbd> (すべて選択) | {{< modkey >}}+<kbd>C</kbd> (コピー){{< /shortcuts >}}
{{< /def >}}

{{< def terms="閉じる" id="info-close" >}}
{{< shortcuts >}}🖱️余白をクリック | <kbd>Esc</kbd> | 余白をフォーカスして <kbd>Enter</kbd> または <kbd>Space</kbd>{{< /shortcuts >}}
{{< /def >}}
{{< /dl >}}
