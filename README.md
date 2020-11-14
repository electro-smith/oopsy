# Oopsy: Gen~ to Daisy

Exporting Max Gen patchers for the ElectroSmith Daisy hardware platforms.

Each Daisy binary can hold several gen~ patcher "apps", which can be switched between using the encoder (or switches on the DaisyField).

## How gen~ features map to Daisy

Mostly this works by adding an appropriate name to the `in`, `out`, and `param` objects.

### Audio

Hardware voltages are mapped to gen~ -1..1

- `in 1`, `in 2` (and `in 3`, `in 4` on the DaisyPatch) are the audio device inputs. 
  - Unavailable inputs re-use existing inputs; e.g. if the hardware has only 2 audio inputs, `in 3` refers to the same data as `in 1`. 
- `out 1`, `out 2` (and `out 3`, `out 4` on the DaisyPatch) are the audio device outputs. 
  - Any hardware outputs that are not explicitly mapped in the patcher will repeat other audio signals (a kind of "normalling"), e.g. if the patcher has no `out 2` then the hardware output 2 will have the same data as hardware output 1. 
  - If the patcher generates *more* audio channels than the hardware supports, these audio signals will be lost.
  
### CV/Gate/Knobs/Switches etc.

- `param cv1`, `param gate2`, `param knob1`, `param key2`  etc. will give you the CV and gate inputs, hardware knobs, keys, etc. Use `@min` and `@max` to remap the normalized ranges. Use `@min` and `@max` to remap normalized ranges as desired. 
- Oopsy will try to auto-map any other `param` objects to any unused Knob/CV inputs.
- `out <n> cv1`, `out <n> gate2`, etc. will send the signal to corresponding CV or Gate outputs if the device has them. `<n>` just needs to be an output channel number that isn't being used for anything else. 
- CV/gate inputs and outputs are sampled at block rate, which on the default configuration of the Daisy is every 48 samples = 1ms. CV/gate output timing might be negatively affected by OLED and MIDI activity.
- Hardware voltages of 0-5v are mapped to gen~ 0..1, which will be remapped for `param` objects with `@min` and/or `@max` attributes accordingly.  
- Gate outs will be high if the gen~ signal is greater than zero.
- Note that on the DaisyPatch, the CV and Knob inputs are the same: knobs can offset the input voltage range, but the result is still 0-5v => 0..1. 

### MIDI

- `in <n> midi` will give you a signal packed with any incoming MIDI bytes (see examples on parsing the signal). `<n>` just needs to be an output channel number that isn't being used for anything else.
- `out <n> midi` will let you fill a signal with MIDI data to send to the device MIDI output (see examples on formatting the signal). `<n>` just needs to be an output channel number that isn't being used for anything else.



## Installing

Developing for an embedded platform like Daisy requires a few low-level tools and libraries to be installed first. First, follow the steps in the [Daisy wiki](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment) to set up `ARM-Toolchain` and `dfu-util`. 

On OSX you can simply run:

```
brew install make armmbed/formulae/arm-none-eabi-gcc dfu-util
```

Second, install Oopsy and build the Daisy library dependencies:

```
# first open a console in your Documents/Max 8/Packages folder, then:
git clone https://github.com/grrrwaaa/oopsy.git
cd oopsy
git submodule update --init
git pull --recurse-submodules
cd source/DaisyExamples
./rebuild_libs.sh
cd ..
```

If you want to use Oopsy without having Max open, you'll also want to have [Node.js](https://nodejs.org/en/) installed. From within Max you'll need the Node4Max package instead.

## Using from the command line via Node.js

From the /source folder, run oopsy.js, telling it what hardware platform to build for, and pointing it to one or more cpp files that have been exported from gen~:

```
# for DaisyPatch:
node oopsy.js patch ../examples/simple.cpp
# for DaisyField:
node oopsy.js field ../examples/simple.cpp
# etc.
```

- If the Daisy is plugged in via USB and ready to accept firmware (the two tact switches on the Daisy Seed have been pressed) then the oopsy script will upload the binary to the hardware (otherwise you'll get the harmless "Error '74") 
- Up to eight cpp files can be mentioned in the arguments; they will all be loaded onto the Daisy, with a simple menu system to switch between them. Use long encoder press then rotate to select (on Patch: long hold SW1 and press SW2 to select).
- If the `watch` keyword is added to the oopsy.js arguments, it will re-run the process every time any of the cpp files change -- which is handy since gen~ will re-export on every edit.
- For a custom hardware configuration (other than Patch/Field/Petal/Pod) you can specify a JSON file in the arguments.

## Using from within Max

Drop a new `oopsy` object into a Max patch that contains one or more `gen~` objects. Make sure the Max patch is saved. 

- Every time the Max patch is saved, it will trigger code generation and compilation, and will try to upload to a Daisy device if one is attached.
- You can also send `bang` to the `oopsy` object to manually trigger this.
- You can configure the export target with `oopsy @target field` etc.

Currently the progress is spewed to the Max console -- hopefully we can replace this with a nice bpatcher view at some point.

If you see "Error 74" you can ignore it.
