#!/usr/bin/env python3
"""Retain only required actual BC720 BEFORE inputs and verify every member."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    capture=args.capture.resolve(strict=True)
    assert capture.is_relative_to(owner) and capture!=owner
    manifest=capture/'capture-entry.json';d=json.loads(manifest.read_text())
    assert d['entry']==0xbc720 and d['reference']=='English TU1.5 only'
    assert set(d)=={'entry','return','reference','boundary','before','regions'}
    expected={manifest.name:dict(bytes=manifest.stat().st_size,sha256=sha(manifest))}
    for row in d['regions']:
        path=capture/row['before_file'];assert path.parent==capture and not path.is_symlink()
        assert path.stat().st_size==row['size']
        expected[path.name]=dict(bytes=row['size'],sha256=sha(path))
    args.output.mkdir(parents=True,exist_ok=False)
    archive=args.output/'actual-native-before.tar.zst'
    listing=owner/('region-transition-retain-'+args.output.name+'.list')
    listing.write_bytes(b''.join(name.encode()+b'\0'for name in expected))
    try:
        subprocess.run(['tar','--zstd','-cf',str(archive.resolve()),'-C',str(capture),'--null','-T',str(listing)],check=True)
    finally:listing.unlink()
    seen={};process=subprocess.Popen(['zstd','-dc',str(archive)],stdout=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdout,mode='r|') as stream:
            for member in stream:
                assert member.isfile() and member.name in expected and member.name not in seen
                seen[member.name]=dict(bytes=member.size,sha256=hashlib.file_digest(stream.extractfile(member),'sha256').hexdigest())
        assert process.wait()==0
    finally:
        process.stdout.close()
        if process.poll() is None:process.kill();process.wait()
    assert seen==expected
    shutil.copyfile(manifest,args.output/manifest.name)
    receipt=dict(reason='Required actual BC720 native-entry BEFORE for independent physical TU1.5 qualification and future regression',
        archive=str(archive),archive_bytes=archive.stat().st_size,archive_sha256=sha(archive),
        capture_manifest_sha256=sha(manifest),bank_bytes=sum(row['size']for row in d['regions']),
        regions=len(d['regions']),member_roundtrip_verified=True,members=len(expected),
        original_xbox_hardware_capture=False,captured_native_after_retained=False,
        expected_after_consumed=False,callee_results_supplied=False,
        raw_capture_remains_in_active_owner=str(capture),runtime_or_build_binary_retained=False,
        tool_sha256=sha(__file__))
    (args.output/'retention.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({key:receipt[key]for key in ('archive','archive_bytes','archive_sha256','bank_bytes','members')}))


if __name__=='__main__':main()
