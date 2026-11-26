"""Bind the authenticated 13A070 contrail bitmap selector."""

from hashlib import sha256

from split_native_translation_unit import BODY


CANONICAL = 'bf5ac54ef1ad4b0a0b153ab512a663eb01fb4cc635c976a8e7c54b6adc74fa02'
PREPARED = '0badfad63d04214fa71e3990dd0688dcf75157e623fba0534627874fa162e377'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_bitmap_selector.hpp"\n'
ENTRY = 'static void run_13a070(WorldState &s) {\n'
HOOK = '    if (halo2::native_contrail_bitmap_selector_detail::bridge(s)) return;\n'
PUBLIC_ENTRY = 'case 0x13a070u:run_13a070(s);break;'


def owner(text):
    matches = [match for match in BODY.finditer(text) if match[1] == 'run_13a070']
    if len(matches) != 1:
        raise ValueError('Expected one contrail bitmap selector owner at 13A070')
    return matches[0]


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    pins = ((original[0], CANONICAL), (current[0], PREPARED))
    if (INCLUDE in prepared or not current[0].startswith(ENTRY) or
            any(sha256(value.encode()).hexdigest() != pin for value, pin in pins) or
            canonical.count(PUBLIC_ENTRY) != 1 or prepared.count(PUBLIC_ENTRY) != 1):
        raise ValueError('Contrail bitmap selector differs from the pinned source or entry')
    bound = current[0].replace(ENTRY, ENTRY + HOOK, 1)
    return INCLUDE + prepared[:current.start()] + bound + prepared[current.end():]
