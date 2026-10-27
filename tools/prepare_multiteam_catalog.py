#!/usr/bin/env python3
"""Prepare, never publish, a native Multi-Team addition to a pinned catalog.

Run through tools/run_test.py. The optional remote validation uses only a
bounded disposable read-only, network-disabled container and public catalog
bytes. No production container, database or configuration is changed.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PIN = '3b16d1b74c4b096e6e5970229ce822a338318d611feff2e66a17b2aefa3e0ae4'
IMAGE = 'sha256:5a385b8201bd30dcde5bcb1b63c4c9fe740b679b61fce72f25008cc64a74ebe3'
REMOTE = r"""
import base64,hashlib,json,pathlib,signal,subprocess,sys,uuid
request=json.load(sys.stdin);name='halo2-catalog-qualify-'+uuid.uuid4().hex[:10]
def run(args,**kw):return subprocess.run(args,text=True,capture_output=True,timeout=60,**kw)
def interrupted(*_):raise TimeoutError('bounded catalog qualification interrupted')
signal.signal(signal.SIGALRM,interrupted);signal.alarm(85)
catalog=pathlib.Path('/opt/halo2/catalogs/tombstone-20261013T225156Z.json')
before=hashlib.sha256(catalog.read_bytes()).hexdigest();assert before==request['before_sha256']
code='''import hashlib,json,pathlib,sys
from multiplayer.master_server.matchmaking_http import load_catalog
from multiplayer.master_server.security_policy import library
p=pathlib.Path('/tmp/catalog.json');p.write_bytes(sys.stdin.buffer.read());entries=load_catalog(p)
assert len(entries)==10 and sum(e.quickmatch for e in entries)==1
e=next(e for e in entries if e.policy.id=='multi-team')
assert (e.policy.team_size,e.policy.team_count,e.policy.minimum_players,e.policy.maximum_players,e.policy.maximum_party)==(3,4,12,12,12)
assert not e.policy.ranked and not e.quickmatch and e.required_compatibility=='halo2-native-queue-v10'
assert [(g.map,g.variant,g.weight) for g in e.policy.games]==[('lockout','Team Slayer',1)]
assert library().h2_security_abi()==2
print(json.dumps({'passed':True,'playlists':[e.policy.id for e in entries],'quickmatch':[e.policy.id for e in entries if e.quickmatch],'native_abi':2,'catalog_sha256':hashlib.sha256(p.read_bytes()).hexdigest()}))'''
try:
 image=run(['docker','image','inspect',request['image']]);assert image.returncode==0,image.stderr
 assert json.loads(image.stdout)[0]['Id']==request['image']
 args=['docker','run','--rm','-i','--name',name,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges:true','--pids-limit','32','--memory','128m','--ulimit','core=0','--tmpfs','/tmp:rw,noexec,nosuid,nodev,size=16m','--entrypoint','python',request['image'],'-c',code]
 result=run(args,input=base64.b64decode(request['catalog']).decode());assert result.returncode==0,result.stderr
 parsed=json.loads(result.stdout);assert parsed['catalog_sha256']==request['merged_sha256']
 assert hashlib.sha256(catalog.read_bytes()).hexdigest()==before
 print(json.dumps({'image':request['image'],'validation':parsed,'production_catalog_unchanged_sha256':before,'production_replaced':False}))
finally:
 subprocess.run(['docker','rm','-f',name],capture_output=True,timeout=15)
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--remote', default='halo2-master')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    original = args.before.read_bytes()
    assert hashlib.sha256(original).hexdigest() == PIN
    before = json.loads(original)
    assert len(before['playlists']) == 9
    example = ROOT / 'multiplayer/master_server/examples/matchmaking-multiteam-native.json'
    offer = json.loads(example.read_bytes())['playlists'][0]
    assert offer['id'] not in {p['id'] for p in before['playlists']}
    # Preserve the existing single Quickmatch default. This new offer is
    # selectable through Optimatch/Game Setup and does not replace that owner.
    offer['quickmatch'] = False
    merged = dict(before, playlists=[*before['playlists'], offer])
    assert merged['playlists'][:9] == before['playlists']
    assert sum(p['quickmatch'] for p in merged['playlists']) == 1
    payload = (json.dumps(merged, indent=2, ensure_ascii=False) + '\n').encode()
    merged_sha = hashlib.sha256(payload).hexdigest()
    request = dict(image=IMAGE, before_sha256=PIN, merged_sha256=merged_sha,
                   catalog=base64.b64encode(payload).decode())
    result = subprocess.run(['ssh', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=10', args.remote,
                             'python3 -c ' + shlex.quote(REMOTE)],
                            input=json.dumps(request), text=True, capture_output=True, timeout=100)
    assert result.returncode == 0, result.stderr
    validation = json.loads(result.stdout)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(payload)
    report = dict(schema='H2_NATIVE_MULTITEAM_CATALOG_PREPARATION_1', passed=True,
                  before_sha256=PIN, merged_sha256=merged_sha, output=str(args.output),
                  preserved_definitions=[p['id'] for p in before['playlists']],
                  example_sha256=hashlib.sha256(example.read_bytes()).hexdigest(),
                  changes_to_example={'quickmatch':False}, remote=validation,
                  production_published=False, production_restart=False,
                  activation_gate='Actual twelve-seat native GPU gameplay/PCR/requeue acceptance remains pending.',
                  historical_boundary='Explicit native single-map offer. Exact historical Multi-Team rotation weights, full variant buffers and downloaded hopper/network configuration are unavailable.',
                  scratch=str(scratch))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
