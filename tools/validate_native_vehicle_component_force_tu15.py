#!/usr/bin/env python3
"""Physical TU1.5 FA430 through 1D6FB0 and the staged BE4A0 binding."""
import copy
import inspect
import struct

import validate_native_object_component_update_bound_tu15 as bound

component = bound.component
ROOT = component.ROOT
STAGE = component.PHYSICS / 'native-vehicle-component-force-stage-20261103/staged'
OWNERS = (*bound.OWNERS, 0xfa430, 0xbcdf0, 0xd3750)
DEPENDENCIES = (*component.DEPENDENCIES,
                ROOT / 'src/rendering/world/native_tu15_render_lighting.cpp')
OBJECT_UNITS = (*bound.OBJECT_UNITS, 'native_tu15_vehicle_component_force.cpp',
                'native_tu15_render_lighting.cpp')


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0] if isinstance(value, float) else value


def cases():
    user = component.graph.base.UI
    old = bound.cases()
    rows = [row for row in old if row.get('required_owner') != 0xfa430]
    seed = next(row for row in old if row['name'] == 'bound_20_dynamic_small')

    def vehicle(name, variant, effect=1.0, contacts=3, flags=0, axis=(1.,0.,1.),
                current=1.0, baseline=0.0, maximum=2.0, fade=2.0, magnitude=0.25,
                scale=2.0, header_type=1, high_speed=False):
        row = copy.deepcopy(seed)
        row['name'] = 'vehicle_force_' + name
        words = dict(row['user_words'])
        words.update({
            0xb024: 12, 0xb038: 2,
            0xc00c: (header_type << 24) | (2 << 16) | 0xabcd,
            0xd25c: bits(effect),
            0xd334: bits(current), 0xd338: bits(baseline),
            0xd1b0: bits(magnitude), 0xd38e: flags,
            0x71f0: variant & 0xffff, 0x72f0: contacts & 0xffffffff,
            0x72e0: bits(scale), 0x71d4: bits(maximum), 0x71dc: bits(fade),
        })
        for i, value in enumerate(axis):
            words[0xd07c + 4*i] = bits(value)
        if high_speed:
            for i, value in enumerate((400., -200., 300., 0., 150., -120., 80., 0.)):
                words[0x5040 + 4*i] = bits(value)
        row['user_words'] = sorted(words.items())
        rows.append(row)

    for index, effect in enumerate((0.0, -0.0, 1.0, -1.0, 0x7fc12345, 0x7f812345)):
        vehicle(f'initial_effect_{index}', 2, effect=effect)
    for variant in (-1, 0, 1, 2, 5, 9):
        vehicle(f'definition_type_{variant}', variant, high_speed=True)
    for count in (-1, 0, 1, 2, 3, 4, 17, 33, 63):
        for mask in (0, 0x0003, 0x8000, 0xffff):
            vehicle(f'wheel_count_{count}_mask_{mask:x}', 0, contacts=count, flags=mask)
    for index, axis in enumerate(((1.,0.,1.), (1.,1.,0.), (0.,0.,-1.),
                                  (0x7fc12789,0.,1.), (0.,0.,0x7f812345))):
        vehicle(f'dot_axis_{index}', 1, axis=axis)
    for index, (current, baseline) in enumerate(((0.,0.), (-1.,0.), (1.,0.), (3.,-0.),
                                                (1.,2.), (2.,2.), (3.,2.),
                                                (0x7fc12345,0.), (1.,0x7fc12345),
                                                (1.,0x7f812345))):
        for mode, magnitude in enumerate((0.25, -0.25, 0x7fc12345, 0x7f812345)):
            vehicle(f'effect_ratio_{index}_magnitude_{mode}', 5, current=current,
                    baseline=baseline, magnitude=magnitude)
    vehicle('ratio_divisor_zero', 5, current=3.0, baseline=1.0, fade=0.0)
    vehicle('ratio_divisor_negative', 5, current=3.0, baseline=1.0, fade=-1.0)
    vehicle('wrapped_object_type', 0, header_type=33)
    for row in list(rows):
        row['eax'] = 0xabcd0001
        if row['name'].startswith(('vehicle_force_effect_ratio_', 'vehicle_force_ratio_divisor_')):
            leaf = copy.deepcopy(row)
            leaf['name'] = 'ratio_direct_' + row['name']
            leaf['entry'] = 0xd3750
            leaf['arguments'] = []
            rows.append(leaf)
    return rows


