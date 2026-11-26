"""Bind the authored activity scan and motion-table dispatch in 1030F0."""

import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest


INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_activity.hpp"\n'
DETAIL = 'halo2::native_death_cycle_activity'
PREPARED = 'd3988adb2e4cc2580e7c2fe636cb0581bba63f0cb1ece6bdfce8acf2c8b398fc'
PHASE = 'c97489f566e5091e5f4fa4d300319984152fa4a243ef30df5be45609f78b156d'
FIRST = '\nL103657: '
MOTION = '\nL103753: '
END = '\nL1037e7: '
DESTINATIONS = ('1037e7', '10387c', '1038f5', '103994', '103a6d', '103d0f', '103d68',
                '103e32', '103ed4', '103f1d', '104006', '1040f0')
START_HOOK = FIRST + f'''
    switch ({DETAIL}::start(s,run_102d50)) {{
        case {DETAIL}::ActivityExit::motion: goto L103753;
        case {DETAIL}::ActivityExit::next_record: goto L10412e;
        case {DETAIL}::ActivityExit::retained: break;
    }}
    '''
MOTION_HOOK = (MOTION + f'\n    switch ({DETAIL}::motion(s,canonical_branch)) {{\n' +
               ''.join(f'        case 0x{pc}u: goto L{pc};\n' for pc in DESTINATIONS) +
               '        default: break;\n    }\n    ')


def bind_body(body):
    if digest(body) != PREPARED:
        raise ValueError('Prepared 1030F0 death-cycle owner changed before activity binding')
    head, first, tail = body.partition(FIRST)
    phase, end, retained = tail.partition(END)
    if not first or not end or digest(phase) != PHASE or phase.count(MOTION) != 1:
        raise ValueError('Prepared 103657–1037E0 activity phase changed')
    if phase.count('s.push(0x103726u,0x10372bu);run_102d50(s);') != 1:
        raise ValueError('103726 original activity child call changed')
    if len(re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\)', phase)) != 108:
        raise ValueError('103657–1037E0 activity instruction set changed')
    for pc in ('103720', '103725', '103726'):
        original = f's.push(0x{pc}u,'
        if phase.count(original) != 1:
            raise ValueError(f'Original activity stack operation {pc} changed')
        phase = phase.replace(original, f'halo2::native_two_bone_solve::push_word(s,0x{pc}u,', 1)
    return head + START_HOOK + phase.replace(MOTION, MOTION_HOOK, 1) + end + retained


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original = BODY.search(canonical)
    prepared = BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical death-cycle activity owner changed')
    if any(text.count('static void run_102d50(WorldState &s) {') != 1 for text in (canonical, output)):
        raise ValueError('103726 original activity child owner changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], [OWNER]
