"""Bind the verified multiplayer player-finished frame and selected slot paths."""

import re

from native_model_marker_lookup_replacements import function, digest
import native_world_leaves_replacements as world


OWNER = '1f0670'
PIN = '4927ceb227aa1eed52de0ecfc0df5c5448a24ada8f5698d5e668fb2549a2374d'
PREPARED = 'd2d902acc5e280e557a1292cd4d833e01d4b280688d7506be5d0750a5b933f39'
INCLUDE = '#include "gameplay/multiplayer/native_tu15_player_finished_1f0670.hpp"\n'
NAMESPACE = 'halo2::native_player_finished_detail'
ENTRY_PC = (
    '1f0674', '1f067d', '1f0683', '1f068b', '1f0697', '1f069b', '1f06aa',
    '1f06ad', '1f06bb', '1f06c0', '1f06cf', '1f06d2', '1f06e1', '1f06e5',
    '1f06eb',
)
EXITS = ('1f0ed4', '1f0ed5', '1f0ed6')
RETURN_PC = ('1f0ed5', '1f0ed6', '1f0eda', '1f0edb', '1f0edc')
SAVES = {'1f0673': 5, '1f067c': 6, '1f06ba': 7, '1f06e0': 3}


def resume_cases(sites):
    return ''.join(f'    case 0x{site}u: goto L_native_finished_{site};\n' for site in sites)


HOOK = (f'\n    switch ({NAMESPACE}::prologue(s)) {{\n'
        + resume_cases(ENTRY_PC)
        + ''.join(f'    case 0x{site}u: goto L{site};\n' for site in EXITS)
        + '    default: break;\n    }\n')


def install(canonical, prepared):
    origin = world.body(canonical, OWNER)
    if (not origin or origin[1] != 'WorldState' or world.digest(origin) != PIN or
            'void source_run_multiplayer_engines_tu15(' not in canonical or
            canonical.count('case 0x1f0670u:run_1f0670(s);break;') != 1):
        return prepared, False
    owner = function(prepared, 'run_1f0670')
    if digest(owner) != PREPARED:
        return prepared, False

    body = owner
    for site, register in SAVES.items():
        before = f's.push(0x{site}u,s.reg({register},4,0));'
        if body.count(before) != 1:
            raise ValueError(f'Original player-finished register save {site} drifted')
        body = body.replace(before, f'{NAMESPACE}::save_word(s,0x{site}u,s.reg({register},4,0));', 1)

    for site in (*ENTRY_PC, *RETURN_PC):
        anchor = rf'(?m)^(?:L{site}: )?(s\.instruction_literal\(0x{site}u\);)'
        body, count = re.subn(anchor, lambda m: m[0][:-len(m[1])] +
                              f'L_native_finished_{site}: {m[1]}', body)
        if count != 1:
            raise ValueError(f'Original player-finished trace continuation {site} drifted')

    for site in EXITS:
        prefix = f'L{site}: '
        later = tuple(pc for pc in RETURN_PC if int(pc, 16) > int(site, 16))
        epilogue = (f'L{site}:\n    switch ({NAMESPACE}::epilogue<0x{site}u>(s)) {{\n'
                    f'    case {NAMESPACE}::done: return;\n'
                    + resume_cases(later)
                    + '    default: break;\n    }\n')
        if body.count(prefix) != 1:
            raise ValueError(f'Original player-finished epilogue {site} drifted')
        body = body.replace(prefix, epilogue, 1)

    dispatch = ('L_native_finished_1f06eb: s.instruction_literal(0x1f06ebu); '
                'switch(canonical_branch(r(s.memory,(0x1f0ee0u+s.gp[2]*4u),4),s.image))')
    if body.count(dispatch) != 1:
        raise ValueError('Original player-finished slot table branch drifted')
    branch = ('L_native_finished_1f06eb: { std::uint32_t target; '
              'const auto canonical=[&](std::uint32_t address){return canonical_branch(address,s.image);}; '
              f'if(!{NAMESPACE}::select_slot(s,canonical,target)) '
              '{ s.instruction_literal(0x1f06ebu); target=canonical(r(s.memory,(0x1f0ee0u+s.gp[2]*4u),4)); } '
              'switch(target)')
    body = body.replace(dispatch, branch, 1)
    end = 'default:throw std::runtime_error("Required original solver dispatch target unavailable");}'
    if body.count(end) != 1:
        raise ValueError('Original player-finished slot table cases drifted')
    body = body.replace(end, end + ' }', 1)

    start = body.index('{') + 1
    if prepared.count(owner) != 1:
        raise ValueError('Expected exactly one authenticated player-finished owner')
    return INCLUDE + prepared.replace(owner, body[:start] + HOOK + body[start:], 1), True
