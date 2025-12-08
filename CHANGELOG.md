# Changelog - Cosmolab Oopsy Support

All notable changes to Cosmolab board support in oopsy.

## [1.0.0] - 2025-12-08

### Added
- **Full Cosmolab hardware support**
  - 32 LED-backlit buttons via CD4021 shift registers
  - 32 button LEDs via PCA9685 I2C drivers
  - 8 potentiometers via CD4051 multiplexer (already supported in oopsy)
  - 16 knob LEDs (2 per knob: illumination + position indicator) via PCA9685
  - 4 CV inputs, 2 CV outputs (Eurorack compatible)
  - 1 Trigger input, 1 Trigger output (Eurorack compatible)
  - MIDI I/O, OLED display (64x32)

- **Updated to libDaisy v8.0.0 (from v5.x)**
  - Improved MIDI handling with DMA listen mode
  - Better hardware compatibility
  - Bug fixes and performance improvements

- **Fixed PCA9685 LED driver support**
  - Added `hardware.LoopProcess()` call in main loop (line 958 of genlib_daisy.h)
  - Enables proper I2C LED driver updates
  - **Benefits all boards using PCA9685 or other I2C peripherals**

- **Added CD4021 shift register support**
  - New `CD4021` parent component for shift register chains
  - New `CD4021Switch` for normal polarity (pull-down resistors)
  - New `CD4021SwitchInverted` for inverted polarity (pull-up resistors)
  - Includes debouncing and edge detection
  - **Supports both common hardware configurations**

### Changed
- Updated `genlib_daisy.h` to support boards with I2C peripherals
- Updated MIDI UART handling for libDaisy v8.0.0 compatibility

### Technical Notes

**LoopProcess() Fix:**
The addition of `hardware.LoopProcess()` was essential for boards with peripherals requiring periodic servicing. This is not oopsy-specific—it's a general requirement for boards using:
- I2C LED drivers (PCA9685, etc.)
- I2C sensors or displays
- Any hardware requiring non-audio-rate updates

**Component Polarity Support:**
Following libDaisy's `Switch::POLARITY_NORMAL/INVERTED` pattern, we added both normal and inverted variants for CD4021 shift registers to support different pull resistor configurations without forcing hardware redesigns.

---

## Testing

Tested on actual Cosmolab hardware:
- ✅ All 32 buttons
- ✅ All 48 LEDs  
- ✅ All 8 potentiometers
- ✅ CV I/O
- ✅ MIDI I/O
- ✅ Display
- ✅ Audio I/O

---

## See Also

- `COSMOLAB_README.md` - Detailed technical documentation
- `source/cosmolab.json` - Board definition
- `source/COSMOLAB_BOARDS_README.md` - Board selection guide
