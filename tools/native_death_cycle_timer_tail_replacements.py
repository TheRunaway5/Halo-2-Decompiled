"""Bind the death-cycle timer update, record back-edge and return at 1044F4."""

from collections import Counter
import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest


INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_timer_tail.hpp"\n'
DETAIL = 'halo2::native_death_cycle_timer_tail'
PHASE = '670880963c1fd73cf93378755572d085093344ec7166dea98ecb0b68129d7c75'
START = '\nL1044f4: '
EPILOGUE = '\nL10463b: '
STOP = '\nthrow std::runtime_error("Original solver fell outside authenticated CFG");'
CALLS = (('104553', '104558', '32cc0c'), ('10456e', '104573', '102fb0'))
HOOK = START + f'''
    switch ({DETAIL}::run(s,[](auto &v, std::uint32_t target, std::uint32_t site) {{
        dispatch(v,target,site,false);
    }})) {{
        case {DETAIL}::Result::next_record: goto L10414b;
        case {DETAIL}::Result::returned: return;
        case {DETAIL}::Result::retained: break;
    }}
    '''
RETURN_HOOK = EPILOGUE + f'\n    if ({DETAIL}::finish(s)) return;\n    '


def exact_replace(phase, original, replacement, expected=1):
    if phase.count(original) != expected:
        raise ValueError(f'Death-cycle timer tail operation changed: {original}')
    return phase.replace(original, replacement)


def bind_body(body):
    prefix, start, tail = body.partition(START)
    phase, stop, remainder = tail.partition(STOP)
    if not start or not stop or digest(phase) != PHASE:
        raise ValueError('Prepared 1044F4–104646 death-cycle timer tail changed')
    sites = re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)', phase)
    if len(sites) != 81 or len(set(sites)) != 81:
        raise ValueError('Death-cycle timer tail instruction set changed')
    inbound = Counter(target for target in re.findall(r'goto L([0-9a-f]+);', prefix) if target in sites)
    if inbound != {'1044f4': 5, '10463b': 4} or not prefix.endswith(
            'L1044f0: s.instruction_literal(0x1044f0u); s.set(7,r(s.memory,(0x28u+s.gp[4]),4),4,0);'):
        raise ValueError('Death-cycle timer tail incoming paths changed')

    for pc, successor, child in CALLS:
        head = f'const auto target=0x{child}u;'
        push = f's.push(0x{pc}u,0x{successor}u);'
        call = f'dispatch(s,target,0x{pc}u,false);'
        fixed_push = f'halo2::native_two_bone_solve::push_word(s,0x{pc}u,0x{successor}u);'
        phase = exact_replace(phase, head + push + call, head + fixed_push + call)

    for original, replacement, count in (
        ('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(', 7),
        ('_mm_sub_ss(', f'{DETAIL}::subtract(', 3),
        ('_mm_add_ss(', 'halo2::native_world_leaves_detail::ordered_add_ss(', 1),
        ('s.fp.push(ext(f(r(s.memory,(0x30u+s.gp[4]),4))));',
         's.fp.push(halo2::native_live_support::x87::load_single(r(s.memory,0x30u+s.gp[4],4)));', 1),
        ('s.fp.push(ext(f(r(s.memory,(0x1cu+s.gp[4]),4))));',
         's.fp.push(halo2::native_live_support::x87::load_single(r(s.memory,0x1cu+s.gp[4],4)));', 2),
        ('s.fp.st(0)=add(s.fp.st(0),ext(f(r(s.memory,(0x40u+s.gp[4]),4))));',
         f'{DETAIL}::scale_extended<false>(s,r(s.memory,0x40u+s.gp[4],4));', 1),
        ('s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,(0x46a148u),4))));',
         f'{DETAIL}::scale_extended<true>(s,r(s.memory,0x46a148u,4));', 1),
        ('s.fp.push(static_cast<long double>(std::bit_cast<double>(native_read64(s.memory,(0x46a0f0u)))));',
         f'{DETAIL}::load_extended_constant(s,0x46a0f0u);', 2),
        ('s.compare_and_pop(s.fp.st(1));', f'{DETAIL}::compare_extended(s);', 1),
        ('s.fp.st(0)=s.fp.st(0);s.fp.pop();', f'{DETAIL}::pop_extended(s);', 1),
        ('w(s.memory,s.observe,0x1045d4u,(0x20u+s.gp[6]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();',
         'halo2::native_two_bone_solve::x87::store_pop(s,0x1045d4u,0x20u+s.gp[6]);', 1),
        (EPILOGUE, RETURN_HOOK, 1),
    ):
        phase = exact_replace(phase, original, replacement, count)
    return prefix + HOOK + phase + stop + remainder


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original, prepared = BODY.search(canonical), BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical death-cycle timer tail owner changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], [OWNER]
