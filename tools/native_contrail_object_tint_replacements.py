"""Bind contrail object tint while keeping the genuine BCD80 call."""

from hashlib import sha256

from native_contrail_vertex_emission_replacements import CANONICAL, owner


CANONICAL_SPAN = '1926742d4ff1436f873e5e56867090e616fec7b2b074228253a039062c80568a'
PREPARED_SPAN = '887912b0384fbbcdd2323b44385d0842104c3307013e14fec9f0882d5f01dcbf'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_object_tint.hpp"\n'
ENTRY = 'L42fc4: '
REJOIN = 'L43080: '
CALL = 's.instruction_literal(0x4303du); s.push(0x4303du,0x43042u);run_bcd80(s);'
RETURN = 's.instruction_literal(0x43042u);'


def span(body):
    if body.count(ENTRY) != 1 or body.count(REJOIN) != 1 or body.index(ENTRY) >= body.index(REJOIN):
        raise ValueError('Contrail object tint boundaries changed')
    return body[body.index(ENTRY):body.index(REJOIN)]


def ordered(block):
    token = '_mm_mul_ss('
    if block.count(token) != 3:
        raise ValueError('Contrail object tint product count changed')
    return block.replace(token, 'halo2::native_tu15_leaves::ordered_mul_ss(')


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    block = span(current[0])
    check = ((original[0], CANONICAL), (span(original[0]), CANONICAL_SPAN), (block, PREPARED_SPAN))
    if (any(sha256(text.encode()).hexdigest() != pin for text, pin in check) or INCLUDE in prepared
            or block.count(CALL) != 1 or block.count(RETURN) != 1):
        raise ValueError('Contrail object tint differs from its pinned source')

    detail = 'halo2::native_contrail_object_tint_detail'
    replacement = ordered(block).replace(ENTRY, ENTRY + '\n'
        '    {\n'
        f'        using namespace {detail};\n'
        '        switch (ObjectTint(s).prepare()) {\n'
        '        case Next::retained: break;\n'
        '        case Next::prepare_orientation: goto L43080;\n'
        '        case Next::object_color: goto L_native_contrail_object_color;\n'
        '        }\n'
        '    }\n', 1)
    replacement = replacement.replace(CALL, 'L_native_contrail_object_color: ' + CALL, 1)
    replacement = replacement.replace(RETURN,
        f'    if ({detail}::ObjectTint(s).apply()) goto L43080;\n' + RETURN, 1)
    start = current.start() + current[0].index(ENTRY)
    return INCLUDE + prepared[:start] + replacement + prepared[start + len(block):]
