{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 8,
			"minor" : 1,
			"revision" : 8,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"classnamespace" : "box",
		"rect" : [ 752.0, 158.0, 681.0, 395.0 ],
		"bglocked" : 0,
		"openinpresentation" : 0,
		"default_fontsize" : 12.0,
		"default_fontface" : 0,
		"default_fontname" : "Arial",
		"gridonopen" : 1,
		"gridsize" : [ 15.0, 15.0 ],
		"gridsnaponopen" : 1,
		"objectsnaponopen" : 1,
		"statusbarvisible" : 2,
		"toolbarvisible" : 1,
		"lefttoolbarpinned" : 0,
		"toptoolbarpinned" : 0,
		"righttoolbarpinned" : 0,
		"bottomtoolbarpinned" : 0,
		"toolbars_unpinned_last_save" : 0,
		"tallnewobj" : 0,
		"boxanimatetime" : 200,
		"enablehscroll" : 1,
		"enablevscroll" : 1,
		"devicewidth" : 0.0,
		"description" : "",
		"digest" : "",
		"tags" : "",
		"style" : "",
		"subpatcher_template" : "",
		"assistshowspatchername" : 0,
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-15",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 95.0, 203.0, 45.0, 22.0 ],
					"text" : "scatter"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-13",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 95.0, 152.0, 46.0, 22.0 ],
					"text" : "dattoro"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-11",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 95.0, 257.0, 45.0, 22.0 ],
					"text" : "modfm"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-10",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 24.0, 203.0, 61.0, 22.0 ],
					"text" : "crossover"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-6",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 24.0, 257.0, 44.0, 22.0 ],
					"text" : "squine"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-1",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 24.0, 152.0, 31.0, 22.0 ],
					"text" : "giga"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-22",
					"linecount" : 4,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 460.600008249282837, 77.59999805688858, 175.0, 60.0 ],
					"text" : "A midi-enabled multi-app Oopsy program will respond to program-change MIDI events to by switching between apps "
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-20",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 471.000008404254913, 290.59999805688858, 47.0, 22.0 ],
					"text" : "midiout"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-19",
					"maxclass" : "number",
					"minimum" : 20,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 546.500008404254913, 160.199996113777161, 50.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-16",
					"maxclass" : "toggle",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "int" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 502.500008404254913, 160.199996113777161, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-9",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 502.500008404254913, 186.199996113777161, 63.0, 22.0 ],
					"text" : "metro 250"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-8",
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 502.500008404254913, 236.99999725818634, 50.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-4",
					"maxclass" : "newobj",
					"numinlets" : 5,
					"numoutlets" : 4,
					"outlettype" : [ "int", "", "", "int" ],
					"patching_rect" : [ 502.500008404254913, 211.399996876716614, 69.0, 22.0 ],
					"text" : "counter 1 8"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-2",
					"maxclass" : "newobj",
					"numinlets" : 7,
					"numoutlets" : 2,
					"outlettype" : [ "int", "" ],
					"patching_rect" : [ 471.000008404254913, 263.399997651576996, 82.0, 22.0 ],
					"text" : "midiformat"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-7",
					"linecount" : 3,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 28.0, 77.59999805688858, 402.0, 47.0 ],
					"text" : "Oopsy will snoop for all gen~ objects in the same patcher or subpatchers, producing a 'multi-app' binary for the Daisy hardwares that support app switching, e.g. Patch, Field, Petal. "
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial Italic",
					"fontsize" : 18.0,
					"id" : "obj-5",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 28.0, 34.5, 266.0, 27.0 ],
					"text" : "Multi-app"
				}

			}
