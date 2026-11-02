#!/usr/bin/env python3
"""Typed actual18A300 update with sole explicit offline genuineCD4A0 camera.

Fresh ordinary original callers marshal only index/dyaw/dpitch/FP from the
actual BEFORE. Native334/65 parents and67/76/marker/angle helpers execute
semantically; camera results are computed by unchanged code, never supplied.
This is not production-native camera closure or captured GP ABI parity.
"""
import argparse,hashlib,inspect,json,os,shutil,struct,subprocess,sys,tarfile
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
UNITS=('src/physics/native_tu15_look_controller.cpp','src/physics/native_tu15_look_context.cpp','src/physics/native_tu15_look_vector_angle.cpp','src/campaign/ai/native_campaign_object_world_point_tu15.cpp','src/campaign/ai/native_campaign_object_marker_request_tu15.cpp','src/campaign/ai/native_campaign_object_markers_tu15.cpp','src/campaign/ai/native_campaign_object_motion_refs_tu15.cpp')
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False,remaining_services_execute_original_cpu=True,production_native_only_closure=False,captured_original_gp_abi=False)
 try:
  assert sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN
  capture=json.loads((a.archive.parent/'capture-native-before.json').read_text());typed=capture['input'];assert typed['owner']==0x18a300 and typed['preserved_x87_depth']==0 and not capture['physics_result_supplied']
  wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=Path(member.name).name
    if name in wanted:r=wanted[name];raw=tar.extractfile(member).read();assert len(raw)==r['size'];banks[r['address']]=raw
  assert stream.wait()==0 and len(banks)==len(wanted);captured_bytes=sum(map(len,banks.values()))
  # Adjacent committed regions remain exact; no gaps or expected AFTER bytes
  # are filled. The separate4096B area is fresh caller-owned native DATA.
  merged={}
  for first,raw in sorted(banks.items()):
   if merged and last+len(merged[last])==first:merged[last]+=raw
   else:merged[first]=raw;last=first
  banks=merged;assert not any(p<=0x06000000<p+len(raw)for p,raw in banks.items());banks[0x06000000]=bytes([0xa5])*4096
  source=inspect.getsource(base.original);source=source[:source.index('        active=dict(m')]+"        return owned/'reference'\n"
  source=source.replace("case['before_fp']=fp","fp=case['before_fp_input'];case['before_fp']=fp")
  source=source.replace(' cld\n fninit',''' cld
 mov $123,%eax
 mov $1,%ebx
 mov $descriptor,%ecx
 mov $16,%edx
 int $0x80
 test %eax,%eax
 jnz setup_failed
 mov $7,%eax
 mov %ax,%fs
 movl $0xffffffff,%fs:0
 fninit''')
  source=source.replace(' int3\n jmp *entry',' mov $'+str(typed['control_index'])+',%eax\n int3\n jmp *entry')
  source=source.replace('entry: .long','setup_failed:\n mov $1,%eax\n mov $99,%ebx\n int $0x80\ndescriptor:.long 0,0x71000000,0xfffff,0x51\nentry: .long')
  anchor="        assembly.append(f'''.code32";assert source.count(anchor)==1;source=source.replace(anchor,"        assembly.append('.section .fs,\"aw\"\\n.zero 4096');sections.append('.fs 0x71000000:{*(.fs)}')\n"+anchor)
  fp=typed['floating_point'];case=dict(entry=0x18a300,receiver=0,arguments=[typed['yaw_delta_bits'],typed['pitch_delta_bits']],before_fp_input=[fp['x87_control_word'],fp['x87_status'],fp['mxcsr']])
  environment=dict(base.__dict__,metadata=lambda i,b:({},{}));exec(source,environment);executable=environment['original'](Xbe(NEW_DEFAULT),banks,[case],scratch)
  bridge=(ROOT/'tools/observe_native_contact_producer_tu15.cpp').read_text().split('struct Memory:SourcePhysicsContactMemory')[0]
  original=(ROOT/'tools/observe_native_contact_producer_tu15.cpp').read_text();begin=original.index('struct Memory:SourcePhysicsContactMemory');end=original.index('std::array<std::uint32_t,8>gp',begin);bridge+=original[begin:end]
  bridge='#include "halo2/native_tu15_look_controller.hpp"\n#include "halo2/native_tu15_look_vector_angle.hpp"\n#include "halo2/native_campaign_object_marker_request_tu15.hpp"\n'+bridge+(ROOT/'tools/observe_native_look_actual_tu15.cpp').read_text();(scratch/'driver.cpp').write_text(bridge)
  results=[]
  for compiler in('g++','clang++'):
   target=scratch/('look-'+compiler);command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),str(scratch/'driver.cpp'),*[str(ROOT/n)for n in UNITS],'-o',str(target)];r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-build.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:]
   result=a.output/(compiler+'-actual.json');r=subprocess.run([str(target),str(executable),str(a.archive.parent/'capture-native-before.json'),str(result)]);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native actual look differs'
  files=[*UNITS,'include/halo2/native_tu15_look_controller.hpp','include/halo2/native_tu15_look_context.hpp','include/halo2/native_tu15_look_vector_angle.hpp','tools/validate_native_look_actual_tu15.py','tools/observe_native_look_actual_tu15.cpp']
  report.update(passed=True,archive_sha256=a.archive_sha256,actual_input=typed,actual_before_bytes=captured_bytes,captured_bytes_excluded=0,results=results,complete_camera_position_and_defined_context_compared=True,source_private_control_ledger_claimed=False,source_hashes={n:sha(ROOT/n)for n in files},scope=__doc__)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
