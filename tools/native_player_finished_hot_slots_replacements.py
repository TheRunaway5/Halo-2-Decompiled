"""Bind the measured TU1.5 player-finished latch and multiplayer-player slots."""

import re

from native_model_marker_lookup_replacements import function, digest


INPUT_PIN = 'a8de1fabde2e1e8da05d3b32e81b4a3c13a5d7a42b87c2db44d4bf74662e7a90'
INCLUDE = '#include "gameplay/multiplayer/native_tu15_player_finished_hot_slots.hpp"\n'
FRAME = 'halo2::native_player_finished_detail'
NAMESPACE = 'halo2::native_player_finished_hot_detail'
LATCHES = {0x1f0abd: 0x19a, 0x1f0ae3: 0x199, 0x1f0b90: 0x198}
LATCH_SUCCESSORS = (6, 15, 16, 21, 25, 26, 33, 34)
PLAYER_SUCCESSORS = (0x1f0bbc, 0x1f0bc0, 0x1f0bcb, 0x1f0bd6, 0x1f0be7, 0x1f0bf2, 0x1f0bfd)
SHARED_SUCCESSORS = (0x1f0862, 0x1f0863, 0x1f0868, 0x1f086c, 0x1f086d, 0x1f086e)
CALLS = {0x1f0bc6: 0x1f0bcb, 0x1f0be2: 0x1f0be7}


def bind(body, entry, call, resumes, transfers=()):
    for pc in resumes:
        literal = f's.instruction_literal(0x{pc:x}u);'
        body, count = re.subn(f'(?m)^{re.escape(literal)}', f'L_native_finished_hot_{pc:x}: {literal}', body)
        if count != 1:
            raise ValueError(f'Original player-finished slot continuation {pc:x} drifted')

    anchor = f'L{entry:x}: s.instruction_literal(0x{entry:x}u);'
    if body.count(anchor) != 1:
        raise ValueError(f'Expected exactly one player-finished slot entry {entry:x}')
    cases = ''.join(f'    case 0x{pc:x}u: goto L_native_finished_hot_{pc:x};\n' for pc in resumes)
    cases += ''.join(f'    case 0x{pc:x}u: goto L{pc:x};\n' for pc in transfers)
    hook = (f'L{entry:x}:\n    switch ({NAMESPACE}::{call}) {{\n'
            f'    case {FRAME}::done: return;\n'
            f'    case {FRAME}::restore_all: goto L1f0ed4;\n'
            + cases + '    default: break;\n    }\n'
            f's.instruction_literal(0x{entry:x}u);')
    return body.replace(anchor, hook, 1)


def install(prepared):
    owner = function(prepared, 'run_1f0670')
    if digest(owner) != INPUT_PIN:
        return prepared, False

    body = owner
    for entry, byte in LATCHES.items():
        body = bind(body, entry, f'consume_latch<0x{entry:x}u,0x{byte:x}u>(s)',
                    tuple(entry + step for step in LATCH_SUCCESSORS))
    for pc, successor in CALLS.items():
        anchor = f's.push(0x{pc:x}u,0x{successor:x}u);'
        if body.count(anchor) != 1:
            raise ValueError(f'Original player-finished slot call {pc:x} drifted')
        body = body.replace(anchor, f'{FRAME}::save_word(s,0x{pc:x}u,0x{successor:x}u);', 1)

    body = bind(body, 0x1f0bb6,
                'multiplayer_player(s, [](WorldState &state) { dispatch(state,0x161530u,0x1f0bc6u,false); }, '
                'run_1274b0)',
                PLAYER_SUCCESSORS, (0x1f085b, 0x1f0861))
    body = bind(body, 0x1f085b, 'gate(s)', (), (0x1f0861,))
    body = bind(body, 0x1f0861, 'complete(s)', SHARED_SUCCESSORS)
    if prepared.count(owner) != 1:
        raise ValueError('Expected exactly one authenticated player-finished owner')
    return INCLUDE + prepared.replace(owner, body, 1), True
