
autowatch = 1;
inlets = 2;
outlets = 3;

var path = "";
var target = "patch";
var samplerate = "48kHz";
var blocksize = "48";
var boost = 1;
var fastmath = 0;
var sep = "/";
var dict = new Dict();

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
	var errors = 0;

	// find gen~ objects in a patcher:
	function find_gens(pat) {
		dict.clear();
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
				if (obj.getattr("title")) { 
					name = obj.getattr("title").toString(); 
				} else if (obj.getattr("gen")) { 
					name = obj.getattr("gen").toString(); 
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

				if (doExport) {
					obj.message("export_json");

					// this might not work on the first pass, since it can take a little time.
					dict.import_json(obj.getattr("exportfolder") + obj.getattr("exportname") + ".json")
					
					var ast = JSON.parse(dict.stringify())
					if (ast.class == "Module") {
						var nodes = ast.block.children;
						for (var i=0; i<nodes.length; i++) {
							var node = nodes[i];
							if (node.typename == "Data") {
								//var bufname = obj.getattr(node.name)
								var bufname = obj.getattr(node.name)
								var buffer = new Buffer(bufname)
								var frames = buffer.framecount()
								var chans = buffer.channelcount()
								if (frames > 0 && chans > 0) {
									var wavname = node.name + ".wav"
									// write out that file so it can be referenced:
									buffer.send("write", obj.getattr("exportfolder") + wavname);
									//post("found buffer mapped Data", node.name, bufname, wavname, frames, chans)
								}
							} else if (node.typename == "Buffer") {
								// find the corresponding buffer~:
								var bufname = obj.getattr(node.name)
								var buffer = new Buffer(bufname)
								var frames = buffer.framecount()
								var chans = buffer.channelcount()

								if (frames < 0 || chans < 0) {
									error("oopsy: can't find buffer~ "+bufname);
									return;
								}

								// write it out:
								//buffer.send("write", obj.getattr("exportfolder") + bufname + ".wav");

    							post("oopsy: consider replacing [buffer "+node.name+"] with [data "+node.name+" "+frames+" "+chans+"]\n"); 
								if (node.name != bufname) { 
									post("and set @"+node.name, bufname, "on the gen~\n"); 
								}
								error("gen~ cannot export with [buffer] objects\n")
								errors = 1;
								return;
							}
						}
					}
				}
				
				if (doExport) {
					obj.message("exportcode");
				}
				names.push(name);

				cpps.push(path + sep + name + ".cpp")
			}
			obj = obj.nextobject;
		}
	}
	find_gens(pat, names, cpps, export_path) 

	if (errors) {
		post("oopsy: aborting due to errors\n")
		return;
	} else if (names.length < 1) {
		post("oopsy: didn't find any valid gen~ objects\n")
		return;
	} 

	var name = names.join("_")
	outlet(1, name)

	var args = [target, samplerate, "block"+blocksize].concat(cpps);
	if (boost) args.push("boost");
	if (fastmath) args.push("fastmath");
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
