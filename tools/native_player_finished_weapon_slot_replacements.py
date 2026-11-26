"""Bind the complete TU1.5 player-finished weapon-status slot."""

import re

from native_model_marker_lookup_replacements import function, digest


INPUT_PIN = '8fac14139bf216526a178cc061efb9bc7b8697b2cf3a80d7a41e0b0d48e2f29d'
INCLUDE = '#include "gameplay/multiplayer/native_tu15_player_finished_weapon_slot.hpp"\n'
FRAME = 'halo2::native_player_finished_detail'
NAMESPACE = 'halo2::native_player_finished_weapon_detail'
ENTRY = '1f0cb5'
RESUMES = (
    '1f0cb8', '1f0cc7', '1f0ccf', '1f0cd4', '1f0ce0', '1f0ce5', '1f0cf4',
    '1f0cfc', '1f0d01', '1f0d02', '1f0d07', '1f0d0d', '1f0d18', '1f0d26',
    '1f0d36', '1f0d3d', '1f0d3e', '1f0d43', '1f0d47', '1f0d48', '1f0d49',
)
SAVES = {
    '1f0ccf': '0x1f0cd4u',
    '1f0ce4': 's.reg(1,4,0)',
    '1f0cfc': '0x1f0d01u',
    '1f0d01': 's.reg(0,4,0)',
    '1f0d02': '0x1f0d07u',
}


def install(prepared):
    owner = function(prepared, 'run_1f0670')
    if digest(owner) != INPUT_PIN:
        return prepared, False

    body = owner
    for site, value in SAVES.items():
        before = f's.push(0x{site}u,{value});'
        if body.count(before) != 1:
            raise ValueError(f'Original weapon-slot stack write {site} drifted')
        body = body.replace(before, f'{FRAME}::save_word(s,0x{site}u,{value});', 1)
    for site in RESUMES:
        anchor = f'(?m)^s\\.instruction_literal\\(0x{site}u\\);'
        body, count = re.subn(anchor, f'L_native_finished_weapon_{site}: s.instruction_literal(0x{site}u);', body)
        if count != 1:
            raise ValueError(f'Original weapon-slot trace continuation {site} drifted')

    anchor = f'L{ENTRY}: s.instruction_literal(0x{ENTRY}u);'
    if body.count(anchor) != 1 or prepared.count(owner) != 1:
        raise ValueError('Expected exactly one authenticated weapon-slot entry')
    cases = ''.join(f'    case 0x{pc}u: goto L_native_finished_weapon_{pc};\n' for pc in RESUMES)
    hook = (f'L{ENTRY}:\n'
            f'    switch ({NAMESPACE}::bridge(s,dispatch)) {{\n'
            f'    case {FRAME}::done: return;\n'
            f'    case {FRAME}::restore_all: goto L1f0ed4;\n'
            + cases + '    default: break;\n    }\n'
            f's.instruction_literal(0x{ENTRY}u);')
    return INCLUDE + prepared.replace(owner, body.replace(anchor, hook, 1), 1), True
