#!/usr/bin/env python3
"""Compare the native processed view with frozen unchanged original outputs.

The original report/buffers remain untouched. Executable data and generated
fixtures stay local; no original tuning payload is committed as native code.
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / 'analysis/multiplayer/title-update-1.5/combat-postprocess/measured/report.json'
REPORT_SHA = '9300d1ac08b64a4f1eb815e01ac8e1d365b37b2483a89bf495aa99922815da11'

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--update-xbe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--content', action='store_true', help='also exercise the live Cache and LAN compatibility identity')
    args = parser.parse_args()
    assert sha(REPORT) == REPORT_SHA, 'frozen original report changed'
    original = json.loads(REPORT.read_text())
    cache = ROOT / 'assets/disc/maps/lockout.map'
    assert sha(cache) == original['cache_sha256']
    raw = cache.read_bytes()
    args.output.mkdir(parents=True, exist_ok=True)
    images = []
    sources = [ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03', args.update_xbe]
    for version, row, xbe in zip(('1.1', '1.5'), original['images'], sources):
        assert sha(xbe) == row['xbe_sha256'], 'selected original image differs'
        expected = bytearray(raw)
        for changed in row['changed_tags']:
            at = changed['tag']['offset']
            before, after = bytes.fromhex(changed['raw_hex']), bytes.fromhex(changed['processed_hex'])
            assert expected[at:at + len(before)] == before
            expected[at:at + len(after)] = after
        assert hashlib.sha256(expected).hexdigest() == row['complete_cache_output_sha256']
        expected_path = args.output / (version + '-original-output.map')
        expected_path.write_bytes(expected)
        stores = [s for s in row['ordered_stores'] if s['region'] == 'original_cache_metadata']
        fixture = args.output / (version + '-stores.tsv')
        fixture.write_text('# instruction file_offset before_bits after_bits\n' + ''.join(
            f"{s['instruction']} {s['file_offset']} {s['before_bits']} {s['after_bits']}\n" for s in stores))
        output = args.output / (version + '-native-output.map')
        command = [str(args.native.resolve()), str(cache), str(xbe), str(fixture), str(output)]
        if args.content:
            command.extend(['--content', str(expected_path)])
        result = subprocess.run(command, text=True, capture_output=True, check=True)
        measured = json.loads(result.stdout)
        ordered = [[int(s['instruction'], 16), int(s['file_offset'], 16), int(s['before_bits'], 16),
                    int(s['after_bits'], 16)] for s in stores]
        assert measured == {'admitted': True, 'stores': ordered}, 'ordered native stores differ'
        assert output.read_bytes() == expected, 'complete native cache differs from original'
        images.append({'version': version, 'xbe_sha256': sha(xbe), 'native_binary_sha256': sha(args.native),
                       'ordered_stores': len(stores), 'complete_output_sha256': sha(output),
                       'fixture_sha256': sha(fixture), 'live_cache_and_lan_identity_checked': args.content,
                       'stderr': result.stderr})
    report = {'tool_sha256': sha(__file__), 'original_report_sha256': REPORT_SHA,
              'raw_cache_sha256': sha(cache), 'images': images, 'all_match': True,
              'scope': 'Exact ordered metadata writes and complete buffers; optional live Cache weapon/damage import and native LAN data identity. This is not a whole 1.5 simulation or live PvP/input/physics validation.'}
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'all_match': True, 'original_stores_per_image': len(stores),
                      'live_cache_and_lan_identity_checked': args.content,
                      'report_sha256': sha(args.output / 'report.json')}))

if __name__ == '__main__':
    main()
