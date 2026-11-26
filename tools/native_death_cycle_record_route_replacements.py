"""Bind the four authenticated death-cycle record handlers."""

from collections import Counter
import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest


INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_record_route.hpp"\n'
DETAIL = 'halo2::native_death_cycle_record_route'
PHASE = '95836836f13ff46c4b16243b3b29800e7b894655948786f6e5117928e9dcf40c'
ENTRIES = (('104204', 'primary'), ('1042a6', 'secondary'), ('10434c', 'final_motion'), ('104360', 'reset_motion'))
START = '\nL104204: '
STOP = '\nL104371: '
DIRECT = (('10428e', '104293', '1074a0'), ('1042bd', '1042c2', '108f30'),
          ('1042f2', '1042f7', '108f30'), ('104316', '10431b', '108f30'),
          ('10432e', '104333', '108f30'), ('104359', '10435e', '109030'), ('10436c', '104371', '108ea0'))
DISPATCHED = (('1042d8', '1042dd', '1071d0'), ('104304', '104309', '108080'))


def label(pc):
    return f'\nL{pc}: '


def hook(pc, entry):
    return label(pc) + f'''
    if ({DETAIL}::run(s,{DETAIL}::Entry::{entry},
        [](auto &state, std::uint32_t target, std::uint32_t site) {{
            switch (target) {{
                case 0x1074a0u: run_1074a0(state); return;
                case 0x108f30u: run_108f30(state); return;
                case 0x109030u: run_109030(state); return;
                case 0x108ea0u: run_108ea0(state); return;
            }}
            dispatch(state,target,site,false);
        }})) goto L104371;
    '''


def bind_body(body):
    prefix, start, tail = body.partition(START)
    phase, stop, remainder = tail.partition(STOP)
    if not start or not stop or digest(phase) != PHASE:
        raise ValueError('Prepared 104204–104371 death-cycle record handlers changed')
    sites = re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)', phase)
    if len(sites) != 123 or len(set(sites)) != 123:
        raise ValueError('Death-cycle record handler instruction set changed')
    inbound = Counter(target for target in re.findall(r'goto L([0-9a-f]+);', prefix+remainder) if target in sites)
    if inbound != {pc:4 for pc,_ in ENTRIES}:
        raise ValueError('Death-cycle record handler incoming paths changed')

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
    if pushes != 22:
        raise ValueError('Death-cycle record handler stack operations changed')
    for pc in ('10420c','1042f9'):
        original = (f'w(s.memory,s.observe,0x{pc}u,(0x4u+s.gp[6]),1,'
                    's.flags.logic((r(s.memory,(0x4u+s.gp[6]),1))&(0xfeu),1));')
        committed = f'halo2::native_death_cycle_motion::update_field(s,0x{pc}u,0x4u+s.gp[6],1,0xfeu,0u);'
        if phase.count(original) != 1:
            raise ValueError(f'Original record flag update {pc} changed')
        phase = phase.replace(original, committed, 1)
    for pc, entry in ENTRIES[1:]:
        if phase.count(label(pc)) != 1:
            raise ValueError(f'Original record handler entry {pc} changed')
        phase = phase.replace(label(pc), hook(pc, entry), 1)
    return prefix + hook(*ENTRIES[0]) + phase + stop + remainder


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original, prepared = BODY.search(canonical), BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical death-cycle record handler owner changed')
    for text in (canonical, output):
        for child in {child for _, _, child in DIRECT}:
            if text.count(f'static void run_{child}(WorldState &s) {{') != 1:
                raise ValueError(f'Original death-cycle child {child} changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], [OWNER]
