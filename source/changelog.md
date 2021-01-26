# Changelog

- Support for Noise Engineering's Versio, including all on-panel controls and LEDs, and template patch. 
- Param view added to Daisy Field; params not tweakable (because of no encoder) but at least visible
- Params like "knob1_int_foo" or "knob2_bool_bar" will be locked to integer/bool values within their respective @min/@max ranges
- Added ability to select custom JSON config from within oopsy.maxpat via "browse" button or @target argument
- Modifiedg config JSON and genlib_daisy.h to create more flexibiity for custom Seed targets
- Fix: ensure program-change midi handling is generated for multi-app even when no apps used midi
- moved midi handling into app-level code to support custom midi handlers as [param]; raw midi handling not generated if not used
- added support for a few [param] midi handling types such as midi_cc, midi_bend, midi_vel, midi_clock, etc. 
- added "boost" option to Max and Node.js which boosts CPU frequency from 400 to 480MHz
- added "fastmath" option to Max and Node.js interfaces to replace transcendental functions with cheaper and smaller approximations
- added blocksize selection to Max and Node.js interfaces
- oopsy reports total binary size
- Code generation cleanup

## v0.3.0-beta

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


