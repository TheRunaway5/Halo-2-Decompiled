"""Bind contrail orientation inputs while preserving the genuine jump table."""

from hashlib import sha256

from native_contrail_vertex_emission_replacements import CANONICAL, owner


CANONICAL_SPAN = 'bc55ff300813a31cf8a0a0117e875c7d8f542ee9802ae7a634912fbe13f5b11c'
PREPARED_SPAN = 'f94d3a38dfdf146a207e8e3e7590abcb2dbd143050111b691f4a43d9acc96213'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_orientation_setup.hpp"\n'
ENTRY = 'L43080: '
REJOIN = 'L430ce: '
DISPATCH = 's.instruction_literal(0x430c7u);'


def span(body):
    if body.count(ENTRY) != 1 or body.count(REJOIN) != 1 or body.index(ENTRY) >= body.index(REJOIN):
        raise ValueError('Contrail orientation input boundaries changed')
    return body[body.index(ENTRY):body.index(REJOIN)]


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    block = span(current[0])
    pins = ((original[0], CANONICAL), (span(original[0]), CANONICAL_SPAN), (block, PREPARED_SPAN))
    if (any(sha256(text.encode()).hexdigest() != pin for text, pin in pins) or INCLUDE in prepared
            or block.count(DISPATCH) != 1 or block.count('_mm_mul_ss(') != 1):
        raise ValueError('Contrail orientation input differs from its pinned source')

    detail = 'halo2::native_contrail_orientation_setup_detail'
    replacement = block.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')
    replacement = replacement.replace(ENTRY, ENTRY + '\n'
        '    {\n'
        f'        using namespace {detail};\n'
        '        switch (prepare(s)) {\n'
        '        case Next::retained: break;\n'
        '        case Next::dispatch: goto L_native_contrail_orientation_dispatch;\n'
        '        case Next::skip: goto L436df;\n'
        '        }\n'
        '    }\n', 1)
    replacement = replacement.replace(DISPATCH, 'L_native_contrail_orientation_dispatch: ' + DISPATCH, 1)
    start = current.start() + current[0].index(ENTRY)
    return INCLUDE + prepared[:start] + replacement + prepared[start + len(block):]
