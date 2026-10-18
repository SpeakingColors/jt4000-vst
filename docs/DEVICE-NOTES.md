# JT-4000: device notes

Model: Behringer JT-4000 (retail name JT-4000M MICRO). Earlier project notes said "JD-4000"; that name is wrong.

## What is confirmed
Source: one retailer spec page (perfectcircuit.com, checked 2026-10-18). Not yet checked against Behringer's own manual.
- 4-voice polyphonic hybrid synth
- Two analog-modeling oscillators per voice, several waveshapes including supersaw, plus a 2-operator FM engine
- 12 dB low-pass filter
- Two ADSR envelopes (filter, amp), two LFOs
- Multimode arpeggiator
- 32 memory presets, 16 touch keys
- USB and 3.5 mm MIDI input, USB-C bus power

## Not confirmed (do not build on these yet)
- CC or NRPN map, SysEx, patch dump format
- USB MIDI port names on Linux
- Effects section

The Behringer manual and MIDI chart are not in the project folder or on the PC (checked 2026-10-18). Send the PDF or a capture and this file gets the real MIDI doc.

## Needed before building
1. Behringer quick start guide and MIDI implementation chart.
2. USB MIDI port names on Linux.
3. Whether parameters are CC or NRPN, and whether presets can be dumped over SysEx.
