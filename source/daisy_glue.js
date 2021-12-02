const assert = require("assert");

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

// .filter for objects that returns object
Object.filter = (obj, predicate) => 
    Object.keys(obj).filter(key => predicate(obj[key]))
    .map(key => obj[key]);

function filter_match(sequence, key, match, key_exclude = null, match_exclude = null)
{
  if (key_exclude !== null && match_exclude !== null)
  {
    return Object.filter(sequence, item => key in item && item[key] == match && ((item[key_exclude] || null) != match_exclude));
  }
  else
    return Object.filter(sequence, item => key in item && item[key] == match);
}

function verify_param_exists(name, original_name, components, input=true)
{
  for (let comp of components)
  {
    if (comp.component == 'CVOuts')
    {
      if (name == comp.name)
      {
        assert(!input, `Parameter ${original_name} cannot be used as an ${input ? 'input' : 'output'}`);
        return;
      }
    }
    else
    {
      let variants = comp.mapping.map(item => stringFormatMap(item.name, comp));
      if (variants.includes(name))
      {
        assert((input && comp.direction == 'input') || (!input && comp.direction == 'output'),
          `Parameter ${original_name} cannot be used as an ${input ? 'input' : 'output'}`);
        return;
      }
    }
  }
  assert(false, `Unkown parameter ${original_name}`);
}

function verify_param_direction(name, components)
{
  for (let comp of components)
  {
    if (comp.component == 'CVOuts')
    {
      if (name == comp.name)
        return true;
    }
    else
    {
      let variants = comp.mapping.map(item => stringFormatMap(item.name, comp));
      if (variants.includes(name))
        return true;
    }
  }
  return false;
}

function get_root_component(variant, original_name, components)
{
  for (let comp of components)
  {
    if (comp.component == 'CVOuts')
    {
      if (variant == comp.name)
        return variant;
    }
    else
    {
      let variants = comp.mapping.map(item => stringFormatMap(item.name, comp));
      if (variants.includes(variant))
        return comp.name;
    }
  }
  assert(false, `Unkown parameter ${original_name}`);
}

function get_component_mapping(component_variant, original_name, component, components)
{
  for (let variant of component.mapping)
  {
    if (component.component == 'CVOuts')
    {
      let stripped = stringFormatMap(variant.name, {name: ''});
      if (component.name.includes(stripped))
        return variant;
    }
    else if (stringFormatMap(variant.name, component) == component_variant)
      return variant;
  }
  assert(false, `Unkown parameter ${original_name}`);
}


function verify_param_used(component, params_in, params_out, params_in_original_name, params_out_original_name, components)
{
  // Exclude parents, since they don't have 1-1 i/o mapping
  if (component.is_parent || false)
    return true;
  
  let combined_params;
  Object.assign(combined_params, params_in, params_out);
  let combined_names;
  Object.assign(combined_names, params_in_original_name, params_out_original_name);
  for (let param in combined_params)
  {
    let root = get_root_component(param, combined_names[param], components);
    if (root == component.name)
      return true;
  }
  return false;
}

function de_alias(name, aliases, components)
{
  let low = name.toLowerCase();
  // simple case
  if (aliases.includes(low))
    return aliases[low];
  // aliased variant
  let potential_aliases = Object.filter(aliases, item => low.includes(item));
  for (let alias of potential_aliases)
  {
    target_component = filter_match(components, 'name', aliases[alias])[0] || undefined;
    if (typeof target_component === 'undefined')
      continue;
    if (target_component.component != 'CVOuts')
    {
      for (let mapping of target_component.mapping)
      {
        if (stringFormatMap(mapping.name, {name: alias}) == low)
          return stringFormatMap(mapping.name, {name: aliases[alias]});
      }
    }
  }
  // otherwise, it's a direct parameter or unkown one
  return low;
}

//  Parses the `parameters` passed from oopsy and generates getters and setters
//  according to the info in `components`. The `aliases` help disambiguate parameters
//  and the `object_name` sets the identifier for the generated Daisy hardware class.
exports.parse_parameters = function parse_parameters(parameters, components, aliases, object_name)
{
  // Verify that the params are valid and remove unused components
  let replacements = {};

  let params_in = {};
  let params_in_original_names = {};
  for (property in parameters.in)
  {
    let de_aliased = de_alias(property, aliases, components);
    params_in[de_aliased] = parameters.in[property];
    params_in_original_names[de_aliased] = property;
  }

  let params_out = {};
  let params_out_original_names = {};
  for (property in parameters.out)
  {
    let de_aliased = de_alias(property, aliases, components);
    params_out[de_aliased] = parameters.out[property];
    params_out_original_names[de_aliased] = property;
  }

  for (property in params_in)
    verify_param_exists(property, params_in_original_names[property], components, input=true);
  for (property in params_out)
    verify_param_exists(property, params_out_original_names[property], components, input=false);

  for (let i = components.length - 1; i > -1; i--)
  {
    let used = verify_param_used(components[i], params_in, params_out,
      params_in_original_names, params_out_original_names, components);
    if (!used)
      components.splice(i, 1);
  }

  let out_idx = 0;
  replacements.parameters = [];
  replacements.output_parameters = [];
  replacements.callback_write_out = [];
  replacements.loop_write_out = '';
  replacements.callback_write_in = [];

  for (let param_name in params_in)
  {
    root = get_root_component(param_name, params_in_original_names[param_name], components);
    let component = filter_match(components, 'name', root)[0];
    let param_struct = {
      name: root,
      type: component.component.toUpperCase()
    };
    replacements.parameters.push(param_struct);
    let mapping = get_component_mapping(param_name, params_in_original_names[param_name], component, components);

    let component_info;
    Object.assign(component_info, component);
    component_info.name = root;
    component_info.class_name = object_name;
    component_info.name_upper = root.toUpperCase();
    component_info.value = `output_data[${out_idx}]`;
    component_info.default_prefix = (component.is_default || false) ? component.default_prefix || '' : '';
    let process = stringFormatMap(mapping.get, component_info);
    replacements.callback_write_in.push({process: process, bool: mapping.bool});
  }

  for (let param_name in params_out)
  {
    root = get_root_component(param_name, params_in_original_names[param_name], components);
    let component = filter_match(components, 'name', root)[0];
    let param_struct = {
      name: root,
      index: out_idx
    };
    replacements.output_parameters.push(param_struct);
    let mapping = get_component_mapping(param_name, params_out_original_names[param_name], component, components);
    let write_location = (mapping.where || 'callback') == 'callback' ? 'callback_write_out' : 'loop_write_out';
    let component_info;
    Object.assign(component_info, component);
    component_info.name = root;
    component_info.class_name = object_name;
    component_info.value = `output_data[${out_idx}]`;
    component_info.default_prefix = (component.is_default || false) ? component.default_prefix || '' : '';
    let write = stringFormatMap(mapping.set, component_info);
    replacements[write_location] += `\n  ${write}`;
  }

  replacements.output_comps = replacements.output_parameters.length;

  return replacements;
}