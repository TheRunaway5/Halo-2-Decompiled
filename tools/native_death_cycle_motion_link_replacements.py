"""Bind three death-cycle motion-link table arms and their shared joins."""

from collections import Counter
import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest


INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_motion_link.hpp"\n'
DETAIL = 'halo2::native_death_cycle_motion_link'
PHASE = '470247b0e3d78d0b722899d7f58688cd0eceee4ac1025693287218ac758737c8'
START = '\nL103ed4: '
STOP = '\nL1040f0: '
ENTRIES = (('103ed4', 'select_left'), ('103f1d', 'update_left'), ('104006', 'select_right'))
DIRECT = ()
DISPATCHED = (('103eff', '103f04', '10b330'), ('103f13', '103f18', '108d10'),
              ('103f41', '103f46', '107610'), ('103f9f', '103fa4', '10b330'),
              ('103fd4', '103fd9', '108ca0'), ('103fe5', '103fea', '108d10'),
              ('10402a', '10402f', '107610'), ('1040ad', '1040b2', '10b270'),
              ('1040cb', '1040d0', '108ca0'), ('1040eb', '1040f0', '108c40'))


def label(pc):
    return f'\nL{pc}: '


def hook(pc, entry):
    return label(pc) + f'''
    if ({DETAIL}::run(s,{DETAIL}::Entry::{entry},
        [](auto &state, std::uint32_t target, std::uint32_t site) {{ dispatch(state,target,site,false); }})) goto L1040f0;
    '''


def bind_body(body):
    prefix, start, tail = body.partition(START)
    phase, stop, remainder = tail.partition(STOP)
    if not start or not stop or digest(phase) != PHASE:
        raise ValueError('Prepared 103ED4–1040F0 death-cycle motion link changed')
    sites = re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)', phase)
    if len(sites) != 148 or len(set(sites)) != 148:
        raise ValueError('Death-cycle motion link instruction set changed')
    inbound = Counter(target for target in re.findall(r'goto L([0-9a-f]+);', prefix+remainder) if target in sites)
    if inbound != {pc:2 for pc,_ in ENTRIES}:
        raise ValueError('Death-cycle motion link incoming paths changed')

    for pc, successor, child in DISPATCHED:
        call = (f'const auto target=0x{child}u;s.push(0x{pc}u,0x{successor}u);'
                f'dispatch(s,target,0x{pc}u,false);')
        if phase.count(call) != 1:
            raise ValueError(f'Original death-cycle dispatched call {pc} changed')
    phase, pushes = re.subn(r'\bs\.push\((0x[0-9a-f]+u,)',
                             r'halo2::native_two_bone_solve::push_word(s,\1', phase)
    if pushes != 27:
        raise ValueError('Death-cycle motion link stack operations changed')
    for pc, address, width, operation, operand, mask, bits in (
        ('103f62', '(0x1a8u+s.gp[0]+s.gp[1]*1u)', 1, '|', '0x1u', '0xffu', '1u'),
        ('103f71', '(0x210u+s.gp[3]+s.gp[5]*1u)', 4, '&', '0xfffffffeu', '0xfffffffeu', '0u'),
        ('10404a', '(0x1a8u+s.gp[1]+s.gp[2]*1u)', 1, '|', '0x1u', '0xffu', '1u'),
    ):
        original = (f'w(s.memory,s.observe,0x{pc}u,{address},{width},'
                    f's.flags.logic((r(s.memory,{address},{width})){operation}({operand}),{width}));')
        replacement = f'halo2::native_death_cycle_motion::update_field(s,0x{pc}u,{address},{width},{mask},{bits});'
        if phase.count(original) != 1:
            raise ValueError(f'Death-cycle link record update {pc} changed')
        phase = phase.replace(original, replacement, 1)
    for pc, entry in ENTRIES[1:]:
        if phase.count(label(pc)) != 1:
            raise ValueError(f'Original motion link table entry {pc} changed')
        phase = phase.replace(label(pc), hook(pc, entry), 1)
    return prefix + hook(*ENTRIES[0]) + phase + stop + remainder


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original, prepared = BODY.search(canonical), BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical death-cycle motion link owner changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], [OWNER]
