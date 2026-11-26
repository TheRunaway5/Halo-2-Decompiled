"""Bind the authenticated C8B65 comparison and original notification edge."""

import re

from native_pose_setup_c85a0_replacements import BODY, CANONICAL, digest


INCLUDE = '#include "physics/runtime/native_tu15_pose_completion_c8b65.hpp"\n'
DETAIL = 'halo2::native_pose_completion_c8b65'
PREPARED = 'd90852026b62dc9e199c31adedd141e59c77c754481b2e072579c6d22f82620a'
LABEL = 'Lc8b65: '
HOOK = f'''{LABEL}
    if({DETAIL}::run(s,[](auto &v){{dispatch(v,0xbda50u,0xc8bffu,false);}}))return;
'''


def bind_body(body):
    if digest(body) != PREPARED or body.count(LABEL) != 1:
        raise ValueError('Prepared C8B65 pose completion changed')
    begin = body.index(LABEL)
    phase = body[begin:]
    argument = r'(r\(s\.memory,\(0x[0-9a-f]+u(?:\+s\.gp\[[45]\])?\),4\))'
    phase, loads = re.subn(r's\.fp\.push\(ext\(f\('+argument+r'\)\)\);',
                            r'halo2::native_pose_angle_c8930::load_single(s,\1);', phase)
    phase, differences = re.subn(r's\.fp\.st\(0\)=sub\(s\.fp\.st\(0\),ext\(f\('+argument+r'\)\)\);',
                                  rf'{DETAIL}::subtract_single(s,\1);', phase)
    if (loads, differences) != (12, 6):
        raise ValueError('C8B65 floating operands changed')
    fixes = (
        ('s.fp.st(0)=std::fabs(s.fp.st(0));', f'{DETAIL}::take_absolute(s);', 6),
        ('s.compare_and_pop(s.fp.st(1));', f'{DETAIL}::compare_threshold(s);', 6),
        ('s.fp.st(0)=s.fp.st(0);s.fp.pop();', f'{DETAIL}::release_difference(s);', 6),
        ('s.push(0xc8bffu,0xc8c04u);', 'halo2::native_two_bone_solve::push_word(s,0xc8bffu,0xc8c04u);', 1),
    )
    for original, precise, count in fixes:
        if phase.count(original) != count:
            raise ValueError('C8B65 floating or notification boundary changed')
        phase = phase.replace(original, precise)
    return body[:begin] + phase.replace(LABEL, HOOK, 1)


def install(canonical, output):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return output, []
    original = BODY.search(canonical)
    prepared = BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical C8B65 pose completion changed')
    dispatch = 'const auto target=0xbda50u;s.push(0xc8bffu,0xc8c04u);dispatch(s,target,0xc8bffu,false);'
    if original[0].count(dispatch) != 1 or prepared[0].count(dispatch) != 1:
        raise ValueError('Genuine C8B65 pose notification changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], ['c85a0']
