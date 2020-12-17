# Changelog

- Added OLED parameter view, displaying input mapping, current value, and ability to modify non-mapped params via encoder/switches
- Added more IO channel options to OLED display
- Multi-app binaries will switch app according to MIDI program-change messages
- Samplerate selection (32/48/96kHz) in the oopsy interface
- Dedicated continual MIDI outputs via e.g. [out 5 midi_cc1], [out 5 midi_cc1_ch3], [out 5 midi_drum36], etc.
- Code generation cleanup
## v0.2.2-beta

- Fixes for windows compatibility

## v0.2.1-beta

- Automatic zeroing / normalling of unused audio outputs
- Better automatic naming from template patches
- Fix for parsing params declared with @default
- Examples:
  - crossover filter
  - pulsar osc
  - mod FM osc
- Updated for libdaisy board API (< do we want stuff like this that doesn't actual affect behavior in the changelog?)

## v0.2-beta

- Initial public beta


