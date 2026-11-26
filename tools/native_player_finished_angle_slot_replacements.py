"""Bind the TU1.5 player-finished angle delta test and its positive exits."""

import re

from native_model_marker_lookup_replacements import function, digest


INPUT_PIN = '5f3703a560e0211b01d4750bb78c97f6004951df07577a371179d62b9cb9c5d3'
INCLUDE = '#include "gameplay/multiplayer/native_tu15_player_finished_angle_slot.hpp"\n'
NAMESPACE = 'halo2::native_player_finished_angle_detail'
RESUMES = (
    '1f07bf', '1f07c0', '1f07c5', '1f07c9', '1f07ca', '1f07cb', '1f0879',
    '1f0885', '1f088f', '1f0895', '1f0899', '1f089a', '1f089b', '1f089c',
    '1f08a2', '1f08a8', '1f08af', '1f08b0', '1f08bc', '1f08c4', '1f08cb',
    '1f08d3', '1f08db', '1f08e1', '1f08e9', '1f08ef', '1f08fb', '1f0903',
    '1f0912', '1f0916', '1f091e', '1f092e', '1f0936', '1f0943', '1f0944',
    '1f0949', '1f094d', '1f094e', '1f094f',
)


def install(prepared):
    owner = function(prepared, 'run_1f0670')
    if digest(owner) != INPUT_PIN:
        return prepared, False
    body = owner
    for site in RESUMES:
        anchor = rf'(?m)^(?:L{site}: )?(s\.instruction_literal\(0x{site}u\);)'
        body, count = re.subn(anchor, lambda m: m[0][:-len(m[1])] +
                              f'L_native_finished_angle_{site}: {m[1]}', body)
        if count != 1:
            raise ValueError(f'Original angle-slot trace continuation {site} drifted')
    for site, early in (('1f07be', True), ('1f0872', False)):
        anchor = f'L{site}: s.instruction_literal(0x{site}u);'
        if body.count(anchor) != 1:
            raise ValueError(f'Original angle-slot entry {site} drifted')
        resumes = tuple(x for x in RESUMES if (int(x, 16) < 0x1f0872) == early)
        cases = ''.join(f'    case 0x{pc}u: goto L_native_finished_angle_{pc};\n' for pc in resumes)
        exits = '' if early else '    case 0x1f0ed4u: goto L1f0ed4;\n    case 0x1f07beu: goto L1f07be;\n'
        hook = (f'L{site}:\n    switch ({NAMESPACE}::bridge<{str(early).lower()}>(s)) {{\n'
                '    case 1u: return;\n' + cases + exits
                + f'    default: break;\n    }}\ns.instruction_literal(0x{site}u);')
        body = body.replace(anchor, hook, 1)
    if prepared.count(owner) != 1:
        raise ValueError('Expected exactly one authenticated angle-slot owner')
    return INCLUDE + prepared.replace(owner, body, 1), True
