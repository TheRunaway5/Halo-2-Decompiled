#!/usr/bin/env python3
"""Export observed natural corpse node inputs and full retained outputs."""
import argparse
import json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN
from audit_mp_spartan_death_physical_1_1 import encoded, sha


def spans(bank):
    start = last = None
    raw = bytearray()
    for p in sorted(bank):
        if last is not None and p != last + 1:
            yield start, raw.hex()
            raw = bytearray()
        if not raw:
            start = p
        raw.append(bank[p])
        last = p
    if raw:
        yield start, raw.hex()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--directory', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    report = a.directory / 'report.json'
    r = json.loads(report.read_text())
    assert r['oracle_sha256'] == sha(ROOT / 'tools/oracle_mp_natural_corpse_nodes_1_1.py')
    assert r['source_sha256'] == PIN and r['natural_admission_snapshot_byte_identical']
    assert not r['instruction_patches'] and not r['supplied_gameplay_results']
    assert all(s['completed'] and not s['used_services'] for s in r['stages'])
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    for listing in r['instructions']:
        file = a.directory / listing['file']
        assert sha(file) == listing['sha256']
        for line in file.read_text().splitlines():
            address, raw, *_ = line.split(' ', 2)
            assert image.read(int(address, 16), len(bytes.fromhex(raw))) == bytes.fromhex(raw)
    initial = {}
    current = {}
    unit = int(r['unit'], 16)
    for i, b in enumerate(bytes.fromhex(r['before_unit_hex'])):
        initial[unit + i] = current[unit + i] = b
    for row in r['player_backing']:
        for i, b in enumerate(bytes.fromhex(row['raw_hex'])):
            at = int(row['address'], 16) + i
            assert at not in current or current[at] == b
            initial[at] = current[at] = b
    events = sorted([(x['order'], 'R', x) for x in r['reads']] +
                    [(x['order'], 'W', x) for x in r['stores']])
    writes = 0
    for _, kind, row in events:
        at = int(row['address'], 16)
        if 0x2000000 <= at < 0x2200000:
            continue
        raw = bytes.fromhex(row['raw_hex'] if kind == 'R' else row['before_hex'])
        for i, b in enumerate(raw):
            p = at + i
            if p in current:
                assert current[p] == b, (kind, row['instruction'], hex(p))
            else:
                initial[p] = current[p] = b
        if kind == 'W':
            for i, b in enumerate(bytes.fromhex(row['written_hex'])):
                current[at + i] = b
            writes += 1
    assert bytes(current[unit + i] for i in range(5750)).hex() == r['after_unit_hex']
    for at, n in ((0x10000000, 32), (0x10000100, 64)):
        for i in range(n):
            initial[at + i] = 0
    rows = [f"C {r['component']} {r['mask']} 10000000 10000100"]
    rows += [f'M {p:08x} {raw}' for p, raw in spans(initial)]
    rows += [f'A {p:08x} {raw}' for p, raw in spans(current)]
    for q in r['queries']:
        rows.append(f"T {r['pool']} {q['datum']} {q['body']} " +
                    (f"{q['node0_hex']} {q['type']} {q['biped_flags']}" if int(q['body'], 16) else '- 0 0'))
    rows.append(f"B {r['player_pool']}")
    for p in r['players']:
        rows.append(f"R {p['datum']} {p['current']} {p['previous']} {p['team']}")
    for q in r['selections']:
        rows.append(f"N {q['own']} {q['watched']} {q['team']} {q['result']}")
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text('\n'.join(rows) + '\n')
    a.output.with_suffix('.provenance.json').write_bytes(encoded(dict(
        tool_sha256=sha(__file__), report_sha256=sha(report),
        fixture_sha256=sha(a.output), source_sha256=PIN,
        initial_bytes=len(initial), compared_output_bytes=len(current), source_stores=writes,
        source_execution_replacements=[], natural_ragdoll_admission=True,
        lookup_queries=len(r['queries']), player_selections=len(r['selections']))))
    print(json.dumps(dict(initial_bytes=len(initial), compared_output_bytes=len(current),
                          source_stores=writes, lookup_queries=len(r['queries']))))


if __name__ == '__main__':
    main()
