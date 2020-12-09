# Oopsy: Gen~ to Daisy

Exporting Max Gen patchers for the ElectroSmith Daisy hardware platforms.

Each Daisy binary can hold several gen~ patcher "apps", which can be switched between using the encoder (or switches on the DaisyField).

## How gen~ features map to Daisy

Mostly this works by adding an appropriate name to the `in`, `out`, and `param` objects, but many features will also auto-map without special naming.

### Audio

Hardware voltages are mapped to gen~ -1..1

- `in 1`, `in 2` (and `in 3`, `in 4` on the DaisyPatch) are the audio device inputs. 
  - Unavailable inputs re-use existing inputs; e.g. if the hardware has only 2 audio inputs, `in 3` refers to the same data as `in 1`. 
- `out 1`, `out 2` (and `out 3`, `out 4` on the DaisyPatch) are the audio device outputs. 
  - Any hardware outputs that are not explicitly mapped in the patcher will repeat other audio signals (a kind of "normalling"), e.g. if the patcher has no `out 2` then the hardware output 2 will have the same data as hardware output 1. 
  - If the patcher generates *more* audio channels than the hardware supports, these audio signals will be lost.
  
### CV/Gate/Knobs/Switches etc.

- `param cv1`, `param gate2`, `param ctrl3`, `param knob1`, `param key2`, `param knob_delay`, `param switch_enable` etc. will give you the CV and gate inputs, hardware knobs, keys, etc. Use `@min` and `@max` to remap normalized ranges as desired. 
- Oopsy will try to auto-map any other `param` objects to any unused Knob/CV inputs.
- `out <n> cv1`, `out <n> gate2`, etc. will send the signal to corresponding CV or Gate outputs if the device has them. `<n>` just needs to be an output channel number that isn't being used for anything else. 
- CV/gate inputs and outputs are sampled at block rate, which on the default configuration of the Daisy is every 48 samples = 1ms. Trigger/gates shorter than 1ms might be missed. CV inputs might want some smoothing or filtering to eliminate noise/stepping etc. -- some example subpatchers are included for this purpose.
- CV output timing might be negatively affected by OLED and MIDI activity.
- Hardware voltages of 0-5v are mapped to gen~ 0..1, which will be remapped for `param` objects with `@min` and/or `@max` attributes accordingly.  
- Gate outs will be high if the gen~ signal is greater than zero. Gate outs shorter than one blocksize (default 48 samples) might be missed, but example subpatchers are included for automatically extending triggers so that this doesn't happen.
- Note that on the DaisyPatch, the CV and Knob inputs are the same: knobs can offset the input voltage range, but the result is still 0-5v => @min..@max 

### MIDI

- `in <n> midi` will give you a signal packed with any incoming MIDI bytes (see examples on parsing the signal). `<n>` just needs to be an output channel number that isn't being used for anything else.
- `out <n> midi` will let you fill a signal with MIDI data to send to the device MIDI output (see examples on formatting the signal). `<n>` just needs to be an output channel number that isn't being used for anything else.


## Using from within Max

Drop a new `oopsy.patch` / `oopsy.field` etc. as desired object into a Max patch that contains one or more `gen~` objects. Or, use the templates from **File > New from Template > Oopsy_X**. Make sure the Max patch is saved. 

- Every time the Max patch is saved, it will trigger code generation and compilation, and will try to upload to a Daisy device if one is attached.
- You can also send `bang` to the `oopsy` bpatcher to manually trigger this.
- All the gen~ objects in the patcher will be uploaded to the device as "apps" which you can switch between dynamically.
### Multi-App

Oopsy supports flashing Patch, Field, and Petal with multiple apps -- by default it will include every gen~ object in the same patcher as your `oopsy` bpatcher. The method to select apps depends on the target:

- **Patch**: Long-hold the encoder to enter mode selection; rotate until you get to the app menu, and release the encoder. The currently-loaded app is displayed in inverted text. Rotate the encoder to select an app (marked with `>`) and push to load it. 
- **Field**: Long-hold SW1 to enter mode selection; tap SW2 until you get to the app menu, and release SW1. The currently-loaded app is displayed in inverted text. Tap SW2 to select an app (marked with `>`) and push SW1 to load it. 
- **Petal**: Hold the encoder down to go app selection. The currently-loaded app is displayed as a white led, and blue leds indicate available app slots. Rotate the encoder to select the desired app and release to load it.

## Using from the command line via Node.js

If you want to use Oopsy without having Max open, you'll also want to have [Node.js](https://nodejs.org/en/) installed. 

From the /source folder, run `oopsy.js`, telling it what hardware platform to build for, and pointing it to one or more cpp files that have been exported from gen~:

```
# for DaisyPatch:
node oopsy.js patch ../examples/simple.cpp
# for DaisyField:
node oopsy.js field ../examples/simple.cpp
# etc.
```

- If the Daisy is plugged in via USB and ready to accept firmware (the two tact switches on the Daisy Seed have been pressed) then the oopsy script will upload the binary to the hardware (otherwise you'll get the harmless "Error '74") 
- Up to eight cpp files can be mentioned in the arguments; they will all be loaded onto the Daisy, with a simple menu system to switch between them. Use long encoder press to go into mode selection, rotate until you get the app menu, and release. Now rotate the encoder to select the app, and short press the encoder to load it. (On the Patch, use SW1 long press to go into mode selection, SW2 to swich mode until you get the app menu, release SW1; then press SW2 to select app, and SW1 to load it.)
- If the `watch` keyword is added to the oopsy.js arguments, it will re-run the process every time any of the cpp files change -- which is handy since gen~ will re-export on every edit.
- For a custom hardware configuration (other than Patch/Field/Petal/Pod) you can specify a JSON file in the arguments.

## Installing

See the instructions [on the wiki page](https://github.com/electro-smith/DaisyWiki/wiki/1e.-Getting-Started-With-Oopsy-(Gen~-Integration))