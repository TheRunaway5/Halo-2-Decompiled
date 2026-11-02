#!/usr/bin/env python3
"""Native E18D0 composed with native vector/basis/curve/finite-acos kernels.

Declared source object/direction/position DATA and40B arithmetic scratch;
the original CRT fmod child executes unchanged only inside the reference.
No native service invokes original code or consumes supplied outcomes.
"""
import argparse,copy,inspect,os,random,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_camera_basis_tu15 as base
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
BASE,OBJECTS,HEADERS,UNIT,TAGS,DEFINITION,POSITION,FORWARD,PERPENDICULAR,DEFAULT=map(lambda p:0x1000000+p,(0,0x100,0x200,0x1000,0x2000,0x3000,0x4000,0x4100,0x4200,0x4300))
DATA=physical.SP-0x28
CPP='src/physics/native_tu15_camera_adjustment.cpp';HPP='include/halo2/native_tu15_camera_adjustment.hpp'
UNITS=(CPP,'src/physics/native_tu15_vector_checks.cpp','src/physics/native_tu15_camera_horizontal_basis.cpp','src/physics/native_tu15_look_vector_angle.cpp','src/core/native_tu15_function_curves.cpp')
OWNERS=(0xe18d0,0xb1720,0xaa200,0xaa270,0xe05d0,0x17f8a0,0x32ccd8,0x33210c,0x32ccf5,0x332099,0x33212f,0x332122)
BACKEND='unchanged-tu15-native-camera-adjustment-v1';sha=base.sha;bits=base.bits;save=base.save
def metadata(image):
 source=inspect.getsource(base.metadata).replace('assert counts==[18,62]','assert counts[0]==142');environment=dict(base.__dict__,OWNERS=OWNERS);exec(source,environment);meta=environment['metadata'](image);code=_Code(image,'.text')
 curve_sites=__import__('generate_physics_live_support_tu15').complete_body(code,0x17f8a0).code
 for site in meta['sites']:
  pc=site['instruction'];ins=code.at(pc)
  if pc in curve_sites and ins.mnemonic=='call':site.update(opaque_call=True,instruction_size=ins.size)
 # Jump-table instruction fetch is architecture control, not a DATA operand
 # of the native curve algorithm. Its numeric table reads remain explicit.
 for site in meta['sites']:
  if code.at(site['instruction']).mnemonic=='jmp':site['operands']=[]
 return meta
