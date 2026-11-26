"""Bind the record blend and its original notification calls at 104371."""

import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest


INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_record_blend.hpp"\n'
DETAIL = 'halo2::native_death_cycle_record_blend'
PHASE = 'ce283aba220d21e1a577b4832e652c26bfc81a500f26272e6ab134583bfd5d52'
FIRST = '\nL104371: '
END = '\nL1044f4: '
HOOK = FIRST + f'''
    if ({DETAIL}::run(s,[](auto &state, std::uint32_t, std::uint32_t) {{
            run_bc820(state);
        }})) goto L1044f4;
    '''


def bind_body(body):
    head, start, tail = body.partition(FIRST)
    phase, stop, remainder = tail.partition(END)
    if not start or not stop or digest(phase) != PHASE:
        raise ValueError('Prepared 104371 record blend changed')
    if len(re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\)', phase)) != 104:
        raise ValueError('Original record blend instruction set changed')
    for site, resume in (('10443f', '104444'), ('1044e7', '1044ec')):
        call = f's.push(0x{site}u,0x{resume}u);run_bc820(s);'
        if phase.count(call) != 1:
            raise ValueError(f'Original record blend call {site} changed')

    phase, pushes = re.subn(r'\bs\.push\((0x[0-9a-f]+u,)',
                             r'halo2::native_two_bone_solve::push_word(s,\1', phase)
    if pushes != 10:
        raise ValueError('Original record blend stack operations changed')
    for site, op, operand, mask, bits in (
        ('104444', '|', '0x8u', '0xffu', '8u'), ('1044ec', '&', '0xf7u', '0xf7u', '0u'),
    ):
        field = '(0x4u+s.gp[6])'
        original = f'w(s.memory,s.observe,0x{site}u,{field},1,s.flags.logic((r(s.memory,{field},1)){op}({operand}),1));'
        if phase.count(original) != 1:
            raise ValueError(f'Original record blend flag update {site} changed')
        phase = phase.replace(original,
            f'halo2::native_death_cycle_motion::update_field(s,0x{site}u,{field},1,{mask},{bits});')
    for intrinsic, ordered, count in (
        ('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(', 2),
        ('_mm_add_ss(', 'halo2::native_world_leaves_detail::ordered_add_ss(', 1),
        ('_mm_sub_ss(', 'halo2::native_death_cycle_expiration::subtract(', 1),
    ):
        if phase.count(intrinsic) != count:
            raise ValueError('Original record blend arithmetic changed')
        phase = phase.replace(intrinsic, ordered)
    return head + HOOK + phase + stop + remainder


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original = BODY.search(canonical)
    prepared = BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical death-cycle record blend owner changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], [OWNER]
