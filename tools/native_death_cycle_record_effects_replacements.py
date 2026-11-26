"""Bind the paired record-effect tables and original calls at 103A6D."""

import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest


INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_record_effects.hpp"\n'
DETAIL = 'halo2::native_death_cycle_record_effects'
PHASE = 'cf2bf78c1026207ae817b573dc41a390b48daa6c94f78f3c549d86b58586370e'
FIRST = '\nL103a6d: '
END = '\nL103cbd: '
CALLS = ((0x103aab, 0x108b60), (0x103abb, 0x108db0), (0x103aeb, 0x107880),
         (0x103b2c, 0x107610), (0x103b71, 0x107610), (0x103baa, 0x108b60),
         (0x103bba, 0x108db0), (0x103bc8, 0x108be0), (0x103c09, 0x107610),
         (0x103c4f, 0x107610), (0x103c87, 0x108b60), (0x103c97, 0x108db0),
         (0x103cac, 0x108be0), (0x103cb8, 0x108ca0))
HOOK = FIRST + f'''
    switch ({DETAIL}::run(s, [](auto &state, std::uint32_t target, std::uint32_t site) {{
            dispatch(state,target,site,false);
        }}, canonical_branch)) {{
        case {DETAIL}::Result::next_effect: goto L103cbd;
        case {DETAIL}::Result::remove_effect: goto L103cc9;
        case {DETAIL}::Result::reset_effect: goto L103cf5;
        case {DETAIL}::Result::expire: goto L1040f0;
        case {DETAIL}::Result::retained: break;
    }}
    '''
FLAG_UPDATE = re.compile(
    r'w\(s\.memory,s\.observe,(0x[0-9a-f]+u),\(([^()]+)\),([14]),'
    r's\.flags\.logic\(\(r\(s\.memory,\(\2\),\3\)\)\|\((0x[0-9a-f]+u)\),\3\)\);')


def bind_body(body):
    head, start, tail = body.partition(FIRST)
    phase, stop, remainder = tail.partition(END)
    if not start or not stop or digest(phase) != PHASE:
        raise ValueError('Prepared 103A6D record-effect region changed')
    if len(re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\)', phase)) != 165:
        raise ValueError('Original record-effect instruction set changed')
    for site, target in CALLS:
        call = f'const auto target=0x{target:x}u;s.push(0x{site:x}u,0x{site+5:x}u);dispatch(s,target,0x{site:x}u,false);'
        if phase.count(call) != 1:
            raise ValueError(f'Original record-effect call {site:x} changed')
    phase, pushes = re.subn(r'\bs\.push\((0x[0-9a-f]+u,)',
                             r'halo2::native_two_bone_solve::push_word(s,\1', phase)
    phase, updates = FLAG_UPDATE.subn(
        r'halo2::native_death_cycle_motion::update_field(s,\1,(\2),\3,0xffffffffu,\4);', phase)
    if pushes != 38 or updates != 7:
        raise ValueError('Original record-effect stack or flag updates changed')
    return head + HOOK + phase + stop + remainder


def install(canonical, output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output, []
    original = BODY.search(canonical)
    prepared = BODY.search(output)
    if not original or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical death-cycle record-effect owner changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], [OWNER]
