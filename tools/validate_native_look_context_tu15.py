#!/usr/bin/env python3
"""Physical look-controller/context prefixes and complete native camera origin.

Objectless334-site parent paths execute fully. ActualCD4A0 stops at its real
entry; no camera output is supplied. All BBF20/BBE00 branches use native code.
"""
import argparse,copy,hashlib,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE=0x1000000
CONTROLS,GLOBALS,DEFINITION,OBJECTS,ROWS,TAGS,ACTORS,ACTOR_ROWS,ENTRIES,ACTOR_POINT,STEP,DATA,DEST=map(lambda n:BASE+n,(0x100,0x1000,0x2000,0x3000,0x4000,0x5000,0x6000,0x7000,0x8000,0x9000,0xa000,0xb000,0x10000))
OWNERS=(0x18a300,0x189fd0,0xbbf20,0xbbe00)
UNITS=('src/physics/native_tu15_look_controller.cpp','src/physics/native_tu15_look_context.cpp','src/campaign/ai/native_campaign_object_world_point_tu15.cpp')
BACKEND='unchanged-tu15-look-context-origin-v1'
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def metadata(image):
 code=_Code(image,'.text');names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];hashes={}
 for owner in OWNERS:
  body=complete_body(code,owner);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access
    if i.mnemonic in('test','cmp'):access|=CS_AC_READ
    assert not op.mem.segment
    ops.append(dict(base=names.get(i.reg_name(op.mem.base),-1),index=names.get(i.reg_name(op.mem.index),-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops))
  hashes[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[334,65,67,76],counts
 return dict(sites=sites,body_sites=counts,listing_sha256=hashes,regions=[],boundaries=[0xcd4a0])
def cases(image):
 rows=[]
 def make(name,kind='controller',yaw=.25,pitch=.1,dy=.2,dp=-.1,minimum=-1.,maximum=1.,unit=False,depth=0,actor_index=None,actor_count=2,rounding=0,modes=0):
  rng=random.Random(len(rows));raw=bytearray(rng.randbytes(0x11000))
  def put(p,n,size=4):raw[p-BASE:p-BASE+size]=(n&((1<<(size*8))-1)).to_bytes(size,'little')
  for p,n in((GLOBALS+0xf4,DEFINITION),(OBJECTS+0x44,ROWS),(ACTORS+0x44,ACTOR_ROWS),(STEP+4,bits(1/30)),(CONTROLS+0x14,1 if unit else 0xffffffff),(CONTROLS+0x24,bits(yaw)),(CONTROLS+0x28,bits(pitch)),(CONTROLS+0x94,bits(minimum)),(CONTROLS+0x98,bits(maximum))):put(p,n)
  for i in range(5):
   object=DATA+i*0x1000;put(ROWS+i*12+8,object);put(object+0x14,i+1 if i<depth else 0xffffffff);put(object+0xb4,0xffffffff);put(object+0x18,0,1);put(object+0x116,0x300,2)
   for axis,value in enumerate((1.25,-2.5,3.75)):put(object+0x64+axis*4,bits(value+i))
   for field in range(13):put(object+0x300+field*4,bits(1. if field in(0,1,5,9)else 0.))
  if actor_index is not None:
   object=DATA+depth*0x1000;put(object+0xb4,0);put(ACTOR_ROWS+0x18,actor_index,1);put(ACTOR_ROWS+0x74,actor_count);put(ACTOR_ROWS+0x70,ENTRIES)
   for i in range(4):put(ENTRIES+i*96+0x40,ACTOR_POINT+i*0x100);put(ACTOR_POINT+i*0x100+0x3c,ACTOR_POINT+i*0x100+0x40)
   for i in range(4):
    for axis,value in enumerate((7.5,-8.25,9.)):put(ACTOR_POINT+i*0x100+0xb0+axis*4,bits(value+i))
  global_words={0x4f9b54:CONTROLS,0x4ecc1c:GLOBALS,0x4ecbd0:OBJECTS,0x4f0414:TAGS,0x52be5c:ACTORS,0x51df54:STEP}
  banks=[dict(address=BASE,before=raw.hex())]+[dict(address=p,before=struct.pack('<I',n).hex())for p,n in sorted(global_words.items())]
  for p,n in((0x46a588,8),(0x44923c,4),(0x46a988,8),(0x46a0d0,4)):banks.append(dict(address=p,before=image.read(p,n).hex()))
  scratch=physical.SP-0xb4
  if kind=='controller':entry=0x18a300;receiver=0;registers={'eax':0};args=[bits(dy),bits(dp)];banks.append(dict(address=scratch+12,before=rng.randbytes(0xa8).hex()))
  elif kind=='context':entry=0x189fd0;receiver=0;registers={'eax':0,'esi':DEST};args=[]
  else:entry=0xbbf20;receiver=DEST;registers={'eax':0};args=[]
  rows.append(dict(name=name,kind=kind,entry=entry,receiver=receiver,arguments=args,entry_registers=registers,index=0,object=0,destination=DEST,scratch=scratch,banks=banks,before_fp=[0x27f|(rounding<<10),0x20,0x1f80|(rounding<<13)|modes],stop_pc=physical.STOP,boundary=None))
 for yaw in(-13.,-.1,0.,6.2831855,14.):
  for rounding in range(4):
   for mode in(0,0x40,0x8000,0x8040):make(f'controller_yaw_{yaw}_{rounding}_{mode}',yaw=yaw,rounding=rounding,modes=mode)
 for pitch in(-2.,0.,2.):
  for minimum,maximum in((-1.,1.),(1.,-1.),(-1.4922565,1.4922565)):make(f'controller_pitch_{pitch}_{minimum}',pitch=pitch,minimum=minimum,maximum=maximum)
 make('controller_real_camera_boundary',unit=True);make('context_empty','context');make('context_real_camera_boundary','context',unit=True)
 for depth in range(4):
  make(f'origin_fallback_{depth}','origin',depth=depth)
  for index,count in((-1,2),(0,0),(0,1),(1,2),(2,2),(127,128),(-128,2)):
   if index==127:continue
   make(f'origin_actor_{depth}_{index}_{count}','origin',depth=depth,actor_index=index,actor_count=count)
 for field in(0x24,0x28,0x94,0x98):
  for value in(0x80000000,1,0x7fc12345,0x7f812345):
   make(f'controller_edge_{field:x}_{value:x}')
   case=rows[-1];bank=next(b for b in case['banks']if b['address']==BASE);raw=bytearray.fromhex(bank['before']);struct.pack_into('<I',raw,CONTROLS-BASE+field,value);bank['before']=raw.hex()
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False,production_full_look_controller_closed=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image)
  def boundary(case,actual):
   if actual['actual_stop']==physical.STOP:return None
   assert actual['actual_stop']==0xcd4a0
   return dict(owner=0xcd4a0,object=actual['boundary_gp'][1],destination=actual['boundary_gp'][0])
  physical.boundary=boundary;rows=physical.physical(image,meta,cases(image),scratch)
  prefixes=[];code=_Code(image,'.text')
  seeds=[next(c for c in rows if c['name']==name)for name in('controller_yaw_-13.0_0_0','context_real_camera_boundary','origin_fallback_1','origin_actor_1_1_2')]
  for seed in seeds:
   for category in('reads','writes'):
    seen=set();occurrences={}
    for event in seed[category]:
     key=event['address'],event['bytes']if category=='reads'else len(event['written'])//2;occurrences[key]=occurrences.get(key,0)+1
     if event['instruction']in seen:continue
     seen.add(event['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='read'if category=='reads'else'write'));prefixes.append(c)
   seen=set()
   for ordinal,event in enumerate(seed['writes']):
    if event['instruction']in seen:continue
    seen.add(event['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(ordinal),stop_pc=event['instruction']+code.at(event['instruction']).size,observer_fault=ordinal+1);prefixes.append(c)
  rows+=physical.physical(image,dict(meta,boundaries=[]),prefixes,scratch)
  for c in rows:
   if c['actual_stop']==physical.STOP:assert c['boundary_gp'][4]==physical.SP+(12 if c['kind']=='controller'else 4)
  fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for name in UNITS:shutil.copyfile(ROOT/name,project/Path(name).name)
  driver=(ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_wrapped_contact_process.hpp"','#include "halo2/native_tu15_look_controller.hpp"')
  driver=driver.replace('unchanged-tu15-native-wrapped-contact-process-v1',BACKEND).replace('J::array({46,40,40})','J::array({334,65,67,76})')
  begin=driver.index('NativeTu15CompoundProcessServices services;');end=driver.index('   const auto cw=',begin)
  driver=driver[:begin]+'''NativeTu15LookControllerServices services;bool boundary_hit{};
   services.context.camera_position=[&](auto object,auto destination,auto&){const auto&b=c.at("boundary");check(!b.is_null()&&b.at("object")==object&&b.at("destination")==destination,name+" actualCD4A0 typed input differs");boundary_hit=true;throw std::runtime_error("actual-island-owner-boundary");};
'''+driver[end:]
  driver=driver.replace('NativeTu15WrappedContactProcessStats stats;','NativeTu15LookControllerStats stats;')
  needle='native_tu15_process_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("proxy")},c.at("geometry")},fp,services,observer,&stats);';assert driver.count(needle)==1
  driver=driver.replace(needle,'''const auto kind=c.at("kind").get<std::string>();
    if(kind=="controller")native_tu15_update_look_controller(m,c.at("index"),c.at("arguments")[0],c.at("arguments")[1],{c.at("scratch")},fp,0,services,observer,&stats);
    else if(kind=="context")native_tu15_build_look_context(m,c.at("index"),c.at("destination"),fp,services.context,observer);
    else (void)native_tu15_object_camera_origin(m,c.at("object"),c.at("destination"),fp,observer);''')
  (project/'driver.cpp').write_text(driver)
  variants={}
  for name,unit,before,after in(('wrong_pitch_rate',UNITS[0],'read(0x46a988u)','read(0x46a98cu)'),('wrong_context_object',UNITS[1],'m.word(0x189fecu,context,object)','m.word(0x189fecu,context,0xffffffffu)'),('wrong_actor_point',UNITS[1],'m.word(data+0x78u)','m.word(data+0x74u)')):
   text=(ROOT/unit).read_text();assert text.count(before)==1;(project/(name+'.cpp')).write_text(text.replace(before,after));variants[name]=Path(unit).name
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(w in r.stderr for w in("differs","differ")),(r.returncode,r.stdout,r.stderr)\nprint("Original look rejects mutation",r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(look LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  objects={}
  for file in(*[Path(n).name for n in UNITS],*[name+'.cpp'for name in variants]):
   target='object_'+str(len(objects));objects[file]=target;cmake+=f'add_library({target} OBJECT {file})\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for name in('native',*variants):
   selected=[objects[name+'.cpp'if variants.get(name)==Path(unit).name else Path(unit).name]for unit in UNITS]
   cmake+=f'add_executable(look_{name} $<TARGET_OBJECTS:driver> '+' '.join('$<TARGET_OBJECTS:'+n+'>'for n in selected)+')\n'
   command=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" '
   cmake+=f'add_test(NAME look_{name} COMMAND {command}$<TARGET_FILE:look_{name}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake)
  results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','look_'+name]for name in('native',*variants)),[]),'--filter','^look_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native look/context differs'
  files=[*[ROOT/n for n in UNITS],ROOT/'include/halo2/native_tu15_look_context.hpp',ROOT/'include/halo2/native_tu15_look_controller.hpp',Path(__file__)]
  report.update(passed=True,original_sha256=NEW_PIN,profiles=len(rows),body_sites=meta['body_sites'],fault_or_observer_profiles=len(prefixes),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),results=results,negative_controls=list(variants),source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},scope=__doc__)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
