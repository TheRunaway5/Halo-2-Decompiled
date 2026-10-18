#!/usr/bin/env python3
"""Read actual TU1.5 objects and players from a paused private xemu session.

No guest memory, CPU register or gameplay result is supplied. Temporary QMP
readbacks use a host memfd, and retained observation bytes are gzip compressed.
This is an emulator observation, not full weapon or physical Xbox certification.
"""
import argparse
import datetime
import gzip
import hashlib
import json
import os
from pathlib import Path
import socket
import struct

from audit_rendering import Cache, file_sha

TEXT_SHA = 'd9c6fd7e7475862b1f19c2da80d7f2f6dc41da53619991002e9546e5f9e629fe'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qmp', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    args.output.mkdir(parents=True)
    observations = []
    with socket.socket(socket.AF_UNIX) as connection:
        connection.settimeout(20)
        connection.connect(str(args.qmp))
        stream = connection.makefile('rwb', buffering=0)
        greeting = json.loads(stream.readline())
        counter = 0

        def call(command, arguments=None):
            nonlocal counter
            counter += 1
            request = dict(execute=command, id=counter)
            if arguments is not None:
                request['arguments'] = arguments
            stream.write((json.dumps(request) + '\n').encode())
            while True:
                response = json.loads(stream.readline())
                if response.get('id') == counter:
                    assert 'error' not in response, response
                    return response['return']

        call('qmp_capabilities')
        prior = call('query-status')
        call('stop')

        def read(address, size, retain=True):
            assert 0 < size <= 4 * 1024 * 1024
            descriptor = os.memfd_create('tu15-read-only-observation', os.MFD_CLOEXEC)
            try:
                call('memsave', dict(val=address, size=size,
                                    filename=f'/proc/{os.getpid()}/fd/{descriptor}'))
                assert os.fstat(descriptor).st_size == size
                raw = os.pread(descriptor, size, 0)
                assert len(raw) == size
            finally:
                os.close(descriptor)
            if retain:
                observations.append(dict(address=address, bytes=raw.hex()))
            return raw

        def word(address):
            return struct.unpack('<I', read(address, 4))[0]

        assert hashlib.sha256(read(0x12000, 3611948, False)).hexdigest() == TEXT_SHA
        pool = word(0x4ecbd0)
        header = read(pool, 0x4c)
        capacity, stride = struct.unpack_from('<II', header, 0x20)
        highest = struct.unpack_from('<I', header, 0x38)[0]
        rows, occupied = struct.unpack_from('<II', header, 0x44)
        assert stride == 12 and 0 < highest <= capacity <= 65536
        occupied = read(occupied, ((capacity + 31) // 32) * 4)
        headers = read(rows, highest * stride)
        tag_rows = word(0x4f0414)
        objects = []
        cache = Cache(args.map)
        try:
            for index in range(highest):
                if not occupied[index // 8] & (1 << (index % 8)):
                    continue
                entry = headers[index * 12:(index + 1) * 12]
                salt = struct.unpack_from('<H', entry)[0]
                extent = struct.unpack_from('<H', entry, 6)[0]
                body = struct.unpack_from('<I', entry, 8)[0]
                assert body and 0x118 <= extent <= 65535
                data = read(body, extent)
                definition = struct.unpack_from('<I', data)[0]
                tag = cache.tags.get(definition)
                live_tag = read(tag_rows + (definition & 65535) * 16, 16)
                assert struct.unpack_from('<I', live_tag, 4)[0] == definition
                assert tag is not None, hex(definition)
                assert int.from_bytes(live_tag[:4], 'big') == int.from_bytes(tag['group'].encode(), 'little')
                objects.append(dict(datum=(salt << 16) | index, body=body,
                    extent=extent, type=data[0xaa], definition=definition,
                    name=tag['name'], group=tag['group'],
                    position=list(struct.unpack_from('<3f', data, 0x64)),
                    forward=list(struct.unpack_from('<3f', data, 0x70)),
                    health=struct.unpack_from('<f', data, 0xec)[0],
                    shield=struct.unpack_from('<f', data, 0xf0)[0]))
        finally:
            cache.close()
        player_pool = word(0x4f54f4)
        player_header = read(player_pool, 0x4c)
        count, stride = struct.unpack_from('<II', player_header, 0x20)
        rows = struct.unpack_from('<I', player_header, 0x44)[0]
        assert stride == 0x21c and 0 < count <= 64
        unit_ids = {o['datum'] for o in objects if o['type'] == 0}
        players = []
        for index in range(count):
            data = read(rows + index * stride, stride)
            current = struct.unpack_from('<I', data, 0x2c)[0]
            if current in unit_ids:
                players.append(dict(datum=(struct.unpack_from('<H', data)[0] << 16) | index,
                                    current_unit=current))
        clock = read(word(0x51df54), 0x24)
        registers = call('human-monitor-command', {'command-line': 'info registers'})
        status = call('query-status')
        assert status['status'] == 'paused'
    raw = json.dumps(observations, separators=(',', ':')).encode()
    with (args.output / 'observed-memory.json.gz').open('wb') as output:
        output.write(gzip.compress(raw, mtime=0))
    report = dict(timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        reference='OG Xbox TU1.5', greeting=greeting, prior_status=prior,
        whole_live_text_sha256=TEXT_SHA, whole_live_text_equal_original_TU15=True,
        map=str(args.map.absolute()), map_sha256=file_sha(args.map),
        observed_memory_sha256=hashlib.sha256(raw).hexdigest(),
        compressed_observations_sha256=file_sha(args.output / 'observed-memory.json.gz'),
        objects=objects, players=players, clock_words=list(struct.unpack('<9I', clock)),
        registers=registers, after_status=status, guest_code_patches=[],
        guest_memory_patches=[], guest_register_writes=[], controller_inputs=[],
        full_weapon_parity=False, physical_xbox_verified=False,
        tool_sha256=file_sha(Path(__file__)))
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(objects=len(objects), players=players,
        weapons=[o for o in objects if o['group'] == 'weap'])))


if __name__ == '__main__':
    main()
