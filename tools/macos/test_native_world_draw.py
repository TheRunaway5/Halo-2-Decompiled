#!/usr/bin/env python3
"""Build/run the real native world fragment chain on both Mac architectures.

Invoke through tools/run_test.py, preferably with a disk scratch root. Freeze
the source before rsync, authenticate that snapshot remotely, and stream the
original map into the remote test owner. Preserve only bounded evidence.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tarfile

ROOT=Path(__file__).resolve().parents[2]
SOURCE_ROOTS=['CMakeLists.txt','src','include','cmake','tools','shaders','data','external','third_party','tests']


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mac',default='codex-mac')
    parser.add_argument('--mac-root',default='/Users/eric/Developer/Halo2-macos-port')
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--mac-output',default='build/test-results/native-world-draw')
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=True)
    snapshot=scratch/'source';snapshot.mkdir()
    listing=subprocess.check_output(['git','ls-files','-z','--cached','--others','--exclude-standard',*SOURCE_ROOTS],cwd=ROOT)
    paths=sorted({name.decode() for name in listing.split(b'\0') if name and (ROOT/name.decode()).is_file()})
    hashes={};total=0
    for relative in paths:
        data=(ROOT/relative).read_bytes();target=snapshot/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
        target.chmod((ROOT/relative).stat().st_mode&0o777)
        hashes[relative]=hashlib.sha256(data).hexdigest();total+=len(data)
    serialized=json.dumps(hashes,sort_keys=True,separators=(',',':')).encode()
    compressed=lzma.compress(serialized,preset=6);assert lzma.decompress(compressed)==serialized and len(compressed)<1024*1024
    (args.output/'source-hashes.json.xz').write_bytes(compressed)
    (snapshot/'source-hashes.json.xz').write_bytes(compressed)
    # No deletion: this is the existing source mirror, not a launcher-selected
    # runtime. Private SDKs, builds, map copies and readbacks live elsewhere.
    command=['rsync','-az','--from0','--files-from=-','-e','ssh -o BatchMode=yes -o ConnectTimeout=15',str(snapshot)+'/',args.mac+':'+args.mac_root+'/']
    subprocess.run(command,input=b'\0'.join(p.encode() for p in paths)+b'\0source-hashes.json.xz\0',check=True)
    shutil.rmtree(snapshot) # The authenticated source mirror and hash receipt now own this snapshot.
    references={}
    for name in ['maps/00a_introduction.map','default.xbe']:
        original=ROOT/'assets/disc'/name
        with original.open('rb') as file:actual=hashlib.file_digest(file,'sha256').hexdigest()
        references[name]={'bytes':original.stat().st_size,'sha256':actual}
    receipt={'source_files':len(hashes),'source_bytes':total,'source_manifest_sha256':hashlib.sha256(serialized).hexdigest(),
      'compressed_manifest_sha256':hashlib.sha256(compressed).hexdigest(),'compressed_manifest_bytes':len(compressed),
      'references':references}
    (args.output/'input-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    remote='receipt='+repr(receipt)+'\n'+'''from pathlib import Path
import hashlib,json,lzma,os,subprocess,sys,tarfile
output=Path(sys.argv[1]);output.mkdir(parents=True,exist_ok=True)
compressed=Path('source-hashes.json.xz').read_bytes();assert hashlib.sha256(compressed).hexdigest()==receipt['compressed_manifest_sha256']
serialized=lzma.decompress(compressed);assert hashlib.sha256(serialized).hexdigest()==receipt['source_manifest_sha256']
sources=json.loads(serialized);assert len(sources)==receipt['source_files']
different=[p for p,h in sources.items() if not Path(p).is_file() or hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h]
assert not different,'Source admission differs: '+str(different[:20])
(output/'input-receipt.json').write_text(json.dumps(receipt,indent=2)+'\\n')
(output/'source-hashes.json.xz').write_bytes(compressed)
Path('source-hashes.json.xz').unlink()
reference=Path(os.environ['HALO2_TEST_SCRATCH'])/'reference';reference.mkdir()
with tarfile.open(fileobj=sys.stdin.buffer,mode='r|gz') as archive:archive.extractall(reference,filter='data')
for name,item in receipt['references'].items():
    path=reference/name;assert path.is_relative_to(reference)
    assert path.stat().st_size==item['bytes']
    with path.open('rb') as file:assert hashlib.file_digest(file,'sha256').hexdigest()==item['sha256']
path=reference/'maps/00a_introduction.map'
command=[sys.executable,'-B','tools/macos/test_port.py','--engine-configure-check','--engine-target','hologram_draw_gpu_tests','--engine-test-filter','^(macos_native_world_draw_arm64|macos_native_world_draw_x86_64|vulkan_portability)$','--cmake-arg=-DHALO2_MACOS_NATIVE_WORLD_SOURCE='+str(path),'--retain-report','macos-native-world-arm64.json','--retain-report','macos-native-world-x86_64.json','--retain-report','macos-dependencies/openssl/manifest.json','--retain-report','macos-dependencies/opus/manifest.json','--retain-report','macos-dependencies/libdeflate/manifest.json','--retain-report','macos-dependencies/sdl/manifest.json','--output',str(output)]
raise SystemExit(subprocess.run(command).returncode)
'''
    remote_command='cd '+shlex.quote(args.mac_root)+' && PATH=/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin MVK_CONFIG_FAST_MATH_ENABLED=1 caffeinate -i '+shlex.join(['python3','-B','tools/run_test.py','--scratch-root','.local/test-scratch','--','python3','-B','-c',remote,args.mac_output])
    process=subprocess.Popen(['ssh','-o','BatchMode=yes','-o','ConnectTimeout=15',args.mac,remote_command],stdin=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdin,mode='w|gz') as archive:
            for name in references:archive.add(ROOT/'assets/disc'/name,arcname=name,recursive=False)
    finally:process.stdin.close()
    status=process.wait()
    (args.output/'execution.json').write_text(json.dumps({'command':remote_command,'exit_code':status,'scratch_owner':str(scratch)},indent=2)+'\n')
    return status

if __name__=='__main__':raise SystemExit(main())
