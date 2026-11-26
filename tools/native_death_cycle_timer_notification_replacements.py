"""Bind death-cycle timer notifications and retained effect transitions at 103436."""

from collections import Counter
import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest


INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_timer_notification.hpp"\n'
DETAIL = 'halo2::native_death_cycle_timer_notification'
PHASE = 'e5eff754091877ed40fee5876e53ad4d6720cab05119c756dc2f9c4938805e8e'
START = '\nL103436: '
STOP = '\nL103657: '
DIRECT = (('10363f', '103644', '17a4a0'), ('103648', '10364d', '17a0f0'))
DISPATCHED = (('10346d', '103472', '106ff0'), ('103479', '10347e', 'cabc0'),
              ('1034a5', '1034aa', '10b060'), ('103571', '103576', '16b44a'),
              ('1035b0', '1035b5', '10b060'), ('10362c', '103631', '108b30'))
RMW = (('103464', '|', '0x2u', '0xffu', '2u'), ('10349a', '&', '0xf7u', '0xf7u', '0u'),
       ('1035a7', '|', '0x4u', '0xffu', '4u'), ('1035d6', '&', '0xf9u', '0xf9u', '0u'))
HOOK = START + f'''
    if ({DETAIL}::run(s,[](auto &state, std::uint32_t target, std::uint32_t site) {{
        if (target == 0x17a4a0u) {{ run_17a4a0(state); return; }}
        if (target == 0x17a0f0u) {{ run_17a0f0(state); return; }}
        dispatch(state,target,site,false);
    }})) goto L103657;
    '''


def exact_replace(phase, original, replacement, expected=1):
    if phase.count(original) != expected:
        raise ValueError(f'Death-cycle timer notification operation changed: {original}')
    return phase.replace(original, replacement)


def bind_body(body):
    prefix, start, tail = body.partition(START)
    phase, stop, remainder = tail.partition(STOP)
    if not start or not stop or digest(phase) != PHASE:
        raise ValueError('Prepared 103436–103657 death-cycle timer notification changed')
    sites = re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)', phase)
    if len(sites) != 135 or len(set(sites)) != 135:
        raise ValueError('Death-cycle timer notification instruction set changed')
    inbound = Counter(target for target in re.findall(r'goto L([0-9a-f]+);', prefix+remainder) if target in sites)
    if inbound != {'103436': 2}:
        raise ValueError('Death-cycle timer notification incoming paths changed')

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
    if pushes != 14:
        raise ValueError('Death-cycle timer notification stack operations changed')
    for pc, operator, operand, mask, bits in RMW:
        address = '(0x16cu+s.gp[5])'
        original = (f'w(s.memory,s.observe,0x{pc}u,{address},1,'
                    f's.flags.logic((r(s.memory,{address},1)){operator}({operand}),1));')
        committed = f'halo2::native_death_cycle_motion::update_field(s,0x{pc}u,{address},1,{mask},{bits});'
        phase = exact_replace(phase, original, committed)
    for original, replacement, count in (
        ('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(', 3),
        ('_mm_sub_ss(', 'halo2::native_death_cycle_expiration::subtract(', 3),
        ('_mm_div_ss(', f'{DETAIL}::divide(', 1),
    ):
        phase = exact_replace(phase, original, replacement, count)
    return prefix + HOOK + phase + stop + remainder


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original, prepared = BODY.search(canonical), BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical death-cycle timer notification owner changed')
    for text in (canonical, output):
        for _, _, child in DIRECT:
            if text.count(f'static void run_{child}(WorldState &s) {{') != 1:
                raise ValueError(f'Original death-cycle child {child} changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], [OWNER]
