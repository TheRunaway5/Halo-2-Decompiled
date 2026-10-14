#!/usr/bin/env python3
"""Retain original TU1.5 grenade body, explosion request and storage evidence."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe, _Code
from generate_physics_live_support_tu15 import complete_body
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ENTRIES = (0xea500, 0xe9c80, 0xfd200, 0xfdde0, 0xfdeb0, 0xff820,
           0x101330, 0x101bc0, 0x178e30, 0x17afb0, 0x17b1f0,
           0x17b450, 0x17b580, 0x186cb0, 0x186d30, 0x2b480,
           0x2b520, 0x3d260, 0x130eb0, 0xba3e0, 0xba490,
           0xbd8b0, 0xc36a0, 0x1d21e0, 0x155820, 0x1693ed,
           0x1611e0, 0x2118a0, 0x17e250, 0x1d9410, 0x324f80,
           0x3dfd0, 0xd3d90, 0xbc200)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    assert code.at(0x101db8).bytes.hex() == 'e873700700'
    assert code.at(0x101a02).bytes.hex() == 'e8b9010000'
    assert code.at(0x1d94ce).bytes.hex() == 'ff5060'
    assert image.read(0xba407, 6) == bytes.fromhex('ff96c44b4700')
    assert struct.unpack('<8I', image.read(0x474bc4, 32)) == (0xbd8b0, 0xc36a0, 0x1d21e0, 0x155820, 0x1693ed, 0x1611e0, 0x2118a0, 0x17e250)
    assert struct.unpack('<III', image.read(0x44b398, 12)) == (0x186cb0, 0x31d400, 0x186d30)
    assert struct.unpack('<III', image.read(0x44b350, 12)) == (0x2b480, 0x2b5a0, 0x2b520)
    args.output.mkdir(parents=True, exist_ok=True)
    rows = []
    for entry in ENTRIES:
        body = complete_body(code, entry)
        instructions = [code.at(p) for p in sorted(body.code)]
        listing = ''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in instructions)
        file = args.output / f'{entry:08x}.asm'
        file.write_text(listing)
        rows.append({'entry': f'{entry:08x}', 'instructions': len(instructions),
                     'listing': file.name, 'sha256': hashlib.sha256(file.read_bytes()).hexdigest()})
    report = {'reference': 'English retail TU1.5 only', 'xbe_sha256': NEW_PIN,
              'routines': rows, 'effect_request_call': '00101db8',
              'caller_return': '00101a07', 'retained_projectile_body_register': 'ebp',
              'effect_request_bytes': 96, 'named_location_count': 4, 'named_location_stride': 28,
              'effect_location_record_stride': 60, 'effect_location_frame_offset': 8,
              'projectile_timer_offset': '158', 'projectile_arming_offset': '160',
              'projectile_age_offset': '1aa',
              'live_prop_lighting_cache': '0003dfd0',
              'live_prop_lighting_query': '000d3d90',
              'live_prop_velocity_query': '000bc200',
              'scope': 'Static evidence only. Native live body, damage, retirement, effect presentation and UDP validation require separate recorded runs.'}
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'routines': len(rows), 'output': str(args.output)}))


if __name__ == '__main__':
    main()
