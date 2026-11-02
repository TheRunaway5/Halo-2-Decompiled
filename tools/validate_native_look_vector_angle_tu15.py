#!/usr/bin/env python3
"""Native11F7E0 and finite acos: external operand order/result bits/full FP.

Normal CW27F/policy-zero and source direct-CW-restoration policy inputs are
qualified separately. Native3334F0 error/hook and NaN branches remain open.
Source-private CRT control temporaries are not claimed as native API outputs.
"""
import argparse,hashlib,inspect,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_island_agent_phase_tu15 as physical
import validate_native_look_context_tu15 as context
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
OWNERS=(0x11f7e0,0x32ccd8,0x33210c,0x32ccf5,0x332099,0x33212f,0x332122)
BASE=0x1000000;FIRST=BASE+0x100;SECOND=BASE+0x200
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def cases(image):
 rng=random.Random(1334);vectors=[((0.,0.,0.),(1.,0.,0.)),((1.,0.,0.),(0.,0.,0.)),((1.,0.,0.),(1.,0.,0.)),((1.,0.,0.),(-1.,0.,0.)),((1.,0.,0.),(0.,1.,0.)),((1.,2.,3.),(-3.,2.,1.)),((1e-22,0.,0.),(0.,1e-22,0.)),((1e-15,0.,0.),(0.,1e-15,0.))]
 vectors +=[(tuple(rng.uniform(-10,10)for _ in range(3)),tuple(rng.uniform(-10,10)for _ in range(3)))for _ in range(8)]
 rows=[]
 for vi,(first,second)in enumerate(vectors):
  for control in(0x27f,0x7f,0x37f,0xe7f):
   for mode in(0,0x40,0x8000,0x8040):
    raw=bytearray(rng.randbytes(0x400))
    for p,values in((FIRST,first),(SECOND,second)):
     for k,value in enumerate(values):struct.pack_into('<I',raw,p-BASE+k*4,bits(value))
    policy=0 if control==0x27f else 1
    banks=[dict(address=BASE,before=raw.hex()),dict(address=0x48d4f4,before=struct.pack('<I',policy).hex())]
    for p,n in((0x46a0d0,4),(0x46a11c,4),(0x46a0dc,4),(0x46a0cc,4)):banks.append(dict(address=p,before=image.read(p,n).hex()))
    # Runtime-initialized pi is authenticated from the retained original-frame
    # BEFORE, not the XBE's pre-startup BSS value at this address.
    banks.append(dict(address=0x556554,before=struct.pack('<I',0x40490fdb).hex()))
    rows.append(dict(name=f'angle_{vi}_{control:x}_{mode:x}',entry=0x11f7e0,receiver=FIRST,entry_registers={'edx':SECOND},arguments=[],first=FIRST,second=SECOND,banks=banks,before_fp=[control,0x20,0x1f80|mode],stop_pc=physical.STOP,boundary=None,policy=policy))
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_private_raw_ledger_claimed=False,production_full_look_controller_closed=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
  globals_file=ROOT/'analysis/physics/native-look-controller-inventory-20261102/actual1473-look-globals.json'
  globals_input=json.loads(globals_file.read_text());assert globals_input['before_only']and next(v for v in globals_input['values']if v['address']=='0x556554')['bits']=='0x40490fdb'
  source=inspect.getsource(context.metadata).replace('assert counts==[334,65,67,76],counts','assert counts[0]==75,counts');environment=dict(context.__dict__,OWNERS=OWNERS);exec(source,environment);meta=environment['metadata'](image);meta['boundaries']=[]
  original_observer=physical.observer_source
  physical.observer_source=lambda:original_observer().replace('{"boundary_gp",gp(regs)}','{"xmm0_result",std::uint32_t(fp.xmm_space[0])},{"boundary_gp",gp(regs)}')
  physical.boundary=lambda c,o:None
  rows=physical.physical(image,meta,cases(image),scratch)
  assert all(c['boundary_gp'][4]==physical.SP+4 for c in rows)
  fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend='unchanged-tu15-look-vector-angle-v1',body_sites=meta['body_sites'],cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');source=ROOT/'src/physics/native_tu15_look_vector_angle.cpp';shutil.copyfile(source,project/'native.cpp')
  driver=(ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_wrapped_contact_process.hpp"','#include "halo2/native_tu15_look_vector_angle.hpp"')
  driver=driver.replace('unchanged-tu15-native-wrapped-contact-process-v1','unchanged-tu15-look-vector-angle-v1').replace('J::array({46,40,40})','J::parse("'+json.dumps(meta['body_sites'])+'")')
  begin=driver.index('NativeTu15CompoundProcessServices services;');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+'bool boundary_hit{};\n'+driver[end:]
  driver=driver.replace('NativeTu15WrappedContactProcessStats stats;','NativeTu15LookVectorAngleStats stats;')
  needle='native_tu15_process_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("proxy")},c.at("geometry")},fp,services,observer,&stats);';assert driver.count(needle)==1
  driver=driver.replace(needle,'check(native_tu15_look_vector_angle(m,c.at("first"),c.at("second"),fp,0,&stats)==c.at("xmm0_result"),name+" original angle scalar differs");check(stats.calls==1&&stats.completed==1&&stats.refusals==0,name+" angle activity differs");')
  (project/'driver.cpp').write_text(driver)
  text=source.read_text();variants={}
  for name,before,after in(('wrong_half','m.word(0x46a0ccu)','m.word(0x46a0d0u)'),('wrong_obtuse_fold','if(negative_dot)result=','if(!negative_dot)result=')):
   assert text.count(before)==1;variants[name]=text.replace(before,after);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differs"in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint("Original angle rejects mutation",r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(angle LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for name in('native',*variants):
   cpp='native.cpp'if name=='native'else name+'.cpp'
   cmake+=f'add_executable(angle_{name} $<TARGET_OBJECTS:driver> {cpp})\ntarget_include_directories(angle_{name} PRIVATE include)\ntarget_compile_features(angle_{name} PRIVATE cxx_std_20)\ntarget_compile_options(angle_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   command=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" '
   cmake+=f'add_test(NAME angle_{name} COMMAND {command}$<TARGET_FILE:angle_{name}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake)
  results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','angle_'+name]for name in('native',*variants)),[]),'--filter','^angle_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native look angle differs'
  report.update(passed=True,original_sha256=NEW_PIN,profiles=len(rows),body_sites=meta['body_sites'],ordered_external_reads=sum(len(c['reads'])for c in rows),results=results,negative_controls=list(variants),runtime_pi_input_provenance=str(globals_file.relative_to(ROOT)),runtime_pi_input_provenance_sha256=sha(globals_file),source_hashes={str(p.relative_to(ROOT)):sha(p)for p in(source,ROOT/'include/halo2/native_tu15_look_vector_angle.hpp',Path(__file__))},scope=__doc__)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
