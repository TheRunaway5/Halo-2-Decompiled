"""Authenticate the vehicle marker emitter's existing callback projection."""

from hashlib import sha256
import re


INCLUDE = '#include "physics/vehicles/vehicle_marker_emission_instruction.hpp"\n'
OWNER = re.compile(r'^static void run_1dc940\(MotionState &s\) \{.*?^\}', re.M | re.S)
RAW_PIN = 'a0b4494e4950ac703db4a13537c1b78a1c6fc1bf356c229a0c90d3917d2d0eb1'
PREPARED_PIN = '7e4a55463a66db5d253b34341f823bb206d5ea32887cdd5aaaa6699ac21a4ffc'
REFERENCE = ('if(s.services.instruction)services.instruction=[&](auto pc){'
             's.services.instruction(pc>=0x1e1200u && pc<=0x1e135au?pc-0x48c0u:pc);};')
FORWARD = ('if(s.services.instruction)'
           'services.instruction=halo2::VehicleMarkerEmissionInstruction{s.services};')


def digest(text):
    return sha256(text.encode()).hexdigest()


def canonical_owner(source):
    owner = OWNER.search(source)
    if (not owner or digest(owner[0]) != RAW_PIN or owner[0].count(REFERENCE) != 1 or
            any(source.count(route) != 1 for route in (
                'case 0x1dc940u:return 0x1e1200u;',
                'case 0x1dc940u:run_1dc940(s);break;',
            ))):
        raise ValueError('Vehicle 1E1200 canonical marker caller changed')
    return owner[0]


def install(canonical, emitted):
    canonical_owner(canonical)
    owner = OWNER.search(emitted)
    if not owner:
        raise ValueError('Vehicle 1E1200 prepared marker caller missing')
    if owner[0].count(FORWARD) == 1 and emitted.count(INCLUDE) == 1:
        if digest(owner[0].replace(FORWARD, REFERENCE, 1)) == PREPARED_PIN:
            return emitted, []
    if digest(owner[0]) != PREPARED_PIN or owner[0].count(REFERENCE) != 1 or INCLUDE in emitted:
        raise ValueError('Vehicle 1E1200 prepared marker caller changed')
    bound = owner[0].replace(REFERENCE, FORWARD, 1)
    # This is a scoped forwarding contract. The weapon emitter still owns
    # the native body and its execution accounting.
    return INCLUDE + emitted[:owner.start()] + bound + emitted[owner.end():], []
