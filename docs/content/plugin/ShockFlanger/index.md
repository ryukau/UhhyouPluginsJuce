---
title: "ShockFlanger"
date: 3000-01-01
draft: false
toc: true
section_number: true
---

![ShockFlanger screenshot.](./ShockFlanger.webp)

ShockFlanger is a hybrid of a distortion and a flanger. It sounds like a guitar amp. The internal is close to a through-zero flanger with saturators and waveshapers on the feedback paths. High CPU load.

## Warnings
Loud output. Place a limiter after ShockFlanger.

Several options in {{< anchor "Saturation Type" >}} may output loud spikes when combined with high {{< anchor "Saturation Gain" >}}. Enabling {{< anchor "More Feedback" >}} can blow up the feedback when {{< anchor "Feedback 0" >}} & {{< anchor "Feedback 1" >}} are high. High amount of {{< anchor "Audio -> Time 0" >}} & {{< anchor "Audio -> Time 1" >}} may cause loud burst. {{< anchor "Feedback Gate" >}} has a fixed threshold for simplicity. External gate before ShockFlanger is advised for the low S/N signal.

## Input/Output
- 1 Stereo input. (2 channel in 1 bus)
- 1 Stereo output. (2 channel in 1 bus)
- 1 MIDI or note event input.

## Parameters

### ∴ (Miscellaneous)
{{< dl >}}
{{< def terms="Invert Wet" >}}
Inverts polarity of wet signal.
{{< /def >}}

{{< def terms="2x Sampling" >}}
Toggles 2x oversampling. Doubles CPU load and changes high.
{{< /def >}}

{{< def terms="More Feedback" >}}
Adds more feeedback if {{< anchor "Flange Blend" >}} is not `0.0`. Use with external gate while turning on {{< anchor "Feedback Gate" >}}.
{{< /def >}}

{{< def terms="Feedback Gate" >}}
Mutes delay tail when input signal drops below threshold.
{{< /def >}}
{{< /dl >}}

### Mix

{{< dl >}}
{{< def terms="Dry" >}}
Sets gain of bypassed input signal.
{{< /def >}}

{{< def terms="Wet" >}}
Sets gain of processed signal.
{{< /def >}}
{{< /dl >}}

### Saturation

{{< dl >}}
{{< def terms="Saturation Type" >}}
Selects saturation algorithm.
{{< /def >}}

{{< def terms="Saturation Gain" >}}
Sets gain applied before saturator stage.
{{< /def >}}
{{< /dl >}}

### Delay & Feedback

{{< dl >}}
{{< def terms="Delay Time 0" >}}
Sets delay time for line 0 in milliseconds.
{{< /def >}}

{{< def terms="Delay Time 1" >}}
Sets relative delay time for line 1 as ratio of {{< anchor "Delay Time 0" >}}.
{{< /def >}}

{{< def terms="Input Blend" >}}
Crossfades input signal between delay lines.
{{< /def >}}

{{< def terms="Flange Blend" >}}
Blends internal routing:
- `0.0`: Size 2 feedback delay network (FDN).
- `1.0`: Through-zero flanger.
{{< /def >}}

{{< def terms="Flange Polarity" >}}
Sets polarity of feedback on through-zero flanger path. Mixes full-wave rectified signal if it's not exactly `-1.0` or `1.0`.
{{< /def >}}

{{< def terms="Feedback 0|Feedback 1" >}}
Sets amount of signal fed back into delay line 0 and 1.
{{< /def >}}

{{< def terms="Lowpass|Highpass" >}}
Sets cutoff frequencies for filters inside feedback loops.
{{< /def >}}
{{< /dl >}}

### LFO

{{< dl >}}
{{< def terms="Tempo Sync." >}}
Sets LFO synchronization mode:
- `Phase`: Locks to host transport. May cause glitch when {{< anchor "LFO Rate" >}} is changed.
- `Frequency`: Locks to host BPM with free-running phase. Suitable for {{< anchor "LFO Rate" >}} automation.
- `Off`: Fixed at 120 BPM.
{{< /def >}}

{{< def terms="LFO Rate" >}}
Sets LFO speed. Displayed in beats or Hz depending on {{< anchor "Tempo Sync." >}}.
{{< /def >}}

{{< def terms="Initial Phase" >}}
Shifts starting phase of LFO.
{{< /def >}}

{{< def terms="Stereo Phase" >}}
Shifts phase offset between left and right LFOs.
{{< /def >}}

{{< def terms="Reset LFO Phase" >}}
Resets LFO to {{< anchor "Initial Phase" >}}.
{{< /def >}}
{{< /dl >}}

### Modulation

{{< dl >}}
{{< def terms="LFO -> Time 0|LFO -> Time 1" >}}
Sets LFO modulation depth for delay times (in octaves).
{{< /def >}}

{{< def terms="Tracking" >}}
Scales modulation depth relative to current delay time.
{{< /def >}}

{{< def terms="Audio Mod Mode" >}}
Crossfades audio-rate modulation source:
- `0.0`: Saturated input signal.
- `1.0`: Viscosity-filtered feedback signal.
{{< /def >}}

{{< def terms="Viscosity LP" >}}
Sets lowpass cutoff frequency for audio-rate modulation source. Effective when {{< anchor "Audio Mod Mode" >}} > 0.
{{< /def >}}

{{< def terms="Audio -> Time 0|Audio -> Time 1" >}}
Sets audio-rate modulation scaling with respect to delay times.
{{< /def >}}

{{< def terms="Audio -> AM 0|Audio -> AM 1" >}}
Sets amplitude modulation (AM) depth applied by audio-rate modulation source.
{{< /def >}}
{{< /dl >}}

### MIDI

{{< dl >}}
{{< def terms="Receive Note" >}}
Toggles modulations via MIDI notes.
{{< /def >}}

{{< def terms="Note Pitch" >}}
Scales delay time modulation from MIDI pitch.
{{< /def >}}

{{< def terms="Note Gain" >}}
Scales pre-saturation gain from MIDI velocity.
{{< /def >}}
{{< /dl >}}