, 			{
				"box" : 				{
					"args" : [ "@samplerate", "48kHz" ],
					"bgmode" : 0,
					"border" : 0,
					"clickthrough" : 0,
					"enablehscroll" : 0,
					"enablevscroll" : 0,
					"id" : "obj-3",
					"lockeddragscroll" : 0,
					"maxclass" : "bpatcher",
					"name" : "oopsy.maxpat",
					"numinlets" : 1,
					"numoutlets" : 0,
					"offset" : [ 0.0, 0.0 ],
					"patching_rect" : [ 200.0, 142.0, 128.0, 128.0 ],
					"viewvisibility" : 1
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-9", 0 ],
					"source" : [ "obj-16", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-9", 1 ],
					"source" : [ "obj-19", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-20", 0 ],
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-8", 0 ],
					"source" : [ "obj-4", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 3 ],
					"source" : [ "obj-8", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-4", 0 ],
					"source" : [ "obj-9", 0 ]
				}

			}
 ],
		"parameters" : 		{
			"obj-10::obj-11" : [ "live.gain~[2]", "live.gain~", 0 ],
			"obj-10::obj-12" : [ "live.gain~[1]", "live.gain~", 0 ],
			"obj-10::obj-7::obj-21::obj-6" : [ "live.tab[4]", "live.tab[1]", 0 ],
			"obj-10::obj-7::obj-35" : [ "[6]", "Level", 0 ],
			"obj-10::obj-9::obj-32" : [ "live.text[12]", "FILTER", 0 ],
			"obj-10::obj-9::obj-33" : [ "live.text[11]", "FILTER", 0 ],
			"obj-10::obj-9::obj-34" : [ "live.text[13]", "FILTER", 0 ],
			"obj-11::obj-11" : [ "live.gain~[4]", "live.gain~", 0 ],
			"obj-11::obj-9::obj-32" : [ "live.text[14]", "FILTER", 0 ],
			"obj-11::obj-9::obj-33" : [ "live.text[15]", "FILTER", 0 ],
			"obj-11::obj-9::obj-34" : [ "live.text[16]", "FILTER", 0 ],
			"obj-13::obj-1::obj-32" : [ "live.text[23]", "FILTER", 0 ],
			"obj-13::obj-1::obj-33" : [ "live.text[22]", "FILTER", 0 ],
			"obj-13::obj-1::obj-34" : [ "live.text[24]", "FILTER", 0 ],
			"obj-13::obj-6" : [ "live.gain~[6]", "live.gain~", 0 ],
			"obj-13::obj-7::obj-21::obj-6" : [ "live.tab[5]", "live.tab[1]", 0 ],
			"obj-13::obj-7::obj-35" : [ "[7]", "Level", 0 ],
			"obj-15::obj-4::obj-32" : [ "live.text[27]", "FILTER", 0 ],
			"obj-15::obj-4::obj-33" : [ "live.text[28]", "FILTER", 0 ],
			"obj-15::obj-4::obj-34" : [ "live.text[29]", "FILTER", 0 ],
			"obj-1::obj-11" : [ "live.gain~", "live.gain~", 0 ],
			"obj-1::obj-3::obj-32" : [ "live.text[6]", "FILTER", 0 ],
			"obj-1::obj-3::obj-33" : [ "live.text[4]", "FILTER", 0 ],
			"obj-1::obj-3::obj-34" : [ "live.text[2]", "FILTER", 0 ],
			"obj-1::obj-6::obj-21::obj-6" : [ "live.tab[3]", "live.tab[1]", 0 ],
			"obj-1::obj-6::obj-35" : [ "[5]", "Level", 0 ],
			"obj-3::obj-32" : [ "live.text[5]", "FILTER", 0 ],
			"obj-3::obj-33" : [ "live.text[1]", "FILTER", 0 ],
			"obj-3::obj-34" : [ "live.text[3]", "FILTER", 0 ],
			"obj-6::obj-3::obj-32" : [ "live.text[7]", "FILTER", 0 ],
			"obj-6::obj-3::obj-33" : [ "live.text[8]", "FILTER", 0 ],
			"obj-6::obj-3::obj-34" : [ "live.text[9]", "FILTER", 0 ],
			"parameterbanks" : 			{

			}
,
			"parameter_overrides" : 			{
				"obj-10::obj-11" : 				{
					"parameter_longname" : "live.gain~[2]"
				}
,
				"obj-10::obj-7::obj-21::obj-6" : 				{
					"parameter_longname" : "live.tab[4]"
				}
,
				"obj-10::obj-7::obj-35" : 				{
					"parameter_longname" : "[6]"
				}
,
				"obj-10::obj-9::obj-32" : 				{
					"parameter_longname" : "live.text[12]"
				}
,
				"obj-10::obj-9::obj-33" : 				{
					"parameter_longname" : "live.text[11]"
				}
,
				"obj-10::obj-9::obj-34" : 				{
					"parameter_longname" : "live.text[13]"
				}
,
				"obj-11::obj-11" : 				{
					"parameter_longname" : "live.gain~[4]"
				}
,
				"obj-11::obj-9::obj-32" : 				{
					"parameter_longname" : "live.text[14]"
				}
,
				"obj-11::obj-9::obj-33" : 				{
					"parameter_longname" : "live.text[15]"
				}
,
				"obj-11::obj-9::obj-34" : 				{
					"parameter_longname" : "live.text[16]"
				}
,
				"obj-13::obj-1::obj-32" : 				{
					"parameter_longname" : "live.text[23]"
				}
,
				"obj-13::obj-1::obj-33" : 				{
					"parameter_longname" : "live.text[22]"
				}
,
				"obj-13::obj-1::obj-34" : 				{
					"parameter_longname" : "live.text[24]"
				}
,
				"obj-13::obj-6" : 				{
					"parameter_longname" : "live.gain~[6]"
				}
,
				"obj-13::obj-7::obj-21::obj-6" : 				{
					"parameter_longname" : "live.tab[5]"
				}
,
				"obj-13::obj-7::obj-35" : 				{
					"parameter_longname" : "[7]"
				}
,
				"obj-15::obj-4::obj-32" : 				{
					"parameter_longname" : "live.text[27]"
				}
,
				"obj-15::obj-4::obj-33" : 				{
					"parameter_longname" : "live.text[28]"
				}
,
				"obj-15::obj-4::obj-34" : 				{
					"parameter_longname" : "live.text[29]"
				}
,
				"obj-1::obj-3::obj-32" : 				{
					"parameter_longname" : "live.text[6]"
				}
,
				"obj-1::obj-3::obj-33" : 				{
					"parameter_longname" : "live.text[4]"
				}
,
				"obj-1::obj-3::obj-34" : 				{
					"parameter_longname" : "live.text[2]"
				}
,
				"obj-3::obj-32" : 				{
					"parameter_longname" : "live.text[5]"
				}
,
				"obj-6::obj-3::obj-32" : 				{
					"parameter_longname" : "live.text[7]"
				}
,
				"obj-6::obj-3::obj-33" : 				{
					"parameter_longname" : "live.text[8]"
				}
,
				"obj-6::obj-3::obj-34" : 				{
					"parameter_longname" : "live.text[9]"
				}

			}
,
			"inherited_shortname" : 1
		}
,
		"dependency_cache" : [ 			{
				"name" : "oopsy.maxpat",
				"bootpath" : "~/Documents/Max 8/Packages/oopsy/patchers",
				"patcherrelativepath" : "../patchers",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "oopsy.snoop.js",
				"bootpath" : "~/Documents/Max 8/Packages/oopsy/javascript",
				"patcherrelativepath" : "../javascript",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "oopsy.node4max.js",
				"bootpath" : "~/Documents/Max 8/Packages/oopsy/javascript",
				"patcherrelativepath" : "../javascript",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "giga.maxpat",
				"bootpath" : "~/Documents/Max 8/Packages/oopsy/examples",
				"patcherrelativepath" : ".",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "oopsy.ctrl.smooth3.gendsp",
				"bootpath" : "~/Documents/Max 8/Packages/oopsy/code",
				"patcherrelativepath" : "../code",
				"type" : "gDSP",
				"implicit" : 1
			}
, 			{
				"name" : "oopsy.ctrl.smooth2.gendsp",
				"bootpath" : "~/Documents/Max 8/Packages/oopsy/code",
				"patcherrelativepath" : "../code",
				"type" : "gDSP",
				"implicit" : 1
			}
, 			{
				"name" : "demosound.maxpat",
				"bootpath" : "C74:/help/msp",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "sine.svg",
				"bootpath" : "C74:/media/max/picts/m4l-picts",
				"type" : "svg",
				"implicit" : 1
			}
, 			{
				"name" : "saw.svg",
				"bootpath" : "C74:/media/max/picts/m4l-picts",
				"type" : "svg",
				"implicit" : 1
			}
, 			{
				"name" : "square.svg",
				"bootpath" : "C74:/media/max/picts/m4l-picts",
				"type" : "svg",
				"implicit" : 1
			}
, 			{
				"name" : "random.svg",
				"bootpath" : "C74:/media/max/picts/m4l-picts",
				"type" : "svg",
				"implicit" : 1
			}
, 			{
				"name" : "interfacecolor.js",
				"bootpath" : "C74:/interfaces",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "squine.maxpat",
				"bootpath" : "~/Documents/Max 8/Packages/oopsy/examples",
				"patcherrelativepath" : ".",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "oopsy.cv2hz.gendsp",
				"bootpath" : "~/Documents/Max 8/Packages/oopsy/code",
				"patcherrelativepath" : "../code",
				"type" : "gDSP",
				"implicit" : 1
			}
, 			{
				"name" : "crossover.maxpat",
				"bootpath" : "~/Documents/Max 8/Packages/oopsy/examples",
				"patcherrelativepath" : ".",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "modfm.maxpat",
				"bootpath" : "~/Documents/Max 8/Packages/oopsy/examples",
				"patcherrelativepath" : ".",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "dattoro.maxpat",
				"bootpath" : "~/Documents/Max 8/Packages/oopsy/examples",
				"patcherrelativepath" : ".",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "scatter.maxpat",
				"bootpath" : "~/Documents/Max 8/Packages/oopsy/examples",
				"patcherrelativepath" : ".",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jsui_2dvectorctrl.js",
				"bootpath" : "C74:/jsui",
				"type" : "TEXT",
				"implicit" : 1
			}
 ],
		"autosave" : 0
	}

}
