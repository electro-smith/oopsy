# Oopsy: Gen~ to Daisy

Exporting Max Gen patchers for the ElectroSmith Daisy hardware platforms

## Prerequisites

Developing for an embedded platform like Daisy requires a few low-level tools and libraries to be installed first. 

First, follow the steps in the [Daisy wiki](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment) to set up `ARM-Toolchain` and `dfu-util`. 

On OSX you can simply run:

```
brew install make armmbed/formulae/arm-none-eabi-gcc dfu-util
```

Second, install Oopsy and build the Daisy library dependencies:

```
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
- Up to eight cpp files can be mentioned in the arguments; they will all be loaded onto the Daisy, with a simple menu system to switch between them (on Patch/Petal: long encoder press then rotate to select, on Patch/Pod: long SW1 hold and SW2 to select)
- If the `watch` keyword is added to the oopsy.js arguments, it will re-run the process every time any of the cpp files change -- which is handy since gen~ will re-export on every edit.
- For a custom hardware configuration (other than Patch/Field/Petal/Pod) you can specify a JSON file in the arguments.


