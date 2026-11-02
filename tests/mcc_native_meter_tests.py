"""Build literal MCC meter inputs only inside owned CTest scratch."""
import argparse
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, required=True)
    parser.add_argument('--asset-root', type=Path)
    parser.add_argument('--test-executable', type=Path, required=True)
    args = parser.parse_args()
    repo = args.repo_root.resolve(strict=True)
    owner = (args.asset_root or repo).resolve(strict=True)
    prepared = owner / 'assets/imported/mcc-sky-water-tu15-20261025-v7'
    if not (prepared / 'halo2/h2_maps_win64_dx11/shared.map').is_file():
        return 77
    sys.path.insert(0, str(repo / 'tools'))
    spec = importlib.util.spec_from_file_location('meter_exporter', repo / 'tools/export_mcc_native_meter.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    _, fixture = module.generate(prepared)
    with tempfile.TemporaryDirectory(prefix='mcc-meter-', dir=os.environ['HALO2_TEST_SCRATCH']) as folder:
        path = Path(folder) / 'fixture.json'
        path.write_bytes(module.canonical(fixture))
        return subprocess.run([str(args.test_executable.resolve(strict=True)), str(path),
            str(repo / 'tools/data/mcc-native-meter.json')], check=False).returncode


if __name__ == '__main__':
    raise SystemExit(main())
