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
	let gloos = cpps.map(cpp_path => {
		assert(fs.existsSync(cpp_path), `couldn't find source C++ file ${cpp_path}`);
		let gloo = analyze_cpp(fs.readFileSync(cpp_path, "utf8"));
		gloo.hardware = hardware;
		gloo.target = target;
		gloo.path = cpp_path;
		return gloo;
	})
	let build_name = gloos.map(v=>v.name).join("_")

	// configure build path:
	const build_path = path.join(__dirname, `build_${build_name}_${target}`)
	console.log(`Building to ${build_path}`)
	// ensure build path exists:
	fs.mkdirSync(build_path, {recursive: true});

	
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

	gloos.map(gloo => {
		gloo.relative_path = path.relative(build_path, gloo.path);
		generate_gloo(gloo);
		return gloo;
	})

	// store for debugging:
	fs.writeFileSync(path.join(build_path, `${build_name}_${target}.json`), JSON.stringify(gloos,null,"  "),"utf8");

	const cppcode = `${
target=="patch" ? "#define GEN_DAISY_TARGET_PATCH 1" :
target=="field" ? "#define GEN_DAISY_TARGET_FIELD 1" : 
target=="petal" ? "#define GEN_DAISY_TARGET_PETAL 1" :
target=="pod" ? "#define GEN_DAISY_TARGET_POD 1" :
""}
${gloos.some(g => g.has_midi_in || g.has_midi_out) ? "#define GEN_DAISY_TARGET_USES_MIDI_UART 1" : "// no midi"}
${(gloos.length > 1) ? "#define GEN_DAISY_MULTI_APP 1" : "// single app"}

#include "../genlib_daisy.h"
#include "../genlib_daisy.cpp"
${gloos.map(gloo => `#include "${posixify_path(gloo.relative_path)}"`).join("\n")}
${gloos.map(gloo => gloo.cpp.struct).join("\n")}

// store apps in a union to re-use memory, since only one app is active at once:
union {
	${gloos.map(gloo => gloo.cpp.union).join("\n\t")}
} apps;

GenDaisy::AppDef appdefs[] = {
	${gloos.map(gloo => gloo.cpp.appdef).join("\n\t")}
};

int main(void) {
	return gendaisy.run(appdefs, sizeof(appdefs)/sizeof(GenDaisy::AppDef));
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
			
			console.log("oopsy uploading", bin_path)
			
			if (os.platform() == "win32") {
				console.log(execSync("set PATH=%PATH%;/usr/local/bin && make program-dfu", { cwd: build_path }).toString())
			} else {
				console.log(execSync("export PATH=$PATH:/usr/local/bin && make program-dfu", { cwd: build_path, stdio:'inherit' }).toString())
				//console.log(execSync("export PATH=$PATH:/usr/local/bin && make program-dfu", { cwd: build_path }).toString())
				//console.log(execSync("export PATH=$PATH:/usr/local/bin && make program-dfu", { cwd: build_path, stdio:'inherit' })
			}

			console.log("oopsy uploaded")
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
	return {
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

		keys: hardware.keys.map((v, i)=>{
			let name = `dsy_key_in${i+1}`
			nodes[name] = {
				// buffername: name,
				// index: i,
				to: [],
				get: `(hardware.KeyboardState(${i})?1.f:0.f)`,
			}
			return name;
		}),
		switches: hardware.switches.map((v, i)=>{
			let name = `dsy_sw_in${i+1}`
			nodes[name] = {
				// buffername: name,
				// index: i,
				to: [],
				get: (target=="pod") ? `(hardware.button${i+1}.Pressed()?1.f:0.f)` : (target=="petal") ? `(hardware.switches[${i}].Pressed()?1.f:0.f)` : `(hardware.GetSwitch(${i})->Pressed()?1.f:0.f)`,
			}
			return name;
		}),
		knobs: hardware.knobs.map((v, i)=>{
			let name = `dsy_knob_in${i+1}`
			nodes[name] = {
				// buffername: name,
				// index: i,
				to: [],
				get: `hardware.GetKnobValue(hardware.KNOB_${i+1})`,
			}
			return name;
		}),
		cv_ins: hardware.cv_ins.map((v, i)=>{
			let name = `dsy_cv_in${i+1}`
			nodes[name] = {
				// buffername: name,
				// index: i,
				to: [],
				get: (target == "field") ? `hardware.GetCvValue(${i})` : `hardware.GetCtrlValue(hardware.CTRL_${i+1})`,
			}
			return name;
		}),
		cv_outs: hardware.cv_outs.map((v, i)=>{
			let name = `dsy_cv_out${i+1}`
			nodes[name] = {
				// buffername: name,
				// index: i,
				from: [],
				set: `dsy_dac_write(DSY_DAC_CHN${i+1}, ${name} * 4095);`
			}
			return name;
		}),
		gate_ins: hardware.gate_ins.map((v, i)=>{
			let name = `dsy_gate_in${i+1}`
			nodes[name] = {
				// buffername: name,
				// index: i,
				to: [],
				get: (target == "field") ? `(hardware.gate_in_.State()?1.f:0.f)` : `(hardware.gate_input[hardware.GATE_IN_${i+1}].State()?1.f:0.f)`
			}
			return name;
		}),
		gate_outs: hardware.gate_outs.map((v, i)=>{
			let name = `dsy_gate_out${i+1}`
			nodes[name] = {
				// buffername: name,
				// index: i,
				from: [],
				set: (target == "field") ? `dsy_gpio_write(&hardware.gate_out_, ${name} > 0.f);` : `dsy_gpio_write(&hardware.gate_output, ${name} > 0.f);`
			}
			return name;
		})	
	}
}

