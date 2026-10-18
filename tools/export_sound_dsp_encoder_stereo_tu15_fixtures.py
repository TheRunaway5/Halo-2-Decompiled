#!/usr/bin/env python3
"""Retain original EP stereo output for declared six-channel/scale inputs.

The oracle executes the complete original EP boot and observes its first real
FIFO0 emission using the pinned analysis emulator. No native expected output
is supplied. Shared emulator timing and physical-console equivalence are not
established. Run under tag_match_memory_budget.py, with a fresh output path.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from audit_uploaded_sound_dsp_tu15 import audit


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--oracle', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    capture_proof = audit(args.capture, root / 'tests/fixtures/sound_dsp_image_tu15',
                          root / 'tools/capture_live_sound_dsp_tu15.py')
    assert capture_proof['encoder_capture_verified']
    witness = json.loads((args.capture / 'report.json').read_text())
    assert witness['xbe_sha256'] == '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    assert all(witness[key] == [] for key in ('original_code_patches', 'original_memory_writes',
                                            'original_register_writes', 'supplied_callee_returns'))
    encoder = witness['encoder']
    assert encoder['enabled'] and encoder['scratch']['stable_readback']
    scratch = args.capture / 'ep_scratch.bin'
    assert scratch.stat().st_size == 0x13000 == encoder['scratch']['bytes']
    assert sha(scratch) == encoder['scratch']['sha256']
    args.output.mkdir(parents=True, exist_ok=False)
    rng = random.Random(0x02150927)
    cases = []

    def add(name, channels, enabled=0, gain=0x65ac8c):
        assert len(channels) == 6 and all(len(channel) == 256 for channel in channels)
        cases.append((name, enabled, gain, channels))

    add('silence', [[0] * 256 for _ in range(6)])
    for channel in range(6):
        values = [[0] * 256 for _ in range(6)]
        values[channel] = [0x100000] * 256
        add(f'channel-{channel}', values)
    boundaries = [0, 1, 0xffffff, 255, 256, 257, 0xffff01, 0xffff00,
                  0xfffeff, 0x7fffff, 0x800000, 0x400000, 0xc00000]
    for enabled in (0, 1, 0xffffff):
        for gain in (0, 1, 0x3fffff, 0x400000, 0x400001, 0x65ac8c, 0x7fffff, 0x800000, 0xffffff):
            add(f'boundaries-{enabled:x}-{gain:x}',
                [[boundaries[(i + channel * 3) % len(boundaries)] for i in range(256)]
                 for channel in range(6)], enabled, gain)
    for index in range(64):
        enabled = index % 2
        gain = rng.randrange(0x1000000)
        add(f'random-{index}', [[rng.randrange(0x1000000) for _ in range(256)]
                                for _ in range(6)], enabled, gain)
    for value in (0x7fffff, 0x800000):
        add(f'all-rail-{value:x}', [[value] * 256 for _ in range(6)])
    rows = []
    for index, (name, enabled, gain, channels) in enumerate(cases):
        stem = f'{index:03d}-{name}'
        input_path = args.output / (stem + '.input')
        expected_path = args.output / (stem + '.output')
        input_path.write_bytes(struct.pack('<1538I', enabled, gain, *[v for c in channels for v in c]))
        command = [str(args.oracle.resolve()), str(scratch.resolve()),
                   str(input_path.resolve()), str(expected_path.resolve())]
        result = subprocess.run(command, capture_output=True, text=True, timeout=15)
        assert result.returncode == 0, result.stdout + result.stderr
        assert expected_path.stat().st_size == 3072
        rows.append(dict(name=stem, input_sha256=sha(input_path), expected_sha256=sha(expected_path)))
    (args.output / 'manifest.txt').write_text(''.join(row['name'] + '\n' for row in rows))
    report = dict(scope=__doc__, xbe_sha256=witness['xbe_sha256'], cases=rows,
                  generator_sha256=sha(Path(__file__)),
                  capture_audit_sha256=sha(root / 'tools/audit_uploaded_sound_dsp_tu15.py'),
                  oracle_sha256=sha(args.oracle), capture_sha256=sha(args.capture / 'report.json'),
                  scratch_sha256=sha(scratch), native_expected_data_supplied=False,
                  samples_per_case=256, original_fifo_packets_per_case=1,
                  native_dsp_pipeline_verified=False, physical_console_verified=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {len(rows)} original EP FIFO/24-bit-output fixtures; explicit offline input ABI', flush=True)


if __name__ == '__main__':
    main()
