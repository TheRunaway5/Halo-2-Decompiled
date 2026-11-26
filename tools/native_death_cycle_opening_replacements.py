"""Bind the authenticated TU1.5 1030F0 death-cycle opening."""

import hashlib
import re


OWNER = '1030f0'
BODY = re.compile(r'^static void run_1030f0\(WorldState &s\) \{.*?^\}(?=\n\n)', re.M | re.S)
CANONICAL = '92a27adb72d2185d63d8d6b192f3f3a2c6a1bae0dc156de8db8fdfd437d7d907'
PREPARED = 'd76f60c18957ebdbd725df0c2b9b7fc661eceac9fd4ee25032f5fc34c28966a5'
INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_opening.hpp"\n'
DETAIL = 'halo2::native_death_cycle_opening'
CONTINUATION = '\nL1031cb: '
CALLS = (('103137', '10313c', '106760'), ('10315d', '103162', 'd39d0'),
         ('1031a2', '1031a7', '107080'))
STACK_SITES = ('1030f7', '1030f8', '1030f9', '103101', '10312a', *(pc for pc, _, _ in CALLS))
HOOK = (f'\n    if ({DETAIL}::run(s,run_106760,run_d39d0,run_107080)) goto L1031cb;\n')


def digest(value):
    return hashlib.sha256(value.encode()).hexdigest()


def bind_body(body):
    if digest(body) != PREPARED:
        raise ValueError('Prepared 1030F0 death-cycle owner changed')
    prefix, join, tail = body.partition(CONTINUATION)
    for pc, next_pc, child in CALLS:
        route = f's.push(0x{pc}u,0x{next_pc}u);run_{child}(s);'
        if prefix.count(route) != 1:
            raise ValueError(f'1030F0 original call {pc} changed')
    if not join or any(int(target, 16) < 0x1031cb for target in re.findall(r'goto L([0-9a-f]+)\b', tail)):
        raise ValueError('1030F0 original continuation or inbound edges changed')
    if len(re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\)', prefix)) != 60:
        raise ValueError('1030F0 native opening instruction set changed')

    # The reference path also needs x86 PUSH/CALL commit order on provider faults.
    for pc in STACK_SITES:
        original = f's.push(0x{pc}u,'
        if prefix.count(original) != 1:
            raise ValueError(f'1030F0 original stack operation {pc} changed')
        prefix = prefix.replace(original, f'halo2::native_two_bone_solve::push_word(s,0x{pc}u,', 1)
    opening = prefix.index('{') + 1
    return prefix[:opening] + HOOK + prefix[opening:] + join + tail


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original, bound = list(BODY.finditer(canonical)), list(BODY.finditer(output))
    if len(original) != 1 or len(bound) != 1 or digest(original[0][0]) != CANONICAL or INCLUDE in output:
        raise ValueError('Canonical 1030F0 death-cycle owner changed')
    for source in (canonical, output):
        if source.count('case 0x1030f0u:run_1030f0(s);break;') != 1:
            raise ValueError('1030F0 public dispatch route changed')
        for _, _, child in CALLS:
            if source.count(f'static void run_{child}(WorldState &s) {{') != 1:
                raise ValueError(f'1030F0 original child owner {child} changed')
    match = bound[0]
    return INCLUDE + output[:match.start()] + bind_body(match[0]) + output[match.end():], [OWNER]
