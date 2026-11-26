"""Bind the 219E00 frame while preserving its authored 219E40 call."""

from hashlib import sha256

from native_contrail_effect_launch_replacements import INCLUDE, TAGGED_LOOKUP
from split_native_translation_unit import BODY


CANONICAL = 'd8f70363e11a8d5721d5ea00c8e928f9ecd5006793e7f229bbae2cb3fe741e9b'
PREPARED = 'b34fd76a53b3bbe34a87a982090111d3a21f44310aff15d0cd576249661b25e7'
ENTRY = 'static void run_219e00(WorldState &s) {\n'
CHILD = 's.instruction_literal(0x219e32u); s.push(0x219e32u,0x219e37u);run_219e40(s);'
CALL_LABEL = 'L_native_contrail_effect_lookup_call: '
BEFORE = ('    if (halo2::native_contrail_effect_launch_detail::prepare_tagged_lookup(s))\n'
          '        goto L_native_contrail_effect_lookup_call;\n')
AFTER = '\n    if (halo2::native_contrail_effect_launch_detail::return_tagged_lookup(s)) return;'


def owner(text):
    matches = [match for match in BODY.finditer(text) if match[1] == 'run_219e00']
    if len(matches) != 1:
        raise ValueError('Expected one contrail effect lookup wrapper at 219E00')
    return matches[0]


def install(canonical, prepared):
    original, active = owner(canonical), owner(prepared)
    if (sha256(original[0].encode()).hexdigest() != CANONICAL or sha256(active[0].encode()).hexdigest() != PREPARED
            or prepared.count(INCLUDE) != 1 or prepared.count(TAGGED_LOOKUP) != 1
            or not active[0].startswith(ENTRY) or active[0].count(CHILD) != 1):
        raise ValueError('Contrail effect lookup differs from its pinned source')
    body = active[0].replace(ENTRY, ENTRY + BEFORE, 1).replace(CHILD, CALL_LABEL + CHILD + AFTER, 1)
    return prepared[:active.start()] + body + prepared[active.end():]
