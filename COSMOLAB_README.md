# Cosmolab Support for Oopsy

This document describes the work done to bring Cosmolab board support to oopsy, including the fixes and enhancements needed to make all hardware components work correctly.

## Overview

Cosmolab is a Eurorack/desktop music production board based on the Daisy Seed platform. It features:
- 32 LED-backlit buttons (via PCA9685 I2C LED drivers)
- 8 potentiometers with dual LED feedback (via CD4051 multiplexer)
  - Each knob has 2 LEDs: one underneath for illumination, one above at 12 o'clock position
- 4 CV inputs, 2 CV outputs (Eurorack compatible)
- 1 Trigger input, 1 Trigger output (Eurorack compatible)
- OLED display (64x32)
- MIDI I/O
- Audio I/O (2 channels)

## Version History

### v1.0.0 (2025-12-08)
- ✅ Full Cosmolab hardware support
- ✅ Updated to libDaisy v8.0.0 (from v5.x)
- ✅ Fixed PCA9685 LED driver support
- ✅ Added CD4021SwitchInverted component for hardware flexibility

---

## Technical Implementation

### 1. LibDaisy v8.0.0 Update

**Challenge:** Oopsy was using an older version of libDaisy (v5.x), which had API differences and missing features needed for Cosmolab.

**Solution:** Updated oopsy to use libDaisy v8.0.0, including:
- Updated MIDI UART handling (now uses DMA listen mode)
- Updated hardware initialization patterns
- Compatibility with newer Daisy Seed bootrom

**Files Modified:**
- `source/genlib_daisy.h` - Updated MIDI callbacks and initialization
- Submodule `source/libdaisy` - Updated to v8.0.0

---

### 2. PCA9685 LED Driver Support

**Problem:** The PCA9685 I2C LED drivers (which control all 48 LEDs on Cosmolab) were not being updated in the main loop, causing LEDs to remain off or not respond to software commands.

**Root Cause:** Oopsy's main loop was missing a call to `hardware.LoopProcess()`, which is required for hardware components that need periodic servicing (like I2C-based LED drivers).

**Solution:** Added `hardware.LoopProcess()` call in the main UI update loop.

**Technical Details:**
```cpp
// In genlib_daisy.h, line 958:
// Call hardware loop processing (LED drivers, etc.)
hardware.LoopProcess();
```

This call triggers:
- PCA9685 buffer swap and I2C transmission
- Any other hardware-specific loop processing needed by the board class

**Why This Fix Helps Others:**
- **Any board with PCA9685 LED drivers** will need this fix
- **Any board with I2C peripherals** that need periodic updates
- The fix is board-agnostic and doesn't break existing boards

**Files Modified:**
- `source/genlib_daisy.h` (line 958)

---

### 3. CD4021 Shift Register Button Support

**Problem:** CD4021 shift registers (used to read 32 buttons) needed proper debouncing and edge detection.

**Solution:** Added comprehensive CD4021 support with two polarity options:
- `CD4021Switch` - For hardware with pull-down resistors (standard)
- `CD4021SwitchInverted` - For hardware with pull-up resistors

**Why Two Components?**
Different hardware designs use different pull resistor configurations:
- Pull-down: Button press = HIGH, released = LOW (less common)
- Pull-up: Button press = LOW, released = HIGH (more common, less wiring)

This mirrors libDaisy's own `Switch::POLARITY_NORMAL` and `Switch::POLARITY_INVERTED` pattern.

**Technical Details:**

Normal polarity (`CD4021Switch`):
```cpp
state == 0xFF  → button released (buffer full of 1s)
state != 0xFF  → button pressed
```

Inverted polarity (`CD4021SwitchInverted`):
```cpp
state == 0xFF  → button pressed (hardware inverted, reads as 1s)
state != 0xFF  → button released
```

**Why This Fix Helps Others:**
- CD4021 is a standard shift register for button matrices
- Both pull-up and pull-down configurations are valid design choices
- Having both options avoids forcing specific hardware decisions
- Useful for any project reading 8+ buttons via shift registers

