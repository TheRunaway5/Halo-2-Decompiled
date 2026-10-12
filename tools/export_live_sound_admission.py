#!/usr/bin/env python3
"""Export actual original game admission inputs and outputs for the native owner.

All mutable bytes come from stopped, initialized guest pages. Expected results
and stores come from unchanged original execution, confirmed against its actual
return. Source-private stack and argument scratch are excluded from the semantic
native ABI. No ready flag, listener, seed, source binding or callee is supplied.
"""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, action='append', required=True)
    parser.add_argument('--pair', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    pair = json.loads((args.pair / 'report.json').read_text())
    pcs = {}
    operands = {}
    for row in pair['owners']:
        before = (args.pair / 'instructions' / f'{row["base"]}-base.asm').read_text().splitlines()
        after = (args.pair / 'instructions' / f'{row["base"]}-old.asm').read_text().splitlines()
        assert len(before) == len(after) == row['rows']
        pcs.update({int(a.split()[0], 16): int(b.split()[0], 16) for a, b in zip(before, after)})
        for operand in row['memory']:
            operands[int(operand['old_va'], 16)] = operand
    # The complete selected original TU1.1 immutable API bank. Fields used by
    # this admission owner must also be byte equal in the paired base body.
    shared_path = ROOT / 'analysis/multiplayer/integration/sound-create-native/measured/providers.json'
    driver_path = ROOT / 'analysis/multiplayer/integration/sound-platform-voice-contract/paired/providers.json'
    shared = json.loads(shared_path.read_text())['words']
    driver = json.loads(driver_path.read_text())['constants']
    admission_path = ROOT / 'analysis/multiplayer/integration/sound-admission-native/paired-v2/providers.json'
    extra = next(r for r in json.loads(admission_path.read_text())['words'] if r['old_va'] == '004651cc')
    shared = [*shared, extra]
    unreferenced = []
    for kind, records, hex_field in [('K', shared, 'raw_hex'), ('P', driver, 'raw4_hex')]:
        for i, row in enumerate(records):
            old = int(row['old_va'], 16)
            if old in operands:
                assert operands[old]['base_hex'] == operands[old]['old_hex'] == row[hex_field]
            else:
                unreferenced.append(dict(bank=kind, field=i, old_va=row['old_va']))
    # Only this driver field is reached by source_sound_admit's numeric tail.
    assert int(driver[14]['old_va'], 16) in operands
    lines = ['LIVE_SOUND_ADMISSION_ORIGINAL_V1',
             'K\t' + '\t'.join(f'{int.from_bytes(bytes.fromhex(r["raw_hex"]),"little"):08x}' for r in shared),
             'P\t' + '\t'.join(f'{int.from_bytes(bytes.fromhex(r["raw4_hex"]),"little"):08x}' for r in driver)]
    captures = []
    names = ('system', 'tag_table', 'current_matg', 'current_ugh', 'class_fades', 'local_rng',
             'alternate_codec_flag', 'first_person_globals', 'current_scene')
    for directory in args.capture:
        replay_path, actual_path = directory / 'replay.json', directory / 'actual-return.json'
        replay, actual = [json.loads(p.read_text()) for p in (replay_path, actual_path)]
        assert replay['image_sha256'] == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
        assert replay['image_sha256'] in pair['image_sha256'].values()
        assert actual['al_equal'] and actual['esp_equal'] and all(r['equal'] for r in actual['writes'])
        assert not any(replay[k] for k in ['instruction_patches', 'intercepted_callees', 'guest_memory_writes'])
        roots = replay['state_roots']
        lines.append('M\t' + '\t'.join(f'{roots[n]:08x}' for n in names))
        pages = {}
        for row in replay['pages']:
            path = directory / f'page-{row["address"]:08x}.bin'
            assert sha(path) == row['sha256'] and path.stat().st_size == 4096
            pages[row['address']] = path.read_bytes()
            lines.append(f'R\t{row["address"]:08x}\t{pages[row["address"]].hex()}')
        def raw(address, size):
            return bytes(pages[(address+i)&~4095][(address+i)&4095] for i in range(size))
        for name, cell in replay['root_cells'].items():
            assert int.from_bytes(raw(cell, 4), 'little') == roots[name]
        lines.append(f'F\t{replay["registers"]["fctrl"]:04x}\t{replay["registers"]["mxcsr"]:08x}')
        lines.append('C\t' + '\t'.join(f'{replay[n]:08x}' for n in
                     ['sound', 'request', 'local_output', 'reason_output', 'replay_al']))
        for row in replay.get('numeric_returns', []):
            assert pcs[row['original_entry']] == row['entry']
            lines.append(f'Q\t{row["entry"]:08x}\t{row["argument"]:08x}\t{row["result"]:08x}')
        watched = {roots['local_rng']+4, replay['local_output'], replay['reason_output']}
        watched.discard(0)
        stores = []
        sp = replay['registers']['esp']
        for row in replay['writes']:
            if sp-0x10000 <= row['address'] < sp+16:
                assert row['address'] not in watched
                continue
            assert row['address'] in watched and row['bytes'] == 4, row
            assert row['instruction'] in pcs, row
            before, after = [int.from_bytes(bytes.fromhex(row[k]), 'little') for k in ('before', 'after')]
            lines.append(f'S\t{pcs[row["instruction"]]:08x}\t{row["address"]:08x}\t4\t{before:08x}\t{after:08x}')
            stores.append(row)
        final = {row['address']: row['after'] for row in stores}
        for address, expected in final.items():
            actual_write = next(r for r in actual['writes'] if r['address'] == address)
            assert actual_write['actual'] == expected
        stored = {r['address'] for r in stores}
        if replay['reason_output']:
            assert replay['reason_output'] in stored
        if replay['replay_al']:
            assert stored == watched
        lines.append('E')
        captures.append(dict(replay_sha256=sha(replay_path), actual_return_sha256=sha(actual_path),
                             directory=str(directory), sound=f'{replay["sound"]:08x}',
                             accepted=bool(replay['replay_al']), pages=len(pages), stores=len(stores),
                             numeric_returns=len(replay.get('numeric_returns', []))))
    fixture = args.output / 'fixtures.tsv'
    fixture.write_text('\n'.join(lines) + '\n')
    report = dict(boundary=__doc__, captures=captures, exporter_sha256=sha(__file__),
                  pair_report_sha256=sha(args.pair / 'report.json'), fixture_sha256=sha(fixture),
                  immutable_provider_sha256={str(p): sha(p) for p in [shared_path, driver_path, admission_path]},
                  immutable_api_fields_not_referenced_in_paired_base_bodies=unreferenced,
                  expected_values_from_native=False,
                  limits=['CPU admission only; actual playback, scheduling and Xbox DSP remain separate.',
                          'Captured requests exercise the branches the actual game reached. No additional request is invented.'])
    (args.output / 'provenance.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Exported {len(captures)} genuine game calls, {sum(r["stores"] for r in captures)} semantic original stores')


if __name__ == '__main__':
    main()
