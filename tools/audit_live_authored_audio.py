#!/usr/bin/env python3
"""Read-only audit of the independently retained native multiplayer audio checkpoint."""
import argparse
import hashlib
import json
from pathlib import Path


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--owned-inputs', action='store_true')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text())
    base = args.manifest.parent
    mismatches = []
    for row in manifest['artifacts']:
        path = base / row['path']
        if not path.is_file() or path.stat().st_size != row['size'] or digest(path) != row['sha256']:
            mismatches.append(str(path))
    if args.owned_inputs:
        for row in manifest['owned_inputs']:
            path = Path(row['path'])
            if not path.is_file() or path.stat().st_size != row['size'] or digest(path) != row['sha256']:
                mismatches.append(str(path))

    evidence = base / 'evidence'
    foot = [json.loads((evidence / name / 'report.json').read_text()) for name in
            ('footstep-original', 'footstep-original-repeat', 'footstep-original-1.1', 'footstep-original-1.5')]
    assert all(len(r['cases']) == 154 for r in foot)
    assert all(r['cases'] == foot[0]['cases'] for r in foot)
    ambient = [json.loads((evidence / name / 'report.json').read_text()) for name in
               ('ambient-track-original', 'ambient-track-original-repeat', 'ambient-track-original-1.1', 'ambient-track-original-1.5')]
    assert all(len(r['cases']) == 4838 and len(r['maps']) == 23 for r in ambient)
    assert all(r['cases'] == ambient[0]['cases'] for r in ambient)
    normal = [r for r in ambient[0]['cases'] if r['alternate'] == 0]
    assert len(normal) == 2419
    assert all(r['selected'] == f"{int.from_bytes(bytes.fromhex(r['raw_track'])[0x20:0x24], 'little'):08x}"
               and r['desired_phase'] == 1 for r in normal)
    assert (evidence / 'ambient-correspondence-final/report.json').read_bytes() == (
        evidence / 'ambient-correspondence-repeat/report.json').read_bytes()
    production = [json.loads((evidence / name / 'report.json').read_text()) for name in
                  ('all-map-normal-volume-final', 'all-map-sanitizer-final')]
    assert all(len(r['cases']) == 23 and all(c['returncode'] == 0 for c in r['cases']) for r in production)
    assert all(a['map_sha256'] == b['map_sha256'] and a['stdout'] == b['stdout'] and a['stderr'] == b['stderr']
               for a, b in zip(production[0]['cases'], production[1]['cases']))
    assert production[0]['binary_sha256'] == digest(base / 'normal/multiplayer_audio_tests')
    assert production[1]['binary_sha256'] == digest(base / 'sanitizer/multiplayer_audio_tests')
    result = dict(manifest_sha256=digest(args.manifest), artifacts=len(manifest['artifacts']),
                  owned_inputs_checked=len(manifest['owned_inputs']) if args.owned_inputs else 0,
                  mismatches=mismatches, original_footstep_controls=154,
                  original_ambient_controls=4838, normal_ambient_controls=2419,
                  original_three_image_outputs_and_repeat_equal=True,
                  final_normal_sanitizer_maps=23, final_normal_sanitizer_outputs_equal=True,
                  scope='Exact frozen artifacts and declared comparisons; no physical speaker, complete scheduler, or full-tree freshness claim.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open('x') as stream:
        stream.write(json.dumps(result, sort_keys=True, indent=2) + '\n')
    assert not mismatches, mismatches
    print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
