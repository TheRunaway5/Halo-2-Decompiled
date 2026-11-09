#!/usr/bin/env python3
"""Physical selected BE4A0 to 1D6FB0 handoff through the native object binding."""
import inspect

import validate_native_object_component_update_tu15 as component

ROOT = component.ROOT
STAGE = component.PHYSICS / 'native-object-component-update-bound-stage-20261103/staged'
OWNERS = (*component.OWNERS, 0xbe4a0, 0x10dd10, 0xd8330, 0xd80a0, 0xc0910)
OBJECT_UNITS = (
    'native_tu15_object_update.cpp',
    'native_tu15_object_update_children.cpp',
    'native_tu15_object_transition.cpp',
    'native_tu15_object_prediction.cpp',
)


def cases():
    user = component.graph.base.UI
    rows = component.cases()
    for row in rows:
        row['name'] = 'bound_' + row['name']
        row['entry'] = 0xbe4a0
        row['arguments'] = [0xabcd0001]
        words = dict(row['user_words'])
        words.update({
            0x0d44: user + 0x1000,  # component pool, index zero
            0x6000 + 0x84: 0,      # no unit-component callback
            0x6800 + 3 * 16 + 8: user + 0x7000,
            0x7000 + 0x1c: 0,      # no attachment rows
            0x7000 + 0x38: 0xffffffff,
            0x980c: 0,             # no prediction owner selected
            0xd000: 3,
            0xd0aa: 2,
            0xd0b0: 0,
            0xd0b4: 0xabcd0000,
            0xd0c0: 0x40,
            0xd12a: 0xffff,
        })
        row['user_words'] = sorted(words.items())
    return rows


def driver():
    text = component.driver().replace('halo2/native_tu15_object_component_update.hpp',
                                      'halo2/native_tu15_object_update_binding.hpp')
    old = '  try{native_tu15_update_object_component(memory,{c.at("edi")},fp,0,{},protected_observer);}'
    assert text.count(old) == 1
    text = text.replace(old, '''  try{
    NativeTu15ObjectUpdateStats stats;
    const auto services=native_tu15_make_bounded_object_update_services(memory,protected_observer.callback());
    result=native_tu15_update_object(memory,{c.at("arguments")[0]},fp,services,protected_observer.callback(),&stats);
    check(stats.service_calls[0x1d6fb0u]==1,"Native BE4A0 failed to enter its component owner");
  }''')
    old = '  if(mutant){++completed;bool changed=false;'
    assert text.count(old) == 1
    text = text.replace(old, '''  if(!failed&&!required)check(result==(c.at("return_word").get<std::uint32_t>()&255u),name+" BE4A0 result differs");
  if(mutant){++completed;bool changed=false;''')
    return text


def main():
    text = inspect.getsource(component.main)
    marker = '    marker = " mov '
    assert text.count(marker) == 1
    text = text.replace(marker, '''    text = text.replace("add_library(common OBJECT driver.cpp ", "add_library(common OBJECT driver.cpp "+" ".join(OBJECT_UNITS)+" ")
    text = text.replace("rows=cases();banks={", "rows=cases();banks={0x52be5c:struct.pack('<I',base.UI+0xd00),0x474b98:struct.pack('<I',base.UI+0x6000),0x4f0414:struct.pack('<I',base.UI+0x6800),0x4f3218:struct.pack('<I',base.UI+0x9800),")
    text = text.replace("files=[*STAGE.rglob('*.cpp'),", "files=[ROOT/'tools/validate_native_object_component_update_tu15.py',*STAGE.rglob('*.cpp'),")
''' + marker)
    env = dict(component.__dict__, STAGE=STAGE, OWNERS=OWNERS, OBJECT_UNITS=OBJECT_UNITS,
               cases=cases, driver=driver, __file__=__file__, __doc__=__doc__)
    exec(text, env)
    return env['main']()


if __name__ == '__main__':
    main()
