#!/usr/bin/env node

/*
	Generates and compiles wrapper code for gen~ export to Daisy hardware

	Oopsy was authored by Graham Wakefield in 2020-2021.

	Main entry-point is `run(...args)`

	Args are a command-line style argument list, see `help` below
	At minimum they should include at least one path to a .cpp file exported from gen~

	`run`:
	- parses these args
	- configures a target (JSON definition, #defines, samplerate, etc)
	- visits each .cpp file via `analyze_cpp()` to define "app" data structures
	- further configuration according to cpp analysis
	- visits each "app" via `generate_app()` to prepare data for code generation
	- generates a .cpp file according to the apps and options
	- invokes arm-gcc to compile .cpp to binary, then dfu-util to upload to daisy

	`analyze_cpp`:
	- defines a "gen" data structure representing features in the gen patcher

	`generate_app`:
	- configure an "daisy" object representing features in the hardware
	- configure a "gen" object representing features in the .cpp patch

	The general idea here is that there is a list of named "nodes" making a graph
	most nodes are sources, 
		they may have a list of `to` destinations
		they may have a `src` field naming another node they map from
	some nodes are sinks, and have a list of 'from' sources

*/
const fs = require("fs"),
	path = require("path"),
	os = require("os"),
	assert = require("assert");
const {exec, execSync, spawn} = require("child_process");

const json2daisy = require(path.join(__dirname, "json2daisy.js"));
const daisy_glue = require(path.join(__dirname, "daisy_glue.js"));

// returns the path `str` with posix path formatting:
function posixify_path(str) {
	return str.split(path.sep).join(path.posix.sep);
}

// returns str with any $<key> replaced by data[key]
function interpolate(str, data) {
	return str.replace(/\$<([^>]+)>/gm, (s, key) => data[key])
}

const template = (template, vars = {}) => {
	const handler = new Function(
	  "vars",
	  [
		"const f = (" + Object.keys(vars).join(", ") + ")=>",
		"`" + template + "`",
		"return f(...Object.values(vars))"
	  ].join("\n")
	);
	// console.log(Object.keys(vars));
	// console.log(handler);
	return handler(vars);
};

// prints a number as a C-style float:
function asCppNumber(n, type="float") {
	let s = (+n).toString();
	if (type == "int" || type == "uint8_t" || type == "bool") {
		return Math.trunc(n).toString()
	} else {
		// add point if needed:
		if (s.includes("e")) {
			return s;
		} else if (s.includes(".")) {
			return s + "f";
		} else {
			return s + ".f";
		}
	}
}

function node_scale(node) {
	if (node.permit_scale == false)
		return `${node.varname} = (${node.type})(${node.src});
		`
	else
		return `${node.varname} = (${node.type})(${node.src}*${asCppNumber(node.range)} + ${asCppNumber(node.min + (node.type == "int" || node.type == "bool" ? 0.5 : 0))});
		`
}

let build_tools_path;
let has_dfu_util;
function checkBuildEnvironment() {
	// check for available build tools:
	if (os.platform == "win32") {
		has_dfu_util = false;
		try {
			execSync("arm-none-eabi-gcc --version")
			execSync("dfu-util --version")
			// assume true for now, until we know how to test for it:
			has_dfu_util = true;
		} catch (e) {
			console.warn(`oopsy can't find the ARM GCC build tools, will not be able to upload binary to the Daisy. Please check https://github.com/electro-smith/DaisyWiki/wiki/1e.-Getting-Started-With-Oopsy-(Gen~-Integration) for installation instructions.`)
			process.exit(-1);
		}
		
	} else {
		// OSX:
		let locations = ["/opt/homebrew/bin", "/usr/local/bin"]
		for (loc of locations) {
			if (fs.existsSync(`${loc}/arm-none-eabi-gcc`)) {
				build_tools_path = loc;
				console.log(`using build tools found in ${build_tools_path}`);
				break;
			}
		}
		if (!build_tools_path) {
			console.log("oopsy can't find an ARM-GCC toolchain. Please check https://github.com/electro-smith/DaisyWiki/wiki/1e.-Getting-Started-With-Oopsy-(Gen~-Integration) for installation instructions.")
			process.exit(-1);
		}
		if (fs.existsSync(`${build_tools_path}/dfu-util`)) {
			has_dfu_util = true; 
		} else {
			console.warn(`oopsy can't find the dfu-util binary in ${build_tools_path}, will not be able to upload binary to the Daisy. Please check https://github.com/electro-smith/DaisyWiki/wiki/1e.-Getting-Started-With-Oopsy-(Gen~-Integration) for installation instructions.`)
		}
	}
}

const help = `
<[cmds]> <target> <[options]> <[cpps]> <watch>

cmds: 	up/upload = (default) generate & upload
	  	gen/generate = generate only

target: path to a JSON for the hardware config, 
		or simply "patch", "patch_sm", "field", "petal", "pod" etc. 
		Defaults to "daisy.patch.json"

32kHz, 48kHz, "96kHz" will set the sampling rate of the binary

block1, block2, etc. up to block256 will set the block size

fastmath will replace some expensive math operations with faster approximations

boost will increase the CPU from 400Mhz to 480Mhz

nooled will disable code generration for OLED (it will be blank)

cpps: 	paths to the gen~ exported cpp files
		first item will be the default app
		  
watch:	script will not terminate
		actions will be re-run each time any of the cpp files are modified
`

const component_defs = {
	Switch: {
		typename: "daisy::Switch",
		pin: "a",
		type: "daisy::Switch::TYPE_MOMENTARY",
		polarity: "daisy::Switch::POLARITY_INVERTED",
		pull: "daisy::Switch::PULL_UP",
		process: "${name}.Debounce();",
		updaterate: "${name}.SetUpdateRate(som.AudioCallbackRate());",
		mapping: [
			{ name: "${name}", get: "(hardware.${name}.Pressed()?1.f:0.f)", range: [0, 1] },
			{
				name: "${name}_rise",
				get: "(hardware.${name}.RisingEdge()?1.f:0.f)",
				range: [0, 1]
			},
			{
				name: "${name}_fall",
				get: "(hardware.${name}.FallingEdge()?1.f:0.f)",
				range: [0, 1]
			},
			{
				name: "${name}_seconds",
				get: "(hardware.${name}.TimeHeldMs()*0.001f)",
				range: null
			}
	  	]
	},
	Switch3: {
		typename: "daisy::Switch3",
		pin: "a,b",
		mapping: [
			{ name: "${name}", get: "(hardware.${name}.Read()*0.5f+0.5f)", range: [0, 2] }
		]
	},
	Encoder: {
		typename: "daisy::Encoder",
		pin: "a,b,click",
		process: "${name}.Debounce();",
		updaterate: "${name}.SetUpdateRate(som.AudioCallbackRate());",
		mapping: [
			{
				name: "${name}",
				get: "hardware.${name}.Increment()",
				range: [-1, 1]
			},
			{
				name: "${name}_press",
				get: "(hardware.${name}.Pressed()?1.f:0.f)",
				range: [0, 1]
			},
			{
				name: "${name}_rise",
				get: "(hardware.${name}.RisingEdge()?1.f:0.f)",
				range: [0, 1]
			},
			{
				name: "${name}_fall",
				get: "(hardware.${name}.FallingEdge()?1.f:0.f)",
				range: [0, 1]
			},
			{
				name: "${name}_seconds",
				get: "(hardware.${name}.TimeHeldMs()*0.001f)",
				range: null
			}
		]
	},
	GateIn: {
		typename: "daisy::GateIn",
		pin: "a",
		mapping: [
			{ name: "${name}", get: "(hardware.${name}.State()?1.f:0.f)", range: [0, 1] },
			{ name: "${name}_trig", get: "(hardware.${name}.Trig()?1.f:0.f)", range: [0, 1] }
		]
	},
	AnalogControl: {
		typename: "daisy::AnalogControl",
		pin: "a",
		flip: false,
		invert: false,
		slew: "1.0/som.AudioCallbackRate()",
		process: "${name}.Process();",
		updaterate: "${name}.SetSampleRate(som.AudioCallbackRate());",
		mapping: [{ name: "${name}", get: "(hardware.${name}.Value())", range: [0, 1] }]
	},
	Led: {
		typename: "daisy::Led",
		pin: "a",
		invert: true,
		postprocess: "${name}.Update();",
		mapping: [{ name: "${name}", set: "hardware.${name}.Set($<name>);" }]
	},
	RgbLed: {
		typename: "daisy::RgbLed",
		pin: "r,g,b",
		invert: true,
		postprocess: "${name}.Update();",
		mapping: [
			{ name: "${name}_red", set: "hardware.${name}.SetRed($<name>);" },
			{ name: "${name}_green", set: "hardware.${name}.SetGreen($<name>);" },
			{ name: "${name}_blue", set: "hardware.${name}.SetBlue($<name>);" },
			{ name: "${name}", set: "hardware.${name}.Set(clamp(-$<name>, 0.f, 1.f), 0.f, clamp($<name>, 0.f, 1.f));" },
			{ name: "${name}_white", set: "hardware.${name}.Set($<name>,$<name>,$<name>);" }
		]
	},
	GateOut: {
		typename: "daisy::dsy_gpio",
		pin: "a",
		mode: "DSY_GPIO_MODE_OUTPUT_PP",
		pull: "DSY_GPIO_NOPULL",
		mapping: [
			{ name: "${name}", set: "dsy_gpio_write(&hardware.${name}, $<name> } 0.f);" }
		]
	},
	CVOuts: {
		typename: "daisy::DacHandle::Config",
		pin: "",
		bitdepth: "daisy::DacHandle::BitDepth::BITS_12",
		buff_state: "daisy::DacHandle::BufferState::ENABLED",
		mode: "daisy::DacHandle::Mode::POLLING",
		channel: "daisy::DacHandle::Channel::BOTH",
		mapping: [
			{
				name: "${name}1",
				set: "hardware.som.dac.WriteValue(daisy::DacHandle::Channel::ONE, $<name> * 4095);",
				where: "main"
			},
			{
				name: "${name}2",
				set: "hardware.som.dac.WriteValue(daisy::DacHandle::Channel::TWO, $<name> * 4095);",
				where: "main"
			}
		]
	}
};

