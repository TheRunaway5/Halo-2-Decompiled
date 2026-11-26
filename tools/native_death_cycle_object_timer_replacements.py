"""Bind object timer updates and original notification calls at 1032A7."""

import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest


INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_object_timer.hpp"\n'
DETAIL = 'halo2::native_death_cycle_object_timer'
PHASE = 'aca5ec3562bc55d9ea4e0846ded99391cd95ebea55b0f6a4225bb36bdc6e609b'
FIRST = '\nL1032a7: '
END = '\nL103436: '
NEXT = '103436'
CALLS = (('1032f9', '1032fe', 'fc8d0'), ('103313', '103318', '10a910'),
         ('1033af', '1033b4', '108a70'))
HOOK = FIRST + f'''
    if ({DETAIL}::run(s,[](auto &state, std::uint32_t target, std::uint32_t site) {{
            dispatch(state,target,site,false);
        }})) goto L103436;
    '''


def replace_once(phase, old, new):
    if phase.count(old) != 1:
        raise ValueError(f'Original object timer statement changed: {old}')
    return phase.replace(old, new, 1)


def bind_body(body):
    head, start, tail = body.partition(FIRST)
    phase, stop, remainder = tail.partition(END)
    if not start or not stop or digest(phase) != PHASE:
        raise ValueError('Prepared 1032A7 object timer phase changed')
    if len(re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\)', phase)) != 93:
        raise ValueError('Original object timer instruction set changed')
    for pc, resume, target in CALLS:
        call = f'const auto target=0x{target}u;s.push(0x{pc}u,0x{resume}u);dispatch(s,target,0x{pc}u,false);'
        if phase.count(call) != 1:
            raise ValueError(f'Original object timer call {pc} changed')
    phase, pushes = re.subn(r'\bs\.push\((0x[0-9a-f]+u,)',
                             r'halo2::native_two_bone_solve::push_word(s,\1', phase)
    if pushes != 6:
        raise ValueError('Original object timer stack operations changed')
    phase = replace_once(phase,
        'w(s.memory,s.observe,0x1032b5u,(0x19eu+s.gp[5]),2,s.flags.increment(r(s.memory,(0x19eu+s.gp[5]),2),2,true));',
        f'{DETAIL}::decrement_counter(s);')
    for pc, operator, operand, mask, bits in (
        ('1032c6', '|', '0x20u', '0xffu', '0x20u'), ('103334', '&', '0xefu', '0xefu', '0u'),
        ('1033b8', '|', '0x10u', '0xffu', '0x10u'),
    ):
        field = '(0x16cu+s.gp[5])'
        original = f'w(s.memory,s.observe,0x{pc}u,{field},1,s.flags.logic((r(s.memory,{field},1)){operator}({operand}),1));'
        accurate = f'halo2::native_death_cycle_motion::update_field(s,0x{pc}u,{field},1,{mask},{bits});'
        phase = replace_once(phase, original, accurate)
    for intrinsic, ordered, count in (
        ('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(', 2),
        ('_mm_add_ss(', 'halo2::native_world_leaves_detail::ordered_add_ss(', 1),
        ('_mm_sub_ss(', 'halo2::native_death_cycle_expiration::subtract(', 1),
    ):
        if phase.count(intrinsic) != count:
            raise ValueError('Original object timer arithmetic changed')
        phase = phase.replace(intrinsic, ordered)
    return head + HOOK + phase + stop + remainder


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original = BODY.search(canonical)
    prepared = BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical death-cycle object timer owner changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], [OWNER]
