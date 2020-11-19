#!/usr/bin/env node

/*
	Generates and compiles wrapper code for gen~ export to Daisy hardware
*/
const fs = require("fs"),
	path = require("path"),
	os = require("os"),
	assert = require("assert");
const {exec, execSync, spawn} = require("child_process");

// returns the path `str` with posix path formatting:
function posixify_path(str) {
	return str.split(path.sep).join(path.posix.sep);
}

// returns str with any $<key> replaced by data[key]
function interpolate(str, data) {
	return str.replace(/\$<([^>]+)>/gm, (s, key) => data[key])
}

// prints a number as a C-style float:
function toCfloat(n) {
	let s = (+n).toString();
	// add point if needed:
	if (s.includes("e")) {
		return s;
	} else if (s.includes(".")) {
		return s + "f";
	} else {
		return s + ".f";
	}
}

const help = `
<[cmds]> <[target]> <[cpps]> <watch>

cmds: 	up/upload = (default) generate & upload
	  	gen/generate = generate only

target: path to a JSON for the hardware config, 
		or simply "patch", "field", "petal", "pod" etc. 
		Defaults to "patch"

cpps: 	paths to the gen~ exported cpp files
		first item will be the default app
		  
watch:	script will not terminate
		actions will be re-run each time any of the cpp files are modified
`

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

	if (args.length == 0) {
		console.log(help)
		return;
	}

	args.forEach(arg => {
		switch (arg) {
			case "help": {console.log(help); process.exit(0);} break;
			case "generate":
			case "gen": {action="generate";} break;
			case "upload":
			case "up": {action="upload";} break;
			case "pod":
			case "field":
			case "petal":
			case "patch": {target = arg;} break;
			case "watch": watch=true; break;
			default: {
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
	});

	// configure target:
	if (!target && !target_path) target = "patch";
	if (!target_path) {
		target_path = path.join(__dirname, `daisy.${target}.json`);
	} else {
		target = path.parse(target_path).name;
	}
	console.log(`Target ${target} configured in path ${target_path}`)
	assert(fs.existsSync(target_path), `couldn't find target configuration file ${target_path}`);
	const hardware = JSON.parse(fs.readFileSync(target_path, "utf8"));
	if (!hardware.keys) hardware.keys = []
	if (!hardware.knobs) hardware.knobs = []
	if (!hardware.switches) hardware.switches = []

	// verify and analyze cpps:
	assert(cpps.length > 0, "an argument specifying the path to at least one gen~ exported cpp file is required");
	if (cpps.length > 8) {
		console.log("maximum 8 apps supported currently")
		cpps.length = 8;
	}
	let apps = cpps.map(cpp_path => {
		assert(fs.existsSync(cpp_path), `couldn't find source C++ file ${cpp_path}`);
		return {
			path: cpp_path,
			patch: analyze_cpp(fs.readFileSync(cpp_path, "utf8"))
		}
	})
	let build_name = apps.map(v=>v.patch.name).join("_")


	// configure build path:
	const build_path = path.join(__dirname, `build_${build_name}_${target}`)
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
	
	// add watcher
	if (watch && watchers.length < 1) {
		watchers = cpps.map(cpp_path => fs.watch(cpp_path, (event, filepath)=>{
			run(...args);
		}))
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
# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
# Include the gen_dsp files
CFLAGS+=-I"${posixify_path(path.relative(build_path, path.join(__dirname, "gen_dsp")))}"
CFLAGS+=-Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-variable
# Enable printing of floats (for OLED display)
LDFLAGS+=-u _printf_float
`, "utf-8");
	console.log(`Will ${action} from ${cpps.join(", ")} by writing to:`)
	console.log(`\t${maincpp_path}`)
	console.log(`\t${makefile_path}`)
	console.log(`\t${bin_path}`)

	apps.map(app => {
		generate_app(app, hardware, target);
		return app;
	})

	let defines = Object.assign({}, hardware.defines);
	if (apps.some(g => g.has_midi_in || g.has_midi_out)) defines.OOPSY_TARGET_USES_MIDI_UART = 1
	if (apps.length) defines.OOPSY_MULTI_APP = 1


	// store for debugging:
	fs.writeFileSync(path.join(build_path, `${build_name}_${target}.json`), JSON.stringify(config,null,"  "),"utf8");

	const cppcode = `${Object.keys(defines).map(k => `
#define ${k} (${defines[k]})`).join("")}
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
	return oopsy::daisy.run(appdefs, ${apps.length});
}
`
	fs.writeFileSync(maincpp_path, cppcode, "utf-8");	

	console.log("oopsy generated code")

	// now try to make:
	try {
		try {
			console.log(execSync("make clean", { cwd: build_path }).toString())
			// TODO: make this cross-platform:
			if (os.platform() == "win32") {
				//console.log(execSync("set PATH=%PATH%;/usr/local/bin && make", { cwd: build_path }).toString())

				// Gather up make output to run command per line as child process
				// TODO: fix the awful output..
				let build_cmd = execSync("make -n", { cwd: build_path }).toString().split(os.EOL)
				build_cmd.forEach(line => {
					if (line.length > 0)
						console.log(execSync(line, { cwd: build_path }).toString())
				})
			
			} else {
				console.log(execSync("export PATH=$PATH:/usr/local/bin && make", { cwd: build_path }).toString())
			}

			console.log("oopsy compiled code")
		} catch (e) {
			// errors from make here
			console.error("make failed");
		}
		// if successful, try to upload to hardware:
		if (fs.existsSync(bin_path) && action=="upload") {
			
			console.log("oopsy flashing...")
			
			if (os.platform() == "win32") {
				console.log(execSync("set PATH=%PATH%;/usr/local/bin && make program-dfu", { cwd: build_path }).toString())
			} else {
				console.log(execSync("export PATH=$PATH:/usr/local/bin && make program-dfu", { cwd: build_path, stdio:'inherit' }).toString())
				//console.log(execSync("export PATH=$PATH:/usr/local/bin && make program-dfu", { cwd: build_path }).toString())
				//console.log(execSync("export PATH=$PATH:/usr/local/bin && make program-dfu", { cwd: build_path, stdio:'inherit' })
			}

			console.log("oopsy flashed")
		}
	} catch (e) {
		// errors from make here
		console.log("upload failed");
	}
	console.log("oopsy done")
}

function analyze_cpp(cpp) {
	let gen = {
		name: /namespace\s+(\w+)\s+{/gm.exec(cpp)[1],
		ins: (/gen_kernel_innames\[\]\s=\s{\s([^}]*)/g).exec(cpp)[1].split(",").map(s => s.replace(/"/g, "").trim()),
		outs: (/gen_kernel_outnames\[\]\s=\s{\s([^}]*)/g).exec(cpp)[1].split(",").map(s => s.replace(/"/g, "").trim()),
		params: [],
		datas: [],
	}
	let paramdefinitions = (cpp.match(/pi = self->__commonstate.params([^\/]+)/gm) || []);
	paramdefinitions.forEach((s, i)=>{
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
			param.default = +new RegExp(`\\s${param.cname}\\s+=\\s+\\(\\(\\w+\\)([^\\)]+)`, "gm").exec(cpp)[1]
			gen.params.push(param)
		} else if (type == "GENLIB_PARAMTYPE_SYM") {
			let match = new RegExp(`\\s([\\w]+)\\.reset\\("${param.name}",\\s+\\(\\(int\\)(\\d+)\\), \\(\\(int\\)(\\d+)\\)\\);`, 'gm').exec(cpp)
			param.cname = match[1], param.dim = match[2], param.chans = match[3]
			gen.datas.push(param)
		}
	})
	return gen;
}

function generate_daisy(hardware, nodes, target) {
	let daisy = {
		audio_ins: hardware.audio_ins.map((v, i)=>{
			let name = `dsy_in${i+1}`
			nodes[name] = {
				// name: name,
				// kind: "input_buffer",
				// index: i,
				to: [],
			}
			return name;
		}),
		audio_outs: hardware.audio_outs.map((v, i)=>{
			let name = `dsy_out${i+1}`
			nodes[name] = {
				// name: name,
				// kind: "output_buffer",
				// index: i,
				to: [], // what non-audio outputs it maps to
				src: null, // what audio content it draws from
			}
			return name;
		}),
		midi_ins: hardware.midi_ins.map((v, i)=>{
			let name = `dsy_midi_in${i+1}`
			nodes[name] = {
				// buffername: name,
				// index: i,
				to: [],
			}
			return name;
		}),
		midi_outs: hardware.midi_outs.map((v, i)=>{
			let name = `dsy_midi_out${i+1}`
			nodes[name] = {
				// buffername: name,
				// index: i,
				from: [],
			}
			return name;
		}),

		// DEVICE INPUTS:
		gpio_ins: Object.keys(hardware.getters).map(v => {
			let name = v
			nodes[name] = {
				to: [],
				get: hardware.getters[v],
			}
			return name;
		}),

		// DEVICE OUTPUTS:
		gpio_outs: Object.keys(hardware.setters).map(v => {
			let name = v
			nodes[name] = {
				from: [],
				set: interpolate(hardware.setters[v], {name: name})
			}
			return name;
		}),
	}
	return daisy
}

function generate_app(app, hardware, target) {
	const nodes = {}
	const daisy = generate_daisy(hardware, nodes, target);
	const gen = {}

	app.audio_outs = []
	app.has_midi_in = false
	app.has_midi_out = false

	app.nodes = nodes;
	app.daisy = daisy;
	app.gen = gen;
	app.nodes = nodes;

	gen.audio_ins = app.patch.ins.map((s, i)=>{
		let name = "gen_in"+(i+1)
		let label = s.replace(/"/g, "").trim();
		let src = null;
		// figure out the src:
		if (label == "midi") {
			if (daisy.midi_ins.length) {
				src = daisy.midi_ins[0]
				app.has_midi_in = true;
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
		nodes[name] = {
			// name: name,
			// label: label,
			// index: i,
			src: src,
		}

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
		if (label == "midi") {
			map = daisy.midi_outs[0]
			app.has_midi_out = true;
		} else if (map) {
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

	gen.params = app.patch.params.map((param, i)=>{
		const varname = "gen_param_"+param.name;
		let src, label;

		// search for a matching [out] name / prefix:
		Object.keys(hardware.labels.params).sort().forEach(k => {
			let match
			if (match = new RegExp(`^${k}_?(.+)?`).exec(param.name)) {
				src = hardware.labels.params[k];
				label = match[1] || param.name
			}
		})

		let node = Object.assign({
			varname: varname,
			label: label || param.name,
			src: src,
		}, param);
		nodes[varname] = node;

		if (src) {
			nodes[src].to.push(varname)
		}
		return varname;
	})

	gen.datas = app.patch.datas.map((param, i)=>{
		const varname = "gen_data_"+param.name;
		let node = Object.assign({
			varname: varname,
			label: param.name,
		}, param);
		nodes[varname] = node;
		return varname;
	})
	

	// fill all my holes
	// map unused cvs/knobs to unmapped params
	let upi=0; // unused param index
	let param = gen.params[upi];
	Object.keys(hardware.getters).forEach(name => {
		const node = nodes[name];
		if (node.to.length == 0) {
			//console.log(name, "not mapped")
			// find next param without a src:
			while (param && !!nodes[param].src) param = gen.params[++upi];
			if (param) {
				//console.log("map to", param)
				nodes[param].src = name;
				node.to.push(param);
			}
		}
	})

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
	// normal cv outs etc. in the same way
	{
		let available = []
		let i=0
		daisy.gpio_outs.forEach(name => {
			const node = nodes[name];
			// does this output have an audio source?
			if (node.from.length) {
				available.push(name);
			} else if (available.length) {
				node.src = available[i++ % available.length];
			}
		})
	}

	const name = app.patch.name;
	const struct = `

struct App_${name} : public oopsy::App<App_${name}> {
	${gen.params
		.filter(name => nodes[name].src)
		.concat(daisy.gpio_outs)
		.map(name=>`
	float ${name};`).join("")}
	${app.audio_outs.map(name=>`
	float ${name}[OOPSY_BUFFER_SIZE];`).join("")}
	
	void init(oopsy::GenDaisy& daisy) {
		daisy.gen = ${name}::create(daisy.samplerate, daisy.blocksize);
		${gen.params
			.filter(name => nodes[name].src)
			.concat(daisy.gpio_outs)
			.map(name=>`
		${name} = 0.f;`).join("")}
	}	

	void mainloopCallback(oopsy::GenDaisy& daisy, uint32_t t, uint32_t dt) {
		// whatever handling is needed here
		Daisy& hardware = daisy.hardware;
		${daisy.gpio_outs
			.filter(name => nodes[name].src || nodes[name].from.length)
			.map((name, i)=>`
		${nodes[name].set};`).join("")}
	}

	void audioCallback(oopsy::GenDaisy& daisy, float **hardware_ins, float **hardware_outs, size_t size) {
		Daisy& hardware = daisy.hardware;
		${name}::State& gen = *(${name}::State *)daisy.gen;
		${daisy.gpio_ins
			.filter(name => nodes[name].to.length)
			.map(name=>`
		float ${name} = ${nodes[name].get};`).join("")}
		${gen.params.filter(name => nodes[name].src).map(name=>`
		// ${nodes[name].label}
		${name} = ${nodes[name].src}*${toCfloat(nodes[name].max-nodes[name].min)} + ${toCfloat(nodes[name].min)};
		gen.set_${nodes[name].name}(${name});`).join("")}
		${daisy.audio_ins.map((name, i)=>`
		float * ${name} = hardware_ins[${i}];`).join("")}
		${daisy.audio_outs.map((name, i)=>`
		float * ${name} = hardware_outs[${i}];`).join("")}
		${app.has_midi_in ? daisy.midi_ins.map(name=>`
		float * ${name} = oopsy::midi.in_data;`).join("") : ''}
		// ${gen.audio_ins.map(name=>nodes[name].label).join(", ")}:
		float * inputs[] = { ${gen.audio_ins.map(name=>nodes[name].src).join(", ")} }; 
		// ${gen.audio_outs.map(name=>nodes[name].label).join(", ")}:
		float * outputs[] = { ${gen.audio_outs.map(name=>nodes[name].src).join(", ")} };
		gen.perform(inputs, outputs, size);
		${daisy.gpio_outs
			.filter(name => nodes[name].src || nodes[name].from.length > 0)
			.map(name => nodes[name].src ? `
		${name} = ${nodes[name].src};` : `
		${name} = ${nodes[name].from.map(name=>name+"[size-1]").join(" + ")};`).join("")}
		${app.has_midi_out ? daisy.midi_outs.map(name=>nodes[name].from.map(name=>`
		oopsy::midi.postperform(${name}, size);`).join("")).join("") : ''}
	}
};`
	app.cpp = {
		union: `App_${name} app_${name};`,
		appdef: `{"${name}", []()->void { oopsy::daisy.reset(apps.app_${name}); } },`,
		struct: struct,
	}
	return app
}
