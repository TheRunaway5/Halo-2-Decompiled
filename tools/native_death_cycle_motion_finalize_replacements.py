"""Bind death-cycle motion completion and the neighboring checked table arm."""

from collections import Counter
import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest


INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_motion_finalize.hpp"\n'
DETAIL = 'halo2::native_death_cycle_motion_finalize'
PHASE = '0eec0044a872e5c2a0005b6cc9d69f186f9351e861a7c8883a71e1f6018374da'
START = '\nL103cbd: '
STOP = '\nL103d68: '
ENTRIES = (('103cbd', 'conditional_pair'), ('103cc9', 'clear_pair'), ('103cd3', 'apply_pair'),
           ('103cf5', 'reset_fields'), ('103d0f', 'check_motion'))
DIRECT = (('103ce0', '103ce5', '108040'), ('103ceb', '103cf0', '108040'), ('103d5e', '103d63', '108040'))
DISPATCHED = (('103d2a', '103d2f', '108b60'), ('103d4a', '103d4f', '108db0'))


def label(pc):
    return f'\nL{pc}: '


def hook(pc, entry):
    return label(pc) + f'''
    if ({DETAIL}::run(s,{DETAIL}::Entry::{entry},
        [](auto &state, std::uint32_t target, std::uint32_t site) {{ dispatch(state,target,site,false); }},
        run_108040)) goto L1040f0;
    '''


def bind_body(body):
    prefix, start, tail = body.partition(START)
    phase, stop, remainder = tail.partition(STOP)
    if not start or not stop or digest(phase) != PHASE:
        raise ValueError('Prepared 103CBD–103D68 death-cycle motion completion changed')
    sites = re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)', phase)
    if len(sites) != 44 or len(set(sites)) != 44:
        raise ValueError('Death-cycle motion completion instruction set changed')
    inbound = Counter(target for target in re.findall(r'goto L([0-9a-f]+);', prefix+remainder) if target in sites)
    if inbound != {'103cbd':11, '103cc9':1, '103cd3':1, '103cf5':1, '103d0f':2}:
        raise ValueError('Death-cycle motion completion incoming paths changed')

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
        raise ValueError('Death-cycle motion completion stack operations changed')
    address = '(0x210u+s.gp[3]+s.gp[5]*1u)'
    original = (f'w(s.memory,s.observe,0x103d3bu,{address},4,'
                f's.flags.logic((r(s.memory,{address},4))|(0x40u),4));')
    if phase.count(original) != 1:
        raise ValueError('Death-cycle 103D3B record update changed')
    phase = phase.replace(original,
        f'halo2::native_death_cycle_motion::update_field(s,0x103d3bu,{address},4,0xffffffffu,0x40u);',1)
    for pc, entry in ENTRIES[1:]:
        if phase.count(label(pc)) != 1:
            raise ValueError(f'Original motion completion entry {pc} changed')
        phase = phase.replace(label(pc), hook(pc, entry), 1)
    return prefix + hook(*ENTRIES[0]) + phase + stop + remainder


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original, prepared = BODY.search(canonical), BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical death-cycle motion completion owner changed')
    for text in (canonical, output):
        if text.count('static void run_108040(WorldState &s) {') != 1:
            raise ValueError('Original motion application owner changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], [OWNER]
