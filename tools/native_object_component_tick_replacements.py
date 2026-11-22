"""Bind the authenticated TU1.5 LiveSupport component-tick caller."""

import hashlib
import re


INCLUDE = '#include "physics/runtime/native_tu15_object_component_tick.hpp"\n'
PREFIX = ('\n    if (native_object_component_tick::bridge(s, dispatch, run_fa430)) return;')
BODY = re.compile(r'^static void run_1d6fb0\(WorldState &s\) \{.*?\n\}', re.M | re.S)
VEHICLE = re.compile(r'^static void run_fa430\(WorldState &s\) \{.*?\n\}', re.M | re.S)
BODY_SHA256 = 'eee9b30b36a9c0520393dff99f203105323e030ab0b185910add990e34b057c7'
VEHICLE_SHA256 = 'ed7e94172ab141e5394bc3ce9641c62f935f31baf1dacba05e2af0c1a857dade'


def canonical_body(body):
    opening = body.index('\n')
    if body.startswith(PREFIX, opening):
        return body[:opening] + body[opening + len(PREFIX):]
    return body


def replace_object_component_tick(source):
    matches = list(BODY.finditer(source))
    vehicles = VEHICLE.findall(source)
    if len(matches) != 1 or len(vehicles) != 1 or source.count(INCLUDE) > 1:
        raise ValueError('TU1.5 component tick requires exactly one parent and its direct vehicle child')
    match = matches[0]
    body = canonical_body(match[0])
    if (hashlib.sha256(body.encode()).hexdigest() != BODY_SHA256 or
            hashlib.sha256(vehicles[0].encode()).hexdigest() != VEHICLE_SHA256):
        raise ValueError('Complete TU1.5 component tick parent or direct vehicle child changed')
    opening = body.index('\n')
    result = source[:match.start()] + body[:opening] + PREFIX + body[opening:] + source[match.end():]
    return result if INCLUDE in result else INCLUDE + result
