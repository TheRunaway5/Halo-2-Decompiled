#!/usr/bin/env python3
"""Retain and verify required typed native simplex BEFORE reference inputs."""
import argparse,hashlib,json,os,shutil,subprocess,tarfile
from pathlib import Path

def sha(path):
 with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);capture=a.capture.resolve(strict=True)
 assert capture!=owner and capture.is_relative_to(owner)
 manifest=capture/'capture-native-before.json';d=json.loads(manifest.read_text())
 assert d['schema']=='native-tu15-convex-simplex-typed-before-v1'and d['input']['owner']==0x30e930
 assert not d['guest_abi_fabricated']and not d['physics_result_supplied']and not d['source_memory_mutated']and not d['expected_after_consumed']
 expected={manifest.name:dict(bytes=manifest.stat().st_size,sha256=sha(manifest))}
 for r in d['regions']:
  path=capture/r['before_file'];assert path.parent==capture and not path.is_symlink()and path.stat().st_size==r['size']
  expected[path.name]=dict(bytes=r['size'],sha256=sha(path))
 assert{path.name for path in capture.iterdir()}==set(expected)
 a.output.mkdir(parents=True,exist_ok=False);archive=a.output/'actual-native-before.tar.zst'
 subprocess.run(['tar','--zstd','-cf',str(archive.resolve()),'-C',str(capture),'.'],check=True)
 process=subprocess.Popen(['zstd','-dc',str(archive)],stdout=subprocess.PIPE);seen={}
 try:
  with tarfile.open(fileobj=process.stdout,mode='r|')as stream:
   for member in stream:
    if member.isdir():continue
    assert member.isfile();name=member.name.removeprefix('./');assert name in expected and name not in seen
    seen[name]=dict(bytes=member.size,sha256=hashlib.file_digest(stream.extractfile(member),'sha256').hexdigest())
  assert process.wait()==0
 finally:
  process.stdout.close()
  if process.poll()is None:process.kill();process.wait()
 assert seen==expected;shutil.copyfile(manifest,a.output/manifest.name)
 receipt=dict(reason='Required genuine native30E930 projection input for unchanged-original/native continuation qualification',archive=str(archive.resolve()),archive_bytes=archive.stat().st_size,archive_sha256=sha(archive),capture_manifest_sha256=sha(manifest),bank_bytes=d['bank_bytes'],regions=len(d['regions']),members=len(expected),member_roundtrip_verified=True,native_input=d['input'],original_gp_frame_captured=False,physics_results_supplied=False,expected_after_consumed=False,runtime_or_build_binary_retained=False,raw_capture_in_active_owner=str(capture),tool_sha256=sha(__file__))
 (a.output/'retention.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))

if __name__=='__main__':main()