def driver():
    text = '#include "halo2/native_tu15_vehicle_component_force.hpp"\n' + bound.driver()
    old = '  try{\n    NativeTu15ObjectUpdateStats stats;'
    assert text.count(old) == 1
    text = text.replace(old, '''  std::uint32_t ratio{};
  const bool ratio_leaf=c.at("entry")==0xd3750u;
  try{
    if(ratio_leaf)ratio=native_tu15_vehicle_effect_ratio(memory,{c.at("eax")},fp,0);
    else{
    NativeTu15ObjectUpdateStats stats;''')
    old = '    check(stats.service_calls[0x1d6fb0u]==1,"Native BE4A0 failed to enter its component owner");'
    assert text.count(old) == 1
    text = text.replace(old, old + '\n    }')
    old = '  if(!failed&&!required)check(result==(c.at("return_word").get<std::uint32_t>()&255u),name+" BE4A0 result differs");'
    assert text.count(old) == 1
    text = text.replace(old, '''  if(!failed&&!required){
    if(ratio_leaf)check(ratio==c.at("scalar_return"),name+" D3750 scalar result differs");
    else check(result==(c.at("return_word").get<std::uint32_t>()&255u),name+" BE4A0 result differs");
  }''')
    text = text.replace('changes{},profiles{};', 'changes{},profiles{},fp_differences{};')
    old = 'check(J::array({fp.control_word,fp.status,fp.mxcsr})==c.at("after_fp"),name+" SourceFP differs");'
    assert text.count(old) == 1
    text = text.replace(old, 'const bool same_fp=J::array({fp.control_word,fp.status,fp.mxcsr})==c.at("after_fp");if(mutant)fp_differences+=!same_fp;else check(same_fp,name+" SourceFP differs");')
    return text.replace('{"refusals",0}', '{"refusals",0},{"fp_differences",fp_differences}')


def main():
    text = inspect.getsource(component.main)
    marker = '    marker = " mov '
    assert text.count(marker) == 1
    text = text.replace(marker, '''    text = text.replace("add_library(common OBJECT driver.cpp ", "add_library(common OBJECT driver.cpp "+" ".join(OBJECT_UNITS)+" ")
    text = text.replace("rows=cases();banks={", "rows=cases();banks={0x52be5c:struct.pack('<I',base.UI+0xd00),0x474b98:struct.pack('<I',base.UI+0x6000),0x4f0414:struct.pack('<I',base.UI+0x6800),0x4f3218:struct.pack('<I',base.UI+0x9800),0x46a330:image.read(0x46a330,4),")
    text = text.replace("files=[*STAGE.rglob('*.cpp'),", "files=[ROOT/'tools/validate_native_object_component_update_tu15.py',ROOT/'tools/validate_native_object_component_update_bound_tu15.py',*STAGE.rglob('*.cpp'),")
    text = text.replace("if f.parent.name=='physics' and f.suffix", "if f.parent.name in ('physics','world') and f.suffix")
    text = text.replace("'FA430 object-specific force owner',", "")
    text = text.replace("'user_words','edi')", "'user_words','edi','eax')")
    text = text.replace(";(scratch/'observer.cpp').write_text(observer)", ";observer=observer.replace(RESULT_FIELD,RESULT_FIELD_PLUS);(scratch/'observer.cpp').write_text(observer)")
''' + marker)
    marker = '    env = dict(graph.__dict__'
    assert text.count(marker) == 1
    text = text.replace(marker, '''    variants = (*variants, ('omit_vehicle_force', 'native_tu15_adjust_vehicle_component_force(memory, {current_object}, step, fp, depth);', '(void)current_object;'))
''' + marker)
    text = text.replace('VARIANTS=variants,', 'VARIANTS=variants, RESULT_FIELD=RESULT_FIELD, RESULT_FIELD_PLUS=RESULT_FIELD_PLUS,')
    text = text.replace('ASM_REPLACEMENT=marker+', 'ASM_REPLACEMENT=EAX_SETUP+marker+')
    field = '{"return_word",std::uint32_t(regs.rax)}'
    env = dict(component.__dict__, STAGE=STAGE, OWNERS=OWNERS, OBJECT_UNITS=OBJECT_UNITS,
               DEPENDENCIES=DEPENDENCIES, cases=cases, driver=driver,
               EAX_SETUP=" mov ${case['eax']},%eax\n", RESULT_FIELD=field, RESULT_FIELD_PLUS=field + ',{"scalar_return",fp.xmm_space[0]}',
               __file__=__file__, __doc__=__doc__)
    exec(text, env)
    return env['main']()


if __name__ == '__main__':
    main()
