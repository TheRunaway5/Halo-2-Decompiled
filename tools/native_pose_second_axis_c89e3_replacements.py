"""Bind the authenticated second-axis basis and fallback before C8B65."""

import re

from native_pose_setup_c85a0_replacements import BODY, CANONICAL, digest


INCLUDE = '#include "physics/runtime/native_tu15_pose_second_axis_c89e3.hpp"\n'
DETAIL = 'halo2::native_pose_second_axis_c89e3'
PREPARED = '5453f1cc0d1ab489df1371dc2932df59a9e087c31d1c5521fd6234b8d3cf671f'
LABEL = 'Lc89e3: '
HOOK = f'''{LABEL}
    if({DETAIL}::run(s,[](auto &v){{run_bbff0(v);}},[](auto &v){{run_c9d20(v);}},
            [](auto &v){{run_121a90(v);}}))goto Lc8b65;
'''
CHILDREN = {'bbff0': ('c8a12', 'c8a17'), 'c9d20': ('c8b34', 'c8b39'), '121a90': ('c8b60', 'c8b65')}


def bind_body(body):
    if digest(body) != PREPARED or body.count(LABEL) != 1:
        raise ValueError('Prepared C89E3 pose basis changed')
    begin, end = body.index(LABEL), body.index('Lc8b65:')
    phase = body[begin:end]
    for child, (site, continuation) in CHILDREN.items():
        if phase.count(f's.push(0x{site}u,0x{continuation}u);run_{child}(s);') != 1:
            raise ValueError(f'C89E3 genuine child edge {site} changed')
    if (phase.count('_mm_mul_ss('), phase.count('_mm_sub_ss(')) != (12, 6):
        raise ValueError('C89E3 scalar operands changed')
    phase = phase.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')
    phase = phase.replace('_mm_sub_ss(', 'halo2::native_pose_first_axis_c8704::subtract(')
    phase, pushes = re.subn(r'\bs\.push\((0x[0-9a-f]+u,[^;]*\);)',
                             r'halo2::native_two_bone_solve::push_word(s,\1', phase)
    if pushes != 11:
        raise ValueError('C89E3 precise stack traffic changed')
    return body[:begin] + phase.replace(LABEL, HOOK, 1) + body[end:]


def install(canonical, output):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return output, []
    original = BODY.search(canonical)
    prepared = BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical C89E3 pose basis changed')
    for child in CHILDREN:
        if any(text.count(f'static void run_{child}(WorldState &s) {{') != 1 for text in (canonical, output)):
            raise ValueError(f'C89E3 genuine child target {child} changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], ['c85a0']
