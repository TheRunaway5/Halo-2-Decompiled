#!/usr/bin/env python3
"""Exact captured18A2A0 typed input plus native FP-field-only Host binding.

The original ordinary formal caller is independently assembled from the actual
index/forward DATA. Complete captured banks, current reads, attributed stores,
RET0 and FP are compared; no GP/SIMD/outcome data is captured or installed.
"""
import argparse,bisect,hashlib,inspect,json,os,subprocess,tarfile
from pathlib import Path
import validate_native_array_growth_tu15 as base
import validate_native_look_reorientation_tu15 as parent
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False,captured_original_gp_abi=False)
 try:
  assert sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN;capture=json.loads((a.archive.parent/'capture-native-before.json').read_text());typed=capture['input'];assert typed['owner']==0x18a2a0 and typed['preserved_x87_depth']==0 and not capture['physics_result_supplied'] and not capture['guest_abi_fabricated']
  wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=Path(member.name).name
    if name in wanted:r=wanted[name];raw=tar.extractfile(member).read();assert len(raw)==r['size'];banks[r['address']]=raw
  assert stream.wait()==0 and len(banks)==len(wanted);captured_bytes=sum(map(len,banks.values()));addresses=sorted(banks)
  def read(p,n):
   i=bisect.bisect_right(addresses,p)-1;assert i>=0;first=addresses[i];off=p-first;assert 0<=off and off+n<=len(banks[first]);return banks[first][off:off+n]
  image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');body=complete_body(code,0x18a2a0);listing=[];authenticated_captured_code_bytes=0
  for pc in sorted(body.code):
   ins=code.at(pc)
   for address,expected in zip(range(pc,pc+ins.size),ins.bytes):
    i=bisect.bisect_right(addresses,address)-1
    if i>=0:
     first=addresses[i];offset=address-first
     if offset<len(banks[first]):assert banks[first][offset]==expected,(hex(address),'Captured code differs from authenticated TU1.5');authenticated_captured_code_bytes+=1
   listing.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n')
  listing=''.join(listing);(a.output/'0018a2a0.txt').write_text(listing);meta=parent.metadata(image);save(scratch/'metadata.json',meta);merged={}
  for first,raw in sorted(banks.items()):
   if merged and last+len(merged[last])==first:merged[last]+=raw
   else:merged[first]=raw;last=first
  source=inspect.getsource(base.original);source=source[:source.index('        active=dict(m')]+"        return owned/'reference'\n";source=source.replace("case['before_fp']=fp","fp=case['before_fp_input'];case['before_fp']=fp")
  fp=typed['floating_point'];case=dict(entry=0x18a2a0,receiver=typed['control_index'],arguments=[],before_fp_input=[fp['x87_control_word'],fp['x87_status'],fp['mxcsr']]);environment=dict(base.__dict__,metadata=lambda i,b:({},{}));exec(source,environment);executable=environment['original'](image,merged,[case],scratch)
  original=(ROOT/'tools/observe_native_contact_producer_tu15.cpp').read_text();bridge=original[:original.index('struct Memory:SourcePhysicsContactMemory')];begin=original.index('struct Memory:SourcePhysicsContactMemory');end=original.index('std::array<std::uint32_t,8>gp',begin);bridge+=original[begin:end];end=original.index('struct Bridge',end);bridge+=original[original.index('std::array<std::uint32_t,8>gp',begin):end]
  bridge='#include "halo2/native_tu15_look_reorientation_binding.hpp"\n'+bridge+(ROOT/'tools/observe_native_look_reorientation_actual_tu15.cpp').read_text();(scratch/'driver.cpp').write_text(bridge);results=[]
  for compiler in('g++','clang++'):
   target=scratch/('reorientation-'+compiler);command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),str(scratch/'driver.cpp'),str(ROOT/'src/physics/native_tu15_look_reorientation.cpp'),'-o',str(target)];r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-build.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:]
   result=a.output/(compiler+'-actual.json');r=subprocess.run([str(target),str(executable),str(a.archive.parent/'capture-native-before.json'),str(scratch/'metadata.json'),str(result)]);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Actual native reorientation differs'
  files=('src/physics/native_tu15_look_reorientation.cpp','include/halo2/native_tu15_look_reorientation.hpp','include/halo2/native_tu15_look_reorientation_binding.hpp','include/halo2/native_tu15_look_initialization_binding.hpp','src/physics/native_tu15_joint_constraint_numeric_internal.hpp','tools/observe_native_look_reorientation_actual_tu15.cpp','tools/validate_native_look_reorientation_actual_tu15.py');report.update(passed=True,archive_sha256=a.archive_sha256,capture_metadata_sha256=sha(a.archive.parent/'capture-native-before.json'),actual_before_bytes=captured_bytes,captured_bytes_excluded=0,authenticated_captured_code_bytes=authenticated_captured_code_bytes,uncaptured_code_from_authenticated_xbe=True,results=results,profiles_per_compiler=4,actual_call_profile=0,other_profiles='Same genuine DATA, independent declared FP rounding controls',body_sites=len(body.code),listing_sha256=hashlib.sha256(listing.encode()).hexdigest(),production_binding=True,source_hashes={n:sha(ROOT/n)for n in files},scope=__doc__)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
