"""Bind the adjacent authored sample selected by the 42CE0 point sampler."""

from hashlib import sha256

from native_contrail_vertex_emission_replacements import CANONICAL, owner


PREPARED = '6a321f3775feac96d0b04e044aaa2a7645054ac823ef3fee89d4aa01d3b5738e'
CANONICAL_SPAN = '1381d61ea14361b0753ac56670a352947deff74d5edd2016a2b9c8697f7c07dd'
PREPARED_SPAN = 'e604e6121529ff59bd5669a0af6d13583bb2fa58eda32e46f1e510b74faafdf0'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_adjacent_sample.hpp"\n'
CANONICAL_ENTRY = 'if(s.services.instruction){s.services.instruction(0x42eabu);}'
ENTRY = 'L_native_contrail_adjacent: '
REJOIN = 'L42fc4: '
HOOK = ('\n    if (halo2::native_contrail_adjacent_sample_detail::bridge(s))\n'
        '        goto L42fc4;\n')


def span(body, entry=ENTRY):
    if body.count(entry) != 1 or body.count(REJOIN) != 1 or body.index(entry) >= body.index(REJOIN):
        raise ValueError('Expected the original adjacent contrail sampling boundaries')
    return body[body.index(entry):body.index(REJOIN)]


def ordered(block):
    for op, name, count in (('mul', 'mul', 10), ('add', 'sum', 9), ('sub', 'difference', 9)):
        token = f'_mm_{op}_ss('
        if block.count(token) != count:
            raise ValueError(f'Unexpected adjacent contrail sample {op} operand sites')
        block = block.replace(token, f'halo2::native_tu15_leaves::ordered_{name}_ss(')
    return block


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    check = ((original[0], CANONICAL), (current[0], PREPARED),
             (span(original[0], CANONICAL_ENTRY), CANONICAL_SPAN), (span(current[0]), PREPARED_SPAN))
    if any(sha256(block.encode()).hexdigest() != pin for block, pin in check) or INCLUDE in prepared:
        raise ValueError('Adjacent contrail sample differs from the pinned source')
    start = current.start() + current[0].index(ENTRY)
    end = current.start() + current[0].index(REJOIN)
    bound = ordered(prepared[start:end]).replace(ENTRY, ENTRY + HOOK, 1)
    return INCLUDE + prepared[:start] + bound + prepared[end:]
