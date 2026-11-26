"""Bind the view fade while preserving the original curve-call frontier."""

from hashlib import sha256

from split_native_translation_unit import BODY


CANONICAL = '5cefe568904e712e9b9b8d7539373a246f52be7020af6b73368d16f1ea4d4cb5'
PREPARED = 'd51c4eba2b9002557b507d6c05488dddf33d8013063b4819a99e195116710e50'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_view_fade.hpp"\n'
BODY_START = 'static void run_42c40(WorldState &s) {\n'
CURVE_CALL = 's.instruction_literal(0x42cb3u); s.push(0x42cb3u,0x42cb8u);run_17f8a0(s);'
CURVE_RETURN = 's.instruction_literal(0x42cb8u);'


def owner(text):
    matches = [match for match in BODY.finditer(text) if match[1] == 'run_42c40']
    if len(matches) != 1:
        raise ValueError('Expected one contrail view-fade owner at 42C40')
    return matches[0]


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    if tuple(sha256(match[0].encode()).hexdigest() for match in (original, current)) != (CANONICAL, PREPARED):
        raise ValueError('Contrail view fade differs from the pinned source')
    body = current[0]
    parent_call = 's.push(0x43573u,0x43578u);run_42c40(s);'
    if (INCLUDE in prepared or body.count(CURVE_CALL) != 1 or body.count(CURVE_RETURN) != 1 or
            prepared.count(parent_call) != 1):
        raise ValueError('Contrail view-fade call boundaries changed')

    hook = ('    {\n'
            '        using namespace halo2::native_contrail_view_fade_detail;\n'
            '        ViewFade fade(s);\n'
            '        switch (fade.run(native_single_spill)) {\n'
            '        case Continuation::retained: break;\n'
            '        case Continuation::finished: return;\n'
            '        case Continuation::curve:\n'
            '            s.push(0x42cb3u,0x42cb8u);run_17f8a0(s);\n'
            '            if (fade.after_curve()) return;\n'
            '            goto L_native_contrail_after_curve;\n'
            '        }\n'
            '    }\n')
    body = body.replace(BODY_START, BODY_START + hook, 1)
    body = body.replace(CURVE_RETURN, 'L_native_contrail_after_curve: ' + CURVE_RETURN, 1)
    return INCLUDE + prepared[:current.start()] + body + prepared[current.end():]
