#!/usr/bin/env python3
"""Check bounded native terrain outer results against frozen original outputs."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/inventory-source/pickup-outer-fixtures'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-multiplayer/terrain_outer_tests')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/terrain-outer-differential.json')
    a=p.parse_args()
    pins={BASE/'manifest.json':'36c1ba9903b5e6e1ca42d8721d9e0061545df5fe5fae597c75c60c66f52e3fe0',
          ROOT/'tools/export_mp_pickup_outer_fixtures_1_1.py':'666b8b0cc79aa2dc1ede057e015a373ff6608e7db931ba46b5fca9e6579ab124',
          ROOT/'docs/MP_PICKUP_OUTER_FIXTURES.md':'a10180f4205e9719893ec40bcecfb04a7f1f3efaf7a339c0584d2a9689b7884f'}
    for file,sha in pins.items():
        if digest(file)!=sha:raise ValueError(f'Changed frozen authority: {file}')
    manifest=json.loads((BASE/'manifest.json').read_text())
    for file,sha in manifest['source_pins'].items():
        if digest(ROOT/file)!=sha:raise ValueError(f'Changed source dependency: {file}')
    for file,record in manifest['payloads'].items():
        if digest(BASE/file)!=record['sha256']:raise ValueError(f'Changed original payload: {file}')
    run=subprocess.run([str(a.binary.resolve()),str(ROOT),str(BASE/'fixtures.tsv')],text=True,capture_output=True)
    print(run.stdout,end='');print(run.stderr,end='')
    report={'passed':run.returncode==0,'scope':__doc__,
            'ordinary_queries':1785,'conditional_active_physics_queries_excluded':2,
            'retreat_iterations':2122,'classifier_calls':2645,'locator_calls':2364,
            'oriented_plane_calls':3,'normal_offsets':107,'leaf_cluster_imports':2313,'material_imports':19,
            'staged_S_observations_compared':False,'live_pickup_integration':False,
            'zero_dot_unordered_paths_executed':False,
            'source_manifest_sha256':digest(BASE/'manifest.json'),
            'native_binary_sha256':digest(a.binary),'tool_sha256':digest(Path(__file__)),
            'stdout':run.stdout,'stderr':run.stderr}
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    raise SystemExit(run.returncode)
if __name__=='__main__':main()
