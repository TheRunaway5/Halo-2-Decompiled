#!/usr/bin/env python3
"""Run the production reload/normal-ambient SDL-manual test on23 owned maps.

This is a CPU/native-provider scenario, not a23-map Vulkan or physical-speaker
claim. Each test runs actual one/four-player clocks and literal ambient tracks.
"""
import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path
from inspect_multiplayer import ROOT, MAPS
from inventory_mp_weapon_effects import DLC


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=False)
    bank = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content')
    image = bank / 'u5English/default.xbe'
    paths = [ROOT / 'assets/disc/maps' / (n + '.map') for n in MAPS]
    paths += [ROOT / 'assets/multiplayer-map-pack/demos/dlc' / b / (n + '.map')
              for b, ns in DLC.items() for n in ns]
    paths += [bank / 'c/4d53006400000004' / (n + '.map') for n in ('derelict', 'highplains2')]
    rows = []
    for path in paths:
        cmd = [str(a.binary.resolve()), str(path), str(ROOT / 'assets/disc/maps'), str(image)]
        r = subprocess.run(cmd, capture_output=True, text=True)
        (a.output / (path.stem + '.log')).write_text(r.stdout + r.stderr)
        rows.append(dict(map=str(path), map_sha256=hashlib.file_digest(path.open('rb'), 'sha256').hexdigest(),
                         command=cmd, returncode=r.returncode, stdout=r.stdout, stderr=r.stderr))
        print(path.stem, r.returncode, flush=True)
    report = dict(scope=__doc__, binary=str(a.binary.resolve()),
                  binary_sha256=hashlib.file_digest(a.binary.open('rb'), 'sha256').hexdigest(),
                  image_sha256=hashlib.file_digest(image.open('rb'), 'sha256').hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  environment={k: os.environ.get(k) for k in ('ASAN_OPTIONS', 'UBSAN_OPTIONS')}, cases=rows)
    (a.output / 'report.json').write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    assert len(rows) == 23 and all(r['returncode'] == 0 for r in rows)
    print('PASS23 owned maps: actual reload clocks, normal ambient track PCM and voice cleanup')


if __name__ == '__main__':
    main()
