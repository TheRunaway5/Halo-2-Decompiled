"""Bind the contrail transition-curve copy and keep its original CRT call."""

from hashlib import sha256

from split_native_translation_unit import BODY


CANONICAL = '7fbddc572524c44f7c58c00c4fe40009b7dc33741d1361d08780fd0015afe055'
PREPARED = '4004c165596e88448e7e9d1593010ca663c18a11d67f31d1c87dc5992282331a'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_transition_curve.hpp"\n'
BODY_START = 'static void run_17f8a0(WorldState &s) {\n'
REMAINDER_CALL = ('s.instruction_literal(0x17f8e9u); '
                  '{const auto target=0x37dc0au;s.push(0x17f8e9u,0x17f8eeu);dispatch(s,target,0x17f8e9u,false);}')
REMAINDER_RETURN = 's.instruction_literal(0x17f8eeu);'


def owner(text):
    matches = [match for match in BODY.finditer(text) if match[1] == 'run_17f8a0']
    if len(matches) != 1:
        raise ValueError('Expected one contrail transition-curve owner at 17F8A0')
    return matches[0]


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    if tuple(sha256(match[0].encode()).hexdigest() for match in (original, current)) != (CANONICAL, PREPARED):
        raise ValueError('Contrail transition curve differs from the pinned source')
    body = current[0]
    if INCLUDE in prepared or body.count(REMAINDER_CALL) != 1 or body.count(REMAINDER_RETURN) != 1:
        raise ValueError('Contrail transition curve call boundaries changed')

    hook = ('    {\n'
            '        using namespace halo2::native_contrail_transition_curve_detail;\n'
            '        TransitionCurve curve(s);\n'
            '        switch (curve.begin(native_read64)) {\n'
            '        case Continuation::retained: break;\n'
            '        case Continuation::finished: return;\n'
            '        case Continuation::remainder:\n'
            '            s.push(0x17f8e9u,0x17f8eeu);dispatch(s,0x37dc0au,0x17f8e9u,false);\n'
            '            if (curve.after_remainder(native_single_spill)) return;\n'
            '            goto L_native_contrail_after_remainder;\n'
            '        }\n'
            '    }\n')
    body = body.replace(BODY_START, BODY_START + hook, 1)
    body = body.replace(REMAINDER_RETURN, 'L_native_contrail_after_remainder: ' + REMAINDER_RETURN, 1)
    return INCLUDE + prepared[:current.start()] + body + prepared[current.end():]
