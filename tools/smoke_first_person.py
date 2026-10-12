#!/usr/bin/env python3
"""Render fixed first-person action poses at original and wide aspect ratios.

Requires a generated first_person_tests TSV trace and an SDL-capable display.
No matching original framebuffer is supplied by this smoke test.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=Path('build/halo2_native'))
    parser.add_argument('--map', type=Path, default=Path('assets/disc/maps/01b_spacestation.map'))
    parser.add_argument('--trace', type=Path, default=Path('analysis/first-person/pose-frames.tsv'))
    parser.add_argument('--output', type=Path, default=Path('analysis/first-person/gpu'))
    parser.add_argument('--validation', action='store_true')
    args = parser.parse_args()
    binary = args.binary.resolve()
    binary_hash = hashlib.sha256(binary.read_bytes()).hexdigest()
    groups = {}
    with args.trace.open() as trace:
        for row in csv.DictReader(trace, delimiter='\t'):
            key = row['weapon'], row['action']
            groups[key] = max(groups.get(key, 0), int(row['frame']))
    if not groups:
        parser.error('pose trace has no action frames')
    args.output.mkdir(parents=True, exist_ok=True)
    results = []
    for (weapon, action), last in sorted(groups.items()):
        for frame in sorted({0, last // 2, last}):
            for aspect, width, height in [('4x3', 640, 480), ('16x9', 1280, 720)]:
                name = f'{weapon}-{action}-{frame}-{aspect}'
                capture = args.output / (name+'.ppm')
                log = args.output / (name+'.log')
                command = [str(binary), '--map', str(args.map), '--weapon', weapon,
                           '--first-person-pose', action, str(frame), '--viewport', str(width), str(height),
                           '--hidden', '--mute', '--frames', '3', '--screenshot', str(capture)]
                if args.validation:
                    command.append('--validation')
                with log.open('w') as output:
                    run = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=60)
                text = log.read_text()
                validation_messages = [line for line in text.splitlines() if 'Vulkan validation:' in line]
                passed = run.returncode == 0 and capture.is_file() and not validation_messages
                passed &= f'First person pose fixture: {action} frame={frame}' in text
                passed &= f'({width}x{height})' in text and 'Rendered 3 frames' in text
                record = {'weapon': weapon, 'action': action, 'frame': frame, 'viewport': [width, height],
                          'passed': bool(passed), 'exit_code': run.returncode,
                          'log': str(log), 'capture': str(capture), 'validation_messages': validation_messages}
                results.append(record)
                print(f'{name}: {"passed" if passed else "FAILED"}', flush=True)
    if hashlib.sha256(binary.read_bytes()).hexdigest() != binary_hash:
        raise RuntimeError('tested binary changed during the sweep')
    report = {'binary': str(binary), 'binary_sha256': binary_hash,
              'synchronization_validation_requested': bool(args.validation and os.environ.get('VK_LAYER_VALIDATE_SYNC') == '1'),
              'cases': len(results), 'passed': sum(r['passed'] for r in results),
              'results': results, 'original_frame_parity_verified': False,
              'scope': 'fixed native first-person action poses and Vulkan validation; no original framebuffer comparison'}
    (args.output/'summary.json').write_text(json.dumps(report, indent=2)+'\n')
    print(f'First person GPU poses: {report["passed"]}/{report["cases"]} passed', flush=True)
    return int(report['passed'] != report['cases'])


if __name__ == '__main__':
    raise SystemExit(main())