let watchers = []

// the script can be invoked directly as a command-line program,
// or it can be embedded as a node module
if (require.main === module) {
	run(...process.argv.slice(2))
} else {
	module.exports = run;
}

function run() {
	let args = [...arguments]
	let action = "upload"
	let target
	let target_path
	let watch = false
	let cpps = []
	let samplerate = 48
	let blocksize = 48
	let options = {}

	checkBuildEnvironment();
	
	if (args.length == 0) {
		console.log(help)
		return;
	}

	args.forEach(arg => {
		switch (arg) {
			case "help": {console.log(help); process.exit(0);} break;
			case "generate":
			case "gen": action="generate"; break;
			case "upload":
			case "up": action="upload"; break;

			case "versio": target = arg; break;
			case "bluemchen": target_path = path.join(__dirname, "bluemchen.json"); break;
			case "nehcmeulb": target_path = path.join(__dirname, "nehcmeulb.json"); break;
			case "pod": target_path = path.join(__dirname, "pod.json"); break;
			case "patch_init": target_path = path.join(__dirname, "patch_init.json"); break;
			case "field": target_path = path.join(__dirname, "field.json"); break;
			case "petal": target_path = path.join(__dirname, "petal.json"); break;
			case "patch": target_path = path.join(__dirname, "patch.json"); break;

			case "watch": watch=true; break;

			case "96kHz": 
			case "48kHz": 
			case "32kHz": samplerate = +(arg.match(/(\d+)kHz/)[1]); break; 

			case "block1":
			case "block2":
			case "block4":
			case "block6":
			case "block8":
			case "block12":
			case "block16":
			case "block24":
			case "block32":
			case "block48": 
			case "block64": 
			case "block96": 
			case "block128":
			case "block256": blocksize = +(arg.match(/block(\d+)/)[1]); break;

			case "writejson":
			case "nooled": 
			case "boost": 
			case "fastmath": options[arg] = true; break;

			default: {
				// assume anything else is a file path:
				if (!fs.existsSync(arg)) {
					console.log(`oopsy error: ${arg} is not a recognized argument or a path that does not exist`)
					process.exit(-1)
				}
				if (fs.lstatSync(arg).isDirectory()) {
					// add a whole folder full of cpps:
					cpps = cpps.concat(fs.readdirSync(arg)
						.filter(s => path.parse(s).ext == ".cpp") 
						.map(s => path.join(arg, s))
					)
				} else {	
					let p = path.parse(arg);
					switch(p.ext) {
						case ".json": {target_path = arg; target = ""}; break;
						case ".cpp": cpps.push(arg); break;
						// case ".gendsp":
						// case ".maxpat":
						// case ".maxhelp": {pat_path = arg}; break;
						default: {
							console.warn("unexpected input", arg);
						}
					}
				}
			}
		}
	});

	// remove duplicates:
	cpps = cpps.reduce(function (acc, s) {
		if (acc.indexOf(s) === -1) acc.push(s)
		return acc
	}, []);
	cpps.sort((a,b)=>{
		return path.basename(a) < path.basename(b) ? -1 : 0;
	})

	let OOPSY_TARGET_SEED = 0

	let valid_soms = ['seed', 'patch_sm'];
	let som = 'seed';

	let old_json = false;

	// configure target:
	if (!target && !target_path) target = "patch";
	if (!target_path) {
		target_path = path.join(__dirname, `daisy.${target}.json`);
		old_json = true;
	} else {
		// TODO -- should a seed target even really exist? Custom boards
		// (and even prototypes / breadboards) should really be defined with a JSON file
		// OOPSY_TARGET_SEED = 1
		target = path.parse(target_path).name.replace(".", "_")
		// som_match = path.parse(target_path).name.match(/([A-Za-z_0-9\-]+)\./)
		// assert(som_match != null, `Daisy SOM undefined. Provide the SOM as in the following: "som.MyBoard.json"`);
		// assert(valid_soms.includes(som_match[1]), `unkown SOM ${som_match[1]}. Valid SOMs: ${valid_soms.join(', ')}`);
		// som = som_match[1];
	}
	console.log(`Target ${target} configured in path ${target_path}`)
	assert(fs.existsSync(target_path), `couldn't find target configuration file ${target_path}`);
	const hardware = JSON.parse(fs.readFileSync(target_path, "utf8"));
	hardware.max_apps = hardware.max_apps || 1
	// hardware.som = som;

	// Ensure som is valid
	assert(valid_soms.includes(hardware.som), `unkown SOM ${hardware.som}. Valid SOMs: ${valid_soms.join(', ')}`);

	// The following is compatibility code, so that the new JSON structure will generate the old JSON structure
	// At the point that the old one can be retired (because e.g. Patch, Petal etc can be defined in the new format)
	// this script should be revised to eliminate the old workflow
	{
		hardware.som = hardware.som || "seed";
		hardware.inputs = hardware.inputs || {}
		hardware.outputs = hardware.outputs || {}
		hardware.datahandlers = hardware.datahandlers || {}
		hardware.labels = hardware.labels || {
			"params": {},
			"outs": {},
			"datas": {}
		}
		hardware.inserts = hardware.inserts || []
		hardware.defines = hardware.defines || {}
		hardware.struct = "";

		let tempname = hardware.name;
		hardware.name = '';
		let board_info = json2daisy.generate_header(hardware);
		hardware.name = tempname;

		hardware.struct = board_info.header;
		hardware.components = board_info.components;
		hardware.aliases = board_info.aliases;

		if (hardware.components) {
			// generate IO
			for (let comp in hardware.components) {
				let component = hardware.components[comp];
				// meta-elements are handled separately
				if (component.meta) {
					
				} else {
					// else it is available for gen mapping:

					for (let mapping of component.mapping) {
						// let name = template(mapping.name, component);
						component.class_name = 'hardware';
						component.name_upper = component.name.toUpperCase();
						let name = json2daisy.format_map(mapping.name, component);
						component.value = name.toLowerCase();
						if (mapping.get) {
							// an input
							hardware.inputs[name] = {
								code: json2daisy.format_map(mapping.get, component),
								automap: component.automap && name == component.name,
								range: mapping.range,
								where: mapping.where,
								permit_scale: mapping.permit_scale != undefined ? mapping.permit_scale : true
							}
							hardware.labels.params[name] = name
						}
						if (mapping.set) {
							// an output
							hardware.outputs[name] = {
								code: json2daisy.format_map(mapping.set, component),
								automap: component.automap && name == component.name,
								range: mapping.range,
								where: mapping.where || "audio"
							}
							hardware.labels.outs[name] = name
						}
					}
				}
			}
		}

		if (old_json)
			hardware.defines.OOPSY_OLD_JSON = 1

		for (let alias in hardware.aliases) {
			let map = hardware.aliases[alias]
			if (hardware.labels.params[map]) hardware.labels.params[alias] = map
			if (hardware.labels.outs[map]) hardware.labels.outs[alias] = map
			if (hardware.labels.datas[map]) hardware.labels.datas[alias] = map
		}

		if (OOPSY_TARGET_SEED) hardware.defines.OOPSY_TARGET_SEED = OOPSY_TARGET_SEED
	}

	// consolidate hardware definition:
	hardware.samplerate = samplerate
	if (hardware.audio)
		hardware.defines.OOPSY_IO_COUNT = hardware.audio.channels || 2;
	else if (!hardware.defines.OOPSY_IO_COUNT)
		hardware.defines.OOPSY_IO_COUNT = 2;
	if (!hardware.max_apps) hardware.max_apps = 1;

	hardware.defines.OOPSY_SAMPLERATE = samplerate * 1000
	hardware.defines.OOPSY_BLOCK_SIZE = blocksize
	hardware.defines.OOPSY_BLOCK_RATE = hardware.defines.OOPSY_SAMPLERATE / blocksize

	//hardware.defines.OOPSY_USE_LOGGING = 1
	//hardware.defines.OOPSY_USE_USB_SERIAL_INPUT = 1

	// verify and analyze cpps:
	assert(cpps.length > 0, "an argument specifying the path to at least one gen~ exported cpp file is required");
	if (hardware.max_apps && cpps.length > hardware.max_apps) {
		console.log(`this target does not support more than ${hardware.max_apps} apps`)
		cpps.length = hardware.max_apps
	}
	let apps = cpps.map(cpp_path => {
		assert(fs.existsSync(cpp_path), `couldn't find source C++ file ${cpp_path}`);
		return {
			path: cpp_path,
			patch: analyze_cpp(fs.readFileSync(cpp_path, "utf8"), hardware, cpp_path)
		}
	})
	let build_name = apps.map(v=>v.patch.name).join("_")

	// configure build path:
	const build_path = path.join(__dirname, `build_${build_name}_${target}`)
	console.log(`build_path ${path.join(build_path, 'build')}`)
	console.log(`Building to ${build_path}`)
	// ensure build path exists:
	fs.mkdirSync(build_path, {recursive: true});

	let config = {
		build_name: build_name,
		build_path: build_path,
		target: target,
		hardware: hardware,
		apps: apps,
	}

	let defines = hardware.defines;

	if (defines.OOPSY_TARGET_HAS_MIDI_INPUT || defines.OOPSY_TARGET_HAS_MIDI_OUTPUT) {
		defines.OOPSY_TARGET_USES_MIDI_UART = 1
	}

	if (apps.length > 1) {
		defines.OOPSY_MULTI_APP = 1
		// generate midi-handling code for any multi-app on a midi-enabled platform
		// so that program-change messages for apps will work:
		if (hardware.defines.OOPSY_TARGET_HAS_MIDI_INPUT) {
			hardware.defines.OOPSY_TARGET_USES_MIDI_UART = 1
		}
	}
	if (options.nooled && defines.OOPSY_TARGET_HAS_OLED) {
		delete defines.OOPSY_TARGET_HAS_OLED;
	}
	if (defines.OOPSY_TARGET_HAS_OLED && defines.OOPSY_HAS_PARAM_VIEW) {
		defines.OOPSY_CAN_PARAM_TWEAK = 1
	}
	if (defines.OOPSY_TARGET_HAS_OLED) {
		if (!defines.OOPSY_OLED_DISPLAY_WIDTH) defines.OOPSY_OLED_DISPLAY_WIDTH = 128
		if (!defines.OOPSY_OLED_DISPLAY_HEIGHT) defines.OOPSY_OLED_DISPLAY_HEIGHT = 64
	}
	if (options.fastmath) {
		hardware.defines.GENLIB_USE_FASTMATH = 1;
	}
	if (hardware.som == 'patch_sm') {
		hardware.defines.OOPSY_SOM_PATCH_SM = 1;
	}

	const makefile_path = path.join(build_path, `Makefile`)
	const bin_path = path.join(build_path, "build", build_name+".bin");
	const maincpp_path = path.join(build_path, `${build_name}_${target}.cpp`);
	fs.writeFileSync(makefile_path, `
# Project Name
TARGET = ${build_name}
# Sources -- note, won't work with paths with spaces
CPP_SOURCES = ${posixify_path(path.relative(build_path, maincpp_path).replace(" ", "\\ "))}
# Library Locations
LIBDAISY_DIR = ${(posixify_path(path.relative(build_path, path.join(__dirname, "libdaisy"))).replace(" ", "\\ "))}
${hardware.defines.OOPSY_TARGET_USES_SDMMC ? `USE_FATFS = 1`:``}
# Optimize (i.e. CFLAGS += -O3):
OPT = -O3
# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
# Include the gen_dsp files
CFLAGS+=-I"${posixify_path(path.relative(build_path, path.join(__dirname, "gen_dsp")))}"
# Silence irritating warnings:
CFLAGS+=-O3 -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-variable
CPPFLAGS+=-O3 -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-variable

`, "utf-8");

	console.log(`Will ${action} from ${cpps.join(", ")} by writing to:`)
	console.log(`\t${maincpp_path}`)
	console.log(`\t${makefile_path}`)
	console.log(`\t${bin_path}`)
	
	// add watcher
	if (watch && watchers.length < 1) {
		watchers = cpps.map(cpp_path => fs.watch(cpp_path, (event, filepath)=>{
			run(...args);
		}))
	}

	apps.map(app => {
		generate_app(app, hardware, target, config);
		return app;
	})

	// store for debugging:
	//if (options.writejson) fs.writeFileSync(path.join(build_path, `${build_name}_${target}.json`), JSON.stringify(config,null,"  "),"utf8");

	const cppcode = `
/* 

This code was generated by Oopsy (https://github.com/electro-smith/oopsy) on ${new Date().toString()}

Oopsy was authored in 2020-2021 by Graham Wakefield.  Copyright 2021 Electrosmith, Corp. and Graham Wakefield.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
	For details of the licensing terms of code exported from gen~ see https://support.cycling74.com/hc/en-us/articles/360050779193-Gen-Code-Export-Licensing-FAQ
*/
${Object.keys(hardware.defines).map(k=>`
#define ${k} (${hardware.defines[k]})`).join("")}
${hardware.struct}

using json2daisy::Daisy;

${hardware.inserts.filter(o => o.where == "header").map(o => o.code).join("\n")}
#include "../genlib_daisy.h"
#include "../genlib_daisy.cpp"

${apps.map(app => `#include "${posixify_path(path.relative(build_path, app.path))}"`).join("\n")}
${apps.map(app => app.cpp.struct).join("\n")}

// store apps in a union to re-use memory, since only one app is active at once:
union {
	${apps.map(app => app.cpp.union).join("\n\t")}
} apps;

oopsy::AppDef appdefs[] = {
	${apps.map(app => app.cpp.appdef).join("\n\t")}
};

int main(void) {
  oopsy::daisy.hardware.Init(${hardware.som == 'seed' ? options.boost|false : ''}); 
	oopsy::daisy.hardware.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_${hardware.samplerate}KHZ);
	oopsy::daisy.hardware.SetAudioBlockSize(${hardware.defines.OOPSY_BLOCK_SIZE});
	${hardware.inserts.filter(o => o.where == "init").map(o => o.code).join("\n\t")}
	// insert custom hardware initialization here
	return oopsy::daisy.run(appdefs, ${apps.length});
}
`
	fs.writeFileSync(maincpp_path, cppcode, "utf-8");	

	console.log("oopsy generated code")

	// now try to make:
	try {
		console.log("oopsy compiling...")
		if (os.platform() == "win32") {
			// Don't use `make clean`, as `rm` is not default on Windows
			// /Q suppresses the Y/n prompt
			console.log(execSync("del /Q build", { cwd: build_path }).toString())
			// Gather up make output to run command per line as child process
			let build_cmd = execSync("make -n", { cwd: build_path }).toString().split(os.EOL)
			build_cmd.forEach(line => {
				// Silently execute the commands line-by-line.
				if (line.length > 0)
					execSync(line, { cwd: build_path }).toString()
			})
			console.log(`oopsy created binary ${Math.ceil(fs.statSync(posixify_path(path.join(build_path, "build", build_name+".bin")))["size"]/1024)}KB`)
			// if successful, try to upload to hardware:
			if (has_dfu_util && action=="upload") {
				console.log("oopsy flashing...")
				
				exec(`make program-dfu`, { cwd: build_path }, (err, stdout, stderr)=>{
					console.log("stdout", stdout)
					console.log("stderr", stderr)
					if (err) {
						if (err.message.includes("No DFU capable USB device available")) {
							console.log("oopsy daisy not ready on USB")
							return;
						} else if (stdout.includes("File downloaded successfully")) {
							console.log("oopsy flashed")
						} else {
							console.log("oopsy dfu error")
							console.log(err.message);
							return;
						}
					} else if (stderr) {
						console.log("oopsy dfu error")
						console.log(stderr);
						return;
					}
				});
			}
		} else {
			exec(`export PATH=$PATH:${build_tools_path} && make clean && make`, { cwd: build_path }, (err, stdout, stderr)=>{
				if (err) {
					console.log("oopsy compiler error")
					console.log(err);
					console.log(stderr);
					return;
				}
				console.log(`oopsy created binary ${Math.ceil(fs.statSync(posixify_path(path.join(build_path, "build", build_name+".bin")))["size"]/1024)}KB`)
				// if successful, try to upload to hardware:
				if (has_dfu_util && action=="upload") {
					console.log("oopsy flashing...")
					exec(`export PATH=$PATH:${build_tools_path} && make program-dfu`, { cwd: build_path }, (err, stdout, stderr)=>{
						console.log("stdout", stdout)
						console.log("stderr", stderr)
						if (err) {
							if (err.message.includes("No DFU capable USB device available")) {
								console.log("oopsy daisy not ready on USB")
								return;
							} else if (stdout.includes("File downloaded successfully")) {
								console.log("oopsy flashed")
							} else {
								console.log("oopsy dfu error")
								console.log(err.message);
								return;
							}
						} else if (stderr) {
							console.log("oopsy dfu error")
							console.log(stderr);
							return;
						}
					});
				}
			});
		}
	} catch (e) {
		// errors from make here
		console.log("oopsy build failed", e);
	}
}

function analyze_cpp(cpp, hardware, cpp_path) {

	// helper function to parse initializers:
	function constexpr(s) {
		return eval(s
			// remove any (int) or (t_sample) casts
			.replace("(int)", "")
			.replace("(t_sample)", "")
			// then replace any samplerate or vectorsize constants
			.replace("samplerate", hardware.defines.OOPSY_SAMPLERATE)
			.replace("vectorsize", hardware.defines.OOPSY_BLOCK_SIZE)
			// remove extraneous whitespace
			.trim()
			// then compute the result via eval
		)
	}

	let gen = {
		name: /namespace\s+(\w+)\s+{/gm.exec(cpp)[1],
		ins: (/gen_kernel_innames\[\]\s=\s{\s([^}]*)/g).exec(cpp)[1].split(",").map(s => s.replace(/"/g, "").trim()),
		outs: (/gen_kernel_outnames\[\]\s=\s{\s([^}]*)/g).exec(cpp)[1].split(",").map(s => s.replace(/"/g, "").trim()),
		params: [],
		datas: [],

		// search for history outs:
		// i.e. any history with "_out" on its name
		histories: (cpp.match(/t_sample\s+(m_([\w]+)_out_\d+);/gm) || []).map(s=>{
			
			const match = /t_sample\s+(m_([\w]+)_out_\d+);/gm.exec(s);
			let cname = match[1]
			let name = match[2];
			let result = {
				cname: cname,
				name: name,
			}

			// if this is a midi output, decode the features
			let midimatch
			if (midimatch = /midi_note(\d*)(_(pitch|vel|press|chan))?/g.exec(name) ) {
				result.midi_type = "note";
				result.midi_num = midimatch[1] !== "" && midimatch[1] !== undefined ? +midimatch[1] : 1;
				result.midi_notetype = midimatch[3] || "pitch"
			} else 
			if (midimatch = /midi_(program|cc|vel|drum)(\d*)(_ch(\d+))?/g.exec(name)) {
				result.midi_type = midimatch[1];
				result.midi_num = midimatch[2] !== "" && midimatch[2] !== undefined ? +midimatch[2] : 1;
				result.midi_chan = +midimatch[4] || 1;
			} else 
			if (midimatch = /midi_(bend|press)(_ch(\d+))?/g.exec(name)) {
				result.midi_type = midimatch[1];
				result.midi_chan = +midimatch[3] || 1;
			} else 
			if (midimatch = /midi_(clock|stop|start|continue|sense|reset)?/g.exec(name)) {
				result.midi_type = midimatch[1];
			} 

			// find the initializer:
			result.default = constexpr( new RegExp(`\\s${cname}\\s+=\\s+([^;]+);`, "gm").exec(cpp)[1] );

			return result;
		}),
	}

	let paramdefinitions = (cpp.match(/pi = self->__commonstate.params([^\/]+)/gm) || []);
	paramdefinitions.forEach((s)=>{
		let type = /pi->paramtype\s+=\s+([^;]+)/gm.exec(s)[1];
		let param = {
			name: /pi->name\s+=\s+"([^"]+)/gm.exec(s)[1],
			cindex: /commonstate.params\s+\+\s+(\d+)/gm.exec(s)[1],
			cname: null,
		}
		if (type == "GENLIB_PARAMTYPE_FLOAT") {
			param.cname = /pi->defaultvalue\s+=\s+self->([^;]+)/gm.exec(s)[1]; 
			param.min = +(/pi->outputmin\s+=\s+([^;]+)/gm.exec(s)[1])
			param.max = +(/pi->outputmax\s+=\s+([^;]+)/gm.exec(s)[1])
			//param.default = +new RegExp(`\\s${param.cname}\\s+=\\s+\\(\\(\\w+\\)([^\\)]+)`, "gm").exec(cpp)[1]
			param.default = +new RegExp(`\\s${param.cname}\\s+=[^\\d\\.-]*([\\d\\.-]+)`, "gm").exec(cpp)[1]
			gen.params.push(param)
			// enable display of params:
			if (hardware.defines.OOPSY_TARGET_HAS_OLED) hardware.defines.OOPSY_HAS_PARAM_VIEW = 1;
		} else if (type == "GENLIB_PARAMTYPE_SYM") {
			/*
				General form:
				m_huge_7.reset("huge", <constexpr>, <constexpr>);

				Some possible variant forms of <constexpr>:
				// (note: same format can appear in length and channels clauses)
				m_huge_7.reset("huge", ((int)16384), ((int)1));
				m_huge_7.reset("huge", samplerate, ((int)1));
				m_huge_7.reset("huge", (samplerate * 2), ((int)1));
				m_huge_7.reset("huge", ((t_sample)3.1415926535898), ((int)1));
				m_huge_7.reset("huge", (3.1415926535898 * 10), ((int)1));
				m_huge_7.reset("huge", (vectorsize * 8), ((int)1));
				m_huge_7.reset("huge", ((16 * 16) * 4096), ((int)1));

				Ignore any (int) or (t_sample)
				Replace any samplerate or vectorsize
				Compute any math

				\s([\w]+)\.reset\("huge",\s+\(\(int\)(\d+)\), \(\(int\)(\d+)\)\);


				\s([\w]+)\.reset\("huge",\s+[^,]+,\s+[^,]+);
			*/

			//let pat = `\\s([\\w]+)\\.reset\\("${param.name}",\\s+\\(\\(int\\)(\\d+)\\), \\(\\(int\\)(\\d+)\\)\\);`;
			let pat = `\\s([\\w]+)\\.reset\\("${param.name}",([^;]+);`
			let match = new RegExp(pat, 'gm').exec(cpp)
			if (match) {
				param.cname = match[1]
				// for the length & channel arguments:
				// first trim of the trailing ")"
				// then split by the comma to [length, channel]
				// then apply a series of replacements and finally eval() the result
				let args = match[2].slice(0, -1).split(",").map(s => constexpr(s))

				assert(typeof args[0] == "number" && args[0] > 0, `failed to derive length of data ${param.name}`)
				assert(typeof args[1] == "number" && args[0] > 0, `failed to derive channels of data ${param.name}`)

				param.dim = Math.round(args[0])
				param.chans = Math.round(args[1])
				gen.datas.push(param)

				let wavname
				let wavmatch = /(\w+)_wav$/g.exec(param.name)
				if (wavmatch) {
					wavname = wavmatch[1]+".wav";
				} else {
					let wavpath = path.join(cpp_path, "..", param.name+".wav")
					if (fs.existsSync(wavpath)) {
						console.log(`[data ${param.name}] has possible source: ${path.resolve( wavpath )}`)
						wavname = param.name+".wav";
						//wavpath = path.resolve( wavpath )
					} 
				}
				if (wavname) {
					param.wavname = wavname
					// mark hardware accordingly:
					hardware.defines.OOPSY_TARGET_USES_SDMMC = 1
					hardware.defines.USE_FATFS = 1
				}
			} else {
				console.error("failed to match details of data "+param.name)
			}
		}
	})
	return gen;
}

function generate_daisy(hardware, nodes) {
	let daisy = {
		som: hardware.som,
		// DEVICE INPUTS:
		device_inputs: Object.keys(hardware.inputs).map(v => {
			let name = v
			nodes[name] = Object.assign({
				name: name,
				to: [],
			}, hardware.inputs[v])
			return name;
		}),

		datahandlers: Object.keys(hardware.datahandlers).map(name => {
			nodes[name] = Object.assign({
				name: name,
				data: null,
			}, hardware.datahandlers[name])
			return name;
		}),

		// DEVICE OUTPUTS:
		device_outs: Object.keys(hardware.outputs).map(name => {
			nodes[name] = {
				name: name,
				config: hardware.outputs[name],
				from: [],
			}
			return name;
		}),
		// configured below
		audio_ins: [],
		audio_outs: [],
	}
	let input_count = hardware.defines.OOPSY_IO_COUNT;
	let output_count = hardware.defines.OOPSY_IO_COUNT;
	for (let i=0; i<input_count; i++) {
		let name = `dsy_in${i+1}`
		nodes[name] = {
			name: name,
			to: [],
		}
		daisy.audio_ins.push(name);
	}
	for (let i=0; i<output_count; i++) {
		let name = `dsy_out${i+1}`
		nodes[name] = {
			name: name,
			to: [],
		}
		daisy.audio_outs.push(name);
	}
	
	if (hardware.defines.OOPSY_TARGET_HAS_MIDI_INPUT) {
		let name = `dsy_midi_in`
		nodes[name] = {
			name: name,
			to: [],
		}
		daisy.midi_ins = [name]
	} else {
		daisy.midi_ins = []
	}
	if (hardware.defines.OOPSY_TARGET_HAS_MIDI_OUTPUT) {
		let name = `dsy_midi_out`
		nodes[name] = {
			name: name,
			from: [],
		}
		daisy.midi_outs = [name]
	} else {
		daisy.midi_outs = []
	}
	return daisy
}

function generate_app(app, hardware, target, config) {
	const defines = hardware.defines
	const nodes = {}
	const daisy = generate_daisy(hardware, nodes, target);
	const gen = {}
	const name = app.patch.name;

	app.audio_outs = []
	app.midi_outs = []
	app.midi_noteouts = []
	app.has_midi_in = false
	app.has_generic_midi_in = false
	app.has_midi_out = false
	app.midi_out_count = 0;
	app.nodes = nodes;
	app.daisy = daisy;
	app.gen = gen;
	app.nodes = nodes;
	app.inserts = [];

	gen.audio_ins = app.patch.ins.map((s, i)=>{
		let name = "gen_in"+(i+1)
		let label = s.replace(/"/g, "").trim();
		let src = null;
		// figure out the src:
		if (label == "midi" || label == "midithru") {
			if (daisy.midi_ins.length) {
				src = daisy.midi_ins[0]
				app.has_midi_in = true;
				app.has_generic_midi_in = true;
				if (label == "midithru") app.has_generic_midi_thru = true;
			}
		} else if (daisy.audio_ins.length > 0) {
			src = daisy.audio_ins[i % daisy.audio_ins.length];
		}
		nodes[name] = {
			// name: name,
			label: label,
			// index: i,
			src: src,
		}
		if (src) {
			nodes[src].to.push(name)
		}
		return name;
	})


	gen.audio_outs = app.patch.outs.map((s, i)=>{
		let name = "gen_out"+(i+1)
		let label = s.replace(/"/g, "").trim();
		let src = daisy.audio_outs[i];
		if (!src) {
			// create a glue node buffer for this:
			src = `glue_out${i+1}`
			nodes[src] = {
				// name: name,
				// kind: "output_buffer",
				// index: i,
				//label: s,
				to: [],
			}
			app.audio_outs.push(src);
		}
		
		let node = {
			name: name,
			// label: label,
			// index: i,
			src: src,
		}
		nodes[name] = node

		// figure out if the out buffer maps to anything:

		// search for a matching [out] name / prefix:
		let map
		let maplabel
		Object.keys(hardware.labels.outs).sort().forEach(k => {
			let match
			if (match = new RegExp(`^${k}_?(.+)?`).exec(label)) {
				map = hardware.labels.outs[k];
				maplabel = match[1] || label
			}
		})

		if (map) {
			label = maplabel
		} else {
			// else it is audio data			
			nodes[src].src = src;
		}
		nodes[name].label = label
		// was this out mapped to something?
		if (map) {
			nodes[map].from.push(src);
			nodes[src].to.push(map)
		}
		return name;
	})

	gen.histories = app.patch.histories.map(history=>{
		const name = history.name
		const varname = "gen_history_"+name;
		let node = Object.assign({
			varname: varname,
		}, history);
		
		if (node.midi_type) {
			if (node.midi_type == "note") {
				let id = node.midi_num-1

				// get or create:
				let noteout = app.midi_noteouts[id]
				if (!noteout) {
					noteout = {}
					app.midi_noteouts[id] = noteout
				}
				noteout.id = id
				noteout.cname = `midinote${node.midi_num}`
				noteout[node.midi_notetype] = node

				// if (node.midi_notetype == "press") {
				// 	node.midi_throttle = true;
				// 	node.setter_src = `gen.${node.cname}`
				// 	app.midi_outs.push(node)
				// }

			} else {
				
				app.midi_outs.push(node)
				node.setter_src = "gen."+node.cname
				if (node.midi_type == "cc") {
					app.has_midi_out = true;
					let statusbyte = 176+((node.midi_chan)-1)%16;
					node.setter = `daisy.midi_message3(${statusbyte}, ${(node.midi_num)%128}, ((uint8_t)(${node.varname}*127.f)) & 0x7F);`; 
					node.type = "float";
					node.midi_throttle = true;
					node.midi_only_when_changed = true;
					nodes[name] = node
				} else if (node.midi_type == "press") {
					app.has_midi_out = true;
					let statusbyte = 208+((node.midi_chan)-1)%16;
					node.setter = `daisy.midi_message2(${statusbyte}, ((uint8_t)(${node.varname}*127.f)) & 0x7F);`; 
					node.type = "float";
					node.midi_throttle = true;
					node.midi_only_when_changed = true;
					nodes[name] = node;
				} else if (node.midi_type == "bend") {
					app.has_midi_out = true;
					let statusbyte = 224+((node.midi_chan)-1)%16;
					let float = `((${node.varname}+1.f)*64.f)`;
					let lsb = `((uint8_t)(${float}*128.f)) & 0x7F`;
					let msb = `((uint8_t)${float}) & 0x7F`;
					node.setter = `daisy.midi_message3(${statusbyte}, ${lsb}, ${msb});`; 
					node.type = "float";
					node.midi_throttle = true;
					node.midi_only_when_changed = true;
					nodes[name] = node;
				} else if (node.midi_type == "program") {
					app.has_midi_out = true;
					let statusbyte = 192+((node.midi_chan)-1)%16;
					node.setter = `daisy.midi_message2(${statusbyte}, ${node.varname} & 0x7F);`; 
					node.type = "uint8_t";
					nodes[name] = node;
				} else if (node.midi_type == "drum") {
					app.has_midi_out = true;
					node.setter = `daisy.midi_message3(153, ${(node.midi_num)%128}, ((uint8_t)(${node.varname}*127.f)) & 0x7F);`;
					node.type = "float";
					nodes[name] = node		
				} else if (node.midi_type == "vel") {
					app.has_midi_out = true;
					let statusbyte = 144+((node.midi_chan)-1)%16;
					node.setter = `daisy.midi_message3(${statusbyte}, ${(node.midi_num)%128}, ((uint8_t)(${node.varname}*127.f)) & 0x7F);`;
					node.type = "float";
					nodes[name] = node		
				} else if (node.midi_type == "clock"
					 	|| node.midi_type == "stop"
					 	|| node.midi_type == "start"
					 	|| node.midi_type == "continue"
					 	|| node.midi_type == "sense"
					 	|| node.midi_type == "reset") {
					app.has_midi_out = true;
					if (node.midi_type == "clock") {
						node.setter = `daisy.midi_message1(248);`;
					} else if (node.midi_type == "stop") {
						node.setter = `daisy.midi_message1(252);`;
					} else if (node.midi_type == "start") {
						node.setter = `daisy.midi_message1(250);`;
					} else if (node.midi_type == "continue") {
						node.setter = `daisy.midi_message1(251);`;
					} else if (node.midi_type == "sense") {
						node.setter = `daisy.midi_message1(254);`;
					} else if (node.midi_type == "reset") {
						node.setter = `daisy.midi_message1(255);`;
					} 
					node.type = "uint8_t";
					nodes[name] = node		
				} 
			}
		} else {

			// search for a matching [out] name / prefix:
			let map
			let maplabel
			Object.keys(hardware.labels.outs).sort().forEach(k => {
				let match
				if (match = new RegExp(`^${k}_?(.+)?`).exec(name)) {
					map = hardware.labels.outs[k];
					maplabel = match[1] || name
				}
			})

			// was this history mapped to something?
			if (map) {
				nodes[name] = node	
				node.type = "t_sample";
				nodes[map].src = "gen."+node.cname; //from.push(src);
				// nodes[src].to.push(map)
			}
			
		}
		return name;
	})

	gen.params = app.patch.params.map((param, i)=>{
		const varname = "gen_param_"+param.name;
		let src, label=param.name, type="float";

		let node = Object.assign({
			varname: varname,
		}, param);

		// figure out parameter range:
		node.max = node.max || 1;
		node.min = node.min || 0;
		node.default = node.default || 0;
		node.range = node.max - node.min;

		let match
		// check for dedicated midi patterns:
		// e.g.
		// [param midi_cc100_ch1] // input is 0..1 for cc value
		// [param midi_cc74]	  // default channel 1
		if (match = (/^midi_cc(\d+)(_(ch)?(\d+))?/g).exec(param.name)) {
			let ch = match[4] ? ((+match[4])+15)%16 : null;
			let cc = (+match[1])%128;
			app.has_midi_in = true;
			node.where = "midi_msg"
			// need to set "src" to something to prevent this being automapped
			src = node.where
			node.code = `if (daisy.midi.lastbyte == 1 && ${ch != null ? `daisy.midi.status == ${176+ch}` : `daisy.midi.status/16 == 11`} && daisy.midi.byte[0] == ${cc}) { 
					${node.varname} = (daisy.midi.byte[1]/127.f)*${asCppNumber(node.range)} + ${asCppNumber(node.min)};
				}`;
		} else 
		if (match = (/^midi_press(_(ch)?(\d+))?/g).exec(param.name)) {
			let ch = match[3] ? ((+match[3])+15)%16 : null;
			app.has_midi_in = true;
			node.where = "midi_msg"
			// need to set "src" to something to prevent this being automapped
			src = node.where
			node.code = `if (daisy.midi.lastbyte == 1 && ${ch != null ? `daisy.midi.status == ${208+ch}` : `daisy.midi.status/16 == 13`}) { 
					${node.varname} = (daisy.midi.byte[0]/127.f)*${asCppNumber(node.range)} + ${asCppNumber(node.min)};
				}`;
		} else 
		if (match = (/^midi_program(_(ch)?(\d+))?/g).exec(param.name)) {
			let ch = match[3] ? ((+match[3])+15)%16 : null;
			app.has_midi_in = true;
			node.where = "midi_msg"
			// need to set "src" to something to prevent this being automapped
			src = node.where
			node.code = `if (daisy.midi.lastbyte == 1 && ${ch != null ? `daisy.midi.status == ${192+ch}` : `daisy.midi.status/16 == 12`}) { 
					${node.varname} = (daisy.midi.byte[0]/127.f)*${asCppNumber(node.range)} + ${asCppNumber(node.min)};
				}`;
		} else 
		if (match = (/^midi_(vel|drum)(\d+)(_(ch)?(\d+))?/g).exec(param.name)) {
			let ch = match[5] ? ((+match[5])+15)%16 : (match[1] == "drum" ? 9 : null);
			let note = (+match[2])%128;
			app.has_midi_in = true;
			node.where = "midi_msg"
			// need to set "src" to something to prevent this being automapped
			src = node.where
			node.code = `if (daisy.midi.lastbyte == 1 && ${ch != null ? `(daisy.midi.status == ${128+ch} || daisy.midi.status == ${144+ch})` : `(daisy.midi.status/16 == 8 || daisy.midi.status/16 == 9)`} && daisy.midi.byte[0] == ${note}) { 
					${node.varname} = (daisy.midi.byte[1]/127.f)*${asCppNumber(node.range)} + ${asCppNumber(node.min)};
				}`;
		} else 
		if (match = (/^midi_bend(_(ch)?(\d+))?/g).exec(param.name)) {
			let ch = match[3] ? ((+match[3])+15)%16 : null;
			app.has_midi_in = true;
			node.where = "midi_msg"
			// need to set "src" to something to prevent this being automapped
			src = node.where
			node.code = `if (daisy.midi.lastbyte == 1 && ${ch != null ? `daisy.midi.status == ${224+ch}` : `daisy.midi.status/16 == 14`}) { 
					${node.varname} = ((daisy.midi.byte[0] + daisy.midi.byte[1]/128.f)/128.f)*${asCppNumber(node.range)} + ${asCppNumber(node.min)};
				}`;
		} else 
		if (param.name == "midi_clock") {
			app.has_midi_in = true;
			node.where = "midi_status"
			// need to set "src" to something to prevent this being automapped
			src = node.where
			node.code = `if (byte == 248) { 
					${node.varname} = 1.f;
				}`;
			// reset:
			app.inserts.push({
				where: "post_audio",
				code: `${node.varname} = 0.f;`
			})
		} else if (param.name == "midi_play") {
			app.has_midi_in = true;
			node.where = "midi_status"
			// need to set "src" to something to prevent this being automapped
			src = node.where
			node.code = `if (byte == 250 || byte == 251) { 
					${node.varname} = 1.f;
				} else if (byte == 252) { 
					${node.varname} = 0.f;
				}`;
			// reset:
			app.inserts.push({
				where: "post_audio",
				code: `${node.varname} = 0.f;`
			})
		} else {
			// search for a matching [out] name / prefix:
			Object.keys(hardware.labels.params).sort().forEach(k => {
				if (match = new RegExp(`^${k}_?(.+)?`).exec(param.name)) {
					src = hardware.labels.params[k];
					label = match[1] || param.name

					// search for any type qualifiers:
					//if (match = label.match(/^((.+)_)?(int|bool)(_(.*))?$/)) {
					if (match = label.match(/^(int|bool)(_(.*))?$/)) {
						//type = match[3];
						type = match[1]
						// trim type from label:
						//label = (match[2] || "") + (match[5] || "") 
						label = match[3] || label
					}
				}
			})
		}

		node.type = type;
		node.src = src;
		node.label = label;

		// Apply input scaling
		if (src in hardware.inputs)
		{
			let input = hardware.inputs[src];
			node.permit_scale = input.permit_scale;

			// TODO -- we should allow no scaling to occur on certain inputs
			if ('range' in input && typeof input.range !== 'undefined')
			{
				let input_min = input.range[0] || 0;
				let input_max = input.range[1] || 1;

				// We ignore this step if the fields are default
				if (input_min != 0 || input_max != 1)
				{
					let new_range = node.range / (input_max - input_min);
					node.range = new_range;

					let new_min = node.min - input_min * new_range;
					node.min = new_min;
				}
			}
		}

		let ideal_steps = 100 // about 4 good twists of the encoder
		if (node.type == "bool" || node.type == "int") {
			node.stepsize = 1
		} else {
			// figure out a suitable encoder step division for this parameter
			if (node.range > 2 && Number.isInteger(node.max) && Number.isInteger(node.max) && Number.isInteger(node.default)) {
				if (node.range < 10) {
					// might be v/oct
					node.stepsize = 1/12
				} else {
					// find a suitable subdivision:
					let power = Math.round(Math.log2(node.range / ideal_steps))
					node.stepsize = Math.pow(2, power)
				}
			} 
		}
		if (!node.stepsize) {
			// general case:
			node.stepsize = node.range / ideal_steps
		}
		
		nodes[varname] = node;
		if (src && nodes[src]) {
			nodes[src].to.push(varname)
		}
		return varname;
	})

	gen.datas = app.patch.datas.map((param, i)=>{
		const varname = "gen_data_"+param.name;
		let src, label;
		// search for a matching [out] name / prefix:
		Object.keys(hardware.labels.datas).sort().forEach(k => {
			let match
			if (match = new RegExp(`^${k}_?(.+)?`).exec(param.name)) {
				src = hardware.labels.datas[k];
				label = match[1] || param.name

				if (param.name == "midi") {
					app.has_midi_out = true;
				}
			}
		})

		let node = Object.assign({
			varname: varname,
			label: param.name,
		}, param);
		nodes[varname] = node;

		if (src) {
			nodes[src].data = "gen." + param.cname;
			//nodes[src].to.push(varname)
			//nodes[src].from.push(src);
		}

		return varname;
	})

	if ((app.has_midi_in && hardware.defines.OOPSY_TARGET_HAS_MIDI_INPUT) || (app.has_midi_out && hardware.defines.OOPSY_TARGET_HAS_MIDI_OUTPUT)) {
		defines.OOPSY_TARGET_USES_MIDI_UART = 1
	}

	// fill all my holes
	// map unused cvs/knobs to unmapped params?
	let upi=0; // unused param index
	let param = gen.params[upi];
	Object.keys(hardware.inputs).forEach(name => {
		const node = nodes[name];
		if (node.to.length == 0 && node.automap) {
			//console.log(name, "not mapped")
			// find next param without a src:
			while (param && !!nodes[param].src) param = gen.params[++upi];
			if (param) {
				// console.log(name, "map to", param)
				nodes[param].src = name;
				node.to.push(param);
			}
		}
	})

	// TODO -- This may or may not be implemented any time soon
	// let board_glue = daisy_glue.parse_parameters(nodes, hardware.components, hardware.aliases, 'hardware');

	// normal any audio outs from earlier (non cv/gate/midi) audio outs
	{
		let available = []
		daisy.audio_outs.forEach((name, i)=>{
			const node = nodes[name];
			// does this output have an audio source?
			if (node.src) {
				available.push(name);
			} else if (available.length) {
				node.src = available[i % available.length];
			}
		});
	}

	let som_or_seed = hardware.defines.OOPSY_OLD_JSON ? 'seed' : 'som';

	const struct = `

struct App_${name} : public oopsy::App<App_${name}> {
	${gen.params
		.map(name=>`
	${nodes[name].type} ${name};`).join("")}
	${app.midi_noteouts.map(note=>`
	oopsy::GenDaisy::MidiNote ${note.cname};`).join("")}
	${gen.histories.map(name=>nodes[name]).filter(node => node && node.midi_type).map(node=>`
	${node.type} ${node.varname};`).join("")}
	${gen.audio_outs.map(name=>nodes[name]).filter(node => node && node.midi_type).map(node=>`
	${node.type} ${node.varname};`).join("")}
	${daisy.device_outs.map(name => nodes[name])
		.filter(node => node.src || node.from.length)
		.map(node=>`
	float ${node.name};`).join("")}
	${app.audio_outs.map(name=>`
	float ${name}[OOPSY_BLOCK_SIZE];`).join("")}
	
	void init(oopsy::GenDaisy& daisy) {
		daisy.gen = ${name}::create(daisy.hardware.${som_or_seed}.AudioSampleRate(), daisy.hardware.${som_or_seed}.AudioBlockSize());
		${name}::State& gen = *(${name}::State *)daisy.gen;
		
		daisy.param_count = ${gen.params.length};
		${(defines.OOPSY_HAS_PARAM_VIEW) ? `daisy.param_selected = ${Math.max(0, gen.params.map(name=>nodes[name].src).indexOf(undefined))};`:``}
		${gen.params.map(name=>nodes[name])
			.map(node=>`
		${node.varname} = ${asCppNumber(node.default, node.type)};`).join("")}
		${daisy.device_outs.map(name => nodes[name])
			.filter(node => node.src || node.from.length)
			.map(node=>`
		${node.name} = 0.f;`).join("")}
		${gen.histories.map(name=>nodes[name]).filter(node => node && node.midi_type).map(node=>`
		${node.varname} = ${asCppNumber(node.default, node.type)};`).join("")}
		${daisy.datahandlers.map(name => nodes[name])
			.filter(node => node.init)
			.filter(node => node.data)
			.map(node =>`
		${interpolate(node.init, node)};`).join("")}
		${gen.datas.map(name=>nodes[name])
			.filter(node => node.wavname)
			.map(node=>`
		daisy.sdcard_load_wav("${node.wavname}", gen.${node.cname});`).join("")}
	}

	void audioCallback(oopsy::GenDaisy& daisy, daisy::AudioHandle::InputBuffer hardware_ins, daisy::AudioHandle::OutputBuffer hardware_outs, size_t size) {
		Daisy& hardware = daisy.hardware;
		${name}::State& gen = *(${name}::State *)daisy.gen;
		${app.inserts.concat(hardware.inserts).filter(o => o.where == "audio").map(o => o.code).join("\n\t")}
		${daisy.device_inputs.map(name => nodes[name])
			.filter(node => node.to.length)
			.filter(node => node.update && node.update.where == "audio")
			.map(node=>`
		${interpolate(node.update.code, node)};`).join("")}
		${daisy.device_inputs.map(name => nodes[name])
			.filter(node => node.to.length)
			.map(node=>`
		float ${node.name} = ${node.code};`).join("")}
		${gen.params
			.map(name=>nodes[name])
			.filter(node => node.src)
			.filter(node => node.where == "audio" || node.where == undefined)
			.map(node_scale).join("")}
		${gen.params
			.map(name=>nodes[name])
			.map(node=>`
		gen.set_${node.name}(${node.varname});`).join("")}
		${daisy.audio_ins.map((name, i)=>`
		float * ${name} = (float *)hardware_ins[${i}];`).join("")}
		${daisy.audio_outs.map((name, i)=>`
		float * ${name} = hardware_outs[${i}];`).join("")}
		${app.has_midi_in ? daisy.midi_ins.map(name=>`
		float * ${name} = daisy.midi_in_data;`).join("") : ''}
		// ${gen.audio_ins.map(name=>nodes[name].label).join(", ")}:
		float * inputs[] = { ${gen.audio_ins.map(name=>nodes[name].src).join(", ")} }; 
		// ${gen.audio_outs.map(name=>nodes[name].label).join(", ")}:
		float * outputs[] = { ${gen.audio_outs.map(name=>nodes[name].src).join(", ")} };
		gen.perform(inputs, outputs, size);
		${daisy.device_outs.map(name => nodes[name])
			.filter(node => node.src || node.from.length)
			.map(node => node.src ? `
		${node.name} = ${node.src};` : `
		${node.name} = ${node.from.map(name=>name+"[ size-1]").join(" + ")}; // device out`).join("")}
		${daisy.device_outs.map(name => nodes[name])
			.filter(node => node.src || node.from.length)
			.filter(node => node.config.where == "audio")
			.map(node=>`
		${interpolate(node.config.code, node)} // set out`).join("")}
		${daisy.datahandlers.map(name => nodes[name])
			.filter(node => node.where == "audio")
			.filter(node => node.data)
			.map(node =>`
		${interpolate(node.code, node)} // data out`).join("")}
		${app.midi_outs
			.filter(node=>!node.midi_throttle)
			.map(node=>`
		if (${node.varname} != (${node.type})${node.setter_src}) {
			${node.varname} = ${node.setter_src};
			${node.setter}
		}`).join("")}		
		${app.midi_noteouts
			.filter(note=>note.vel && note.pitch)
			.map(note=>`
		${note.cname}.update(daisy, 
			((uint8_t)(gen.${note.vel.cname}*127.f)) & 0x7F, 
			((uint8_t)gen.${note.pitch.cname}) & 0x7F, 
			${note.chan ? `((uint8_t)(gen.${note.chan.cname})-1) % 16` : "0"});`).join("")}
		// msgs: ${(app.midi_outs.filter(node=>node.midi_throttle).length + app.midi_noteouts.filter(note=>note.press).length)}
		// rate: ${hardware.defines.OOPSY_BLOCK_RATE/500}
		${(app.midi_outs.filter(node=>node.midi_throttle).length + app.midi_noteouts.filter(note=>note.press).length) > 0 ? `
		if (daisy.blockcount % ${ Math.ceil((app.midi_outs.filter(node=>node.midi_throttle).length + app.midi_noteouts
			.filter(note=>note.press).length) * hardware.defines.OOPSY_BLOCK_RATE/500)} == 0){ // throttle output for MIDI baud limits
			${app.midi_outs
				.filter(node=>node.midi_throttle)
				.map(node=>`
			${node.midi_only_when_changed ? `if (${node.varname} != (${node.type})${node.setter_src}) {` : `{`}
				${node.varname} = ${node.setter_src};
				${node.setter}
			}`).join("")}
			${app.midi_noteouts
				.filter(note=>note.press)
				.map(note=>`
			${note.cname}.update_pressure(daisy, ((uint8_t)gen.${note.press.cname}) & 0x7F);`).join("")}
		}` : ''}
		${app.has_midi_out ? daisy.midi_outs.map(name=>nodes[name].from.map(name=>`
		daisy.midi_postperform(${name}, size);`).join("")).join("") : ''}
		${daisy.audio_outs.map(name=>nodes[name])
			.filter(node => node.src != node.name)
			.map(node=>node.src ? `
		memcpy(${node.name}, ${node.src}, sizeof(float)*size);` : `
		memset(${node.name}, 0, sizeof(float)*size);`).join("")}
		${app.inserts.concat(hardware.inserts).filter(o => o.where == "post_audio").map(o => o.code).join("\n\t")}
		${hardware.som == 'seed' ? "hardware.PostProcess();" : ""}
	}	

	void mainloopCallback(oopsy::GenDaisy& daisy, uint32_t t, uint32_t dt) {
		Daisy& hardware = daisy.hardware;
		${name}::State& gen = *(${name}::State *)daisy.gen;
		${app.inserts.concat(hardware.inserts).filter(o => o.where == "main").map(o => o.code).join("\n\t")}
		${daisy.datahandlers.map(name => nodes[name])
			.filter(node => node.where == "main")
			.filter(node => node.data)
			.map(node =>`
		${interpolate(node.code, node)}`).join("")}
		${daisy.device_outs.map(name => nodes[name])
			.filter(node => node.src || node.from.length)
			.filter(node => node.config.where == "main")
			.map(node=>`
		${interpolate(node.config.code, node)}`).join("")}
		${defines.OOPSY_TARGET_USES_MIDI_UART ? `
		while(daisy.uart.Readable()) {
			uint8_t byte = daisy.uart.PopRx();
			if (byte >= 128) { // status byte
				${gen.params
				.map(name=>nodes[name])
				.filter(node => node.where == "midi_status")
				.map(node=>node.code)
				.concat(`if (byte == 0xFF) { // reset event -> go to bootloader
					daisy.log("reboot");
					daisy::System::ResetToBootloader();
				} 
				if (byte <= 240 || byte == 247) {
					daisy.midi.status = byte; 
					daisy.midi.lastbyte = 255; // means 'no bytes received'
				}`)
				.join(" else ")}
			} else {
				daisy.midi.lastbyte = !daisy.midi.lastbyte; 
				daisy.midi.byte[daisy.midi.lastbyte] = byte;
				${gen.params
					.map(name=>nodes[name])
					.filter(node => node.where == "midi_msg")
					.map(node=>node.code)
					.concat(defines.OOPSY_MULTI_APP ? `
					if (daisy.midi.status/16 == 12) { // program change -> app change
						daisy.schedule_app_load(daisy.midi.byte[daisy.midi.lastbyte]);
					}`:[])
					.join(" else ")}
			}
			${app.has_generic_midi_in ? `
			${app.has_generic_midi_thru ? `daisy.midi_message1(byte); // thru` : ``}
			if (daisy.midi_in_written < OOPSY_BLOCK_SIZE) {
				// scale (0, 255) to (0.0, 1.0) to protect hardware from accidental patching
				daisy.midi_in_data[daisy.midi_in_written] = byte / 256.0f;
				daisy.midi_in_written++;
			}` : ""}
			daisy.midi_in_active = 1;
		}` : "// no midi input handling"}
		hardware.LoopProcess();
	}

	void displayCallback(oopsy::GenDaisy& daisy, uint32_t t, uint32_t dt) {
		Daisy& hardware = daisy.hardware;
		${name}::State& gen = *(${name}::State *)daisy.gen;
		${app.inserts.concat(hardware.inserts).filter(o => o.where == "display").map(o => o.code).join("\n\t")}
		${daisy.datahandlers.map(name => nodes[name])
			.filter(node => node.where == "display")
			.filter(node => node.data)
			.map(node =>`
		${interpolate(node.code, node)}`).join("")}
		${daisy.device_outs.map(name => nodes[name])
			.filter(node => node.src || node.from.length)
			.filter(node => node.config.where == "display")
			.map(node=>`
		${interpolate(node.config.code, node)}`).join("")}
		${hardware.som == 'seed' ? "hardware.Display();" : ""}
	}

	${defines.OOPSY_HAS_PARAM_VIEW ? `
	float setparam(int idx, float val) {
		switch(idx) {
			${gen.params
				.map(name=>nodes[name])
				.map((node, i)=>`
			case ${i}: return ${node.varname} = (${node.type})(val > ${asCppNumber(node.max, node.type)}) ? ${asCppNumber(node.max, node.type)} : (val < ${asCppNumber(node.min, node.type)}) ? ${asCppNumber(node.min, node.type)} : val;`).join("")}
		}
		return 0.f;	
	}

	${defines.OOPSY_TARGET_HAS_OLED && defines.OOPSY_HAS_PARAM_VIEW ? `
	void paramCallback(oopsy::GenDaisy& daisy, int idx, char * label, int len, bool tweak) {
		switch(idx) { ${gen.params.map(name=>nodes[name]).map((node, i)=>`
		case ${i}: ${defines.OOPSY_CAN_PARAM_TWEAK ? `
		if (tweak) setparam(${i}, ${node.varname} + daisy.menu_button_incr ${node.type == "float" ? '* ' + asCppNumber(node.stepsize, node.type) : ""});` : ""}
		${defines.OOPSY_OLED_DISPLAY_WIDTH < 128 ? `snprintf(label, len, "${node.label.substring(0,5).padEnd(5," ")}" FLT_FMT3 "", FLT_VAR3(${node.varname}) );` : `snprintf(label, len, "${node.src ? 
			`${node.src.substring(0,3).padEnd(3," ")} ${node.label.substring(0,11).padEnd(11," ")}" FLT_FMT3 ""` 
			: 
			`%s ${node.label.substring(0,11).padEnd(11," ")}" FLT_FMT3 "", (daisy.param_is_tweaking && ${i} == daisy.param_selected) ? "enc" : "   "`
			}, FLT_VAR3(${node.varname}) );`}
		break;`).join("")}
		}	
	}
	` : ""}
	` : ""}
};`
	app.cpp = {
		union: `App_${name} app_${name};`,
		appdef: `{"${name}", []()->void { oopsy::daisy.reset(apps.app_${name}); } },`,
		struct: struct,
	}
	return app
}
