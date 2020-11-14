// Node4Max wrapper of the gen2daisy.node.js script

const path = require("path")
// 	fs = require("fs"),
// 	os = require("os"),
// 	assert = require("assert");
// const {exec, spawn} = require("child_process");

const maxAPI = require("max-api");
const run = require(path.join(__dirname, "..", "source", "oopsy.js"));

try {

	// I'd like to filter the stdout/stderr streams here
	// especially for the DFU upload stuff (no point showing errors that are not really errors!)

	run(...process.argv.slice(2))
} catch(e) {
	maxAPI.post(e.message ? e.message : e, maxAPI.POST_LEVELS.ERROR);
}