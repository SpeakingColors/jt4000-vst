# JT-4000: device notes

Model: Behringer JT-4000 MICRO (also sold as JT-4000M). Earlier notes said "JD-4000": wrong. Source: Behringer Quick Start Guide V1.0.

## Hardware facts
- Portable 4-voice hybrid synth, two analog-modeling oscillators per voice ("based on 1990s JP8K" per the guide), optional two-operator FM
- 12 dB/octave VCF, separate ADSR for VCA and VCF, two LFOs, arpeggiator
- 32 editable presets, archivable by SysEx
- USB-C (power, USB MIDI, firmware update), 3.5 mm MIDI input, 3.5 mm TRS headphones, 16-key touch keyboard, OLED display with oscilloscope trace

## MIDI
See [MIDI-IMPLEMENTATION.md](MIDI-IMPLEMENTATION.md).

## Existing editors
A free browser editor also exists (behringer-jt4000m-editor.co.uk, Web MIDI, v1.03); it works in Chromium on Linux and is a handy way to test the CCs.

Momo's JT-4000 Editor (paid, about 7 EUR, Windows/Mac VST/AU/standalone, iOS) already exists. It has no Linux build. Our controller will be free, open source, Linux-first.

## Needed next
1. Capture the synth's SysEx dump and any CC output with a MIDI monitor.
2. USB MIDI port names on Linux.
