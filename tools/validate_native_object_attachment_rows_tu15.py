#!/usr/bin/env python3
"""TU1.5 BE4A0 plus actual C0910 positive row copy and ordered SIMD clamp."""
import copy
import inspect
import struct

import validate_native_object_update_atomic_tu15 as atomic
from validate_native_object_update_atomic_tu15 import BYTE_STAGE, graph

STAGE = graph.ROOT / 'analysis/physics/native-object-update-stage-v6-attachments-20261103/staged'


def cases():
    u = graph.base.UI
    rows = graph.cases()
    rows = [row for row in rows if row['name'] != 'boundary_positive_attachments']
    seed = next(row for row in rows if row['name'] == 'empty_attachments_0')

    def make(name, values, counts, offset=-128, functions=(), mutations=()):
        row = copy.deepcopy(seed)
        row['name'] = name
        size = len(values)
        words = dict(row['user_words'])
        words[0x3124] = size * 24 | ((offset & 0xffff) << 16)
        words[0x70b0] = u + 0xe000
        for n, (vector, count) in enumerate(zip(values, counts)):
            descriptor = 0xe000 + n * 16
            table = 0xe100 + n * 0x80
            words[descriptor + 8] = count & 0xffffffff
            words[descriptor + 12] = u + table
            for k, bits in enumerate(vector):
                words[0x3000 + offset + n * 12 + k * 4] = bits
            for k in range(3):
                words[table + k * 0x28 + 0x20] = 0
                words[table + k * 0x28 + 0x24] = 0
        for n, slot, kind in functions:
            words[0xe100 + n * 0x80 + slot * 0x28 + (0x24 if kind == 'add' else 0x20)] = 0x12340001
        if functions:
            row.update(boundaries=[0xbcb70], required_owner=0xbcb70)
        if mutations:
            row['mutations'] = [dict(instruction=pc, address=u + p, written=struct.pack('<I', v).hex()) for pc, p, v in mutations]
        row['user_words'] = sorted(words.items())
        rows.append(row)

    vectors = (
        (0, 0x80000000, 0x3f800000),
        (0xc0000000, 0x40000000, 0x3f000000),
        (0x7fc12345, 0x7f812345, 0xffc54321),
        (0xff800000, 0x7f800000, 0x00000001),
        (0x80000001, 0xbf800000, 0x3f800001),
        (0x3f800001, 0x3f800001, 0x3f800001),
    )
    for n, v in enumerate(vectors):
        for count in (-1, 0, 2):
            for offset in (-128, 0x200):
                make(f'attachment_row_{n}_{count}_{offset}', [v], [count], offset)
        make(f'attachment_two_rows_{n}', [v, vectors[(n + 1) % len(vectors)]], [2, 0])
    make('attachment_actual_add_owner', [vectors[1]], [2], functions=[(0, 1, 'add')])
    make('attachment_actual_scale_owner', [vectors[2]], [2], functions=[(0, 1, 'scale')])
    make('attachment_actual_second_row_owner', [vectors[0], vectors[1]], [0, 1], functions=[(1, 0, 'add')])
    out = 0x3000 - 128 + 12
    make('attachment_count_changes_after_first_copy', [vectors[1]], [2], mutations=[(0xc09b5, 0xe008, 0)])
    make('attachment_cached_negative_compare', [(0, 0xc0000000, 0xc0000000)], [0],
         mutations=[(0xc0acc, out + 4, 0x40000000), (0xc0aed, out + 8, 0x40000000)])
    make('attachment_current_values_after_compare', [(0, 0x40000000, 0x40000000)], [0],
         mutations=[(0xc0acc, out + 4, 0xc0000000), (0xc0aed, out + 8, 0xc0000000)])
    return rows


def main():
    original = inspect.getsource(atomic.main)
    marker = '    ns = dict(graph.__dict__, STAGE=STAGE, BYTE_STAGE=BYTE_STAGE, driver=driver,'
    assert original.count(marker) == 1
    original = original.replace(marker, '''    text = text.replace("0x4dc054:struct.pack('<I',base.UI+0xa200)}", "0x4dc054:struct.pack('<I',base.UI+0xa200),0x46a0d0:image.read(0x46a0d0,4)}")
    text = text.replace("files=[*STAGE.rglob('*.cpp'),", "files=[ROOT/'tools/validate_native_object_update_atomic_tu15.py',*STAGE.rglob('*.cpp'),")
    text = text.replace("'positiveC0910attachmentgraph'", "'selectedBCB70additive_and_scale_formulas_and_134730'")
    text = text.replace("  cmake='cmake_minimum_required", CLAMP_MUTANT + "  cmake='cmake_minimum_required")
    text = text.replace("native_tu15_object_transition.cpp native_tu15_object_update_children.cpp", "native_tu15_object_transition.cpp")
    text = text.replace("  for name,unit,argv in targets:cmake+=f'add_executable({name} $<TARGET_OBJECTS:common> {unit})", "  for name,unit,argv in targets:\\n   child='mutant_attachment_upper_clamp.cpp' if name.endswith('_attachment_upper_clamp') else 'native_tu15_object_update_children.cpp'\\n   cmake+=f'add_executable({name} $<TARGET_OBJECTS:common> {unit} {child})")
    ns = dict(graph.__dict__, STAGE=STAGE, BYTE_STAGE=BYTE_STAGE, driver=driver,cases=cases,''')
    mutant = '''  path=project/'native_tu15_object_update_children.cpp';child=path.read_text()
  old='second=above(current,one)?one:word(m,output+4u);';assert child.count(old)==1
  (project/'mutant_attachment_upper_clamp.cpp').write_text(child.replace(old,'second=above(current,one)?current:word(m,output+4u);'))
  variants=(*variants,('attachment_upper_clamp','',''))
  targets.append(('object_update_attachment_upper_clamp','native_tu15_object_update.cpp',' --mutant attachment_upper_clamp'))
'''
    env = dict(atomic.__dict__, STAGE=STAGE, cases=cases, CLAMP_MUTANT=mutant, __file__=__file__, __doc__=__doc__)
    exec(original, env)
    return env['main']()


if __name__ == '__main__':
    main()
