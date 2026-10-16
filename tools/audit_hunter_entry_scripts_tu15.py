#!/usr/bin/env python3
"""Read the owned Xbox Hunter placement scripts with typed literal operands."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from inspect_multiplayer import Cache, ROOT

MAP = ROOT / 'assets/disc/maps/03a_oldmombasa.map'
PIN = '7ea355ef79f13be83fe78546c0fd3b1009ef71c0dd2257b24996420d856f78f4'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    assert hashlib.file_digest(MAP.open('rb'), 'sha256').hexdigest() == PIN
    cache = Cache(MAP)
    try:
        roots = cache.block(cache.scenario + 0x1b8, 0x28)
        nodes = cache.block(cache.scenario + 0x238, 20)
        strings = cache.block(cache.scenario + 0x1b0, 1)
        string_base, string_size = strings[0], len(strings)

        def node(handle):
            assert handle != 0xffffffff and (handle & 65535) < len(nodes)
            values = struct.unpack_from('<4H3I', cache.data, nodes[handle & 65535])
            assert values[0] == handle >> 16
            return values

        def text(offset):
            assert offset < string_size
            return cache.text(string_base + offset)

        def expression(handle, depth=0):
            assert depth < 128
            salt, opcode, kind, flags, next_, offset, value = node(handle)
            raw = dict(handle=f'{handle:08x}', opcode=opcode, type=kind,
                       flags=flags, value_bits=f'{value:08x}')
            if flags & 4:
                raw.update(global_name=text(offset))
            elif flags & 1:
                value_type = 2 if kind == 2 else opcode
                if value_type == 5:
                    raw['literal'] = bool(value & 255)
                elif value_type == 6:
                    raw['literal'] = struct.unpack('<f', struct.pack('<I', value))[0]
                elif value_type == 7:
                    raw['literal'] = struct.unpack('<h', struct.pack('<H', value & 65535))[0]
                elif value_type == 8:
                    raw['literal'] = struct.unpack('<i', struct.pack('<I', value))[0]
                elif value_type in (2, 9):
                    raw['literal'] = text(offset)
            else:
                function = node(value)
                raw['function'] = text(function[5])
                args = []
                current = function[4]
                seen = set()
                while current != 0xffffffff:
                    assert current not in seen
                    seen.add(current)
                    args.append(expression(current, depth + 1))
                    current = node(current)[4]
                raw['arguments'] = args
            return raw

        scripts = []
        names = {'cs_e2_cov_hunters0_0_entry', 'cs_e2_cov_hunters0_1_entry',
                 'cs_e2_cov_hunters0_taunt'}
        for index, record in enumerate(roots):
            name = cache.text(record)
            if name in names:
                scripts.append(dict(name=name, index=index, kind=cache.h(record + 32),
                                    expression=expression(cache.u(record + 36))))
        assert {s['name'] for s in scripts} == names
        initial_delays = []
        for script in scripts:
            if script['name'].endswith('_entry'):
                calls = script['expression']['arguments']
                assert calls[0]['function'] == 'cs_queue_command_script'
                assert calls[1]['function'] == 'sleep'
                delay = calls[1]['arguments'][0]['literal']
                assert delay == 220
                initial_delays.append(dict(script=script['name'], delay_authored_ticks=delay))
        report = dict(scope=__doc__, map=str(MAP), map_sha256=PIN,
                      tool_sha256=hashlib.file_digest(Path(__file__).open('rb'), 'sha256').hexdigest(),
                      initial_delays=initial_delays, scripts=scripts,
                      runtime_script_progress_verified=False, full_weapon_parity=False)
        args.output.write_text(json.dumps(report, indent=2, allow_nan=False) + '\n')
        print(json.dumps(initial_delays))
    finally:
        cache.data.close()
        cache.stream.close()


if __name__ == '__main__':
    main()
