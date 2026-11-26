"""Bind the 103D68 and 103E32 death-cycle cleanup table arms."""

from collections import Counter
import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest


INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_motion_cleanup.hpp"\n'
DETAIL = 'halo2::native_death_cycle_motion_cleanup'
PHASE = '8b44348cc26890f142320c75fef25252b666549c0ba5849957891122557e167d'
START = '\nL103d68: '
STOP = '\nL103ed4: '
ENTRIES = (('103d68', 'active_record'), ('103e32', 'pending_record'))
DIRECT = (('103e0b', '103e10', '17a0f0'), ('103eb7', '103ebc', '17a0f0'))
DISPATCHED = (('103e28', '103e2d', '107e30'), ('103e54', '103e59', '107ee0'), ('103e70', '103e75', '107fe0'))


def label(pc):
    return f'\nL{pc}: '


def hook(pc, entry):
    return label(pc) + f'''
    if ({DETAIL}::run(s,{DETAIL}::Entry::{entry},
        [](auto &state, std::uint32_t target, std::uint32_t site) {{ dispatch(state,target,site,false); }},
        run_17a0f0)) goto L1040f0;
    '''


def bind_body(body):
    prefix, start, tail = body.partition(START)
    phase, stop, remainder = tail.partition(STOP)
    if not start or not stop or digest(phase) != PHASE:
        raise ValueError('Prepared 103D68–103ED4 death-cycle motion cleanup changed')
    sites = re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)', phase)
    if len(sites) != 76 or len(set(sites)) != 76:
        raise ValueError('Death-cycle motion cleanup instruction set changed')
    inbound = Counter(target for target in re.findall(r'goto L([0-9a-f]+);', prefix+remainder) if target in sites)
    if inbound != {'103d68':2, '103e32':2}:
        raise ValueError('Death-cycle motion cleanup incoming paths changed')

    for pc, successor, child in DIRECT:
        if phase.count(f's.push(0x{pc}u,0x{successor}u);run_{child}(s);') != 1:
            raise ValueError(f'Original death-cycle direct call {pc} changed')
    for pc, successor, child in DISPATCHED:
        call = (f'const auto target=0x{child}u;s.push(0x{pc}u,0x{successor}u);'
                f'dispatch(s,target,0x{pc}u,false);')
        if phase.count(call) != 1:
            raise ValueError(f'Original death-cycle dispatched call {pc} changed')
    phase, pushes = re.subn(r'\bs\.push\((0x[0-9a-f]+u,)',
                             r'halo2::native_two_bone_solve::push_word(s,\1', phase)
    if pushes != 7:
        raise ValueError('Death-cycle motion cleanup stack operations changed')
    for pc, address, width, operation, operand, mask, bits in (
        ('103dc7', '(0x1a8u+s.gp[0]+s.gp[1]*1u)', 1, '|', '0x1u', '0xffu', '1u'),
        ('103ec7', '(0x210u+s.gp[3]+s.gp[5]*1u)', 4, '&', '0xffffffbfu', '0xffffffbfu', '0u'),
    ):
        original = (f'w(s.memory,s.observe,0x{pc}u,{address},{width},'
                    f's.flags.logic((r(s.memory,{address},{width})){operation}({operand}),{width}));')
        replacement = f'halo2::native_death_cycle_motion::update_field(s,0x{pc}u,{address},{width},{mask},{bits});'
        if phase.count(original) != 1:
            raise ValueError(f'Death-cycle cleanup record update {pc} changed')
        phase = phase.replace(original, replacement, 1)
    if phase.count(label('103e32')) != 1:
        raise ValueError('Original motion cleanup table entry changed')
    phase = phase.replace(label('103e32'), hook(*ENTRIES[1]), 1)
    return prefix + hook(*ENTRIES[0]) + phase + stop + remainder


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original, prepared = BODY.search(canonical), BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical death-cycle motion cleanup owner changed')
    for text in (canonical, output):
        if text.count('static void run_17a0f0(WorldState &s) {') != 1:
            raise ValueError('Original effect teardown owner changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], [OWNER]
