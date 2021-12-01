#!/usr/bin/env node

// TODO -- we'll reimplement the filter / map stuff here if just for the exclusion capabilities

const assert = require("assert");
const path = require("path");
const seed_defs = require(path.join(__dirname, "component_defs.json"));
const patchsm_defs = require(path.join(__dirname, "component_defs_patchsm.json"));

var global_definitions;

// .filter for objects that returns array
Object.filter = (obj, predicate) => 
    Object.keys(obj).filter(key => predicate(obj[key]))
    .map(key => obj[key]);

function generateCodecs(external_codecs)
{
  codec_string = `
  // External Codec Initialization
  daisy::SaiHandle::Config sai_config[${1 + external_codecs.length}];

  // Internal Codec
  sai_config[0].periph          = daisy::SaiHandle::Config::Peripheral::SAI_1;
  sai_config[0].sr              = daisy::SaiHandle::Config::SampleRate::SAI_48KHZ;
  sai_config[0].bit_depth       = daisy::SaiHandle::Config::BitDepth::SAI_24BIT;
  sai_config[0].a_sync          = daisy::SaiHandle::Config::Sync::MASTER;
  sai_config[0].b_sync          = daisy::SaiHandle::Config::Sync::SLAVE;
  sai_config[0].a_dir           = daisy::SaiHandle::Config::Direction::TRANSMIT;
  sai_config[0].b_dir           = daisy::SaiHandle::Config::Direction::RECEIVE;
  sai_config[0].pin_config.fs   = {DSY_GPIOE, 4};
  sai_config[0].pin_config.mclk = {DSY_GPIOE, 2};
  sai_config[0].pin_config.sck  = {DSY_GPIOE, 5};
  sai_config[0].pin_config.sa   = {DSY_GPIOE, 6};
  sai_config[0].pin_config.sb   = {DSY_GPIOE, 3};
  `

  for (let i = 0; i < external_codecs.length; i++)
  {
    codec_string += `
  sai_config[${i + 1}].periph          = daisy::SaiHandle::Config::Peripheral::${external_codecs[i].periph};
  sai_config[${i + 1}].sr              = daisy::SaiHandle::Config::SampleRate::SAI_48KHZ;
  sai_config[${i + 1}].bit_depth       = daisy::SaiHandle::Config::BitDepth::SAI_24BIT;
  sai_config[${i + 1}].a_sync          = daisy::SaiHandle::Config::Sync::${external_codecs[i].a_sync};
  sai_config[${i + 1}].b_sync          = daisy::SaiHandle::Config::Sync::${external_codecs[i].b_sync};
  sai_config[${i + 1}].a_dir           = daisy::SaiHandle::Config::Direction::${external_codecs[i].a_dir};
  sai_config[${i + 1}].b_dir           = daisy::SaiHandle::Config::Direction::${external_codecs[i].b_dir};
  sai_config[${i + 1}].pin_config.fs   = som.GetPin(${external_codecs[i].pin.fs});
  sai_config[${i + 1}].pin_config.mclk = som.GetPin(${external_codecs[i].pin.mclk});
  sai_config[${i + 1}].pin_config.sck  = som.GetPin(${external_codecs[i].pin.sck});
  sai_config[${i + 1}].pin_config.sa   = som.GetPin(${external_codecs[i].pin.sa});
  sai_config[${i + 1}].pin_config.sb   = som.GetPin(${external_codecs[i].pin.sb});
  `;
  }

  codec_string += `
  daisy::SaiHandle sai_handle[${external_codecs.length + 1}];
  sai_handle[0].Init(sai_config[0]);
  `;

  for (let i = 0; i < external_codecs.length; i++)
  {
    codec_string += `
  sai_handle[${i + 1}].Init(sai_config[${i + 1}]);
  `;
  }

  codec_string += `
  dsy_gpio_pin codec_reset_pin = som.GetPin(29);
  daisy::Ak4556::Init(codec_reset_pin);

  daisy::AudioHandle::Config cfg;
  cfg.blocksize  = 48;
  cfg.samplerate = daisy::SaiHandle::Config::SampleRate::SAI_48KHZ;
  cfg.postgain   = 0.5f;
  som.audio_handle.Init(
    cfg, 
    sai_handle[0]
  `;

  for (let i = 0; i < external_codecs.length; i++)
  {
    if (i < external_codecs.length - 1)
      codec_string += ",\n    ";
    codec_string += `sai_handle[${i + 1}]`;
  }

  codec_string += `
    );
  `;

  return codec_string;
}

