"""Bind the original death-cycle descriptor and activity transition at 1031CB."""

import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest


INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_transition.hpp"\n'
DETAIL = 'halo2::native_death_cycle_transition'
PREPARED = 'c413dc88c128322c26c85f6c64076eb9edd72e9b4f0c1df39167c5a1a4a47298'
PHASE = 'da573f0c8d3677c6a0ed3471bbbdeca2fc216db9cdebac62d2e15f4d4423329d'
START = '\nL1031cb: '
STOP = '\nL1032a7: '
DIRECT = (('1031e6', '1031eb', '10af40'), ('10327d', '103282', '10a8d0'))
DISPATCHED = (('1031d6', '1031db', 'bcdf0'), ('103239', '10323e', '1e3150'),
              ('103250', '103255', '1d39c0'), ('103263', '103268', '10a810'))
HOOK = START + f'''
    if ({DETAIL}::run(s,
        [](auto &v, std::uint32_t target, std::uint32_t site) {{ dispatch(v,target,site,false); }},
        run_10af40,run_10a8d0)) goto L1032a7;
    '''


def bind_body(body):
    if digest(body) != PREPARED:
        raise ValueError('Prepared 1030F0 owner changed before transition binding')
    head, start, tail = body.partition(START)
    phase, stop, retained = tail.partition(STOP)
    if not start or not stop or digest(phase) != PHASE:
        raise ValueError('Prepared 1031CB–1032A7 death-cycle transition changed')
    if len(re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\)', phase)) != 65:
        raise ValueError('Death-cycle transition instruction set changed')
    for site, successor, child in DIRECT:
        if phase.count(f's.push(0x{site}u,0x{successor}u);run_{child}(s);') != 1:
            raise ValueError(f'Original death-cycle direct call {site} changed')
    for site, successor, child in DISPATCHED:
        call = (f'const auto target=0x{child}u;s.push(0x{site}u,0x{successor}u);'
                f'dispatch(s,target,0x{site}u,false);')
        if phase.count(call) != 1:
            raise ValueError(f'Original death-cycle dispatched call {site} changed')
    phase, pushes = re.subn(r'\bs\.push\((0x[0-9a-f]+u,)',
                             r'halo2::native_two_bone_solve::push_word(s,\1', phase)
    if pushes != 14:
        raise ValueError('Death-cycle transition stack operations changed')
    return head + HOOK + phase + stop + retained


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original, bound = BODY.search(canonical), BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not bound or INCLUDE in output:
        raise ValueError('Canonical death-cycle transition owner changed')
    for text in (canonical, output):
        for _, _, child in DIRECT:
            if text.count(f'static void run_{child}(WorldState &s) {{') != 1:
                raise ValueError(f'Original death-cycle child {child} changed')
    return INCLUDE + output[:bound.start()] + bind_body(bound[0]) + output[bound.end():], [OWNER]
