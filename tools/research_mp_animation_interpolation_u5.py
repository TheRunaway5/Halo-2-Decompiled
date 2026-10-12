#!/usr/bin/env python3
"""Original animation sampling/transition/render body audit; no gameplay execution."""
import argparse
import csv
import json
import struct
from pathlib import Path

import research_mp_inventory_switch_u5 as source

PIN = 'fe51d55efa84a5e6bc4809bedf424fa9d56aa6320ac2cdeed7ed520e77f1c219'
assert source.sha(Path(source.__file__)) == PIN
ROOT = source.ROOT
OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/animation-interpolation'
ENTRIES = (
    0x3D4E0, 0xBB240, 0xBB2C0, 0xBEA50, 0x1CEFE0, 0x1DCC30,
    0x1DCD10, 0x1DCD60, 0x1DCE20, 0x1DCE60, 0x1D0ED0,
    0x1CA950, 0x27FB50, 0x2929F0, 0x292CA0, 0x16872A, 0x168866, 0x1690FD,
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=OUT)
    args = parser.parse_args()
    pins = {**source.SOURCE_PINS, str(Path(source.__file__)): PIN}
    for path, pin in source.DEPENDENCIES.items():
        pins[str(ROOT / path)] = pin
    prior = (
        'analysis/multiplayer/map-pack/spartan-live-pose-1.1-oracle.json',
        'tools/oracle_spartan_live_pose_1_1.py',
        'tools/oracle_spartan_equipped_animation_1_1.py',
        'tools/oracle_campaign_animation.py',
        'analysis/campaign/animation-playback-golden.json',
        'analysis/frontend/profile-regression/frontend-model-primary-three-clips-original.json',
        'analysis/multiplayer/map-pack/smg-fp-clock/source-fixture.json',
    )
    for path in prior:
        pins[str(ROOT / path)] = source.sha(ROOT / path)
    for path, pin in pins.items():
        assert source.sha(path) == pin, path
    images = [source.Xbe(source.OLD), source.Xbe(source.NEW)]
    codes = [source._Code(x, '.text') for x in images]
    starts = [{int(row['address'], 16) for row in csv.DictReader(path.open(), delimiter='\t')}
              for path in source.INVENTORIES]
    buckets = {}
    for start in sorted(starts[1]):
        buckets.setdefault(tuple(source.prefix(codes[1], images[1], start)), []).append(start)
    rows = []
    for entry in ENTRIES:
        old_trace = source._trace(codes[0], entry, starts[0])
        old = [codes[0].at(address) for address in sorted(old_trace.code)]
        indices = {ins.address: n for n, ins in enumerate(old)}
        expected = [source.norm(ins, images[0], indices) for ins in old]
        candidates = sorted(set(
            buckets.get(tuple(source.prefix(codes[0], images[0], entry)), []) +
            source.raw_prefix_candidates(codes[0], images[0], codes[1], entry)))
        matches, misses = [], []
        for candidate in candidates:
            try:
                trace = source._trace(codes[1], candidate, starts[1])
                new = [codes[1].at(address) for address in sorted(trace.code)]
                nindices = {ins.address: n for n, ins in enumerate(new)}
                if [source.norm(ins, images[1], nindices) for ins in new] == expected:
                    matches.append((candidate, trace, new))
                else:
                    misses.append({'entry': f'{candidate:08x}', 'rows': len(new)})
            except Exception as error:
                misses.append({'entry': f'{candidate:08x}', 'error': str(error)})
        row = {'old_entry': f'{entry:08x}', 'old_rows': len(old),
               'candidates': [f'{address:08x}' for address in candidates],
               'full_body_matches': [f'{address:08x}' for address, _, _ in matches],
               'unmatched_candidates': misses, 'listings': []}
        paired = [("old", old, images[0])]
        if len(matches) == 1:
            address, trace, new = matches[0]
            row.update(u5_entry=f'{address:08x}', normalized_instruction_and_internal_branch_equality=True)
            paired.append(('u5', new, images[1]))
            mapping = {a.address: b.address for a, b in zip(old, new)}
            assert len(old_trace.tables) == len(trace.tables)
            row['jump_tables'] = []
            for (op, os, oc), (np, ns, nc) in zip(old_trace.tables, trace.tables):
                assert os == ns == 4 and oc == nc
                ov = struct.unpack('<' + 'I' * oc, images[0].read(op, oc * 4))
                nv = struct.unpack('<' + 'I' * nc, images[1].read(np, nc * 4))
                assert all(mapping[a] == b for a, b in zip(ov, nv))
                row['jump_tables'].append(dict(old_va=f'{op:08x}', u5_va=f'{np:08x}',
                    old_targets=[f'{v:08x}' for v in ov], u5_targets=[f'{v:08x}' for v in nv]))
            calls, operands = [], []
            for a, b in zip(old, new):
                if a.mnemonic == 'call' and a.operands[0].type == b.operands[0].type == 2:
                    calls.append(dict(old_site=f'{a.address:08x}', u5_site=f'{b.address:08x}',
                        old_target=f'{a.operands[0].imm:08x}', u5_target=f'{b.operands[0].imm:08x}'))
                for oa, ob in zip(a.operands, b.operands):
                    if oa.type != 3 or ob.type != 3 or oa.mem.base or ob.mem.base:
                        continue
                    if not all(any(s.va <= o.mem.disp < s.va + s.vsize for s in image.sections)
                               for o, image in ((oa, images[0]), (ob, images[1]))):
                        continue
                    assert oa.size == ob.size
                    operands.append(dict(old_site=f'{a.address:08x}', u5_site=f'{b.address:08x}',
                        old_va=f'{oa.mem.disp:08x}', u5_va=f'{ob.mem.disp:08x}', bytes=oa.size,
                        old_hex=images[0].read(oa.mem.disp, oa.size).hex(),
                        u5_hex=images[1].read(ob.mem.disp, ob.size).hex()))
            row.update(direct_calls=calls, image_memory_operands=operands)
        for label, instructions, image in paired:
            path = args.output_dir / 'instructions' / f'{entry:08x}-{label}.asm'
            for ins in instructions:
                assert image.read(ins.address, ins.size) == ins.bytes
            source.save(path, ''.join(f'{ins.address:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n'
                                     for ins in instructions).encode())
            row['listings'].append(dict(file=str(path.relative_to(args.output_dir)),
                rows=len(instructions), sha256=source.sha(path)))
        rows.append(row)
        print(json.dumps(dict(old=row['old_entry'], matches=row['full_body_matches'], rows=len(old))), flush=True)
    mapping = {row['old_entry']: row['u5_entry'] for row in rows if 'u5_entry' in row}
    for row in rows:
        for call in row.get('direct_calls', []):
            call['target_full_body_proven_here'] = call['old_target'] in mapping
            if call['old_target'] in mapping:
                assert mapping[call['old_target']] == call['u5_target']
    original_report = json.loads((ROOT / prior[0]).read_text())
    dispatches = [event['source_sampler_dispatch'] for event in original_report['events']
                  if 'source_sampler_dispatch' in event]
    decoder_records = []
    for codec in (1, 3, 4, 8):
        old_record = images[0].read(0x487028 + codec * 40, 40)
        new_record = images[1].read(0x48C338 + codec * 40, 40)
        old_words, new_words = (struct.unpack('<10I', data) for data in (old_record, new_record))
        for old_target, new_target in zip(old_words[1:7], new_words[1:7]):
            if f'{old_target:08x}' in mapping:
                assert int(mapping[f'{old_target:08x}'], 16) == new_target
        decoder_records.append(dict(codec=codec, old_va=f'{0x487028 + codec * 40:08x}',
            u5_va=f'{0x48C338 + codec * 40:08x}', old_raw40_hex=old_record.hex(),
            u5_raw40_hex=new_record.hex(), old_targets=[f'{v:08x}' for v in old_words[1:7]],
            u5_targets=[f'{v:08x}' for v in new_words[1:7]],
            semantics='First three targets are nonfractional rotation/translation/scale; '
                      'next three are fractional targets. Indexed table delivery is listed '
                      'by original27FB50/2835B0; only separately paired bodies are proven here.'))
    report = dict(schema=1, tool_sha256=source.sha(__file__), source_pins=pins,
        inventories={str(path): source.sha(path) for path in source.INVENTORIES}, owners=rows,
        codec_dispatch_records=decoder_records,
        frozen_spartan_live_sampler_dispatches=dispatches,
        authority='Unchanged owned original executable bytes; labels and decompiler starts only navigate. '
                  'Full-body equality preserves normalized operands/internal branches and jump tables; '
                  'calls outside this report remain unverified correspondence.',
        scope='Static rendering/current-bank, channel fractional sampler and pose-transition audit. '
              'Existing actual 1.1 oracle dispatches are correlated verbatim. No new game execution, '
              'render-rate clock producer, invented channel or Xbox hardware parity is asserted.',
        production_edits=[], instruction_patches=[])
    for path, pin in pins.items():
        assert source.sha(path) == pin, path
    source.save(args.output_dir / 'report.json', source.encoded(report))
    print(json.dumps(dict(output=str(args.output_dir / 'report.json'),
        sha256=source.sha(args.output_dir / 'report.json'), proven=len(mapping), total=len(rows))))


if __name__ == '__main__':
    main()
