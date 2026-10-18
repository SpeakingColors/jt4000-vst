# JT-4000 MICRO: MIDI implementation

Sources:
- Behringer JT-4000 MICRO Quick Start Guide V1.0 (our project folder), controls section.
- midi.guide database page for "Behringer JT-4000M MICRO" (last update 2026-02-04, 31 parameters), licensed CC BY-SA 4.0: https://midi.guide/d/behringer/jt-4000m-micro/ . The CC table below is from that dataset; please keep this credit and license if you copy it.
- Second source for the CC numbers: the free web editor page https://www.behringer-jt4000m-editor.co.uk/ (v1.03, Web MIDI, not Safari) publishes the same table. All 31 CCs match midi.guide except OSC2 Wave (22 there, 25 on midi.guide). It publishes no SysEx, NRPN, wave names or value ranges.
- The "User manual (32 pages)" PDF in the folder is only a 2-page stub (cover), nothing usable.

Cross-check: the Quick Start Guide says "Extensive parameter control via MIDI CCs" and lists 32 editable presets that can be archived with SysEx. It does not print the CC numbers. The CC numbers come only from midi.guide and should be confirmed on the hardware (see Open items).

## CC map (from midi.guide, all 0 to 127)

| Group | Parameter | CC | Note |
| --- | --- | --- | --- |
| Osc | OSC1 Wave | 24 | |
| Osc | OSC1 PWM / Supersaw detune / FM feedback | 113 | meaning depends on OSC1 wave |
| Osc | OSC1 Coarse | 115 | centered |
| Osc | OSC1 Fine | 111 | centered |
| Osc | OSC2 Wave | 25 or 22 | **sources disagree**: midi.guide says 25, behringer-jt4000m-editor.co.uk says 22. Test on hardware |
| Osc | OSC2 PWM | 114 | |
| Osc | OSC2 Coarse | 116 | centered |
| Osc | OSC2 Fine | 112 | centered |
| Osc | OSC balance | 29 | |
| VCA env | Attack, Decay, Sustain, Release | 81, 82, 83, 84 | |
| Filter | Resonance | 71 | |
| Filter | Cutoff | 74 | |
| Filter | Envelope amount | 47 | |
| VCF env | Attack, Decay, Sustain, Release | 85, 86, 87, 88 | |
| LFO1 | Destination | 56 | |
| LFO1 | Wave | 54 | |
| LFO1 | Amount | 70 | |
| LFO1 | Rate | 72 | |
| LFO2 | Wave | 55 | |
| LFO2 | Rate | 73 | |
| LFO2 | Amount | 28 | |
| Mod | Mod wheel | 1 | |
| Mod | Ring mod amount | 95 | |
| Mod | Ring mod on/off | 96 | 0 to 64 off, 65 to 127 on |
| General | Portamento time | 5 | |

## Panel menus (Quick Start Guide), A = first parameter, B = second, per menu button
- OSC-X: OSC1 A wave (Off, triangle, square, PWM, saw, supersaw, FM, noise), B detune (supersaw) or feedback (FM, 0 to 99%); OSC1 PWM; OSC1 coarse / fine; OSC2 A wave (Off, triangle, square, PWM, saw, noise), PWM, coarse / fine; Porta A amount, B mode (portamento or glissando); Ring A amount, B on/off; program name edit.
- FM: operator 1 (OSC1) is the carrier, operator 2 (OSC2) the modulator, feedback 0 to 99%.
- LFO-X: LFO1 A wave (triangle, square, sawtooth), B destination (VCF, OSC, PWM when PWM wave selected, detune when supersaw selected); LFO1 speed, amount; LFO2 wave, speed, amount.
- FILTER: VCF attack, decay, sustain, release (0 to 99), VCF amount, VCF trigger (filter envelope retrigger on or off).
- AMP: VCA attack, decay, sustain, release (0 to 99), VCA amount (envelope modulation amount).
- ARP: motif (Up, Down, Up and Down), BPM 20 to 240, scale (1/8, 1/8T, 1/16, 1/16T, 1/32), swing 50 to 75%, clock source (Internal, USB; with USB a Start/Stop must be sent over USB).
- SETTINGS: octave of internal keyboard, mod (keyboard or arpeggiator), MIDI TX channel (can be OFF) and RX channel, MIDI velocity on/off, PROG BULK SYX SEND (all programs), PROG SING SYX SEND (current program), SYSEX RECEIVE, FACTORY RESET.
- Edited presets show `*` until saved with ENTER. Knob values apply only after the knob passes about 9 o'clock (pickup behaviour), per the guide.

## Open items
0. OSC2 Wave CC: 25 or 22? Send both on the hardware and watch the display.
1. CC list is from one community source: verify each CC on the hardware with a MIDI monitor (turn each control, watch the CC out, if the synth transmits).
2. The panel has controls with no CC in the list: LFO2 destination, OSC2 PWM wave dependence, VCA amount, VCF trigger, arp settings, portamento mode, OSC/LFO wave value-to-name mapping (which CC value picks which wave). Need captures.
3. SysEx patch dump format (PROG BULK SYX SEND, PROG SING SYX SEND) is not documented in our sources. Plan: put the synth in dump mode and capture the bytes with `amidi` / aseqdump on Linux, then document them here. That also gives a clean-room librarian format without touching any third-party editor.
4. Third-party editors exist: Momo's JT-4000 Editor (paid, closed) and a free browser editor (Web MIDI). Public pages of both were used for the CC list only. We do not decompile Momo's paid editor: its terms are not stated, and capturing the synth's own traffic gets the same data legally.
