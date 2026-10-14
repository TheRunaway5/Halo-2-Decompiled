#!/usr/bin/env python3
"""Audit real TU1.5 sound requests against exact original admission replays.

The cold-device diagnosis is intentionally bounded. This does not enable a
source sound system, replace an unavailable voice, or establish PCM parity.
"""
import argparse
import bisect
import hashlib
import json
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Bank:
    def __init__(self, metadata, path):
        self.regions = metadata['regions']
        self.starts = [r['address'] for r in self.regions]
        assert self.starts == sorted(self.starts)
        self.bytes = path.read_bytes()
        assert sum(r['size'] for r in self.regions) == len(self.bytes)

    def read(self, address, size):
        i = bisect.bisect_right(self.starts, address) - 1
        assert i >= 0, f'Unretained source address {address:08x}'
        region = self.regions[i]
        assert address + size <= region['address'] + region['size']
        offset = region['offset'] + address - region['address']
        return self.bytes[offset:offset + size]

    def word(self, address):
        return struct.unpack('<I', self.read(address, 4))[0]


def audit(directory, replay):
    original = Xbe(NEW_DEFAULT)
    assert sha(NEW_DEFAULT) == NEW_PIN
    # Authenticate the exact cold gate, target, and relevant tag operands.
    assert original.read(0x223c61, 5).hex() == 'a1502c4f00'
    assert original.read(0x223c66, 3).hex() == '8a4878'
    assert original.read(0x223c6e, 2).hex() == '7451'
    assert original.read(0x18ff67, 6).hex() == '8b1514044f00'
    assert original.read(0x18ff8b, 3).hex() == '8a410d'
    assert original.read(0x18ff8e, 2).hex() == '3cff'
    accepted = json.loads(replay.read_text())
    assert accepted['status'] == 'pass' and accepted['original_sha256'] == NEW_PIN
    hardware = accepted['hardware']
    assert hardware['oracle_backend'] == 'original_i386_hardware_single_step'
    assert not hardware['original_instruction_patches'] and not hardware['native_expected_data_used']
    cases = {case['tick']: case for case in hardware['cases']}
    rows = json.loads((directory / 'source-sound-submissions.json').read_text())
    assert rows and len(rows) == len(cases)
    output = []
    for row in rows:
        identity = row['id']
        case = cases[identity]
        assert case['status'] == 'pass' and not case['errors']
        paths = [directory / 'sound-submit-oracle' / f'{identity}-{phase}.json'
                 for phase in ('before', 'after')]
        before, after = [json.loads(p.read_text()) for p in paths]
        assert before['entry'] == after['entry'] == 0x223b20
        assert before['original_sha256'] == after['original_sha256'] == NEW_PIN
        assert before['regions'] == after['regions']
        banks = [Bank(metadata, p.with_suffix('.bin')) for metadata, p in zip((before, after), paths)]
        first, last = banks
        assert row['request'] == before['abi']['gp'][0]
        assert first.word(before['abi']['gp'][4] + 4) == row['sound']
        assert first.word(before['abi']['gp'][4]) == 0x18cf01
        assert row['request_before_hex'] == first.read(row['request'], 160).hex()
        assert row['request_after_hex'] == last.read(row['request'], 160).hex()
        assert row['result'] == after['abi']['gp'][0]
        system = first.word(0x4f2c50)
        availability = first.read(system + 0x78, 3)
        driver = first.word(0x52d674)
        assert row['system'] == system and row['availability_bytes_hex'] == availability.hex()
        assert row['driver'] == driver
        trace = after['instructions']
        assert row['original_instruction_events'] == len(trace) == case['original_instructions']
        assert trace[0] == 0x223b20 and 0x224740 in trace
        # This checkpoint tests the actual cold fallback, not all positive
        # admissions or every custom-playback path. A newly activated driver
        # needs its own positive replay and cannot silently inherit this claim.
        assert availability[0] == 0 and driver == 0 and row['result'] == 0xffffffff
        branch = trace.index(0x223c6e)
        assert trace[branch + 1] == 0x223cc1 and 0x128b10 not in trace
        assert 0x18ff94 in trace, 'Actual ordinary lookup did not reject its authored FF custom index'
        tag_index = row['sound'] & 0xffff
        signed_index = struct.unpack('<h', struct.pack('<H', tag_index))[0]
        table = first.word(0x4f0414)
        assert first.word((table + signed_index * 16) & 0xffffffff) == 0x736e6421
        definition = first.word(table + tag_index * 16 + 8)
        custom_index = first.read(definition + 0x0d, 1)[0]
        assert custom_index == 0xff
        assert case['all_committed_bytes_accounted'] == len(first.bytes) == len(last.bytes)
        output.append(dict(id=identity, tick=row['tick'], sound=f"{row['sound']:08x}",
            request=f"{row['request']:08x}", definition=f'{definition:08x}',
            request160_sha256=hashlib.sha256(first.read(row['request'], 160)).hexdigest(),
            availability_bytes_hex=availability.hex(), driver_pointer=driver,
            authored_custom_playback_index=custom_index, original_return=row['result'],
            ordinary_create_reached=False, original_instruction_events=len(trace),
            all_committed_bytes_accounted=case['all_committed_bytes_accounted'],
            input_sha256={p.name:sha(p) for p in paths + [p.with_suffix('.bin') for p in paths]}))
    return dict(status='pass', original_sha256=NEW_PIN, tool_sha256=sha(Path(__file__)),
        original_replay_sha256=sha(replay), request_ledger_sha256=sha(directory/'source-sound-submissions.json'),
        cases=output, admitted_voices=0, fabricated_gameplay_or_platform_returns=0,
        diagnosis='Original custom lookup rejects the authored FF index; original ordinary fallback stops at system+78=0 with no constructed driver.',
        native_pcm_verified=False, xbox_pcm_verified=False,
        remaining='Complete real source driver/DSP/stream/plugin owners, positive source admission, cache/voice/completion lifecycle and native PCM delivery.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--original-replay', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    report = audit(args.capture, args.original_replay)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(status=report['status'], cases=len(report['cases']),
        admitted_voices=report['admitted_voices'], native_pcm_verified=False, xbox_pcm_verified=False)))


if __name__ == '__main__':
    main()
