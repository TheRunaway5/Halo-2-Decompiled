"""Bind the 1037E7 motion result and preserve both original calls."""

import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest


INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_motion.hpp"\n'
DETAIL = 'halo2::native_death_cycle_motion'
PHASE = '658463b9117fdbc3122208ecbfcaff67bebd453bb57e54f7438b2a1947d3b6f4'
FIRST = '\nL1037e7: '
END = '\nL10387c: '
DESTINATION = '1040f0'
HOOK = FIRST + f'''
    if ({DETAIL}::run(s,
        [](auto &state, std::uint32_t target, std::uint32_t site) {{ dispatch(state,target,site,false); }},
        run_108040)) goto L1040f0;
    '''
RMW = (
    ('10383d', '(0x1a8u+s.gp[2]+s.gp[0]*1u)', 1, '0xffu', '1u'),
    ('10384e', '(0x210u+s.gp[3]+s.gp[5]*1u)', 4, '0xffffffdfu', '0u'),
)


def bind_body(body):
    head, first, tail = body.partition(FIRST)
    phase, end, remainder = tail.partition(END)
    if not first or not end or digest(phase) != PHASE:
        raise ValueError('Prepared 1037E7 motion result changed')
    if len(re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\)', phase)) != 40:
        raise ValueError('Original 1037E7 motion instruction set changed')
    call = ('const auto target=0x107610u;s.push(0x10380bu,0x103810u);'
            'dispatch(s,target,0x10380bu,false);')
    if phase.count(call) != 1 or phase.count('s.push(0x103872u,0x103877u);run_108040(s);') != 1:
        raise ValueError('Original 1037E7 motion child route changed')
    for pc in ('103803', '103805', '10380a', '10380b', '103872'):
        original = f's.push(0x{pc}u,'
        if phase.count(original) != 1:
            raise ValueError(f'Original motion stack instruction {pc} changed')
        phase = phase.replace(original, f'halo2::native_two_bone_solve::push_word(s,0x{pc}u,', 1)
    for pc, address, width, mask, bits in RMW:
        operation = f'(r(s.memory,{address},{width}))'
        operation += '|(0x1u)' if bits != '0u' else '&(0xffffffdfu)'
        original = f'w(s.memory,s.observe,0x{pc}u,{address},{width},s.flags.logic({operation},{width}));'
        if phase.count(original) != 1:
            raise ValueError(f'Original motion update {pc} changed')
        committed = f'{DETAIL}::update_field(s,0x{pc}u,{address},{width},{mask},{bits});'
        phase = phase.replace(original, committed, 1)
    return head + HOOK + phase + end + remainder


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original = BODY.search(canonical)
    prepared = BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical death-cycle motion owner changed')
    if any(text.count('static void run_108040(WorldState &s) {') != 1 for text in (canonical, output)):
        raise ValueError('103872 original motion-application child changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], [OWNER]
