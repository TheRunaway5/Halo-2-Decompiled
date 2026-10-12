#!/usr/bin/env python3
"""Freeze the actual38450 entry/return plus independently corroborated ISA trace.

Only saved artifacts are read. The firstRSQRT pair is directly captured; the
remaining answers come from unchanged full-call replay whose lasting written
bytes independently match the actual original return. No GPU or guest access.
"""
from pathlib import Path
import hashlib
import json
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from differential_source_object_admission_snapshot import Snapshot, uint, words


def sha(raw): return hashlib.sha256(raw).hexdigest()


def main():
    directory = ROOT / 'analysis/multiplayer-shading/weather-particle-simulation-original'
    report = json.loads((directory / 'report.json').read_text())
    assert report['replay']['completed_whole38450'] and report['replay']['nonstack_differing_written_bytes'] == 0
    assert report['actual_literal_RSQRT']['first_replay_matches_actual']
    entry = Snapshot(ROOT / report['inputs']['entry']['directory'])
    result = Snapshot(ROOT / report['inputs']['return']['directory'])
    r = report['entry']
    system, owner = int(r['EBX_system'], 16), int(r['owner_weat'], 16)
    count = r['count'];render, state = int(r['render'], 16), int(r['state'], 16)
    aux = uint(entry.read(0x4e7408, 4))
    view_pool = uint(entry.read(0x4e8c20, 4))
    view_count = struct.unpack('<h', entry.read(view_pool + 8, 2))[0]
    values = [uint(entry.read(aux + 4, 4)), uint(entry.read(0x509418, 4)), uint(entry.read(0x4670fc, 4)),
              int(r['portal_distance'], 16), view_count & 0xffff, int(r['FCW'], 16), int(r['MXCSR'], 16), count]
    banks = entry.read(system, 140) + entry.read(0x485600, 244) + entry.read(owner, 176)
    banks += entry.read(render, count * 20) + entry.read(state, count * 32)
    values += words(banks)
    values.append(r['exclusions'])
    pointer = int(r['exclusions_pointer'], 16)
    for i in range(r['exclusions']):
        raw = entry.read(pointer + i * 24, 24)
        n, planes = words(raw[16:24])
        assert n <= 1024
        values += words(raw[:20]) + words(entry.read(planes, n * 16))
    isa = report['actual_literal_RSQRT']['replay_observations']
    values.append(len(isa))
    for row in isa:
        index = (int(row['state'], 16) - state) // 32
        assert state + index * 32 == int(row['state'], 16)
        values += [index, int(row['operand'], 16), int(row['XMM0_after'], 16) & 0xffffffff]
    expected = words(result.read(system, 140) + result.read(render, count * 20) + result.read(state, count * 32))
    values += [uint(result.read(aux + 4, 4)), 1, report['replay']['auxiliary_rng_draws'], len(isa)] + expected
    output = ROOT / 'tests/fixtures/retail_source_weather_particle_simulation.tsv'
    output.write_text('# Actual ready Lockout38450 call,FCW023F/MXCSR1FA0; explicitRSQRT operand/result bank.\n' +
                      ' '.join(f'{v:08X}' for v in values) + '\n')
    provenance = dict(authority=__doc__, actual_complete_original_calls=1, count=count,
        source_readiness='Complete original12DE70, linked-ready descriptor; actual resource age refresh compared separately.',
        source_written_bytes_exact=report['replay']['nonstack_written_bytes'],
        direct_RSQRT_pairs=1, complete_call_replay_RSQRT_pairs=len(isa),
        native_RSQRT_default=False, physical_Xbox_precision_claimed=False,
        entry_RAM_SHA256=report['inputs']['entry']['RAM_sha256'], return_RAM_SHA256=report['inputs']['return']['RAM_sha256'],
        original_whole_call_report_sha256=sha((directory / 'report.json').read_bytes()),
        tool_sha256=sha(Path(__file__).read_bytes()), fixture_sha256=sha(output.read_bytes()),
        source_input_words=len(values)-4-len(expected), native_expected_bank_words=len(expected),
        expected_control_words=4, source_RNG_draws=report['replay']['auxiliary_rng_draws'])
    path = output.with_suffix('.provenance.json');path.write_text(json.dumps(provenance, indent=2) + '\n')
    print(json.dumps(dict(fixture=str(output.relative_to(ROOT)), words=len(values), sha256=sha(output.read_bytes()))))


if __name__ == '__main__': main()
