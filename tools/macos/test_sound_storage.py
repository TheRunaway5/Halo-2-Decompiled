#!/usr/bin/env python3
"""Run the original sound-storage comparison on both Mac slices in owned scratch.

Invoke through tools/run_test.py. Only bounded receipts and test reports survive;
the authenticated original fixture and frozen source copies are temporary.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[2]
REFERENCE = 'analysis/multiplayer/integration/sound-cache-startup-native/fixtures.tsv'
SOURCES = [
    'src/audio/native_sound_storage.cpp', 'src/audio/sound_cache_startup.cpp',
    'include/halo2/native_sound_storage.hpp', 'include/halo2/sound_cache_startup.hpp',
    'tests/native_sound_storage_tests.cpp', 'tests/macos/sound_storage/CMakeLists.txt',
    'tests/macos/sound_storage/granularity_tests.cpp', 'cmake/TestStorage.cmake',
    'cmake/macos/Defaults.cmake', 'tools/run_test.py', 'tools/run_tests.py',
    'tools/macos/test_sound_storage.py', REFERENCE,
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mac', default='codex-mac')
    parser.add_argument('--mac-root', default='/Users/eric/Developer/Halo2-macos-port')
    parser.add_argument('--mac-output', default='build/test-results/sound-storage-original')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output = args.output.resolve(); args.output.mkdir(parents=True, exist_ok=True)
    source = scratch/'source'; source.mkdir()
    files = {}
    for name in SOURCES:
        data = (ROOT/name).read_bytes()
        target = source/name; target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        files[name] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    provenance = json.loads((ROOT/Path(REFERENCE).with_suffix('.json')).read_text())
    assert files[REFERENCE]['sha256'] == provenance['fixture_sha256']
    receipt = {'schema': 'macos-sound-storage-input-v1', 'files': files,
               'reference_provenance': provenance, 'local_scratch_owner': str(scratch)}
    (args.output/'input-receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    remote = 'receipt = '+repr(receipt)+'\n'+'''from pathlib import Path
import hashlib,json,os,subprocess,sys,tarfile
scratch=Path(os.environ['HALO2_TEST_SCRATCH']); source=scratch/'source'; source.mkdir()
output=Path(sys.argv[1]).resolve(); output.mkdir(parents=True,exist_ok=True)
with tarfile.open(fileobj=sys.stdin.buffer,mode='r|gz') as archive:
    archive.extractall(source,filter='data')
for name,item in receipt['files'].items():
    path=source/name
    assert path.stat().st_size==item['bytes']
    assert hashlib.sha256(path.read_bytes()).hexdigest()==item['sha256'],name
(output/'input-receipt.json').write_text(json.dumps(receipt,indent=2)+'\\n')
command=[sys.executable,'-B',str(source/'tools/run_tests.py'),
    '--source',str(source/'tests/macos/sound_storage'),
    '--scratch-root',str(scratch/'build-owners'),
    '--target','sound_storage_granularity_tests','--target','native_sound_storage_tests',
    '--filter','^sound_storage_','--output',str(output)]
for arch in ['arm64','x86_64']:
    for suffix in [arch,'original-'+arch,'82e44000-'+arch,'82e45000-'+arch]:
        command+=['--retain-report','sound-storage-'+suffix+'.json']
status=subprocess.run(command).returncode
(output/'execution.json').write_text(json.dumps({'command':command,'exit_code':status,
    'remote_scratch_owner':str(scratch)},indent=2)+'\\n')
raise SystemExit(status)
'''
    remote_command = 'cd '+shlex.quote(args.mac_root)+' && PATH=/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin caffeinate -i '+shlex.join([
        'python3','-B','tools/run_test.py','--scratch-root','.local/test-scratch','--',
        'python3','-B','-c',remote,args.mac_output])
    ssh = ['ssh','-o','BatchMode=yes','-o','ConnectTimeout=15',args.mac]
    process = subprocess.Popen([*ssh,remote_command], stdin=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdin,mode='w|gz') as archive:
            for name in SOURCES: archive.add(source/name,arcname=name,recursive=False)
    finally:
        process.stdin.close()
    status = process.wait()
    remote_output = str(Path(args.mac_root)/args.mac_output)
    subprocess.run(['rsync','-az','-e','ssh -o BatchMode=yes -o ConnectTimeout=15',
                    args.mac+':'+remote_output+'/',str(args.output/'mac')+'/'],check=True)
    execution = json.loads((args.output/'mac/execution.json').read_text())
    result = json.loads((args.output/'mac/report.json').read_text())
    paths = [execution['remote_scratch_owner'],str(Path(result['build']).parent)]
    verify = 'from pathlib import Path; paths='+repr(paths)+'; assert not any(Path(p).exists() for p in paths); print("Verified owned source, fixture and build removal")'
    cleanup = subprocess.run([*ssh,shlex.join(['/usr/bin/python3','-c',verify])]).returncode
    (args.output/'execution.json').write_text(json.dumps({
        'remote_command':remote_command,'exit_code':status,'remote_cleanup_verified':cleanup==0,
        'removed_remote_owners':paths,'local_scratch_owner':str(scratch)},indent=2)+'\n')
    return status or cleanup


if __name__ == '__main__':
    raise SystemExit(main())
