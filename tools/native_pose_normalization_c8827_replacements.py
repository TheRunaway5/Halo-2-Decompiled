"""Bind the authored C8827 normalization and its four original child edges."""

import re

from native_pose_setup_c85a0_replacements import BODY, CANONICAL, digest


INCLUDE = '#include "physics/runtime/native_tu15_pose_normalization_c8827.hpp"\n'
DETAIL = 'halo2::native_pose_normalization_c8827'
PREPARED = '77948ed8a81cc743284b55b696800bf59db72b90720b1930d0b6cc4aabc8a06b'
LABEL = 'Lc8827: '
HOOK = f'''{LABEL}
    if({DETAIL}::run(s,[](auto &v){{run_11f7e0(v);}},[](auto &v){{run_122b60(v);}}))goto Lc8930;
'''


def bind_body(body):
    if digest(body) != PREPARED or body.count(LABEL) != 1:
        raise ValueError('Prepared C8827 pose normalization changed')
    begin, end = body.index(LABEL), body.index('Lc8930:')
    phase = body[begin:end]
    if (phase.count('_mm_mul_ss('), phase.count('_mm_div_ss(')) != (1, 2):
        raise ValueError('C8827 scalar operands changed')
    for child, edges in (('11f7e0', (('c886a', 'c886f'), ('c88f8', 'c88fd'))),
                         ('122b60', (('c88e2', 'c88e7'), ('c88ed', 'c88f2')))):
        for site, continuation in edges:
            if phase.count(f's.push(0x{site}u,0x{continuation}u);run_{child}(s);') != 1:
                raise ValueError(f'C8827 genuine child edge {site} changed')
    phase = phase.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')
    phase = phase.replace('_mm_div_ss(', f'{DETAIL}::divide(')
    phase, pushes = re.subn(r'\bs\.push\((0x[0-9a-f]+u,[^;]*\);)',
                             r'halo2::native_two_bone_solve::push_word(s,\1', phase)
    if pushes != 4:
        raise ValueError('C8827 precise call stack sites changed')
    return body[:begin] + phase.replace(LABEL, HOOK, 1) + body[end:]


def install(canonical, output):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return output, []
    original = BODY.search(canonical)
    prepared = BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical C8827 pose normalization changed')
    for child in ('11f7e0', '122b60'):
        if any(text.count(f'static void run_{child}(WorldState &s) {{') != 1 for text in (canonical, output)):
            raise ValueError(f'C8827 genuine child target {child} changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], ['c85a0']
