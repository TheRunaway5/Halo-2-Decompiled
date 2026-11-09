#!/usr/bin/env python3
"""Physical TU1.5 1D7260 component snapshot, reusing the native body-frame owner."""
import copy
import inspect
import struct

import validate_native_object_update_bounded_tu15 as graph

STAGE = graph.ROOT / 'analysis/physics/native-object-component-snapshot-stage-20261103/staged'
OWNERS = (0x1d7260, 0x1d8b20, 0x1d8b90, 0x2e4650, 0x2e44d0)


def cases():
    u = graph.base.UI
    rows = []

    def bits(value):
        return struct.unpack('<I', struct.pack('<f', value))[0] if isinstance(value, float) else value

    def make(name, slot=0, kind=0, flags=0, shape=0, alias=False, mutations=()):
        offset = ((slot * 0x60 + 0x80000000) & 0xffffffff) - 0x80000000
        record = 0x2000 + offset
        motion = record - 0x20 if alias else 0x6000
        words = {0x1004: flags, 0x1070: u + 0x2000, record + 0x40: u + 0x5000,
                 0x503c: u + motion, 0x5040: kind}

        def vector(at, values):
            words.update({at + 4 * n: bits(value) for n, value in enumerate(values)})

        vector(0x7000, (8.0, -9.0, 10.0))
        vector(motion + 0x40, (-0.125, 2.0, 0x80000000))
        vector(motion + 0x50, (4.0, -5.0, 6.0))
        for i, v in enumerate(((1.0, 0.0, 0.0, 0.0), (0.0, 1.0, 0.0, 0.0),
                               (0.0, 0.0, 1.0, 0.0), (11.0, -12.0, 13.0, 1.0))):
            vector(motion + 0x80 + i * 16, v)
        transforms = (
            ((1.0, 0.0, 0.0, 0.0), (0.0, 1.0, 0.0, 0.0), (0.0, 0.0, 1.0, 0.0), (0.0, 0.0, 0.0, 1.0)),
            ((0.5, 2.0, -1.25, 0.0), (1.25, -0.25, 5.0, 0.0), (0.125, -3.0, 1.0, 0.0), (-6.0, 7.0, 8.0, 1.0)),
            ((0x7fc12345, 0.0, -1.0, 0.0), (0.0, 0x7f812345, 0.0, 0.0), (0.0, 0.0, 0x7f800000, 0.0), (0.125, 2.0, -3.0, 1.0)),
        )
        for i, v in enumerate(transforms[shape]):
            vector(0x1030 + i * 16, v)
        row = dict(name=name, entry=0x1d7260, receiver=0, arguments=[u + 0x1000], slot=slot, user_words=sorted(words.items()))
        if mutations:
            row['mutations'] = [dict(instruction=pc, address=u + addr, written=struct.pack('<I', value).hex()) for pc, addr, value in mutations]
        rows.append(row)

    for slot in (0, 1, 0xffffffff):
        for kind in (0, 1, 0xff):
            for flags in (0, 0x40):
                for shape in range(3):
                    make(f'snapshot_{slot:x}_{kind:x}_{flags:x}_{shape}', slot, kind, flags, shape)
    for slot in (0, 1, 0xffffffff):
        for flags in (0, 0x40):
            make(f'aliased_motion_{slot:x}_{flags:x}', slot=slot, flags=flags, shape=1, alias=True)
    make('publication_keeps_captured_row_and_velocity', flags=0x40, shape=1,
         mutations=[(0x1d731b, 0x1070, u + 0x3000), (0x1d731b, 0x6054, 0x12345678)])
    make('publication_keeps_captured_motion_alias', alias=True,
         mutations=[(0x1d731b, 0x2020, 0x12345678), (0x1d731b, 0x2034, 0x76543210)])
    return rows


def driver():
    text = graph.driver().replace('halo2/native_tu15_object_update_binding.hpp', 'halo2/native_tu15_object_component_snapshot.hpp')
    marker = '  void write(std::uint32_t p,std::span<const std::byte>in)override'
    assert text.count(marker) == 1
    text = text.replace(marker, '  void read_vector(std::uint32_t p,std::span<std::byte,16>out)const override{read(p,out);}\n' + marker)
    begin = text.index('  NativeTu15ObjectUpdateStats stats;')
    end = text.index('  const auto name=', begin)
    text = text[:begin] + '''  try{native_tu15_snapshot_component_body(memory,{c.at("arguments")[0]},c.at("slot"),fp,observer);}catch(const Fault&){failed=true;}
''' + text[end:]
    text = text.replace('bool changed=result!=(c.at("return_word").get<std::uint32_t>()&255u);', 'bool changed=false;')
    text = text.replace('  if(!failed&&!required)check(result==(c.at("return_word").get<std::uint32_t>()&255u),name+" boolean result differs");\n', '')
    text = text.replace('completed>100&&changes>0', 'completed>50&&changes>0')
    return text


def main():
    text = inspect.getsource(graph.main)
    text = text.replace("ROOT/'src/physics/native_tu15_object_publication.cpp',ROOT/'src/physics/native_tu15_activation_services.cpp'",
                        "ROOT/'src/physics/native_tu15_body_constraint_update.cpp',ROOT/'src/physics/native_tu15_geometry.cpp',ROOT/'tools/validate_native_object_update_bounded_tu15.py'")
    text = text.replace("('entry','receiver','arguments','user_words')", "('entry','receiver','arguments','user_words','slot')")
    start = text.index('rows=cases();table=')
    end = text.index('\n  fixture,authority=', start)
    text = text[:start] + "rows=cases();banks={0x474d04:struct.pack('<I',base.UI+0x7000),0x46a0d0:image.read(0x46a0d0,4)}" + text[end:]
    text = text.replace(";ns=dict(base.__dict__);exec(source,ns)", ";source=source.replace(ASM_MARKER,ASM_REPLACEMENT);ns=dict(base.__dict__);exec(source,ns)")
    start = text.index(";variants=(('wrong_transition_offset'")
    end = text.index('\n  text=(project/', start)
    text = text[:start] + ";variants=VARIANTS" + text[end:]
    text = text.replace('native_tu15_object_update.cpp', 'native_tu15_object_component_snapshot.cpp')
    text = text.replace('native_tu15_object_transition.cpp native_tu15_object_update_children.cpp native_tu15_object_publication.cpp native_tu15_activation_services.cpp',
                        'native_tu15_body_constraint_update.cpp native_tu15_geometry.cpp')
    start = text.index("remaining=['B37A0'")
    end = text.index('])', start)
    text = text[:start] + "remaining=['1D6FB0 parent composition','Live/canonical World admission'" + text[end:]
    marker = " mov ${case['receiver']},%ecx"
    variants = (
        ('wrong_position_axis', 'z=word(m,motion+0xb8u)', 'z=word(m,motion+0xb4u)'),
        ('wrong_angular_order', 'angular[n]);', 'angular[(n+1u)%3u]);'),
    )
    env = dict(graph.__dict__, STAGE=STAGE, OWNERS=OWNERS, cases=cases, driver=driver,
               ASM_MARKER=marker, ASM_REPLACEMENT=marker + "\n mov ${case['slot']},%eax",
               VARIANTS=variants, __file__=__file__, __doc__=__doc__)
    exec(text, env)
    return env['main']()


if __name__ == '__main__':
    main()