function stringFormatMap(template, formatMap)
{
  if (typeof template === 'undefined')
    return '';
  const format_match = /{\s*([^{}\s]*)\s*}/g;
  const open_curly = /{{/g;
  const close_curly = /}}/g;
  let pass1 = template.replace(open_curly, () => {
    return '{'
  });
  let pass2 = pass1.replace(close_curly, () => {
    return '}'
  });
  let pass3 = pass2.replace(format_match, (substring, value, index) => {
    return formatMap[value] || '';
  });
  return pass3;
}

exports.format_map = stringFormatMap;

function map_load(key, item)
{
  item.name = key;
  assert(item.component in global_definitions, `Undefined component kind "${item.component}"`);
  component = global_definitions[item.component];
  
  for (property in component)
  {
    if (!(property in item))
      item[property] = component[property];
  }

  return item;
}

function filter_match(sequence, key, match, key_exclude = null, match_exclude = null)
{
  if (key_exclude !== null && match_exclude !== null)
  {
    return Object.filter(sequence, item => key in item && key == match && (!(key_exclude in item) || (key_exclude in item && key_exclude != match_exclude)));
  }
  else
    return Object.filter(sequence, item => key in item && key == match);
}

function filter_matches(sequence, key, matches, key_exclude=null, match_exclude=null)
{
  if (key_exclude !== null && match_exclude !== null)
  {
    return Object.filter(sequence, item => {
      let items_key = item[key] || '';
      let items_key_exclude = item[key_exclude] || '';
      return matches.includes(items_key) && items_key_exclude != match_exclude;
    });
  }
  else
  {
    return Object.filter(sequence, item => {
      let items_key = item[key] || '';
      return matches.includes(items_key);
    });
  }
}

function filter_has(sequence, key, key_exclude=null, match_exclude=null)
{
  if (key_exclude !== null && match_exclude !== null)
  {
    return Object.filter(sequence, item => {
      let items_key_exclude = item[key_exclude] || '';
      return key in item && items_key_exclude != match_exclude;
    });
  }
  else
  {
    return Object.filter(sequence, item => key in item);
  }
}

function filter_map_init(set, key, match, key_exclude=null, match_exclude=null)
{
  filtered = filter_match(set, key, match, key_exclude, match_exclude);
  return filtered.map(item => stringFormatMap(item.map_init, item)).join("\n    ");
}

function filter_map_set(set, key, match, key_exclude=null, match_exclude=null)
{
  filtered = filter_match(set, key, match, key_exclude, match_exclude);
  return filtered.map(item => stringFormatMap(stringFormatMap(item.mapping[0].set, item.mapping[0].name, item))).join("\n    ");
}

function filter_map_ctrl(set, key, matches, init_key, key_exclude=null, match_exclude=null)
{
  set = filter_matches(set, key, matches, key_exclude, match_exclude);
  set = set.map((item, i) => Object.assign(item, item, {i: i}));
  return set.map(item => stringFormatMap(item[init_key], item)).join("\n    ");
}

function filter_map_template(set, name, key_exclude=null, match_exclude=null)
{
  filtered = filter_has(set, name, key_exclude, match_exclude);
  return filtered.map(item => stringFormatMap(item.name, item)).join("\n    ");
}

function flatten_pin_dicts(comp)
{
  flattened = {};
  Object.assign(flattened, comp); // maybe not actually necessary to copy
  if ('pin' in comp && typeof comp.pin === 'object')
  {
    for (property in comp.pin)
    {
      flattened[`pin_${property}`] = comp.pin[property];
    }
  }
  return flattened;
}

function flatten_index_dicts(comp)
{
  flattened = {};
  Object.assign(flattened, comp); // maybe not actually necessary to copy
  if ('index' in comp && typeof comp.pin === 'object')
  {
    for (property in comp.pin)
    {
      flattened[`index_${property}`] = comp.pin[property];
    }
  }
  return flattened;
}

