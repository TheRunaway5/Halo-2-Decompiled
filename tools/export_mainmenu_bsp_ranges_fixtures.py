#!/usr/bin/env python3
"""Reexecute unchanged original scheduling and freeze semantic producer outputs.

The original joined runner supplies complete camera/frame admission. This
adds read-only observation of2CBF0's actual metric-byte stream, then verifies
every prior retained JSON field is identical. No expected scheduling/range
behavior is computed by this exporter. All-resident BSP-prefix boundary only.
"""
import argparse
import copy
import csv
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT / 'analysis/frontend/title-partial-range-live/generate-original.py'
spec = importlib.util.spec_from_file_location('range_original', GENERATOR)
original = importlib.util.module_from_spec(spec)
spec.loader.exec_module(original)
RETAINED = ROOT / 'analysis/frontend/title-partial-range-live/retained'
MANIFEST_SHA = '48d9dba463f9f276393ac56bb6d6e763a0d4e7fd3afe44680a6eaacc8f601992'


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


class Original(original.Original):
    def observe(self, machine, pc, size, context):
        super().observe(machine, pc, size, context)
        if pc in (0x2cfbf, 0x2cfd6, 0x2cfec, 0x2cff6):
            event = self.collectors[-1]
            if event['accepted']:
                # Literal2CBF0 output at4C0B84+actual unit ordinal. This is
                # observed after its unchanged16AE0/floor/clamp instruction path.
                event['coverage_byte'] = machine.mem_read(0x4c0b84 + event['returned_unit'], 1)[0]


class Output:
    def __init__(self):
        self.data = bytearray(b'H2MPR001')

    def words(self, *values):
        self.data += struct.pack('<' + 'I' * len(values), *(v & 0xffffffff for v in values))

    def bank(self, values):
        self.words(len(values), *values)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, default=ROOT / 'tests/fixtures/mainmenu_bsp_ranges_original.bin')
    args = p.parse_args()
    manifest_raw = (RETAINED / 'manifest.json').read_bytes()
    assert sha(manifest_raw) == MANIFEST_SHA
    manifest = json.loads(manifest_raw)
    out = Output()
    out.data += bytes.fromhex(MANIFEST_SHA) + bytes.fromhex(manifest['map_sha256'])
    out.words(len(manifest['cases']))
    unit_count = draw_count = index_count = 0
    with tempfile.TemporaryDirectory(prefix='halo2-original-bsp-ranges-') as temporary:
        source = Original(Path(temporary))
        for case in manifest['cases']:
            raw = (RETAINED / case['file']).read_bytes()
            assert sha(raw) == case['sha256']
            retained = json.loads(raw)
            current = source.partial_frame(retained['input_words'])
            byte_values = [event['coverage_byte'] for event in current['collector_calls']]
            plain = copy.deepcopy(current)
            for event in plain['collector_calls']:
                del event['coverage_byte']
            assert plain == retained, 'original reexecution differs from frozen retained case'
            assert current['arena']['initial_used'] == 0 and not any(current['arena']['initial_words'])
            out.words(case['tick'])
            out.bank(current['joined_record'][:32])
            out.words(current['arena']['final_used'])
            out.bank(current['arena']['final_words'])
            out.bank([event['id'] for event in current['instance_alpha_results'] if event['alpha'] == 0])
            out.words(len(current['units']))
            assert len(byte_values) == len(current['units'])
            for unit, collector, coverage in zip(current['units'], current['collector_calls'], byte_values):
                assert collector['returned_unit'] == unit['unit'] and collector['accepted']
                # Current allocated mask offsets are meaningful only on0040;
                # contained source unit slots may retain stale unused pointers.
                offset = unit['mask_word_offset'] if unit['after'][0] & 0x40 else None
                out.words(unit['geometry'], unit['section'], unit['after'][0], coverage,
                          collector['arguments'][6] & 255, 0xffffffff if offset is None else offset)
                out.bank(unit['transform_words'] or [])
                out.words(len(unit['parts']))
                for part in unit['parts']:
                    out.words(part['part'], part['returned_index_count'], len(part['draws']))
                    for draw in part['draws']:
                        out.words(draw['primitive'], draw['first'], draw['count'])
                        out.bank(draw['indices'])
                        draw_count += 1
                        index_count += draw['count']
                unit_count += 1
        assert (unit_count, draw_count, index_count) == (344, 851, 316896)
        assert not source.pending_collectors
        instruction_sha = sha(b''.join(original.joined.packed([at]) + raw
            for at, raw in sorted(source.executed.items())))
        assert instruction_sha == manifest['executed_instruction_bank_sha256']
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(out.data)
    print(json.dumps(dict(file=str(args.output.relative_to(ROOT)), bytes=len(out.data),
        sha256=sha(out.data), cases=4, units=unit_count, draws=draw_count,
        indices=index_count, original_instruction_bank_sha256=instruction_sha,
        exporter_sha256=sha(Path(__file__).read_bytes()), prior_retained_reexecution_exact=True)))


if __name__ == '__main__':
    main()
