# Development notes

The Daisy platform has quite a wide range of different features and capabilities. Gen~ code export makes very few assumptions about what it will be used for. In order to glue the two together, a simple drop-in C++ template isn't enough. Instead, Oopsy inspects the gen~ to identify desired features and a wrapper C++ code is generated to bridge the gen~ exported code with the selected Daisy platform features and API. 

This code generation is done using Node.js (the `oopsy.js` script). That way, it can be invoked both within Max (via node4max) or also at the command line:

1. A patcher-based interface for editing gen~ patchers with Daisy features, which can download to the device directly on each edit (if the device is ready to receive code). This uses a Max abstraction to configure the gen~ objects for export and invoke the Node.js script accordingly.
2. A command-line interface for converting gen~-exported cpp code to Daisy binaries (which is used by (1)). This interface was written using Node.js, and thus can be invoked in Max via [node.script]. 

## MIDI

MIDI input can be bursty and sparse. A [param] is not a good match for this, since it can only handle 1 value per block size (even at 1000Hz this might not be enough to capture really bursty midi). Instead we feed the raw midi bytes in via an [in] signal, one byte per sample. 

MIDI bytes are normalized to unit range (mapping 0-256 to 0.0-1.0) so that any accidental patching doesn't burn hardware (electrical or human). Testing locally verified that this reconstructs the integer values exactly. 

If no more midi data is available, the midi signal value will be a negative number (e.g. -0.1).

Example patchers show how to turn this into everything from notes, CCs, wheel, clock, sysex dumps, etc... 

## Memory

Memory allocation for the exported gen~ code happens only when an app is loaded. 

Oopsy uses two pre-allocated blocks of memory, a smaller one in SRAM (around 500Kb) and a larger one in SDRAM (64Mb). Both memory blocks are reset when an app is loaded, so that each gen~ has the full blocks available. Generally SRAM seems to offer faster access, so allocations go to this block if they will fit, which is the case for most gen~ patchers and gen~ operators. Only `data` and `delay` operators with large contents that do not fit in SRAM will use the SDRAM block.

The Daisy offers 128k for code size. Initial testing showed that the baseline for libdaisy and Oopsy is about 50-60k, and each app adds around 5-10k. 