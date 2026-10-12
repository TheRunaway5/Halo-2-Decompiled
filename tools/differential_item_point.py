#!/usr/bin/env python3
"""Compare native FBFA0 item points with immutable original-state observations."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/inventory-source/pickup-item-point'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def require(value,message):
    if not value:raise ValueError(message)
def verify_authority():
    pins={BASE/'manifest.json':'9ef827dcd6a4f949243c8d9bbd7288c066c9a06141f79b1c465c1ff7186f0673',
        ROOT/'docs/MP_ITEM_POINT_RESEARCH.md':'8cda6cb63a81d239858129eecc1c9b6a6939d9d944e7e53ff7e3f9534db84872',
        ROOT/'tools/oracle_mp_item_point_1_1.py':'f9b51db79291538e07dba3d44517f80f0701d623702bd627b965ed9d2e5aab28',
        ROOT/'tools/oracle_mp_item_point_audit_1_1.py':'88358aef555c7aab268188b42fcb0195a627fabd283ee4b5cae0a22cfa029f27'}
    for path,sha in pins.items():require(digest(path)==sha,f'Changed independent authority: {path}')
    manifest=json.loads((BASE/'manifest.json').read_text())
    for name,record in manifest['payloads'].items():require(digest(BASE/name)==record['sha256'],f'Changed original payload: {name}')
    for path,sha in manifest['source_pins'].items():require(digest(ROOT/path)==sha,f'Changed original dependency: {path}')
    for ins in manifest['instructions']:require(digest(BASE/'instructions'/ins['name'])==ins['sha256'],'Changed original instruction listing')
    require((BASE/'report.json').read_bytes()==(BASE/'reexecution/report.json').read_bytes(),'Original reexecution differs')
    return manifest
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-multiplayer/item_point_tests')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/item-point-differential.json')
    a=p.parse_args()
    verify_authority()
    metadata=json.loads((BASE/'metadata.json').read_text());rows=[]
    for d in metadata:
        group=next(g for g in d['marker_groups'] if g['record']['raw_hex'].startswith('c100000c'))
        rows.append(['D',d['tag']['datum'],d['model']['datum'],d['render_model']['datum'],group['index'],group['markers'][0]['raw_hex']])
    supplied=ROOT/'analysis/multiplayer/integration/item-point-metadata.tsv'
    supplied.write_text('\n'.join(' '.join(map(str,r)) for r in rows)+'\n')
    run=subprocess.run([str(a.binary.resolve()),str(ROOT),str(BASE/'fixtures.tsv'),str(supplied)],text=True,capture_output=True)
    print(run.stdout,end='');print(run.stderr,end='')
    report=dict(passed=run.returncode==0,scope=__doc__,original_points=208,root_fallbacks=205,marker_projections=3,original_input_nodes=832,defined_stages=21,
        live_pickup_integration=False,pose_contact_registry_production_proven=False,
        limits='Literal original unparented current states; three projections reuse one SMG node/contact state. Active ordinary collision bank explicitly supplied. BR contact, parent/mirror/region/remap/instanced/negative-plane branches and live world producers remain open.',
        original_fixture_sha256=digest(BASE/'fixtures.tsv'),metadata_fixture_sha256=digest(supplied),native_binary_sha256=digest(a.binary),tool_sha256=digest(Path(__file__)),stdout=run.stdout,stderr=run.stderr)
    a.output.write_text(json.dumps(report,indent=2)+'\n');raise SystemExit(run.returncode)
if __name__=='__main__':main()