**Files Modified:**
- `source/component_defs.json` - Added CD4021, CD4021Switch, and CD4021SwitchInverted

---

### 4. Board Definition Files

Created comprehensive board definition for Cosmolab hardware:

**`source/cosmolab.json`** - Main board definition with:
- PCA9685 LED driver configuration (3 drivers, 48 LEDs total)
- CD4021 shift register configuration (4 chips, 32 buttons total)
- CD4051 multiplexer configuration (8 potentiometers)
- CV inputs/outputs, switches, MIDI, display

**Component Hierarchy:**
```
cosmolab.json
├── Parents (shared peripherals):
│   ├── i2c (for LED drivers and display)
│   ├── led_driver (PCA9685 x3)
│   ├── pad_shift (CD4021 x4)
│   └── pot_mux (CD4051)
│
└── Components:
    ├── 32 buttons (pada1-padd8) via pad_shift
    ├── 32 LEDs for buttons (led_key_a*-d*) via led_driver
    ├── 8 knobs (knob1-8) via pot_mux
    ├── 16 LEDs for knobs (2 per knob: led_knob_*, led_knob_under_*) via led_driver
    ├── 4 CV inputs
    ├── 2 CV outputs
    ├── 1 Trigger input (Eurorack)
    ├── 1 Trigger output (Eurorack)
    ├── 2 switches
    └── OLED display (64x32)
```

---

## Key Learnings & Best Practices

### 1. Hardware Loop Processing
**Always call `hardware.LoopProcess()` in the main loop** if your board has:
- I2C peripherals (LED drivers, displays, sensors)
- SPI peripherals that need periodic updates
- Any component requiring non-audio-rate servicing

### 2. Component Design Patterns
When adding new component types to oopsy:
- Follow lib Daisy's naming and patterns (e.g., POLARITY_NORMAL/INVERTED)
- Provide both hardware configuration options when reasonable
- Document hardware requirements clearly
- Test with actual hardware, not just simulation

### 3. Multiplexer Best Practices
- CD4051 (analog mux): Great for potentiometers, saves ADC inputs
- CD4021 (shift register): Perfect for button matrices, saves GPIO
- Document select pin mapping clearly
- Consider debouncing for shift register inputs

### 4. Pull Resistor Considerations
- Pull-up: More common, less wiring, better noise immunity
- Pull-down: Less common, explicit about "pressed" state
- Support both in software for maximum hardware flexibility

---

## Files Modified Summary

| File | Purpose | Lines Changed |
|------|---------|---------------|
| `source/genlib_daisy.h` | Added LoopProcess() call | ~1 |
| `source/component_defs.json` | Added CD4021, CD4051, variants | ~100 |
| `source/cosmolab.json` | Board definition | New file |
| `source/libdaisy/` (submodule) | Updated to v8.0.0 | Submodule |

---

## Testing

All features tested on actual Cosmolab hardware:
- ✅ All 32 buttons working with correct LED feedback
- ✅ All 8 potentiometers reading correctly
- ✅ CV inputs/outputs functioning
- ✅ OLED display working
- ✅ MIDI I/O operational
- ✅ Audio I/O clean and stable

---

## Future Enhancements

- [ ] Consider contributing fixes upstream to official oopsy repo
- [ ] Add polarity as a parameter instead of separate components
- [ ] Document more complex board definition patterns
- [ ] Add examples using Cosmolab-specific features

---

## Credits

- **Faselunare** - Cosmolab hardware design
- **Electro-Smith** - Daisy platform and libDaisy
- **oopsy** - Gen~ to Daisy bridge

## References

- [Oopsy GitHub](https://github.com/electro-smith/oopsy)
- [libDaisy Documentation](https://electro-smith.github.io/libDaisy/)
- [Daisy Wiki](https://github.com/electro-smith/DaisyWiki/wiki)
- [PCA9685 Datasheet](https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf)
- [CD4051 Datasheet](https://www.ti.com/product/CD4051B)
- [CD4021 Datasheet](https://www.ti.com/product/CD4021B)
