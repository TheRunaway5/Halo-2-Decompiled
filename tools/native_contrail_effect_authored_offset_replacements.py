"""Bind the complete 21A2A0 authored offset sampler."""

from hashlib import sha256

from native_contrail_effect_authored_vector_replacements import CALL_LABEL, CHILD
from native_contrail_effect_launch_replacements import INCLUDE
from split_native_translation_unit import BODY


CANONICAL = '6e7aa6f33b4f70306a468096ab4c0a1e9ebca687f7772304d9079e662f07bc62'
PREPARED = '89b1a43c1d33a0543b2f560cc3a4ef27580de178daf4de7c1e6dc7a1679372d6'
ENTRY = 'static void run_21a2a0(WorldState &s) {\n'
HOOK = '    if (halo2::native_contrail_effect_launch_detail::sample_authored_offset(s)) return;\n'


def owner(text):
    matches = [match for match in BODY.finditer(text) if match[1] == 'run_21a2a0']
    if len(matches) != 1:
        raise ValueError('Expected one contrail authored offset owner at 21A2A0')
    return matches[0]


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    if (sha256(original[0].encode()).hexdigest() != CANONICAL or sha256(current[0].encode()).hexdigest() != PREPARED
            or any(prepared.count(token) != 1 for token in (INCLUDE, CALL_LABEL + CHILD))
            or not current[0].startswith(ENTRY)):
        raise ValueError('Contrail authored offset differs from its pinned source')
    body = current[0].replace(ENTRY, ENTRY + HOOK, 1)
    return prepared[:current.start()] + body + prepared[current.end():]
