
autowatch = 1;
inlets = 2;
outlets = 2;

var path = ""
var target = "patch"
var sep = "/"

function bang() {
	configure(true);
}

function configure(doExport) {
	var pat = this.patcher.parentpatcher;
	if (!pat.filepath) {
		error("oopsy: your patcher needs to be saved first\n");
		pat.message("write");
		return false;
	}
	var export_path = extractFilepath(pat.filepath);

	var default_name = pat.name || "gen";

	// send message out to convert this path
	// response will update the variable `path`:
	outlet(1, export_path);

	var names = [];
	var cpps = [];

	// iterate all gen~ objects in the patcher
	// to set their export name, path, and scripting name
	var gen = pat.firstobject;
	while (gen) {
		if (gen.maxclass.toString() == "gen~") {
			var name = default_name;
			if (gen.getattr("exportname")) { 
				name = gen.getattr("exportname").toString(); 
			} else if (gen.getattr("title")) { 
				name = gen.getattr("title").toString(); 
			} else if (gen.getattr("gen")) { 
				name = gen.getattr("gen").toString(); 
			} else if (gen.varname.toString()) {
				name = gen.varname.toString();
			}
			// sanitize:
			name = safename(name);
			// sanitize via scripting name too:
			while (name != gen.varname.toString()) {
				// try to apply it:
				gen.varname = name;
				name = safename(gen.varname);
			}
			// ensure exportname matches:
			gen.message("exportname", name);
			// ensure each gen~ has an export path configured:
			gen.message("exportfolder", export_path);
			
			if (doExport) {
				gen.message("exportcode");
			}
			names.push(name);

			cpps.push(path + sep + name + ".cpp")
		}
		gen = gen.nextobject;
	}

	var args = [target].concat(cpps);
	outlet(0, args)
}

// convert names to use only characters safe for variable names
function safename(s) {
	if (/[^a-zA-Z\d_]/.test(s)) {
		return s.replace(/[^a-zA-Z\d_]/g, function(x) {
			return '_' + x.charCodeAt(0).toString(16);
		});
	} else {
		return s;
	}
}

// get the containing folder from a filepath:
function extractFilepath(path) {
	var x;
	x = path.lastIndexOf('/');
	if (x >= 0) { // Unix-based path
		sep = "/"
		return path.substr(0, x+1);
	}
	x = path.lastIndexOf('\\');
	if (x >= 0) { // Windows-based path
		sep = "\\"
		return path.substr(0, x+1);
	}
	return path; // just the filename
}
