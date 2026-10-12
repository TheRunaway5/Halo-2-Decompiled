#!/usr/bin/env python3
"""Read original Xbox campaign BSP ordinals, compiled requests and transition volumes.

With no path, inspects the local15 authored campaign caches. This tool reads
serialized primary metadata only; it does not perform tag-name resolution or
emulate script execution. Findings go to analysis/campaign/bsp-authored-cache.json.
"""
import argparse
import hashlib
import json
import mmap
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]

def inspect(path):
    with path.open('rb') as stream, mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
        def u(at): return struct.unpack_from('<I', data, at)[0]
        def h(at): return struct.unpack_from('<H', data, at)[0]
        def text(at):
            end = data.find(b'\0', at)
            assert 0 <= at <= end < len(data)
            return data[at:end].decode('latin1')
        assert u(0) == 0x68656164 and u(4) == 8 and u(8) == len(data)
        assert text(288) == '02.09.27.09809'
        index = u(16); count = u(index + 24)
        table = u(index + 8) - (u(index) - (index + 32))
        magic = u(table + 8) - (index + u(20))
        tag_by_datum = {u(table + i * 16 + 4): table + i * 16 for i in range(count)}
        scnr = u(tag_by_datum[u(index + 12)] + 8) - magic
        name_base, name_table = u(708), u(716)
        def tag_name(datum):
            tag = tag_by_datum[datum]
            return text(name_base + u(name_table + (tag - table) // 16 * 4))
        bsps = []
        for i in range(u(scnr + 0x210)):
            at = u(scnr + 0x214) - magic + i * 0x44
            datum = u(at + 0x14)
            bsps.append(dict(index=i, datum=f'{datum:08x}', name=tag_name(datum),
                             file_offset=f'{u(at):08x}', size=u(at + 4),
                             memory_address=f'{u(at + 8):08x}',
                             lightmap_datum=f'{u(at + 0x1c):08x}'))
        node_count = u(scnr + 0x238); nodes = u(scnr + 0x23c) - magic
        strings = u(scnr + 0x1b4) - magic; string_size = u(scnr + 0x1b0)
        calls = []
        for i in range(node_count):
            at = nodes + i * 20
            if h(at + 2) not in [508, 509] or h(at + 4) != 4 or h(at + 6) & 1:
                continue
            function = u(at + 16) & 0xffff
            argument_datum = u(nodes + function * 20 + 8)
            argument = nodes + (argument_datum & 0xffff) * 20
            raw = u(argument + 16); flags = h(argument + 6)
            value = raw & 0xffff; value = value if value < 0x8000 else value - 0x10000
            string_offset = u(argument + 12)
            literal = bool(flags & 1) and not bool(flags & 4)
            token = text(strings + string_offset) if h(argument + 4) == 26 and string_offset < string_size else None
            calls.append(dict(opcode=h(at + 2), datum=f'{h(at):04x}{i:04x}',
                              file_offset=f'{at:08x}', argument_datum=f'{argument_datum:08x}',
                              argument_type=h(argument + 4), argument_flags=flags,
                              literal=literal, raw_value=f'{raw:08x}',
                              signed_short=value if literal else None, token=token,
                              bsp_name=bsps[value]['name'] if literal and 0 <= value < len(bsps) else None))
        transitions = []
        for i in range(u(scnr + 0x380)):
            at = u(scnr + 0x384) - magic + i * 8
            key, volume, padding = struct.unpack_from('<Ihh', data, at)
            entry = dict(file_offset=f'{at:08x}', key=f'{key:08x}',
                         from_bsp=key & 0xffff, to_bsp=key >> 16,
                         trigger_volume=volume, padding=padding)
            if volume != -1:
                assert 0 <= volume < u(scnr + 0x108)
                trigger = u(scnr + 0x10c) - magic + volume * 0x44
                entry['volume'] = dict(object_name=struct.unpack_from('<h', data, trigger + 4)[0],
                                       node_string_id=f'{u(trigger + 8):08x}',
                                       forward=struct.unpack_from('<3f', data, trigger + 0xc),
                                       up=struct.unpack_from('<3f', data, trigger + 0x18),
                                       position=struct.unpack_from('<3f', data, trigger + 0x24),
                                       extents=struct.unpack_from('<3f', data, trigger + 0x30))
            transitions.append(entry)
        stream.seek(0); sha256 = hashlib.file_digest(stream, 'sha256').hexdigest()
        return dict(map=path.name, sha256=sha256, scenario_file_offset=f'{scnr:08x}',
                    bsps=bsps, switch_calls=calls, transition_volumes=transitions)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('caches', nargs='*', type=Path)
    args = parser.parse_args()
    paths = args.caches or [p for p in sorted((ROOT / 'assets/disc/maps').glob('*.map'))
                            if re.match(r'^\d\d[a-z]_', p.name)]
    rows = [inspect(path) for path in paths]
    out = ROOT / 'analysis/campaign/bsp-authored-cache.json'
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(rows, indent=2) + '\n')
    print(f'Inspected {len(rows)} original campaign caches; {sum(len(r["switch_calls"]) for r in rows)} compiled BSP requests')

if __name__ == '__main__': main()
