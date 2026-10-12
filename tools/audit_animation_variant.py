#!/usr/bin/env python3
"""Read-only audit of the separately frozen original animation variant study."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, Cache


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=ROOT/'analysis/multiplayer/integration/animation-variant')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    base = args.directory.resolve()
    pairs = []
    for first, second in (('measured', 'repeat'), ('u5', 'u5-repeat'), ('clock-measured', 'clock-repeat')):
        files = [p for p in (base/first).rglob('*') if p.is_file() and p.suffix in ('.json', '.asm')]
        assert files and (base/first/'report.json').exists(), first
        for path in files:
            assert path.read_bytes() == (base/second/path.relative_to(base/first)).read_bytes(), str(path)
        pairs.append(dict(first=first, second=second, exact_files=len(files), report_sha256=sha(base/first/'report.json')))
    report = json.loads((base/'measured/report.json').read_text())
    image = Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest() == report['xbe_sha256']
    cache = Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(cache.data).hexdigest() == report['cache_sha256']
    old_rows = 0
    for listing in report['listings']:
        path = base/'measured'/listing['file']
        assert sha(path) == listing['sha256']
        for line in path.read_text().splitlines():
            address, raw, *_ = line.split()
            assert image.read(int(address,16),len(bytes.fromhex(raw))) == bytes.fromhex(raw)
            old_rows += 1
    for path, expected in report['source_pins'].items():
        assert sha(ROOT/path) == expected, path
    for address, bits in report['constants'].items():
        assert struct.unpack('<I',image.read(int(address,16),4))[0] == int(bits,16)
    graph_regions = 0
    for graph in report['graphs']:
        tag = cache.tags[int(graph['datum'],16)]
        assert tag['group'] == 'jmad' and tag['offset']+cache.magic == int(graph['address'],16)
        for region in graph['regions']:
            start = int(region['address'],16)-cache.magic
            assert bytes(cache.data[start:start+region['bytes']]).hex() == region['raw_hex']
            graph_regions += 1
    assert report['new_guest_seed_or_graph_fields'] == []
    assert report['new_selected_variant_or_resource_results'] == []
    assert report['instruction_patches'] == []
    assert all(root['completion']=='complete_original_return' for root in report['declared_pure_roots'])
    assert all('return_order' in call for call in report['calls'])
    rng_stores = [row for row in report['stores'] if row['instruction'] in ('001e10d1','001e10ef')]
    for row in rng_stores:
        assert row['size'] == 4
        before = int.from_bytes(bytes.fromhex(row['before_hex']),'little')
        after = int.from_bytes(bytes.fromhex(row['written_hex']),'little')
        assert after == (before*0x19660d+0x3c6ef35f)&0xffffffff
    comparison = json.loads((base/'u5/report.json').read_text())
    correspondence_rows = 0
    for owner in comparison['owners']:
        assert owner['normalized_complete_body_and_internal_branch_equal'] and owner['unique_full_body_match']
        correspondence_rows += owner['rows']
    clock = json.loads((base/'clock-measured/report.json').read_text())
    assert clock['total_ticks'] == 519
    clock_calls = 0
    for case in clock['cases']:
        path = base/'clock-measured'/case['file']
        assert sha(path) == case['sha256']
        sample = json.loads(path.read_text())
        assert sample['new_guest_inputs'] == sample['new_supplied_results'] == sample['instruction_patches'] == []
        assert sample['original_runtime_correlation']['exact_fields']
        assert sha(ROOT/sample['original_runtime_correlation']['path']) == sample['original_runtime_correlation']['sha256']
        assert all('return_order' in call for call in sample['calls'])
        clock_calls += len(sample['calls'])
    assert (base/'native/fixtures.txt').read_bytes() == (base/'native/fixtures-repeat.txt').read_bytes()
    result = dict(schema='animation-variant-audit-v1',tool_sha256=sha(__file__),
        repeated_pairs=pairs,original_instruction_rows=old_rows,u5_correspondence_rows=correspondence_rows,
        source_calls=len(report['calls']),source_stores=len(report['stores']),
        actual_rng_stores=len(rng_stores),authored_graphs=len(report['graphs']),graph_regions=graph_regions,
        pure_roots=len(report['declared_pure_roots']),clock_ticks=519,clock_variant_calls=clock_calls,
        native_fixture_sha256=sha(base/'native/fixtures.txt'),
        boundaries=['Inherited bounded seed/resource/fixture inputs remain explicit; this audit does not establish ordinary initialization.',
                    'u5 complete variant bodies and operands match; resource/service owners are separately required.',
                    'Native callbacks replay complete recorded source boundaries; no live resource success or global RNG order is inferred.'])
    raw=(json.dumps(result,sort_keys=True,indent=2)+'\n').encode()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    if args.output.exists(): assert args.output.read_bytes()==raw, 'Immutable audit differs'
    else: args.output.write_bytes(raw)
    print(json.dumps(result,sort_keys=True))


if __name__=='__main__':main()
