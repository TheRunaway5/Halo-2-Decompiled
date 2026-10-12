#!/usr/bin/env python3
"""Compare native C8140/CEA70 with frozen original transfer-state observations."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/inventory-source/pickup-transfer-measured'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def require(value,message):
    if not value:raise ValueError(message)
def verify_authority():
    pins={BASE/'manifest.json':'69f76789503370fb268add2d046b39e7556834f185d3eff8274755a6384cfac7',
        ROOT/'docs/MP_PICKUP_TRANSFER_MEASURED_RESEARCH.md':'6b715bb92a1757d1f2ff7752a952a429073aeaaf6b92ffae9c6e2e3a0ba3746b',
        ROOT/'tools/oracle_mp_pickup_transfer_measured_1_1.py':'e22428cddbb685b8496b52774ad87ae9430febdf14164bc665fdf9f44c061fed',
        ROOT/'tools/oracle_mp_pickup_transfer_measured_audit_1_1.py':'8e00e7077b0879d0856549379c101b9bd9a0c9f20ab1df5e300881284e7a4ebf'}
    for path,sha in pins.items():require(digest(path)==sha,f'Changed independent authority: {path}')
    manifest=json.loads((BASE/'manifest.json').read_text())
    for name,record in manifest['payloads'].items():require(digest(BASE/name)==record['sha256'],f'Changed observed payload: {name}')
    for name,sha in manifest['source_pins'].items():require(digest(ROOT/name)==sha,f'Changed original dependency: {name}')
    for listing in manifest['instructions']:require(digest(ROOT/listing['path'])==listing['sha256'],'Changed original instruction listing')
    require((BASE/'report.json').read_bytes()==(BASE/'reexecution.json').read_bytes(),'Original independent repeat differs')
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-multiplayer/unit_input_tests')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/unit-input-differential.json')
    a=p.parse_args();verify_authority();original=json.loads((BASE/'report.json').read_text());rows=[]
    for case in original['cases']:
        for call in case['calls']:
            if call['entry'] not in ['000c8140','000cea70']:continue
            before=case['states'][call['before_state']];after=case['states'][call['after_state']]
            require(before['id']==call['before_state'] and after['id']==call['after_state'],'Stored original state correlation differs')
            prefix=before['unit']['body_prefix_hex']
            if call['entry']=='000c8140':
                rows.append(['I',case['id'],call['id'],call['unit_control_input_hex'],prefix,after['unit']['body_prefix_hex']])
            else:rows.append(['S',case['id'],call['id'],prefix,f"{int(call['return_eax'],16)&65535:04x}"])
    fixture=ROOT/'analysis/multiplayer/integration/unit-input-fixtures.tsv'
    fixture.write_text('\n'.join(' '.join(map(str,row)) for row in rows)+'\n')
    run=subprocess.run([str(a.binary.resolve()),str(fixture)],text=True,capture_output=True)
    print(run.stdout,end='');print(run.stderr,end='')
    report=dict(passed=run.returncode==0,scope=__doc__,source_cases=5,unit_input_calls=10,slot_scans=9,
        complete_unit_prefixes_compared=True,full_inventory_scan_executed=False,live_input_transfer_integration=False,
        limits='Literal original 0x7C-byte marshalling and source unit storage. This does not produce controls/current live units, admission, ownership, world removal, attachment or physical settling. Source game-mode/sequence-advance orchestration is separate.',
        original_manifest_sha256=digest(BASE/'manifest.json'),original_report_sha256=digest(BASE/'report.json'),
        fixture_sha256=digest(fixture),native_binary_sha256=digest(a.binary),tool_sha256=digest(Path(__file__)),stdout=run.stdout,stderr=run.stderr)
    a.output.write_text(json.dumps(report,indent=2)+'\n');raise SystemExit(run.returncode)
if __name__=='__main__':main()
