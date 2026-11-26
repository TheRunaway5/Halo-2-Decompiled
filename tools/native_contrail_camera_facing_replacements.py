"""Bind the camera-facing 42CE0 branch at the authored orientation target."""

from hashlib import sha256

from native_contrail_vertex_emission_replacements import CANONICAL, owner


PREPARED = '38a8c07d719d06b45312a3f170af15f700e535ea9f08351bfb80628a1fdc664c'
CANONICAL_SPAN = '0e4d47db828397992552c1976ad87290071ccf337763f4d820bb4d6435c79c1a'
PREPARED_SPAN = 'f604e1007e848f14c60738496f4e588cee7caff1d2e6e49cb72840924d4b6aae'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_camera_facing.hpp"\n'
ENTRY = 'L432bd: '
REJOIN = 'L43542: '
HOOK = ('\n    if (halo2::native_contrail_camera_facing_detail::bridge(s,native_single_spill))\n'
        '        goto L43542;\n')


def span(body):
    if body.count(ENTRY) != 1 or body.count(REJOIN) != 1 or body.index(ENTRY) >= body.index(REJOIN):
        raise ValueError('Expected the original contrail camera-facing branch boundaries')
    return body[body.index(ENTRY):body.index(REJOIN)]


def ordered(block):
    for op, count in (('mul', 9), ('add', 3)):
        token = f'_mm_{op}_ss('
        if block.count(token) != count:
            raise ValueError(f'Unexpected camera-facing {op} operand sites')
        block = block.replace(token, f'halo2::native_tu15_leaves::ordered_{op}_ss(')
    return block


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    check = ((original[0], CANONICAL), (current[0], PREPARED),
             (span(original[0]), CANONICAL_SPAN), (span(current[0]), PREPARED_SPAN))
    if any(sha256(block.encode()).hexdigest() != pin for block, pin in check) or INCLUDE in prepared:
        raise ValueError('Contrail camera-facing branch differs from the pinned source')
    start = current.start() + current[0].index(ENTRY)
    end = current.start() + current[0].index(REJOIN)
    bound = ordered(prepared[start:end]).replace(ENTRY, ENTRY + HOOK, 1)
    return INCLUDE + prepared[:start] + bound + prepared[end:]
