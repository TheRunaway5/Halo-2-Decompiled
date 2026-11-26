"""Bind the authored angle conversion and second-axis reset in C85A0."""

import re

from native_pose_setup_c85a0_replacements import BODY, CANONICAL, digest


INCLUDE = '#include "physics/runtime/native_tu15_pose_angle_c8930.hpp"\n'
DETAIL = 'halo2::native_pose_angle_c8930'
PREPARED = '86884b535b4285fe94aaf2c7638459c24e733ed455d6715fad57a7dab2a1a7b6'
LABEL = 'Lc8930: '
HOOK = f'''{LABEL}
    switch({DETAIL}::run(s,[](auto &v){{run_32cc0c(v);}},[](auto &v){{run_cdf30(v);}})) {{
    case {DETAIL}::Resume::authored_axis: goto Lc89e3;
    case {DETAIL}::Resume::finish: goto Lc8b65;
    case {DETAIL}::Resume::retained: break;
    }}
'''


def bind_body(body):
    if digest(body) != PREPARED or body.count(LABEL) != 1:
        raise ValueError('Prepared C8930 angle continuation changed')
    begin, end = body.index(LABEL), body.index('Lc89e3:')
    phase = body[begin:end]
    phase, loads = re.subn(r's\.fp\.push\(ext\(f\((r\(s\.memory,\(0x(?:14|18)u\+s\.gp\[4\]\),4\))\)\)\);',
                            rf'{DETAIL}::load_single(s,\1);', phase)
    phase, products = re.subn(r's\.fp\.st\(0\)=mul\(s\.fp\.st\(0\),ext\(f\((r\(s\.memory,\(0x46a148u\),4\))\)\)\);',
                               rf'{DETAIL}::scale_angle(s,\1);', phase)
    if (loads, products, phase.count('_mm_mul_ss(')) != (2, 2, 2):
        raise ValueError('C8930 x87 or SSE operands changed')
    phase = phase.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')
    phase, pushes = re.subn(r'\bs\.push\((0x[0-9a-f]+u,[^;]*\);)',
                             r'halo2::native_two_bone_solve::push_word(s,\1', phase)
    if pushes != 5:
        raise ValueError('C8930 child or argument stack sites changed')
    return body[:begin] + phase.replace(LABEL, HOOK, 1) + body[end:]


def install(canonical, output):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return output, []
    original = BODY.search(canonical)
    prepared = BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical C8930 angle continuation changed')
    for child in ('32cc0c', 'cdf30'):
        if any(text.count(f'static void run_{child}(WorldState &s) {{') != 1 for text in (canonical, output)):
            raise ValueError(f'C8930 genuine call target {child} changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], ['c85a0']
