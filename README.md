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

From the /source folder, run oopsy.js:

