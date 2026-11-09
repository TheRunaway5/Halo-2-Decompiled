#!/usr/bin/env python3
"""Physical selected 1D6FB0 using the sealed native velocity and snapshot owners."""
import inspect
import struct

import validate_native_object_update_bounded_tu15 as graph

ROOT = graph.ROOT
PHYSICS = ROOT / 'analysis/physics'
STAGE = PHYSICS / 'native-object-component-update-stage-20261103/staged'
VELOCITY = PHYSICS / 'native-phantom-body-velocity-qualified-20261103/source'
SNAPSHOT = PHYSICS / 'native-object-component-snapshot-qualified-20261103/source'
BYTE = PHYSICS / 'native-object-atomic-byte-qualified-20261103/source'
OWNERS = (
    0x1d6fb0, 0x1d9ec0, 0x315c90, 0x315cb0, 0x3165d0, 0x316d40, 0x322660,
    0x1d9080, 0x1d9100, 0x1d92c0, 0x3239f0, 0x323a00, 0x31dd70, 0x31edf0,
    0x1d7260, 0x1d8b20, 0x1d8b90, 0x2e4650, 0x2e44d0,
    0xbda50, 0xbcfb0, 0xbe7e0, 0xb9390,
)
DEPENDENCIES = (
    BYTE / 'src/physics/native_tu15_byte_update.cpp',
    *(VELOCITY / 'src/physics' / name for name in (
        'native_tu15_phantom_body_velocity.cpp', 'native_tu15_island_activation.cpp',
    )),
    *(SNAPSHOT / 'src/physics' / name for name in (
        'native_tu15_object_component_snapshot.cpp', 'native_tu15_body_constraint_update.cpp',
        'native_tu15_geometry.cpp',
    )),
)
HEADERS = (
    BYTE / 'include/halo2/native_tu15_byte_update.hpp',
    VELOCITY / 'include/halo2/native_tu15_phantom_body_velocity.hpp',
    VELOCITY / 'include/halo2/native_tu15_retained_counter_update.hpp',
    SNAPSHOT / 'include/halo2/native_tu15_object_component_snapshot.hpp',
)


def cases():
    user = graph.base.UI
    rows = []

    def bits(value):
        return struct.unpack('<I', struct.pack('<f', value))[0] if isinstance(value, float) else value

    def make(name, flags=0x20, mode='dynamic', count=1, typ=2, velocities='high',
             header_flags=2, mutations=(), boundary=None):
        words = {
            0x1004: flags, 0x1008: 0xabcd0001, 0x1070: user + 0x2000, 0x1074: count,
            0x8000: 0x31dd70, 0x8018: 0x31dd70, 0x8050: 0x3239f0, 0x8054: 0x323a00,
            0x8118: 0x31edf0, 0x8150: 0x3239f0, 0x8154: 0x323a00,
            0x8218: 0xa9520,
            0x9004: bits(0.033333335), 0xa000: bits(9.8), 0xb044: user + 0xc000,
            0xc00c: (typ << 24) | (header_flags << 16) | 1, 0xc014: user + 0xd000,
            0xd010: 0xffffffff, 0xd014: 0xffffffff,
            0xe008: user + 0xe600, 0xe00c: 0, 0xe010: 2,
            0xe014: user + 0xe700, 0xe018: 1, 0xe700: user + 0x4200,
            0xe07c: 0, 0xe10c: 0,
        }
        for lane, value in enumerate((0, 0, 0)):
            words[0xf000 + lane * 4] = value
        for i in range(3):
            body = 0x3000 + i * 0x200
            island = 0x4000 + i * 0x200
            motion = 0x5000 + i * 0x200
            table = 0x8100 if mode == 'class6' else 0x8200 if mode == 'unknown' else 0x8000
            active = mode not in ('inactive', 'no_island') and (mode != 'wake' or i == 0)
            words.update({
                0x2040 + i * 0x60: user + body,
                body + 8: user + 0xe000 if mode == 'wake' and i == 1 else 0,
                body + 0x3c: user + motion,
                body + 0x40: int(mode == 'fixed'),
                body + 0x44: 0 if mode == 'no_island' else user + island,
                body + 0x74: 0,
                island + 0x24: 0,
                island + 0x28: 0x100 if active else 0,
                island + 0x40: 0,
                motion: user + table,
            })
            linear, angular = {
                'high': ((400.0, -200.0, 300.0), (150.0, -120.0, 80.0)),
                'small': ((0.0, 0.0, -0.0), (0.125, 0.0, 0.25)),
                'nan': ((0x7fc12345, 0x7f812345, 1), (0x7f800000, 1, 0x7fc32145)),
            }[velocities]
            for offset, values in ((0x40, linear), (0x50, angular), (0x80, (1.,0.,0.,0.)),
                                   (0x90, (0.,1.,0.,0.)), (0xa0, (0.,0.,1.,0.)),
                                   (0xb0, (11.+i,-12.,13.,1.))):
                for lane, v in enumerate(values):
                    words[motion + offset + 4*lane] = bits(v)
        for offset, values in ((0x30,(0.5,2.,-1.25,0.)), (0x40,(1.25,-0.25,5.,0.)),
                               (0x50,(0.125,-3.,1.,0.)), (0x60,(-6.,7.,8.,1.))):
            for lane, v in enumerate(values):
                words[0x1000 + offset + 4*lane] = bits(v)
        row = dict(name=name, entry=0x1d6fb0, receiver=0, edi=user+0x1000, arguments=[],
                   user_words=sorted(words.items()))
        if mutations:
            row['mutations'] = [dict(instruction=pc, address=user+p,
                                     written=int(v).to_bytes(width, 'little').hex())
                                for pc,p,v,width in mutations]
        if boundary:
            row.update(required_owner=boundary, boundaries=[boundary])
        rows.append(row)

    for flags in (0, 0x20, 0x22, 0x24, 0x60):
        for mode in ('dynamic', 'fixed', 'class6', 'inactive', 'no_island'):
            for speeds in ('high', 'small', 'nan'):
                make(f'{flags:x}_{mode}_{speeds}', flags, mode, velocities=speeds)
    for count in (0, 0xffffffff, 2, 3):
        make(f'count_{count:x}', count=count, velocities='small')
    make('wake_second_island', mode='wake', count=2)
    make('vehicle_force_owner', typ=1, boundary=0xfa430)
    make('vehicle_force_bypassed', flags=0x24, typ=1)
    make('wrapped_type_selects_force', typ=33, boundary=0xfa430)
    make('unknown_motion_owner', mode='unknown', boundary=0xa9520)
    make('current_count_after_publication', header_flags=3, mutations=[(0xbda93,0x1074,0,4)])
    make('current_flags_after_snapshot', velocities='small',
         mutations=[(0x1d7349,0x1004,0x24,4)])
    make('current_type_after_snapshot', velocities='small',
         mutations=[(0x1d7349,0x8018,0x31edf0,4)])
    return rows


def driver():
    text = graph.driver().replace('halo2/native_tu15_object_update_binding.hpp',
                                  'halo2/native_tu15_object_component_update.hpp')
    text = '#include "halo2/native_tu15_retained_counter_update.hpp"\n#include "halo2/native_tu15_byte_update.hpp"\n' + text
    text = text.replace('struct Memory:SourcePhysicsContactMemory {',
                        'struct Memory:SourcePhysicsContactMemory,NativeTu15RetainedCounterStorage,NativeTu15ByteUpdateStorage {')
    marker = '  void write(std::uint32_t p,std::span<const std::byte>in)override'
    assert text.count(marker) == 1
    text = text.replace(marker, '''  void read_vector(std::uint32_t p,std::span<std::byte,16>out)const override{read(p,out);}
  NativeTu15ByteUpdate update_byte(std::uint32_t p,std::uint8_t and_mask,std::uint8_t or_mask)override{
    if(!fault.is_null()&&fault.value("write",false)&&p==fault.at("address")&&fault.at("bytes")==1u&&
       ++fault_count==fault.at("occurrence"))throw Fault{};
    const auto i=at(p,1);std::array<std::byte,1>before{};read(p,before);
    return native_tu15_update_byte_value(banks.at(i->first)[p-i->first],and_mask,or_mask);
  }
  NativeTu15RetainedCounterUpdate adjust_counter(std::uint32_t p,unsigned width,std::uint32_t adjustment)override{
    if(!fault.is_null()&&fault.value("write",false)&&p==fault.at("address")&&width==fault.at("bytes")&&
       ++fault_count==fault.at("occurrence"))throw Fault{};
    const auto i=at(p,width);std::array<std::byte,4>before{};read(p,std::span(before).first(width));
    const auto result=native_tu15_adjust_counter_value(before,width,adjustment);
    std::copy_n(result.written.begin(),width,banks.at(i->first).begin()+p-i->first);return result;
  }
''' + marker)
    start = text.index('  NativeTu15ObjectUpdateStats stats;')
    end = text.index('  const auto name=', start)
    text = text[:start] + '''  NativeTu15RetainedStoreObserver protected_observer(observer);
  try{native_tu15_update_object_component(memory,{c.at("edi")},fp,0,{},protected_observer);}
  catch(const Fault&){failed=true;}
  catch(const std::runtime_error&e){const auto owner=c.value("required_owner",0u);if(owner&&std::string(e.what()).find("requires owner="+std::to_string(owner))!=std::string::npos)required=true;else throw;}
''' + text[end:]
    text = text.replace('bool changed=result!=(c.at("return_word").get<std::uint32_t>()&255u);', 'bool changed=false;')
    text = text.replace('  if(!failed&&!required)check(result==(c.at("return_word").get<std::uint32_t>()&255u),name+" boolean result differs");\n', '')
    return text.replace('completed>100&&changes>0', 'completed>60&&changes>0')


