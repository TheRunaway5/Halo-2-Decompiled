"""Bind the authenticated 219330 launch preparation around its true child calls."""

from hashlib import sha256

from split_native_translation_unit import BODY


CANONICAL = '515cb19831f2062826290c6aa0d1e962ec0f4b41f078814be460f8a1b2fea56f'
PREPARED_OPEN = '688ddd3dd19fb6abe246676ddc050e23521d4ccfd9c9e6c6ac4cfd9d587ebf9e'
PREPARED_LAUNCH = '14956578c64c4f26f8135d3b19e2a22666ff12711c12560b4484038597f14483'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_effect_launch.hpp"\n'
ENTRY = 'static void run_219330(WorldState &s) {\n'
RESUME = 'L2193dd: '
LOOP = 'L2195b8: '
DIRECT_LOOKUP = 's.instruction_literal(0x2193c0u); s.push(0x2193c0u,0x2193c5u);run_219e40(s);'
TAGGED_LOOKUP = 's.instruction_literal(0x2193d6u); s.push(0x2193d6u,0x2193dbu);run_219e00(s);'
DIRECT_LABEL = 'L_native_contrail_effect_direct_lookup: '
TAGGED_LABEL = 'L_native_contrail_effect_tagged_lookup: '
OPEN_HOOK = ('    {\n'
             '        using namespace halo2::native_contrail_effect_launch_detail;\n'
             '        switch (open(s)) {\n'
             '        case Begin::retained: break;\n'
             '        case Begin::tagged_lookup: goto L_native_contrail_effect_tagged_lookup;\n'
             '        case Begin::direct_lookup: goto L_native_contrail_effect_direct_lookup;\n'
             '        case Begin::finished: return;\n'
             '        }\n'
             '    }\n')
LAUNCH_HOOK = ('\n    {\n'
               '        using namespace halo2::native_contrail_effect_launch_detail;\n'
               '        switch (prepare(s,native_read64,native_single_spill)) {\n'
               '        case Launch::retained: break;\n'
               '        case Launch::projection: goto L219847;\n'
               '        case Launch::allocation: goto L2195c1;\n'
               '        }\n'
               '    }\n')


def owner(text):
    matches = [match for match in BODY.finditer(text) if match[1] == 'run_219330']
    if len(matches) != 1:
        raise ValueError('Expected one contrail effect owner at 219330')
    return matches[0]


def spans(body):
    if (not body.startswith(ENTRY) or any(body.count(label) != 1 for label in (RESUME, LOOP))
            or body.index(RESUME) >= body.index(LOOP)):
        raise ValueError('Contrail effect preparation boundaries changed')
    start, stop = body.index(RESUME), body.index(LOOP)
    return body[:start], body[start:stop], body[stop:]


def ordered(block, opening):
    counts = (2, 0, 0) if opening else (12, 11, 4)
    for (op, helper), count in zip((('mul', 'mul'), ('add', 'sum'), ('sub', 'difference')), counts, strict=True):
        token = f'_mm_{op}_ss('
        if block.count(token) != count:
            raise ValueError(f'Unexpected 219330 preparation {op} operand sites')
        block = block.replace(token, f'halo2::native_tu15_leaves::ordered_{helper}_ss(')
    return block


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    opening, launch, remainder = spans(current[0])
    pins = ((original[0], CANONICAL), (opening, PREPARED_OPEN), (launch, PREPARED_LAUNCH))
    if (INCLUDE in prepared or any(sha256(block.encode()).hexdigest() != pin for block, pin in pins)
            or any(opening.count(call) != 1 for call in (DIRECT_LOOKUP, TAGGED_LOOKUP))
            or any(remainder.count(label) != 1 for label in ('L2195c1: ', 'L219847: ', 'L2198b7: '))):
        raise ValueError('Contrail effect launch differs from the pinned source')
    opening = ordered(opening, True).replace(ENTRY, ENTRY + OPEN_HOOK, 1)
    opening = opening.replace(DIRECT_LOOKUP, DIRECT_LABEL + DIRECT_LOOKUP, 1)
    opening = opening.replace(TAGGED_LOOKUP, TAGGED_LABEL + TAGGED_LOOKUP, 1)
    launch = ordered(launch, False).replace(RESUME, RESUME + LAUNCH_HOOK, 1)
    return INCLUDE + prepared[:current.start()] + opening + launch + remainder + prepared[current.end():]
