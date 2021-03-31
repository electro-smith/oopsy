# Changelog

- Params:
  - Params like "knob1_int_foo" or "knob2_bool_bar" will be locked to integer/bool values within their respective @min/@max ranges
- MIDI:
  - API change: [out N midi*] no longer supported. Use [history midi_*_out] instead. Raw MIDI output was causing unpredictable behaviour and could overload the MIDI baud rate too easily. Better handlers for specific MIDI messages have been added instead.
  - Moved midi handling into app-level code to support custom midi handlers as [param]; raw midi handling not generated if not used
  - Added support for a few [param] midi handling types such as midi_cc, midi_bend, midi_vel, midi_clock, etc. 
  - Added support for history-based midi outputs:
    - [history midi_cc1_out], [history midi_cc11_ch2_out], etc.: 0..1 outputs midi CC (default channel 1)
    - [history midi_bend_out], [history midi_bend_ch2_out], etc.: -1..1 outputs midi CC (default channel 1)
    - [history midi_press_out], [history midi_press_ch2_out], etc.: 0..1 outputs midi channel pressure (default channel 1)
    - [history midi_program_out], [history midi_program_ch2_out], etc.: 0..127 outputs midi program change (default channel 1)
    - [history midi_drum36_out] etc.: 0..1 outputs note velocity on channel 10
    - [history midi_vel36_out], [history midi_vel36_ch2_out] etc.: 0..1 outputs note velocity (default channel 1)
    - [history midi_clock_out], [history midi_stop_out], [history midi_start_out], [history midi_continue_out], [history midi_reset_out], [history midi_sense_out]: rising edge gate outputs transport etc. midi messages
  - Added midi note output mechanics:
    - [history midi_note1_pitch_out] and [history midi_note1_vel_out] at minimum required to create a note out. Use note1, note2, noteN etc. to define your polyphony.
    - [history midi_note1_chan_out] and [history midi_note1_press_out] are optional for setting a midi channel (other than channel 1) and polyphonic pressure (aka polyphonic aftertouch)
  - Added [in N midithru], which works the same as [in N midi] (giving you raw bytes scaled by 1/256), but also passes all MIDI received in the hardware input through to the hardware output
  - Fix: ensure program-change midi handling is generated for multi-app even when no apps used midi
- Data/Buffer & SDCard:
  - [data foo_wav 512 2] will try to auto-load from "foo.wav" from the SDcard
  - [data foo 512 2] will try to auto-load from "foo.wav" from the SDcard if there is a "foo.wav" in the same folder as the Max patcher
  - gen~ cannot export patchers with [buffer]; oopsy now warns with messages like "consider replacing [buffer fooz] with [data fooz 10 1]"
- Patcher UI:
  - Added "boost" option to Max and Node.js which boosts CPU frequency from 400 to 480MHz
  - Added "fastmath" option to Max and Node.js interfaces to replace transcendental functions with cheaper and smaller approximations
  - Added blocksize selection to Max and Node.js interfaces
  - Better error handling and messaging, also fixes endless node.script restart bug
  - Template patcher cosmetics
- Code generation:
  - Oopsy checks for gcc/dfu-util in /opt/homebrew or /usr/local and flags an error if neither are found
  - Oopsy reports total binary size
  - Code generation now uses the xoshiro PRNG for [noise], to match MSP's [noise~]
  - Code generation cleanup
- Targets:
  - Support for Noise Engineering's Versio, including all on-panel controls and LEDs, and template patch. 
  - Param view added to Daisy Field; params not tweakable (because of no encoder) but at least visible
  - Added ability to select custom JSON config from within oopsy.maxpat via "browse" button or @target argument
  - Modified config JSON and genlib_daisy.h to create more flexibiity for custom Seed targets
- OLED UI:
  - Removed endless menu rotation so that scrolling to beginning/end is easier

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


