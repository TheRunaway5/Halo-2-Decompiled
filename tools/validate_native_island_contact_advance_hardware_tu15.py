#!/usr/bin/env python3
"""Native3205B0 with actual BEFORE-only MCC input and genuine owner execution.

Requires an explicitly retained actual3205B0 input. No captured after image,
physics result or supplied motion/collision callback outcome is consumed.
Both original and native+real-owner executions start from those same inputs.
Private returned callee/argument storage is excluded by an explicit void API;
all persistent bytes, ordered native accesses and complete FP are compared.
"""
import argparse,bisect,hashlib,json,os,struct,subprocess,sys,tarfile
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from replay_live_phantom_hardware_tu15 import reference_text_gaps
from validate_native_island_contact_advance_tu15 import metadata

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',required=True,type=Path);p.add_argument('--archive-sha256');p.add_argument('--loop-variants',action='store_true');p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;source=a.capture.resolve();archive_sha=None
 if source.is_file():
  assert a.archive_sha256 and sha(source)==a.archive_sha256,'The required archive must match its explicit retention receipt';archive_sha=a.archive_sha256;unpack=scratch/'before-inputs';unpack.mkdir()
  with tarfile.open(source,'r:*')as archive:
   for member in archive.getmembers():
    target=(unpack/member.name).resolve();assert target==unpack or unpack in target.parents,'Archive member escapes owned input directory';assert member.isfile()or member.isdir(),'Only regular before inputs and directories are admitted'
   archive.extractall(unpack,filter='data')
  candidates=[]
  for path in unpack.rglob('capture.json'):
   value=json.loads(path.read_text())
   if value.get('entry')==0x3205b0:candidates.append(path)
  assert len(candidates)==1,'Need exactly one actual3205B0 before capture metadata';capture=candidates[0].parent
 else:capture=source
 d=json.loads((capture/'capture.json').read_text());assert d['entry']==0x3205b0 and d['reference']=='English TU1.5 only';assert d['before']['x87_values']==[],'Concrete full-owner proof currently requires the actual empty incoming x87 stack';regions=sorted(d['regions'],key=lambda r:r['address']);addresses=[r['address']for r in regions]
 def read(address,size):
  region=regions[bisect.bisect_right(addresses,address)-1];offset=address-region['address'];assert offset>=0 and offset+size<=region['size'];path=capture/region['before_file']
  with path.open('rb')as stream:stream.seek(offset);raw=stream.read(size)
  assert len(raw)==size;return raw
 def word(address):return struct.unpack('<I',read(address,4))[0]
 sp=d['before']['gp'][4];private_first=sp&~0xffff;args=[word(sp+4+n*4)for n in range(4)];island=d['before']['gp'][1];config,entry,output,time=args;header=word(output)
 assert not any(private_first<=value<sp+20 for value in(island,config,entry,output,header)),'A semantic caller packet aliases returned callee/argument storage';assert private_first<=sp-0x4000 and any(r['address']<=private_first and sp+0x100<=r['address']+r['size']for r in regions),'Actual native stack owner range unavailable'
 image=Xbe(NEW_DEFAULT);text,gaps,authentication=reference_text_gaps(image,capture,regions,scratch);assembly=[];sections=[]
 for n,r in enumerate(sorted([*regions,*gaps],key=lambda r:r['address'])):
  path=r.get('reference_file',capture/r.get('before_file',''));assembly.append(f'.section .bank{n},"awx"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{r["address"]:x}:{{*(.bank{n})}}');assert not r['address']<=0x70000000<r['address']+r['size']
 fs=d['before']['fs_base'];assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 mov $123,%eax
 mov $1,%ebx
 mov $fs_descriptor,%ecx
 mov $16,%edx
 int $0x80
 test %eax,%eax
 js setup_failure
 mov $7,%eax
 mov %ax,%fs
 fninit
 mov ${sp},%esp
 int3
setup_failure:
 mov $1,%eax
 mov $125,%ebx
 int $0x80