exports.generate_header = function generate_header(board_description_object)
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
    seed: seed_defs,
    patch_sm: patchsm_defs,
  };

  assert(som in temp_defs, `Unkown som "${som}"`);

  definitions = temp_defs[som];
  global_definitions = definitions;

  for (let comp in components)
  {
    components[comp] = map_load(comp, components[comp]);
    components[comp] = flatten_pin_dicts(components[comp]);
    components[comp] = flatten_index_dicts(components[comp]);
  }

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
  replacements.init = filter_map_template(components, 'init', 'is_default', true);

  replacements.cd4021 = filter_map_init(components, 'component', 'CD4021', key_exclude='is_default', match_exclude=true);
  replacements.i2c = filter_map_init(components, 'component', 'i2c', key_exclude='is_default', match_exclude=true);
  replacements.pca9685 = filter_map_init(components, 'component', 'PCA9685', key_exclude='is_default', match_exclude=true);
  replacements.switch = filter_map_init(components, 'component', 'Switch', key_exclude='is_default', match_exclude=true);
  replacements.gatein = filter_map_init(components, 'component', 'GateIn', key_exclude='is_default', match_exclude=true);
  replacements.encoder = filter_map_init(components, 'component', 'Encoder', key_exclude='is_default', match_exclude=true);
  replacements.switch3 = filter_map_init(components, 'component', 'Switch3', key_exclude='is_default', match_exclude=true);
  replacements.analogcount = filter_matches(components, 'component', ['AnalogControl', 'AnalogControlBipolar', 'CD4051'], key_exclude='is_default', match_exclude=true).length;

  replacements.init_single = filter_map_ctrl(components, 'component', ['AnalogControl', 'AnalogControlBipolar', 'CD4051'], 'init_single', key_exclude='is_default', match_exclude=true);
  replacements.ctrl_init = filter_map_ctrl(components, 'component', ['AnalogControl', 'AnalogControlBipolar'], 'map_init', key_exclude='is_default', match_exclude=true);

  replacements.ctrl_mux_init = filter_map_init(components, 'component', 'CD4051AnalogControl', key_exclude='is_default', match_exclude=true);

  replacements.led = filter_map_init(components, 'component', 'Led', key_exclude='is_default', match_exclude=true);
  replacements.rgbled = filter_map_init(components, 'component', 'RgbLed', key_exclude='is_default', match_exclude=true);
  replacements.gateout = filter_map_init(components, 'component', 'GateOut', key_exclude='is_default', match_exclude=true);
  replacements.dachandle = filter_map_init(components, 'component', 'CVOuts', key_exclude='is_default', match_exclude=true);
  
  replacements.display = !('display' in target) ? '' : `
  daisy::OledDisplay<${target.display.driver}>::Config display_config;
  display_config.driver_config.transport_config.Defaults();
  display.Init(display_config);
  display.Fill(0);
  display.Update();
  `;

  replacements.process = filter_map_template(components, 'process', key_exclude='is_default', match_exclude=true);
  // There's also this after {process}. I don't see any meta in the defaults json at this time. Is this needed?
  // ${components.filter((e) => e.meta).map((e) => e.meta.map(m=>`${template(m, e)}`).join("")).join("")}
  replacements.loopprocess = filter_map_template(components, 'loopprocess', key_exclude='is_default', match_exclude=true);

  replacements.postprocess = filter_map_template(components, 'postprocess', key_exclude='is_default', match_exclude=true);
  replacements.displayprocess = filter_map_template(components, 'display', key_exclude='is_default', match_exclude=true);
  replacements.hidupdaterates = filter_map_template(components, 'updaterate', key_exclude='is_default', match_exclude=true);

  component_decls = Object.filter(components, item => item.default || false);
  component_decls = Object.filter(component_decls, item => 'typename' in item);
  replacements.comps = component_decls.map(item => `${stringFormatMap(item.typename, item)} ${item.name}`).join(";\n  ") + ';';
  non_class_decls = Object.filter(component_decls, item => 'non_class_decl' in item);
  replacements.non_class_declarations = non_class_decls.map(item => stringFormatMap(item.non_class_decl, item)).join("\n");

  replacements.dispdec = 'display' in target ? `daisy::OledDisplay<${target.display.driver}> display;` : "";

  let header = `
#ifndef __JSON2DAISY_${replacements.name.toUpperCase()}_H__
#define __JSON2DAISY_${replacements.name.toUpperCase()}_H__

#include "daisy_${replacements.som}.h"
${replacements.som == 'seed' ? '#include "dev/codec_ak4556.h"' : ''}

#define ANALOG_COUNT ${replacements.analogcount}

namespace json2daisy {

${replacements.non_class_declarations}

struct Daisy${replacements.name[0].toUpperCase()}${replacements.name.slice(1)} {

  /** Initializes the board according to the JSON board description
   *  \\param boost boosts the clock speed from 400 to 480 MHz
   */
  void Init(bool boost=true) 
  {
    ${replacements.som == 'seed' ? `som.Configure();
    som.Init(boost);` : `som.Init();`}
    ${replacements.init}
    ${replacements.i2c != '' ? '    // i2c\n    ' + replacements.i2c : ''}
    ${replacements.pca9685 != '' ? '    // LED Drivers\n    ' + replacements.pca9685 : ''}
    ${replacements.switch != '' ? '    // Switches\n    ' + replacements.switch : ''}
    ${replacements.switch3 != '' ? '    // SPDT Switches\n    ' + replacements.switch3 : ''}
    ${replacements.cd4021 != '' ? '    // Muxes\n    ' + replacements.cd4021 : ''}
    ${replacements.gatein != '' ? '    // Gate ins\n    ' + replacements.gatein : ''}
    ${replacements.encoder != '' ? '    // Rotary encoders\n    ' + replacements.encoder : ''}
    ${replacements.init_single != '' ? '    // Single channel ADC initialization\n    ' + replacements.init_single : ''}
    ${replacements.som == 'seed' ? 'som.adc.Init(cfg, ANALOG_COUNT);' : ''}
    ${replacements.ctrl_init != '' ? '    // AnalogControl objects\n    ' + replacements.ctrl_init : ''}
    ${replacements.ctrl_mux_init != '' ? '    // Multiplexed AnlogControl objects\n    ' + replacements.ctrl_mux_init : ''}
    ${replacements.led != '' ? '    // LEDs\n    ' + replacements.led : ''}
    ${replacements.rgbled != '' ? '    // RBG LEDs \n    ' + replacements.rgbled : ''}
    ${replacements.gateout != '' ? '    // Gate outs\n    ' + replacements.gateout : ''}
    ${replacements.dachandle != '' ? '    // DAC\n    ' + replacements.dachandle : ''}
    ${replacements.display != '' ? '    // Display\n    ' + replacements.display : ''}

    ${replacements.external_codecs.length == 0 ? '' : generateCodecs(replacements.external_codecs)}

    ${replacements.som == 'seed' ? '    som.adc.Start();' : ''}
  }

  /** Handles all the controls processing that needs to occur at the block rate
   * 
   */
  void ProcessAllControls() 
  {
    ${replacements.process}
    ${replacements.som == 'patch_sm' ? 'som.ProcessAllControls();' : ''}
  }

  /** Handles all the maintenance processing. This should be run last within the audio callback.
   * 
   */
  void PostProcess()
  {
    ${replacements.postprocess}
  }

  /** Handles processing that shouldn't occur in the audio block, such as blocking transfers
   * 
   */
  void LoopProcess()
  {
    ${replacements.loopprocess}
  }

  /** Sets the audio sample rate
   *  \\param sample_rate the new sample rate in Hz
   */
  void SetAudioSampleRate(size_t sample_rate) 
  {
    ${som == 'patch_sm' ? 'som.SetAudioSampleRate(sample_rate);' : 
    `daisy::SaiHandle::Config::SampleRate enum_rate;
    if (sample_rate >= 96000)
      enum_rate = daisy::SaiHandle::Config::SampleRate::SAI_96KHZ;
    else if (sample_rate >= 48000)
      enum_rate = daisy::SaiHandle::Config::SampleRate::SAI_48KHZ;
    else if (sample_rate >= 32000)
      enum_rate = daisy::SaiHandle::Config::SampleRate::SAI_32KHZ;
    else if (sample_rate >= 16000)
      enum_rate = daisy::SaiHandle::Config::SampleRate::SAI_16KHZ;
    else
      enum_rate = daisy::SaiHandle::Config::SampleRate::SAI_8KHZ;
    som.SetAudioSampleRate(enum_rate);
    `}
    ${replacements.hidupdaterates}
  }

  /** Sets the audio block size
   *  \\param block_size the new block size in words
   */
  inline void SetAudioBlockSize(size_t block_size) 
  {
    som.SetAudioBlockSize(block_size);
  }

  /** Starts up the audio callback process with the given callback
   * 
   */
  inline void StartAudio(daisy::AudioHandle::AudioCallback cb)
  {
    som.StartAudio(cb);
  }

  /** This is the board's "System On Module"
   */
  ${replacements.som_class} som;
  ${replacements.som == 'seed' ? 'daisy::AdcChannelConfig cfg[ANALOG_COUNT]' : ''}

  // I/O Components
  ${replacements.comps}
  ${replacements.dispdec}
};

} // namespace json2daisy

#endif // __JSON2DAISY_${replacements.name.toUpperCase()}_H__
`;

  let audio_info = target.audio || null;
  let audio_channels = audio_info != null ? audio_info.channels || 2 : 2;

  let board_info = {
    header: header,
    name: target.name,
    components: components,
    aliases: target.aliases,
    channels: audio_channels
  };

  return board_info;

}