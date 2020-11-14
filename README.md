# Oopsy: Gen~ to Daisy

Exporting Max Gen patchers for the ElectroSmith Daisy hardware platforms

## Prerequisites

Developing for an embedded platform like Daisy requires a few low-level tools and libraries to be installed first. 

First, follow the steps in the [Daisy wiki](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment) to set up `ARM-Toolchain` and `dfu-util`. 

On OSX you can simply run:

```
brew install make armmbed/formulae/arm-none-eabi-gcc dfu-util
```

Second, install and build the Daisy library dependencies:

```
git submodule update --init
git pull --recurse-submodules
cd source/DaisyExamples
./rebuild_libs.sh
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

- If the Daisy is plugged in via USB and ready to accept firmware (the two tact switches on the Daisy Seed have been pressed) then the oopsy script will try to upload the binary. 
- Up to eight cpp file can be mentioned in the arguments; they will all be loaded onto the Daisy, with a simple menu system to switch between them. 
- For a custom hardware configuration (other than Patch/Field/Petal/Pod) you can specify a JSON file in the arguments.
- If the `watch` keyword is added to the oopsy.js arguments, it will re-run the process every time any of the cpp files change.

