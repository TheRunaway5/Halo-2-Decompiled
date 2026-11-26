"""Bind the C8704 authored basis and fallback before C8827 normalization."""

import re

from native_pose_setup_c85a0_replacements import BODY, CANONICAL, digest


INCLUDE = '#include "physics/runtime/native_tu15_pose_first_axis_c8704.hpp"\n'
DETAIL = 'halo2::native_pose_first_axis_c8704'
PREPARED = '86fe44afb38bd3736486c791e106512c4504037d895b4d8428f54c4a97e5d41a'
LABEL = 'Lc8704: '
HOOK = f'''{LABEL}
    if({DETAIL}::run(s,[](auto &v){{run_bbff0(v);}},[](auto &v){{run_c9d20(v);}},
            [](auto &v){{run_121a90(v);}}))goto Lc8827;
'''
CHILDREN = {'bbff0': ('c872a', 'c872f'), 'c9d20': ('c87e9', 'c87ee'), '121a90': ('c8815', 'c881a')}


def bind_body(body):
    if digest(body) != PREPARED or body.count(LABEL) != 1:
        raise ValueError('Prepared C8704 pose basis changed')
    begin, end = body.index(LABEL), body.index('Lc8827:')
    phase = body[begin:end]
    for child, (site, continuation) in CHILDREN.items():
        if phase.count(f's.push(0x{site}u,0x{continuation}u);run_{child}(s);') != 1:
            raise ValueError(f'C8704 genuine child edge {site} changed')
    if (phase.count('_mm_mul_ss('), phase.count('_mm_sub_ss(')) != (6, 3):
        raise ValueError('C8704 scalar operands changed')
    phase = phase.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')
    phase = phase.replace('_mm_sub_ss(', f'{DETAIL}::subtract(')
    phase, pushes = re.subn(r'\bs\.push\((0x[0-9a-f]+u,[^;]*\);)',
                             r'halo2::native_two_bone_solve::push_word(s,\1', phase)
    if pushes != 11:
        raise ValueError('C8704 precise stack traffic changed')
    return body[:begin] + phase.replace(LABEL, HOOK, 1) + body[end:]


def install(canonical, output):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return output, []
    original = BODY.search(canonical)
    prepared = BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical C8704 pose basis changed')
    for child in CHILDREN:
        if any(text.count(f'static void run_{child}(WorldState &s) {{') != 1 for text in (canonical, output)):
            raise ValueError(f'C8704 genuine child target {child} changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], ['c85a0']
