#!/usr/bin/env python3
"""Whole E3310/11FA50 native camera leaves against unchanged physical TU1.5.

Declared direction/object DATA, complete FP and source-ordered retained operands.
No original child, GP file, saved-control bytes or result injection is used.
"""
import argparse,copy,hashlib,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE,DIRECTION,DEST,OBJECTS,HEADERS,UNIT,BASIS=0x1000000,0x1000100,0x1000200,0x1000300,0x1001000,0x1003000,0x1004000
OWNERS=(0xe3310,0x11fa50)
CPP='src/physics/native_tu15_camera_basis.cpp'
HPP='include/halo2/native_tu15_camera_basis.hpp'
BACKEND='unchanged-tu15-native-camera-basis-v1'
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def save(p,d):p.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
def metadata(image):
 code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
 for owner in OWNERS:
  body=complete_body(code,owner);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access
    if i.mnemonic in('cmp','test'):access|=CS_AC_READ
    assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),i.mnemonic)
    ops.append(dict(base=registers.get(i.reg_name(op.mem.base),-1),index=registers.get(i.reg_name(op.mem.index),-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops))
  listings[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[18,62],counts
 return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[],boundaries=[])
def cases(image):
 rows=[]
 def make(name,kind='basis',vector=(1.25,-2.5,3.75),cw=0x27f,mx=0x1f80,index=0,mode=6,active=1,flags=0x2000,alias=0,raw=False):
  data=bytearray(random.Random(len(rows)).randbytes(0x5000))
  def put(p,v,n=4):data[p-BASE:p-BASE+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
  for n,v in enumerate(vector):put(DIRECTION+n*4,v if raw else bits(v))
  for n,v in enumerate((.25,-.75,1.5)):put(BASIS+n*4,bits(v))
  put(OBJECTS+0x44,HEADERS);put(HEADERS+index*12+8,UNIT);put(UNIT+0x3dc,mode,1);put(UNIT+0x3f8,active,1);put(UNIT+0x348,flags,2)
  banks=[dict(address=BASE,before=data.hex()),dict(address=0x4ecbd0,before=struct.pack('<I',OBJECTS).hex()),dict(address=0x474d08,before=struct.pack('<I',BASIS).hex())]
  for p in(0x46a0ec,0x46a0d0):banks.append(dict(address=p,before=image.read(p,4).hex()))
  destination=DIRECTION+alias if alias else DEST
  rows.append(dict(name=name,kind=kind,entry=OWNERS[0]if kind=='transition'else OWNERS[1],receiver=DIRECTION,entry_registers={'eax':0xabcd0000|index,'edx':destination},arguments=[],object=0xabcd0000|index,direction=DIRECTION,destination=destination,banks=banks,before_fp=[cw,0x4520,mx],stop_pc=physical.STOP,boundary=None))
 for index in(0,85,255):
  for mode in(0,5,6,7,255):
   for active,flags in((0,0x2000),(1,0),(1,0x2000),(255,0xffff)):make(f'transition_{index}_{mode}_{active}_{flags}',kind='transition',index=index,mode=mode,active=active,flags=flags)
 vectors=((0.,0.,0.),(1.,0.,0.),(0.,1.,0.),(-2.,3.,4.),(.125,-.5,-4.),(0.,0.,1.),(1e-12,-1e-12,7.),(1e18,1e18,-1e18))
 for vi,v in enumerate(vectors):
  for rounding in range(4):
   for precision in(0,2,3):make(f'basis_{vi}_{rounding}_{precision}',vector=v,cw=0x7f|(precision<<8)|(rounding<<10),mx=0x1f80|(rounding<<13))
 for value in(0x80000000,1,0x7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
  for mode in(0,0x40,0x8000,0x8040):make(f'edge_{value:x}_{mode:x}',vector=(value,bits(.5),bits(-.75)),raw=True,mx=0x1f80|mode)
 for alias in(1,4,8):make(f'alias_{alias}',alias=alias)
 make('fault_seed_normal');make('fault_seed_fallback',vector=(0.,0.,1.));make('fault_seed_transition',kind='transition')
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(exist_ok=False,parents=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False,production_full_camera_closed=False,preserved_x87_depth_physically_tested=0)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);original_observer=physical.observer_source
  physical.observer_source=lambda:original_observer().replace('timestamps=J::array();','timestamps=J::array(),executed=J::array();').replace('const auto site=sites.find(pc);','executed.push_back(pc);const auto site=sites.find(pc);').replace('{"boundary_gp",gp(regs)}','{"executed_sites",executed},{"boundary_gp",gp(regs)}')
  physical.boundary=lambda c,o:None
  rows=physical.physical(image,meta,cases(image),scratch);assert all(c['boundary_gp'][4]==physical.SP+4 for c in rows)
  prefixes=[];code=_Code(image,'.text')
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   for category in('reads','writes'):
    seen=set();counts={}
    for event in seed[category]:
     key=event['address'],event['bytes']if category=='reads'else len(event['written'])//2;counts[key]=counts.get(key,0)+1
     if event['instruction']in seen:continue
     seen.add(event['instruction']);case=copy.deepcopy(seed);case.update(name=seed['name']+'_'+category+'_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=counts[key],access='read'if category=='reads'else'write'));prefixes.append(case)
   for n,event in enumerate(seed['writes']):
    case=copy.deepcopy(seed);case.update(name=seed['name']+'_observer_'+str(n),stop_pc=event['instruction']+code.at(event['instruction']).size,observer_fault=n+1);prefixes.append(case)
  rows+=physical.physical(image,meta,prefixes,scratch);fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');source=(ROOT/CPP).read_text();(project/'native.cpp').write_text(source)
  driver=(ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_wrapped_contact_process.hpp"','#include "halo2/native_tu15_camera_basis.hpp"').replace('unchanged-tu15-native-wrapped-contact-process-v1',BACKEND).replace('J::array({46,40,40})','J::array({18,62})')
  begin=driver.index('NativeTu15CompoundProcessServices services;');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+'bool boundary_hit{};std::uint32_t result{};\n'+driver[end:];driver=driver.replace('NativeTu15WrappedContactProcessStats stats;','NativeTu15CameraBasisStats stats;')
  needle='native_tu15_process_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("proxy")},c.at("geometry")},fp,services,observer,&stats);';assert driver.count(needle)==1
  driver=driver.replace(needle,'if(c.at("kind")=="transition")result=native_tu15_unit_camera_transition(m,c.at("object"));else result=native_tu15_camera_perpendicular(m,c.at("direction"),c.at("destination"),fp,0,observer,&stats);')
  needle='   check(failed==fault_case';driver=driver.replace(needle,'   if(!fault_case&&!c.contains("observer_fault"))check(result==c.at("boundary_gp")[0],name+" exact camera scalar return differs");\n'+needle);(project/'driver.cpp').write_text(driver)
  mutants={'wrong_transition_bit':('>>13u','>>12u'),'wrong_cross_sign':('sub(z_second,y_third)','sub(y_third,z_second)'),'wrong_fallback':('first=scalar(a);second=scalar(b);','first=scalar(b);second=scalar(a);')}
  for name,(before,after)in mutants.items():assert source.count(before)==1;(project/(name+'.cpp')).write_text(source.replace(before,after))
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(w in r.stderr for w in("differ","differs")),(r.returncode,r.stdout,r.stderr)\nprint(r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(camera LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for name in('native',*mutants):
   cmake+=f'add_executable(camera_{name} {name}.cpp $<TARGET_OBJECTS:driver>)\ntarget_include_directories(camera_{name} PRIVATE include)\ntarget_compile_features(camera_{name} PRIVATE cxx_std_20)\ntarget_compile_options(camera_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME camera_{name} COMMAND {command}$<TARGET_FILE:camera_{name}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','camera_'+n]for n in('native',*mutants)),[]),'--filter','^camera_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];result=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=result.returncode));assert result.returncode==0,'Native camera leaves differ'
  executed=set(pc for c in rows for pc in c['executed_sites']);assert len(executed)==sum(meta['body_sites']),(len(executed),meta['body_sites'])
  files=(ROOT/CPP,ROOT/HPP,Path(__file__));report.update(passed=True,original_sha256=NEW_PIN,body_sites=meta['body_sites'],physically_executed_sites=len(executed),profiles=len(rows),fault_or_observer_profiles=len(prefixes),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),results=results,negative_controls=list(mutants),source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},listing_sha256=meta['listing_sha256'],scope=__doc__)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
