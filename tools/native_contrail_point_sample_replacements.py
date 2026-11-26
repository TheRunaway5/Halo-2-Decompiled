"""Bind the shared authored point sampling in the 42CE0 geometry owner."""

from hashlib import sha256

from native_contrail_vertex_emission_replacements import CANONICAL, owner


PREPARED = '5e2edd8663f2e263b4befe7481008aee7626f8f5d54facac6765f4b1fda1b71d'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_point_sample.hpp"\n'
RELOAD = 'L42d8b: '
SAMPLE = 'L42d90: '
ADJACENT = 's.instruction_literal(0x42eabu);'


def ordered(body):
    # Scalar x86 arithmetic gives the destination priority for NaN payloads.
    for intrinsic, native, count in (('_mm_mul_ss', 'ordered_mul_ss', 5),
                                     ('_mm_sub_ss', 'ordered_difference_ss', 4),
                                     ('_mm_add_ss', 'ordered_sum_ss', 4)):
        token = intrinsic + '('
        if body.count(token) != count:
            raise ValueError(f'Unexpected contrail point sampling sites for {intrinsic}')
        body = body.replace(token, f'halo2::native_tu15_leaves::{native}(')
    return body


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    if tuple(sha256(match[0].encode()).hexdigest() for match in (original, current)) != (CANONICAL, PREPARED):
        raise ValueError('Contrail point sampling differs from the pinned source')
    body = current[0]
    if INCLUDE in prepared or any(body.count(site) != 1 for site in (RELOAD, SAMPLE, ADJACENT)):
        raise ValueError('Contrail point sampling boundaries changed')

    begin, end = body.index(SAMPLE), body.index(ADJACENT)
    body = body[:begin] + ordered(body[begin:end]) + body[end:]
    detail = 'halo2::native_contrail_point_sample_detail'
    body = body.replace(RELOAD, RELOAD + '\n'
        f'    if ({detail}::PointSample(s).reload_output()) goto L42d90;\n', 1)
    body = body.replace(SAMPLE, SAMPLE + '\n'
        '    {\n'
        f'        using namespace {detail};\n'
        '        switch (PointSample(s).prepare()) {\n'
        '        case Continuation::retained: break;\n'
        '        case Continuation::adjacent: goto L_native_contrail_adjacent;\n'
        '        case Continuation::emit: goto L42fc4;\n'
        '        }\n'
        '    }\n', 1)
    body = body.replace(ADJACENT, 'L_native_contrail_adjacent: ' + ADJACENT, 1)
    return INCLUDE + prepared[:current.start()] + body + prepared[current.end():]