function generate_gloo(gloo) {
	const {hardware, target} = gloo;

	const nodes = {}
	const daisy = generate_daisy(hardware, nodes, target);
	const gen = {}

	gloo.audio_outs = []
	gloo.has_midi_in = false
	gloo.has_midi_out = false

	gloo.nodes = nodes;
	gloo.daisy = daisy;
	gloo.gen = gen;
	gloo.nodes = nodes;


	
	gen.audio_ins = gloo.ins.map((s, i)=>{
		let name = "gen_in"+(i+1)
		let label = s.replace(/"/g, "").trim();
		let src = null;
		// figure out the src:
		if (label == "midi") {
			if (daisy.midi_ins.length) {
				src = daisy.midi_ins[0]
				gloo.has_midi_in = true;
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

	gen.audio_outs = gloo.outs.map((s, i)=>{
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
			gloo.audio_outs.push(src);
		}
		nodes[name] = {
			// name: name,
			// label: label,
			// index: i,
			src: src,
		}

		// figure out if the out buffer maps to anything:
		let map
		let match
		if (label == "midi") {
			map = daisy.midi_outs[0]
			gloo.has_midi_out = true;
		} else 
		// cv/ctrl[n]
		if (match = label.match(/^(cv|ctrl)(\d*)$/)) {
			map = daisy.cv_outs[(match[2] || 1) - 1]
			label = match[3] || label
		} else 
		// gate[n]
		if (match = label.match(/^(gate)(\d*)$/)) {
			map = daisy.gate_outs[(match[2] || 1) - 1]
			label = match[3] || label
		} else 
		// else it is audio data
		{			
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

	gen.params = gloo.params.map((param, i)=>{
		const varname = "gen_param_"+param.name;

		let src;
		let label;
		let match;
		if (match = param.name.match(/^(cv|ctrl)(\d*)_?(.+)?/)) {
			src = daisy.cv_ins[(match[2] || 1) - 1]
			label = match[3] || param.name
		}
		else if (match = param.name.match(/^(gate)(\d*)_?(.+)?/)) {
			src = daisy.gate_ins[(match[2] || 1) - 1]
			label = match[3] || param.name
		} 
		else if (match = param.name.match(/^(knob)(\d*)_?(.+)?/)) {
			src = daisy.knobs[(match[2] || 1) - 1]
			label = match[3] || param.name
		}
		else if (match = param.name.match(/^(key)(\d*)_?(.+)?/)) {
			src = daisy.keys[(match[2] || 1) - 1]
			label = match[3] || param.name
		}

		let node = Object.assign({
			varname: varname,
			label: label,
			src: src,
		}, param);
		
		nodes[varname] = node;
		if (src) {
			nodes[src].to.push(varname)
		}
		return varname;
	})

	gen.datas = gloo.datas.map((param, i)=>{
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
	daisy.knobs.forEach((name, i)=>{
		const node = nodes[name];
		if (node.to.length == 0) {
			// find next param without a src:
			while (param && !!nodes[param].src) param = gen.params[++upi];
			if (param) {
				nodes[param].src = name;
				node.to.push(param);
			}
		}
	})
	daisy.cv_ins.forEach((name, i)=>{
		const node = nodes[name];
		if (node.to.length == 0) {
			// find next param without a src:
			while (param && !!nodes[param].src) param = gen.params[++upi];
			if (param) {
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
		daisy.cv_outs.forEach((name, i)=>{
			const node = nodes[name];
			// does this output have an audio source?
			if (node.from.length) {
				available.push(name);
			} else if (available.length) {
				//node.from.push( available[i % available.length] );
				node.src = available[i % available.length];
			}
		});
	}

	const name = gloo.name;
	const struct = `

struct App_${name} : public StaticApp<App_${name}> {
	${gen.params
		.filter(name => nodes[name].src)
		.concat(daisy.cv_outs, daisy.gate_outs)
		.map(name=>`
	float ${name};`).join("")}
	${gloo.audio_outs.map(name=>`
	float ${name}[GEN_DAISY_BUFFER_SIZE];`).join("")}
	
	void init(GenDaisy& gendaisy) {
		gendaisy.gen = ${name}::create(gendaisy.samplerate, gendaisy.blocksize);
		${gen.params
			.filter(name => nodes[name].src)
			.concat(daisy.cv_outs, daisy.gate_outs)
			.map(name=>`
		${name} = 0.f;`).join("")}
	}	

	void mainloopCallback(GenDaisy& gendaisy, uint32_t t, uint32_t dt) {
		// whatever handling is needed here
		Daisy& hardware = gendaisy.hardware;
		${daisy.cv_outs.concat(daisy.gate_outs)
			.filter(name => nodes[name].src || nodes[name].from.length)
			.map((name, i)=>`
		${nodes[name].set}`).join("")}
	}

	void audioCallback(GenDaisy& gendaisy, float **hardware_ins, float **hardware_outs, size_t size) {
		Daisy& hardware = gendaisy.hardware;
		${name}::State& gen = *(${name}::State *)gendaisy.gen;
		${daisy.knobs
			.concat(daisy.switches, daisy.keys, daisy.cv_ins, daisy.gate_ins)
			.filter(name => nodes[name].to.length)
			.map(name=>`
		float ${name} = ${nodes[name].get};`).join("")}
		${gen.params.filter(name => nodes[name].src).map(name=>`
		// ${nodes[name].label}
		${name} = ${nodes[name].src}*${nodes[name].max-nodes[name].min} + ${nodes[name].min};
		gen.set_${nodes[name].name}(${name});`).join("")}
		${daisy.audio_ins.map((name, i)=>`
		float * ${name} = hardware_ins[${i}];`).join("")}
		${daisy.audio_outs.map((name, i)=>`
		float * ${name} = hardware_outs[${i}];`).join("")}
		${gloo.has_midi_in ? daisy.midi_ins.map(name=>`
		float * ${name} = gendaisy.midi.in_data;`).join("") : ''}
		float * inputs[] = { ${gen.audio_ins.map(name=>nodes[name].src).join(", ")} }; 
		float * outputs[] = { ${gen.audio_outs.map(name=>nodes[name].src).join(", ")} };
		gen.perform(inputs, outputs, size);
		${daisy.cv_outs.concat(daisy.gate_outs)
			.filter(name => nodes[name].src || nodes[name].from.length > 0)
			.map(name => nodes[name].src ? `
		${name} = ${nodes[name].src};` : `
		${name} = ${nodes[name].from.map(name=>name+"[size-1]").join(" + ")};`).join("")}
		${gloo.has_midi_out ? daisy.midi_outs.map(name=>nodes[name].from.map(name=>`
		gendaisy.midi.postperform(${name}, size);`).join("")).join("") : ''}
	}
};`
	gloo.cpp = {
		union: `App_${name} app_${name};`,
		appdef: `{"${name}", []()->void { gendaisy.reset(apps.app_${name}); } },`,
		struct: struct,
	}
	return gloo
}
