"""Bind the 219E40 admission and default-vector exits around its authored path."""

from hashlib import sha256

from native_contrail_effect_launch_replacements import DIRECT_LOOKUP, INCLUDE
from native_contrail_effect_tagged_lookup_replacements import CHILD as TAGGED_LOOKUP
from split_native_translation_unit import BODY


CANONICAL = 'b35b905cc22804551a720632a651394819d480dc1dd8022e7512158e3b4f9853'
PREPARED = '5d409148494c10e201dacdd83f8e4b88fdc34d7627ef0441a0fa814dcbc8ee7e'
ENTRY = 'static void run_219e40(WorldState &s) {\n'
AUTHORED = 's.instruction_literal(0x219e76u);'
CHILD = 's.instruction_literal(0x219ec2u); s.push(0x219ec2u,0x219ec7u);run_21a2a0(s);'
LABEL = 'L_native_contrail_effect_authored_vector: '
HOOK = ('    {\n'
        '        using namespace halo2::native_contrail_effect_launch_detail;\n'
        '        switch (admit_default_vector(s)) {\n'
        '        case DefaultVector::retained: break;\n'
        '        case DefaultVector::authored: goto L_native_contrail_effect_authored_vector;\n'
        '        case DefaultVector::finished: return;\n'
        '        }\n'
        '    }\n')


def owner(text):
    matches = [match for match in BODY.finditer(text) if match[1] == 'run_219e40']
    if len(matches) != 1:
        raise ValueError('Expected one contrail effect vector owner at 219E40')
    return matches[0]


def install(canonical, prepared):
    original, active = owner(canonical), owner(prepared)
    if (sha256(original[0].encode()).hexdigest() != CANONICAL or sha256(active[0].encode()).hexdigest() != PREPARED
            or any(prepared.count(token) != 1 for token in (INCLUDE, DIRECT_LOOKUP, TAGGED_LOOKUP))
            or not active[0].startswith(ENTRY) or any(active[0].count(token) != 1 for token in (AUTHORED, CHILD))):
        raise ValueError('Contrail effect vector owner differs from its pinned source')
    body = active[0].replace(ENTRY, ENTRY + HOOK, 1).replace(AUTHORED, LABEL + AUTHORED, 1)
    return prepared[:active.start()] + body + prepared[active.end():]
