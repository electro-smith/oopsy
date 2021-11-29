#!/usr/bin/env node

// TODO -- we'll reimplement the filter / map stuff here if just for the exclusion capabilities

const seed_definitions = {

};

const patchsm_definitions = {

};

function generate_header(board_description_object)
{
  let target = board_description_object;

  let components = target.components;
  let parents = target.parents || {};

  for (let comp in parents)
  {
    parents[comp].is_parent = true;
  }
  Object.assign(components, components, parents);

  som = target.som || 'seed';

  let temp_defs = {
    seed: seed_definitions,
    patch_sm: patchsm_definitions,
  };

  assert(som in temp_defs, `Unkown som "${som}"`);

  definitions = temp_defs[som];

  target.components = components;
  target.name = target.name || 'custom';
  target.aliases = target.aliases || {};

  if ("display" in target)
  {
    // shouldn't this only be done if the display property is empty?
    target.display = {
      driver: "daisy::SSD130x4WireSpi128x64Driver",
      config: [],
      dim: [128, 64],
    };

    target.defines.OOPSY_TARGET_HAS_OLED = 1;
    target.defines.OOPSY_OLED_DISPLAY_WIDTH = target.display.dim[0]
    target.defines.OOPSY_OLED_DISPLAY_HEIGHT = target.display.dim[1]
  }

  let replacements = {}
  replacements.name = target.name;
  replacements.som = som;
  replacements.external_codecs = target.external_codecs || [];
  replacements.som_class = som == 'seed' ? 'daisy::DaisySeed' : 'daisy::patch_sm::DaisyPatchSM';

  replacements.display_conditional = 'display' in target ? '#include "dev/oled_ssd130x.h' : '';
  replacements.target_name = target.name; // TODO -- redundant?
  replacements.init = filter_map_template(components, 'init', 'default', true);

  replacements.cd4021 = filter_map_init(components, 'component', 'CD4021', key_exclude='default', match_exclude=True)
  replacements.i2c = filter_map_init(components, 'component', 'i2c', key_exclude='default', match_exclude=True)
  replacements.pca9685 = filter_map_init(components, 'component', 'PCA9685', key_exclude='default', match_exclude=True)
  replacements.switch = filter_map_init(components, 'component', 'Switch', key_exclude='default', match_exclude=True)
  replacements.gatein = filter_map_init(components, 'component', 'GateIn', key_exclude='default', match_exclude=True)
  replacements.encoder = filter_map_init(components, 'component', 'Encoder', key_exclude='default', match_exclude=True)
  replacements.switch3 = filter_map_init(components, 'component', 'Switch3', key_exclude='default', match_exclude=True)
  replacements.analogcount = len(list(filter_matches(components, 'component', ['AnalogControl', 'AnalogControlBipolar', 'CD4051'], key_exclude='default', match_exclude=True)))

  replacements.init_single = filter_map_ctrl(components, 'component', ['AnalogControl', 'AnalogControlBipolar', 'CD4051'], 'init_single', key_exclude='default', match_exclude=True)
  replacements.ctrl_init = filter_map_ctrl(components, 'component', ['AnalogControl', 'AnalogControlBipolar'], 'map_init', key_exclude='default', match_exclude=True)	

  replacements.ctrl_mux_init = filter_map_init(components, 'component', 'CD4051AnalogControl', key_exclude='default', match_exclude=True)

  replacements.led = filter_map_init(components, 'component', 'Led', key_exclude='default', match_exclude=True)
  replacements.rgbled = filter_map_init(components, 'component', 'RgbLed', key_exclude='default', match_exclude=True)
  replacements.gateout = filter_map_init(components, 'component', 'GateOut', key_exclude='default', match_exclude=True)
  replacements.dachandle = filter_map_init(components, 'component', 'CVOuts', key_exclude='default', match_exclude=True)
  
  replacements.display = !('display' in target) ? '' : `
  daisy::OledDisplay<${target.display.driver}>::Config display_config;
  display_config.driver_config.transport_config.Defaults();
  display.Init(display_config);
  display.Fill(0);
  display.Update();
  `

  replacements.process = filter_map_template(components, 'process', key_exclude='default', match_exclude=True)
  // There's also this after {process}. I don't see any meta in the defaults json at this time. Is this needed?
  // ${components.filter((e) => e.meta).map((e) => e.meta.map(m=>`${template(m, e)}`).join("")).join("")}
  replacements.loopprocess = filter_map_template(components, 'loopprocess', key_exclude='default', match_exclude=True)

  replacements.postprocess = filter_map_template(components, 'postprocess', key_exclude='default', match_exclude=True)
  replacements.displayprocess = filter_map_template(components, 'display', key_exclude='default', match_exclude=True)
  replacements.hidupdaterates = filter_map_template(components, 'updaterate', key_exclude='default', match_exclude=True)

}