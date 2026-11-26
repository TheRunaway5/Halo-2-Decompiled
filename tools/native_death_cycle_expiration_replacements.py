"""Bind the 1030F0 motion completion and per-record timer expiration."""

import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest


INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_expiration.hpp"\n'
DETAIL = 'halo2::native_death_cycle_expiration'
PHASE = 'e7e1d5ab271c0bb347915e8266883c83f93e2576a0a140e58e52ec14423ae44c'
FIRST = '\nL1040f0: '
END = '\nL104204: '
ENTRIES = (('1040f0', 'finish_motion'), ('10412e', 'begin_records'), ('10414b', 'expire_record'))
DESTINATIONS = ('103753', '104204', '1042a6', '10434c', '104360', '104371', '10463b')


def label(pc):
    return f'\nL{pc}: '


def hook(pc, entry):
    return (label(pc) + f'\n    switch ({DETAIL}::run(s,{DETAIL}::Entry::{entry},canonical_branch)) {{\n'
            + ''.join(f'        case 0x{target}u: goto L{target};\n' for target in DESTINATIONS)
            + '        default: break;\n    }\n    ')


def bind_body(body):
    head, first, tail = body.partition(FIRST)
    phase, end, remainder = tail.partition(END)
    if not first or not end or digest(phase) != PHASE:
        raise ValueError('Prepared 1040F0–1041FD death-cycle expiration changed')
    if phase.count('_mm_mul_ss(') != 2 or phase.count('_mm_sub_ss(') != 2:
        raise ValueError('Original death-cycle timer arithmetic changed')
    if len(re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\)', phase)) != 69:
        raise ValueError('Death-cycle expiration instruction set changed')
    phase = phase.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')
    phase = phase.replace('_mm_sub_ss(', f'{DETAIL}::subtract(')
    for pc, entry in ENTRIES[1:]:
        if phase.count(label(pc)) != 1:
            raise ValueError(f'Original expiration entry {pc} changed')
        phase = phase.replace(label(pc), hook(pc, entry), 1)
    return head + hook(*ENTRIES[0]) + phase + end + remainder


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original = BODY.search(canonical)
    prepared = BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical death-cycle expiration owner changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], [OWNER]