def cases(image):
 rows=[]
 declared_math_data={section.va:image.section_bytes(section)+bytes(max(0,section.vsize-len(image.section_bytes(section))))for section in image.sections if section.name!='.text'}
 def make(name,forward=(.8,0.,-.6),perp=(0.,1.,0.),upper=1.2,lower=.2,enabled=1,cw=0x27f,mx=0x1f80):
  raw=bytearray(random.Random(len(rows)).randbytes(0x5000))
  def put(at,value,width=4):raw[at-BASE:at-BASE+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
  put(OBJECTS+0x44,HEADERS);put(HEADERS+8,UNIT);put(UNIT,0);put(TAGS+8,DEFINITION);put(UNIT+0x116,0x400,2);put(UNIT+0x3c8,enabled,1)
  for at,values in((FORWARD,forward),(PERPENDICULAR,perp),(POSITION,(2.,3.,4.)),(UNIT+0x428,(1.,2.,3.)),(UNIT+0x3cc,(.1,.2,.3)),(DEFAULT,(.25,-.75))):
   for n,value in enumerate(values):put(at+n*4,bits(value))
  for off,value in((0x224,lower),(0x228,upper),(0x22c,.5),(0x230,.25),(0x234,.75),(0x238,.125)):put(DEFINITION+off,bits(value))
  banks=[dict(address=BASE,before=raw.hex()),dict(address=DATA,before=random.Random(len(rows)+42).randbytes(40).hex())]
  for at,value in((0x4ecbd0,OBJECTS),(0x4f0414,TAGS),(0x474cd8,DEFAULT),(0x48d4f4,0)):banks.append(dict(address=at,before=struct.pack('<I',value).hex()))
  for at,width in((0x41c4cc,4),(0x46a0c4,4),(0x46a0d0,4),(0x46a0dc,4),(0x46a0e8,4),(0x46a0ec,4),(0x46a178,12),(0x46a1d0,8),(0x46a43c,4),(0x4796a7,1026)):
   banks.append(dict(address=at,before=image.read(at,width).hex()))
  # The genuine CRT wrapper also reads its authenticated dispatch/default
  # environment DATA. Preserve those complete XBE sections, with declared
  # loader zero fill, then overlay only the authored scalar input controls.
  merged={at:bytearray(raw)for at,raw in declared_math_data.items()}
  for bank in banks:
   at=bank['address'];raw=bytes.fromhex(bank['before']);owner=next((first for first,value in merged.items()if first<=at and at+len(raw)<=first+len(value)),None)
   if owner is None:merged[at]=bytearray(raw)
   else:merged[owner][at-owner:at-owner+len(raw)]=raw
  rows.append(dict(name=name,entry=0xe18d0,receiver=0,entry_registers={'eax':0},arguments=[POSITION,FORWARD,PERPENDICULAR],object=0,position=POSITION,forward=FORWARD,perpendicular=PERPENDICULAR,scratch=DATA,banks=[dict(address=at,before=raw.hex())for at,raw in sorted(merged.items())],before_fp=[cw,0x4520,mx],stop_pc=physical.STOP,boundary=None))
 for direction in((1.,0.,0.),(.8,0.,-.6),(0.,0.,-1.),(0.,0.,1.),(.5,.5,-.70710677),(0.,0.,0.)):
  for upper,lower in((1.2,.2),(.2,.2),(-.2,.2),(1.2,0.)):
   for enabled in(0,1):make(f'camera_{direction}_{upper}_{lower}_{enabled}',forward=direction,upper=upper,lower=lower,enabled=enabled)
 for rounding in range(4):make(f'rounding_{rounding}',mx=0x1f80|(rounding<<13))
 for mode in(0,0x40,0x8000,0x8040):make(f'denormal_modes_{mode}',mx=0x1f80|mode)
 make('fault_seed_skip',upper=.2,lower=.2);make('fault_seed_positive');return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False,native_original_services=False,preserved_x87_depth_physically_tested=0,private_scratch_provenance='Declared40B arithmetic DATA at original private allocation only; no GP-save/control field overlaps')
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);original=physical.observer_source
  def observer():
   text=original().replace('timestamps=J::array();','timestamps=J::array(),executed=J::array();').replace('const auto site=sites.find(pc);','executed.push_back(pc);const auto site=sites.find(pc);').replace('{"boundary_gp",gp(regs)}','{"executed_sites",executed},{"boundary_gp",gp(regs)}')
   needle='    request(PTRACE_SINGLESTEP,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original pose instruction fault");';assert text.count(needle)==1
   text=text.replace(needle,needle+'''
    if(site->second.value("opaque_call",false)){
      const auto target=pc+site->second.at("instruction_size").get<std::uint32_t>();
      request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(offsetof(user,u_debugreg)),reinterpret_cast<void*>(std::uintptr_t(target)));
      request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(offsetof(user,u_debugreg)+7*sizeof(unsigned long)),reinterpret_cast<void*>(1));
      request(PTRACE_CONT,child.pid);wait();user_regs_struct done{};request(PTRACE_GETREGS,child.pid,nullptr,&done);check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,(std::string("Original opaque CRT child failed at=")+std::to_string(done.rip)+" signal="+std::to_string(WSTOPSIG(status))).c_str());check(done.rip==target,"Wrong original CRT return");
      request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(offsetof(user,u_debugreg)+7*sizeof(unsigned long)),nullptr);
    }
''');return text
  physical.observer_source=observer;physical.boundary=lambda c,o:None;rows=physical.physical(image,meta,cases(image),scratch);assert all(c['boundary_gp'][4]==physical.SP+16 for c in rows)
  # Ordered full-parent/child numeric DATA reads and stores are compared.
  # CRT code-private traffic is separately covered by the native curve proof.
  fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for unit in UNITS:shutil.copyfile(ROOT/unit,project/Path(unit).name)
  driver=(ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_wrapped_contact_process.hpp"','#include "halo2/native_tu15_camera_adjustment.hpp"').replace('unchanged-tu15-native-wrapped-contact-process-v1',BACKEND).replace('J::array({46,40,40})','J::parse("'+__import__('json').dumps(meta['body_sites'])+'")');begin=driver.index('NativeTu15CompoundProcessServices services;');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+'bool boundary_hit{};\n'+driver[end:];driver=driver.replace('NativeTu15WrappedContactProcessStats stats;','NativeTu15CameraAdjustmentStats stats;')
  needle='native_tu15_process_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("proxy")},c.at("geometry")},fp,services,observer,&stats);';assert driver.count(needle)==1;driver=driver.replace(needle,'native_tu15_adjust_unit_camera_position(m,c.at("object"),c.at("position"),c.at("forward"),c.at("perpendicular"),{c.at("scratch")},fp,0,observer,&stats);');(project/'driver.cpp').write_text(driver)
  source=(ROOT/CPP).read_text();mutants={'wrong_axis_coefficient':('definition+0x22cu','definition+0x230u'),'missing_final_check':('if(stats)++stats->orthogonality_calls;(void)native_tu15_vectors_orthonormal(memory,forward,perpendicular,fp,depth);\n        if(stats)++stats->completed;','if(stats)++stats->completed;'),'wrong_vertical_sign':('sub(pz,nz)','sub(nz,pz)')}
  for name,(before,after)in mutants.items():assert source.count(before)==1;(project/(name+'.cpp')).write_text(source.replace(before,after))
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differ"in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint(r.stderr.strip())\n');cmake='cmake_minimum_required(VERSION 3.20)\nproject(adjustment LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  objects={}
  for file in[*[Path(u).name for u in UNITS],*[name+'.cpp'for name in mutants]]:
   target='obj_'+str(len(objects));objects[file]=target;cmake+=f'add_library({target} OBJECT {file})\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for name in('native',*mutants):
   selected=[objects[name+'.cpp'if name!='native'and u==CPP else Path(u).name]for u in UNITS];cmake+=f'add_executable(adjustment_{name} $<TARGET_OBJECTS:driver> '+' '.join('$<TARGET_OBJECTS:'+n+'>'for n in selected)+')\n';command=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME adjustment_{name} COMMAND {command}$<TARGET_FILE:adjustment_{name}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','adjustment_'+n]for n in('native',*mutants)),[]),'--filter','^adjustment_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native camera adjustment differs'
  executed=set(pc for c in rows for pc in c['executed_sites']);parent_sites=set(__import__('generate_physics_live_support_tu15').complete_body(_Code(image,'.text'),0xe18d0).code)
  files=[*[ROOT/u for u in UNITS],ROOT/HPP,Path(__file__)];report.update(passed=True,original_sha256=NEW_PIN,body_sites=meta['body_sites'],parent_executed_sites=len(parent_sites&executed),parent_unexecuted=[hex(pc)for pc in sorted(parent_sites-executed)],profiles=len(rows),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),results=results,negative_controls=list(mutants),source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},scope=__doc__)
 except BaseException as error:
  report['error']=str(error)or repr(error)
  if getattr(error,'stderr',None):report['diagnostic']=error.stderr.decode(errors='replace')if isinstance(error.stderr,bytes)else error.stderr
  raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
