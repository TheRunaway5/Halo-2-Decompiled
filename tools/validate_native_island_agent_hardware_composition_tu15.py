#!/usr/bin/env python3
"""Complete native island phases with dynamically executed original owners.

Both sides start from retained imported-MCC BEFORE banks. Native dependencies
execute unchanged TU1.5 code against each current bank, never captured outcomes.
Use run_test.py; binaries, assembler products and bank copies stay disposable.
"""
import argparse,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from replay_live_phantom_hardware_tu15 import reference_text_gaps
from validate_native_island_agent_phase_tu15 import metadata
SP,STACK,FS=0x200f000,0x2000000,0x60200000

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
 capture=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-04-gemini-column0/original/required-failure-inputs';d=json.loads((capture/'capture.json').read_text());regions=sorted(d['regions'],key=lambda r:r['address']);assert d['before']['fs_base']==FS;text,gaps,authentication=reference_text_gaps(image,capture,regions,scratch);assembly=[];sections=[]
 for n,r in enumerate(sorted([*regions,*gaps],key=lambda r:r['address'])):
  path=r.get('reference_file',capture/r.get('before_file',''));assembly.append(f'.section .bank{n},"awx"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{r["address"]:x} : {{ *(.bank{n}) }}')
 assembly.append(f'''.code32
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
 mov ${SP},%esp
 int3
 jmp setup_failure
.balign 4
fs_descriptor: .long 0,{FS},0xfffff,0x51
setup_failure:
 mov $1,%eax
 mov $125,%ebx
 int $0x80
.section .stack,"aw"
.space 65536
''');sections.extend((f'.stack 0x{STACK:x} : {{ *(.stack) }}','.bridge 0x70000000 : { *(.bridge) }'));(scratch/'reference.s').write_text('\n'.join(assembly));(scratch/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n');subprocess.run(['as','--32',str(scratch/'reference.s'),'-o',str(scratch/'reference.o')],check=True,capture_output=True);elf=scratch/'reference';subprocess.run(['ld','-melf_i386','-T',str(scratch/'link.ld'),str(scratch/'reference.o'),'-o',str(elf)],check=True,capture_output=True)
 def read(address,size):
  r=next(r for r in regions if r['address']<=address and address+size<=r['address']+r['size'])
  with(capture/r['before_file']).open('rb')as f:f.seek(address-r['address']);return f.read(size)
 def word(p):return struct.unpack('<I',read(p,4))[0]
 world=word(0x52be48);active=word(world+8);islands=[word(active+n*4)for n in range(min(2,word(world+0xc)))];cases=[]
 for index,island in enumerate(islands):
  for entry in(0x321d20,0x321a70):
   for rank in(None,1,5,0xfffffffe):
    entries=word(island+0x68);count=word(island+0x6c);patches=[]if rank is None else[[entries+16*n+0xc,rank]for n in range(count)]
    cases.append(dict(name=f'Gemini_{index}_{entry:x}_rank_{rank}',entry=entry,island=island,configuration=world+0x140,before_fp=[d['before']['x87_control_word'],d['before']['x87_status'],d['before']['mxcsr']],patches=patches))
 for index,island in enumerate(islands):
  for value in(0x3f800000,0x41200000):
   entries=word(island+0x68);count=word(island+0x6c);cases.append(dict(name=f'Gemini_{index}_actual_agents_declared_threshold_{value:x}',entry=0x321a70,island=island,configuration=world+0x140,before_fp=[d['before']['x87_control_word'],d['before']['x87_status'],d['before']['mxcsr']],patches=[[world+0x244,value],*[[entries+16*n+0xc,1]for n in range(count)]]))
 meta=metadata(image);meta.update(elf=str(elf),text_first=text.va,text_end=text.va+text.vsize,memory_bytes=sum(r['size']for r in regions),regions=[dict(address=r['address'],bytes=r['size'],file=str(capture/r['before_file']))for r in regions],cases=cases);(scratch/'input.json').write_text(json.dumps(meta));project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_island_agent_phase.cpp';(project/source.name).write_bytes(source.read_bytes());native_text=source.read_text();assert native_text.count('const auto lower=actual_rank-1u;')==1;(project/'mutated.cpp').write_text(native_text.replace('const auto lower=actual_rank-1u;','const auto lower=actual_rank+1u;'));(project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(s in r.stderr for s in ("current native packet differs","phase ordered store differs","phase FP differs","phase read order differs")),(r.returncode,r.stdout,r.stderr)\nprint("Actual post-agent-return rank mutation rejected:",r.stderr.strip())\n');test=ROOT/'tests/native_tu15_island_agent_hardware_composition_tests.cpp';cmake=f'''cmake_minimum_required(VERSION 3.20)
project(native_agent_hardware LANGUAGES CXX)
include(CTest)
add_executable(native_agent_hardware "{test}" "{project/source.name}")
target_include_directories(native_agent_hardware PRIVATE "{ROOT}/include")
target_compile_features(native_agent_hardware PRIVATE cxx_std_20)
target_compile_options(native_agent_hardware PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME native_agent_hardware COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_agent_hardware> "{scratch/'input.json'}" "${{CMAKE_BINARY_DIR}}/composition.json")
''';cmake+=f'''add_executable(native_agent_counter_mutation "{test}" "{project/'mutated.cpp'}")
target_include_directories(native_agent_counter_mutation PRIVATE "{ROOT}/include")
target_compile_features(native_agent_counter_mutation PRIVATE cxx_std_20)
target_compile_options(native_agent_counter_mutation PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME native_agent_hardware_counter_mutation COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project/'negative.py'}" $<TARGET_FILE:native_agent_counter_mutation> "{scratch/'input.json'}" "${{CMAKE_BINARY_DIR}}/negative.json")
''';(project/'CMakeLists.txt').write_text(cmake);results=[]
 for compiler in('g++','clang++'):
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_agent_hardware','--target','native_agent_counter_mutation','--filter','^native_agent_hardware','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1','--retain-report','composition.json'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
 files=[source,ROOT/'include/halo2/native_tu15_island_agent_phase.hpp',test,Path(__file__)];pins={str(f.relative_to(ROOT)):sha(project/f.name if f==source else f)for f in files};temporary=[f for f in scratch.rglob('*')if f.is_file()];report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,required_backend='complete-native-phases-with-dynamic-unchanged-original-owner-services-v1',original_sha256=NEW_PIN,capture=str(capture.relative_to(ROOT)),capture_sha256=sha(capture/'capture.json'),before_inputs_only=True,supplied_physics_results=False,required_owner_input_context='actual independently observed GP/flags and0x4100private caller stack bytes; current native packets overwrite and are asserted equal before real owner entry',negative_control='actual post-agent-return rank lower changes minus1to plus1',code_authentication=authentication,declared_cases=len(cases),source_hashes=pins,current_sources_match_snapshot=all(sha(ROOT/f)==v for f,v in pins.items()),owned_scratch=str(scratch),temporary_allocated_bytes_before_cleanup=sum(f.stat().st_blocks*512 for f in temporary),retained_large_artifacts=[],excluded=['native conversion of actual complex agent/advance owners','full live world original byte parity','all physics interactions and multiplayer acceptance']);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
