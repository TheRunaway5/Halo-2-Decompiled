"""Bind the genuine BCD80 RGB-row provider in the contrail draw unit."""

from hashlib import sha256

from split_native_translation_unit import BODY


CANONICAL = 'abef96497193be7d2b7a1fd55a080bf54fa219d31393de1760b4afea9e303ec9'
PREPARED = 'd30d80c566d7b836b1287601d1f44dc9356d97d4b23f43de895f7d2a080619fd'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_object_color.hpp"\n'
ENTRY = 'static void run_bcd80(WorldState &s) {\n'
CALL = 's.instruction_literal(0x4303du); s.push(0x4303du,0x43042u);run_bcd80(s);'
DIRECT_ENTRY = 'case 0xbcd80u:run_bcd80(s);break;'


def owner(source):
    candidates = [match for match in BODY.finditer(source) if match[1] == 'run_bcd80']
    if len(candidates) != 1:
        raise ValueError('Expected the original BCD80 contrail provider')
    return candidates[0]


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    pins = ((original[0], CANONICAL), (current[0], PREPARED))
    if (any(sha256(body.encode()).hexdigest() != pin for body, pin in pins)
            or INCLUDE in prepared or not current[0].startswith(ENTRY)
            or any(prepared.count(edge) != 1 for edge in (CALL, DIRECT_ENTRY))):
        raise ValueError('BCD80 contrail source or call boundary changed')
    body = current[0].replace(ENTRY, ENTRY +
        '    if (halo2::native_contrail_object_color_detail::bridge(s)) return;\n', 1)
    return INCLUDE + prepared[:current.start()] + body + prepared[current.end():]
