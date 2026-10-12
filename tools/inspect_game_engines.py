#!/usr/bin/env python3
"""Execute original engine-table registration and inventory literal51-slot vtables.

Registration copies the executable's static object pointers into its engine
table. This runs no game mode update or objective lifecycle and supplies no
engine pointer, vtable or callback result. Slot semantics remain unresolved
unless independently recovered from an original caller.
"""
import argparse
import hashlib
import json
from pathlib import Path

from oracle_game_variants import ROOT, Source, VERSIONS
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

REGISTRATION = {
    'retail': (0x378500, 0x55e4d0),
    'tu11': (0x37eb40, 0x566084),
}
TYPES = (1, 2, 3, 4, 7, 8, 9)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
        default=ROOT / 'analysis/multiplayer/game-variants/engine-tables.json')
    args = parser.parse_args()
    output = dict(scope=__doc__, versions={},
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    for version, (entry, table) in REGISTRATION.items():
        source = Source(VERSIONS[version])
        before = bytes(source.g.mem_read(table, 11 * 4)).hex()
        source.call(entry)
        objects = []
        for type_id in TYPES:
            obj = source.u(table + type_id * 4)
            vtable = source.u(obj)
            callbacks = []
            for slot in range(51):
                function = source.u(vtable + slot * 4)
                assert any(s.va <= function < s.va + s.vsize and s.flags & 4
                           for s in source.image.sections), (version, type_id, slot)
                callbacks.append(dict(slot=slot, offset=f'{slot * 4:02x}',
                                      function=f'{function:08x}'))
            objects.append(dict(source_type=type_id, object=f'{obj:08x}',
                                vtable=f'{vtable:08x}', callbacks=callbacks))
        decoder = Cs(CS_ARCH_X86, CS_MODE_32)
        lines = []
        for address in sorted(source.executed):
            i = next(decoder.disasm(source.image.read(address, 15), address))
            lines.append(f'{address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}')
        listing = args.output.parent / f'{version}-engine-registration.txt'
        listing.write_text('\n'.join(lines) + '\n')
        output['versions'][version] = dict(xbe_sha256=VERSIONS[version]['pin'],
            entry=f'{entry:08x}', table=f'{table:08x}', before_hex=before,
            after_hex=bytes(source.g.mem_read(table, 11 * 4)).hex(), engines=objects,
            listing_sha256=hashlib.sha256(listing.read_bytes()).hexdigest())
        # The original CTF/Assault source modes share their engine object.
        assert objects[0]['object'] == objects[-1]['object']
    args.output.write_text(json.dumps(output, indent=2) + '\n')
    print('Recorded 7 source mode registrations and 357 literal callbacks for each of 2 unchanged executables')


if __name__ == '__main__':
    main()
