// Node4Max wrapper of the gen2daisy.node.js script

const path = require("path"), 
	os = require("os")
const maxAPI = require("max-api");
const run = require(path.join(__dirname, "..", "source", "oopsy.js"));

let filter
if (os.platform() == "win32") {
	filter = function(str) {
		maxAPI.outlet(str)
	}
} else {
	filter = function(str) {
		let match
		if (match = label.match(/^Building/gm)) {
			maxAPI.outlet(str)
		} else {
			maxAPI.outlet("--" + str)
		}
	}
}

// duplicate stdout here so we can filter it and display more useful things in Max:
// dup stdout to our handler:
process.stdout.write = (function() {
	const stdout_write = process.stdout.write;
	return function(str) {
		stdout_write.apply(process.stdout, arguments);
		let match
		if (match = str.match(/^oopsy (.*)/i)) {
			maxAPI.outlet(match[1])
		} 
	}
})();

try {
	run(...process.argv.slice(2))
} catch(e) {
	maxAPI.post(e.message ? e.message : e, maxAPI.POST_LEVELS.ERROR);
}