
autowatch = 1;
inlets = 2;
outlets = 3;

var path = "";
var target = "patch";
var samplerate = "48";
var boost = 0;
var sep = "/";

function bang() {
	configure(true);
}

function configure(doExport) {
	var pat = this.patcher.parentpatcher;
	var topmost_pat = pat;
	while (topmost_pat.parentpatcher) {
		topmost_pat = topmost_pat.parentpatcher;
	}
	if (!topmost_pat.filepath) {
		error("oopsy: your patcher needs to be saved first\n");
		pat.message("write");
		return false;
	}
	var export_path = extractFilepath(topmost_pat.filepath);
	// send message out to convert this path
	// response will update the variable `path`:
	outlet(2, export_path);

	var names = [];
	var cpps = [];

	// find gen~ objects in a patcher:
	function find_gens(pat) {
		var default_name = pat.name || "gen";
		// iterate all gen~ objects in the patcher
		// to set their export name, path, and scripting name
		var obj = pat.firstobject;
		while (obj) {
			if (obj.maxclass.toString() == "patcher") {
				var subpat = obj.subpatcher()
				if (subpat) find_gens(subpat);
				
			} else if (obj.maxclass.toString() == "gen~") {
				var name = default_name;
				if (obj.getattr("exportname")) { 
					name = obj.getattr("exportname").toString(); 
				} else if (obj.getattr("title")) { 
					name = obj.getattr("title").toString(); 
				} else if (obj.getattr("gen")) { 
					name = obj.getattr("gen").toString(); 
				} else if (obj.varname.toString()) {
					name = obj.varname.toString();
				}
				// sanitize:
				name = safename(name);
				// sanitize via scripting name too:
				while (name != obj.varname.toString()) {
					// try to apply it:
					obj.varname = name;
					name = safename(obj.varname);
				}
				// ensure exportname matches:
				obj.message("exportname", name);
				// ensure each gen~ has an export path configured:
				obj.message("exportfolder", export_path);
				
				if (doExport) obj.message("exportcode");
				
				names.push(name);

				cpps.push(path + sep + name + ".cpp")
			}
			obj = obj.nextobject;
		}
	}
	find_gens(pat, names, cpps, export_path) 

	if (names.length < 1) {
		post("oopsy: didn't find any gen~ objects\n")
		return;
	} 

	var name = names.join("_")
	outlet(1, name)

	var args = [target, samplerate].concat(cpps);
	if (boost) args.push("boost");
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
