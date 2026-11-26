"""Authenticate the vehicle BAD60 callback projection in its existing route."""

from hashlib import sha256
import re


INCLUDE = '#include "physics/vehicles/vehicle_marker_matrix_instruction.hpp"\n'
OWNER = re.compile(r'^static void run_b9c20\(MotionState &s\) \{.*?^\}', re.M | re.S)
RAW_PIN = '34e07f5b74c320ca0365434f1a91517485f6b4e7f177e6ac683b9ad9e9ba2336'
PREPARED_PIN = '730bde07769c5cc81079a8b5628ab23b801bd92b3e169e2608afe21f6e3d1d5a'
REFERENCE = ('if(s.services.instruction)services.instruction=[&](auto pc){'
             's.services.instruction(pc>=0xbad60u && pc<=0xbaf05u?pc-0x1140u:pc);};')
FORWARD = ('if(s.services.instruction)'
           'services.instruction=halo2::VehicleMarkerMatrixInstruction{s.services};')


def install(canonical, prepared):
    original, current = OWNER.search(canonical), OWNER.search(prepared)
    if (not original or sha256(original[0].encode()).hexdigest() != RAW_PIN
            or original[0].count(REFERENCE) != 1
            or any(canonical.count(route) != 1 for route in (
                'case 0xb9c20u:return 0xbad60u;', 'case 0xb9c20u:run_b9c20(s);break;'))):
        raise ValueError('Vehicle BAD60 caller or entry map changed')
    if not current:
        raise ValueError('Vehicle BAD60 prepared caller missing')
    if current[0].count(FORWARD) == 1 and prepared.count(INCLUDE) == 1:
        if sha256(current[0].replace(FORWARD, REFERENCE, 1).encode()).hexdigest() == PREPARED_PIN:
            return prepared, []
    if (sha256(current[0].encode()).hexdigest() != PREPARED_PIN
            or current[0].count(REFERENCE) != 1 or INCLUDE in prepared):
        raise ValueError('Vehicle BAD60 prepared callback projection changed')
    bound = current[0].replace(REFERENCE, FORWARD, 1)
    return INCLUDE + prepared[:current.start()] + bound + prepared[current.end():], []
