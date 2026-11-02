#!/usr/bin/env python3
"""Retain a required typed BEFORE reference, verifying every archive member."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    capture=args.capture.resolve(strict=True)
    assert capture.is_relative_to(owner) and capture!=owner
    manifest=capture/'capture-native-before.json';d=json.loads(manifest.read_text())
    assert d['schema']=='native-tu15-contact-agent-factory-before-v1'
    assert not d['expected_after_consumed'] and not d['physics_result_supplied']
    expected={manifest.name:dict(bytes=manifest.stat().st_size,sha256=sha(manifest))}
    for r in d['regions']:
        path=capture/r['before_file'];assert path.parent==capture and not path.is_symlink()
        assert path.stat().st_size==r['size']
        expected[path.name]=dict(bytes=r['size'],sha256=sha(path))
    assert {p.name for p in capture.iterdir()}==set(expected)
    args.output.mkdir(parents=True,exist_ok=False)
    archive=args.output/'actual-native-before.tar.zst'
    subprocess.run(['tar','--zstd','-cf',str(archive.resolve()),'-C',str(capture),'.'],check=True)
    seen={}
    process=subprocess.Popen(['zstd','-dc',str(archive)],stdout=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdout,mode='r|') as stream:
            for member in stream:
                if member.isdir():continue
                assert member.isfile()
                name=member.name.removeprefix('./');assert name in expected and name not in seen
                data=stream.extractfile(member)
                seen[name]=dict(bytes=member.size,sha256=hashlib.file_digest(data,'sha256').hexdigest())
        assert process.wait()==0
    finally:
        process.stdout.close()
        if process.poll() is None:process.kill();process.wait()
    assert seen==expected
    shutil.copyfile(manifest,args.output/manifest.name)
    receipt=dict(reason='Required actual typed native BEFORE for original-code and native factory qualification',
        archive=str(archive),archive_bytes=archive.stat().st_size,archive_sha256=sha(archive),
        capture_manifest_sha256=sha(manifest),bank_bytes=d['bank_bytes'],regions=len(d['regions']),
        member_roundtrip_verified=True,members=len(expected),native_input=d['input'],
        original_gp_frame_captured=False,expected_after_consumed=False,physics_results_supplied=False,
        raw_capture_remains_in_active_owner=str(capture),runtime_or_build_binary_retained=False,
        tool_sha256=sha(__file__))
    (args.output/'retention.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:receipt[k] for k in ['archive','archive_bytes','archive_sha256','bank_bytes','members']}))


if __name__=='__main__':main()