def main():
    text = inspect.getsource(graph.main)
    text = text.replace("ROOT/'src/physics/native_tu15_object_publication.cpp',", "")
    text = text.replace("files=[*STAGE.rglob('*.cpp'),*STAGE.rglob('*.hpp'),",
                        "files=[*STAGE.rglob('*.cpp'),*STAGE.rglob('*.hpp'),*DEPENDENCIES,*HEADERS,ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',ROOT/'tools/validate_native_object_update_bounded_tu15.py',")
    text = text.replace("('entry','receiver','arguments','user_words')",
                        "('entry','receiver','arguments','user_words','edi')")
    start = text.index('rows=cases();table=')
    end = text.index('\n  fixture,authority=', start)
    text = text[:start] + "rows=cases();banks={0x4ecbd0:struct.pack('<I',base.UI+0xb000),0x474d04:struct.pack('<I',base.UI+0xf000),0x51df54:struct.pack('<I',base.UI+0x9000),0x52be68:struct.pack('<I',base.UI+0xa000),**{p:image.read(p,4)for p in (0x46a348,0x4500b8,0x46a814,0x46a1a4,0x46a0d0)}}" + text[end:]
    text = text.replace(";ns=dict(base.__dict__);exec(source,ns)",
                        ";source=source.replace(ASM_MARKER,ASM_REPLACEMENT);ns=dict(base.__dict__);exec(source,ns)")
    text = text.replace("for h in STAGE.rglob('*.hpp'):", "for h in [*STAGE.rglob('*.hpp'),*HEADERS]:")
    text = text.replace("if f.suffix=='.cpp'and f.parent.name=='physics':",
                        "if f.parent.name=='physics' and f.suffix in ('.cpp','.hpp'):")
    start = text.index(";variants=(('wrong_transition_offset'")
    end = text.index('\n  text=(project/', start)
    text = text[:start] + ';variants=VARIANTS' + text[end:]
    text = text.replace('native_tu15_object_update.cpp', 'native_tu15_object_component_update.cpp')
    text = text.replace('native_tu15_object_transition.cpp native_tu15_object_update_children.cpp',
                        'native_tu15_byte_update.cpp native_tu15_phantom_body_velocity.cpp native_tu15_island_activation.cpp native_tu15_object_component_snapshot.cpp native_tu15_body_constraint_update.cpp native_tu15_geometry.cpp')
    start = text.index("remaining=['B37A0'")
    end = text.index('])', start)
    text = text[:start] + "remaining=['FA430 object-specific force owner','unqualified motion virtuals','World and outer object scheduler admission'" + text[end:]
    marker = " mov ${case['receiver']},%ecx"
    variants = (
        ('wrong_gravity_sign', 'subss %1,%0', 'addss %1,%0'),
        ('wrong_linear_limit', 'linear, 0x46a348u, 0x4500b8u', 'linear, 0x46a348u, 0x46a1a4u'),
    )
    env = dict(graph.__dict__, STAGE=STAGE, OWNERS=OWNERS, cases=cases, driver=driver,
               DEPENDENCIES=DEPENDENCIES, HEADERS=HEADERS, VARIANTS=variants,
               ASM_MARKER=marker, ASM_REPLACEMENT=marker+"\n mov ${case['edi']},%edi",
               __file__=__file__, __doc__=__doc__)
    exec(text, env)
    return env['main']()


if __name__ == '__main__':
    main()