.balign 4
fs_descriptor:.long 0,{fs},0xfffff,0x51
''');sections.append('.bridge 0x70000000:{*(.bridge)}');(scratch/'reference.s').write_text('\n'.join(assembly));(scratch/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n');subprocess.run(['as','--32',str(scratch/'reference.s'),'-o',str(scratch/'reference.o')],check=True,capture_output=True);elf=scratch/'reference';subprocess.run(['ld','-melf_i386','-T',str(scratch/'link.ld'),str(scratch/'reference.o'),'-o',str(elf)],check=True,capture_output=True)
 case=dict(name='actual_Warthog_first_contact_time',island=island,configuration=config,agent_entry=entry,output=output,time_bits=time);cases=[case]
 if a.loop_variants:
  first_motion=word(word(word(entry)+0x10)+0x3c);second_motion=word(word(word(entry+4)+0x10)+0x3c);source_step=word(config+0x10)
  def add(name,first,second,contact_time=time,extra=()):
   cases.append(dict(case,name=name,time_bits=contact_time,patches=[[first_motion+0x7c,first],[second_motion+0x7c,second],*extra]))
  add('declared_paired_equal_half',0x3f000000,0x3f000000)
  add('declared_paired_equal_actual_step',source_step,source_step)
  # -1000 is the actual source's own final restore classification threshold.
  # These are declared incoming distance/time values, never callback outcomes.
  add('declared_single_actual_caches_source_sentinel',word(first_motion+0x7c),word(second_motion+0x7c),0xc47a0000)
  add('declared_single_swapped_caches_source_sentinel',word(second_motion+0x7c),word(first_motion+0x7c),0xc47a0000)
  add('declared_single_actual_caches_negative_one',word(first_motion+0x7c),word(second_motion+0x7c),0xbf800000)
  add('declared_paired_half_source_sentinel',0x3f000000,0x3f000000,0xc47a0000)
  half_step=struct.unpack('<I',struct.pack('<f',struct.unpack('<f',struct.pack('<I',source_step))[0]*.5))[0]
  add('declared_paired_half_configuration_step',0x3f000000,0x3f000000,extra=([config+0x10,half_step],))
  # The retained dynamic325810 owner reads its real source linear vector at
  # +C0 and subtracts old_time*step then adds new_time*step. The actual initial
  # capture has a zero vector, so cached-time changes alone cannot separate
  # these shapes. These are declared velocity inputs, never contact outputs.
  first_owner=word(word(first_motion)+0x74);assert first_owner==0x325810
  for label,velocity in(('eight',0xc1000000),('thirty',0xc1f00000),('sixty',0xc2700000)):
   add('declared_single_source_velocity_'+label,0x3f800000,0,extra=([first_motion+0xc8,velocity],))
 before=d['before'];meta=metadata(image,set());meta.update(elf=str(elf),entry_sp=sp,private_stack_first=private_first,incoming_x87_depth=0,before_gp=before['gp'],before_eflags=before['eflags'],before_xmm=before['xmm'],before_fp=[before['x87_control_word'],before['x87_status'],before['mxcsr']],text_first=text.va,text_end=text.va+text.vsize,regions=[dict(address=r['address'],bytes=r['size'],file=str(capture/r['before_file']))for r in regions],cases=cases,require_single_motion_iterations=a.loop_variants,require_paired_motion_iterations=a.loop_variants,compared_persistent_bytes=sum(r['size']-max(0,min(r['address']+r['size'],sp+20)-max(r['address'],private_first))for r in regions));(scratch/'input.json').write_text(json.dumps(meta));hashes=[dict(address=r['address'],bytes=r['size'],file=r['before_file'],sha256=sha(capture/r['before_file']))for r in regions];(a.output/'before-bank-hashes.json').write_text(json.dumps(hashes,separators=(',',':'))+'\n')
 project=scratch/'project';project.mkdir();kernel=ROOT/'src/physics/native_tu15_island_contact_advance.cpp';(project/kernel.name).write_bytes(kernel.read_bytes());test=ROOT/'tests/native_tu15_island_contact_advance_hardware_tests.cpp';cmake=f'''cmake_minimum_required(VERSION 3.20)
project(native_contact_advance_hardware LANGUAGES CXX)
include(CTest)
add_executable(native_contact_advance_hardware "{test}" "{project/kernel.name}")
target_include_directories(native_contact_advance_hardware PRIVATE "{ROOT}/include")
target_compile_features(native_contact_advance_hardware PRIVATE cxx_std_20)
target_compile_options(native_contact_advance_hardware PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME native_contact_advance_hardware COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_contact_advance_hardware> "{scratch/'input.json'}" "${{CMAKE_BINARY_DIR}}/composition.json")
''';(project/'CMakeLists.txt').write_text(cmake);results=[]
 for compiler in('g++','clang++'):
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_contact_advance_hardware','--filter','^native_contact_advance_hardware$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1','--retain-report','composition.json'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
 files=[kernel,ROOT/'include/halo2/native_tu15_island_contact_advance.hpp',test,Path(__file__)];pins={str(p.relative_to(ROOT)):sha(project/p.name if p==kernel else p)for p in files};temporary=[p for p in scratch.rglob('*')if p.is_file()];report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,required_backend='actual-before-only-3205B0-native-kernel-and-genuine-original-owner-return-v1',original_sha256=NEW_PIN,capture=str(source),archive_sha256=archive_sha,capture_metadata_sha256=sha(capture/'capture.json'),before_only_inputs=True,captured_after_consumed=False,source_hashes=pins,current_sources_match_snapshot=all(sha(ROOT/p)==v for p,v in pins.items()),code_authentication=authentication,private_returned_stack=[private_first,sp+20],compared_persistent_bytes=meta['compared_persistent_bytes'],actual_argument_inputs=case,declared_input_cases=cases,loop_variants_requested=a.loop_variants,explicit_dependency_inputs='actual independent owner-entry GP/flags/private caller bytes and initial actual SIMD; native current persistent bank data; genuine current owner returns only',owned_scratch=str(scratch),temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in temporary),retained_large_artifacts=[],excluded=['production construction of original private caller ABI state','native conversion of actual remaining motion/agent services','full world/all-branch physics acceptance','unmasked native trap/unwind owners']);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
