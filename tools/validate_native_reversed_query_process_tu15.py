#!/usr/bin/env python3
"""Native reversed/conditional/wrapped/convex query parents: exact child entries.

Closed empty-query profiles execute genuine8EB50; nonempty queries stop at
the actual30E930 entry. No solver result or callback afterimage is supplied.
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
AGENT,QUERY,CHILD,FIRST,SECOND,SHAPE_A,SHAPE_B,MOTION_A,MOTION_B,CONTEXT,OUTPUT,HEADER,TABLE_A,TABLE_B=map(lambda n:BASE+n,(0x100,0x200,0x300,0x400,0x500,0x800,0x1000,0x1800,0x1a00,0x1c00,0x1d00,0x1e00,0x2000,0x2100))
OWNERS=(0x2f1a50,0x2f0f10,0x2f6f40,0x2f8410,0x30ba80,0x8eb50,0x2e44d0,0x2e4550)
UNITS=('native_tu15_reversed_query_process.cpp','native_tu15_conditional_contact_process.cpp','native_tu15_wrapped_contact_query.cpp','native_tu15_convex_overlap_query.cpp','native_tu15_geometry.cpp','native_tu15_inverse_geometry.cpp')
BACKEND='unchanged-tu15-reversed-query-parents-v1'
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def metadata(image):
 code=_Code(image,'.text');names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];hashes={}
 for entry in OWNERS:
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   ins=code.at(pc);listing.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n');ops=[]
   for op in ins.operands:
    if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue
    if op.mem.segment:assert ins.reg_name(op.mem.segment)=='fs';continue
    access=CS_AC_WRITE if ins.mnemonic in('fst','fstp')else op.access
    if ins.mnemonic in('test','cmp'):access|=CS_AC_READ
    base,index=ins.reg_name(op.mem.base),ins.reg_name(op.mem.index)
    ops.append(dict(base=names.get(base,-1),index=names.get(index,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops))
  hashes[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts[:6]==[39,125,46,91,2,1],counts
 return dict(sites=sites,body_sites=counts,listing_sha256=hashes,regions=[],boundaries=[])
def cases(image):
 rows=[]
 def make(name,entry,rounding=0,empty=False,changes=()):
  rng=random.Random(len(rows));data=bytearray(rng.randbytes(0x3000))
  def word(p,n):struct.pack_into('<I',data,p-BASE,n&0xffffffff)
  word(AGENT+12,QUERY);word(AGENT+16,0);word(QUERY,TABLE_A);word(TABLE_A+4,0x8eb50 if empty else 0x2f6f40)
  word(QUERY+12,CHILD);word(CHILD,TABLE_B);word(TABLE_B+4,0x2f8410)
  word(FIRST,SHAPE_A);word(FIRST+4,0x12345678);word(FIRST+8,MOTION_A)
  word(SECOND,SHAPE_B);word(SECOND+4,0x87654321);word(SECOND+8,MOTION_B)
  word(SHAPE_A+12,SHAPE_B);word(OUTPUT,HEADER);word(HEADER,BASE+0x2800);word(HEADER+4,3)
  for address in(MOTION_A,MOTION_B):
   for offset in range(0,0x60,4):word(address+offset,struct.unpack('<I',struct.pack('<f',rng.uniform(-3,3)))[0])
  for offset in range(0,64,4):word(SHAPE_A+16+offset,struct.unpack('<I',struct.pack('<f',rng.uniform(-3,3)))[0])
  for address,value in changes:word(address,value)
  proxy,geometry,collector,convex=physical.SP-0x80,physical.SP-0x70,physical.SP-36,physical.SP-0x1f0
  body=physical.SP-28
  first,second,output=FIRST,SECOND,OUTPUT
  receiver=AGENT;boundary=0x2f0f10;kind='reversed'
  private=[]
  if entry==0x2f0f10:
   kind='conditional';boundary=0x2f6f40;private=[(collector,24)]
  elif entry==0x2f6f40:
   kind='wrapped';receiver=QUERY;boundary=0x2f8410;private=[(proxy,112)]
  elif entry==0x2f8410:
   kind='convex';receiver=CHILD;boundary=0x30e930;private=[(convex+0x10,0x1c0)]
  elif entry==0x30ba80:
   kind='collector';receiver=OUTPUT;boundary=None
  elif entry==0x8eb50:
   kind='noop';boundary=None
  if empty:
   kind='reversed_empty' if entry==0x2f1a50 else 'conditional_empty';boundary=None
   if entry==0x2f1a50:body-=52;collector-=52
   private=[(collector,24)]
  args=[first,second,CONTEXT,output]
  if entry==0x30ba80:args=args[:2]
  banks=[dict(address=BASE,before=data.hex()),dict(address=0x48c974,before=bytes(8).hex())]
  for address,size in private:banks.append(dict(address=address,before=rng.randbytes(size).hex()))
  rows.append(dict(name=name,entry=entry,kind=kind,receiver=receiver,agent=receiver,first=first,second=second,configuration=CONTEXT,output=output,proxy=proxy,geometry=geometry,body=body,collector=collector,convex=convex,arguments=args,banks=banks,before_fp=[0x27f|(rounding<<10),0x20,0x1f80|(rounding<<13)],stop_pc=physical.STOP,boundary=None,declared_boundary=boundary))
 for rounding in range(4):
  for entry in(0x2f1a50,0x2f0f10,0x2f6f40,0x2f8410):
   for n in range(3):make(f'prefix_{entry:x}_{rounding}_{n}',entry,rounding)
  for entry in(0x2f1a50,0x2f0f10):make(f'closed_empty_{entry:x}_{rounding}',entry,rounding,True)
  make(f'actual_collector_{rounding}',0x30ba80,rounding)
  make(f'actual_noop_{rounding}',0x8eb50,rounding)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False,production_native_only_closure=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image)
  def boundary(case,actual):
   if actual['actual_stop']==physical.STOP:return None
   gp,args=actual['boundary_gp'],actual['boundary_stack']
   return dict(owner=actual['actual_stop'],receiver=gp[1],arguments=args[1:6 if actual['actual_stop']==0x30e930 else 5])
  physical.boundary=boundary
  rows=[]
  for case in cases(image):
   specific=dict(meta,boundaries=[] if case['declared_boundary']is None else[case['declared_boundary']]);rows+=physical.physical(image,specific,[case],scratch)
  code=_Code(image,'.text');prefixes=[]
  for seed in rows:
   if not seed['name'].endswith('_0_0'):continue
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
  for c in prefixes:rows+=physical.physical(image,dict(meta,boundaries=[]),[c],scratch)
  for c in rows:
   if c['actual_stop']==physical.STOP:
    assert c['boundary_gp'][4]==physical.SP+(12 if c['kind']=='collector' else 20),c['name']
  fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp')
  for name in UNITS:shutil.copyfile(ROOT/'src/physics'/name,project/name)
  driver=(ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp').read_text()
  driver=driver.replace('#include "halo2/native_tu15_wrapped_contact_process.hpp"','#include "halo2/native_tu15_reversed_query_process.hpp"\n#include "halo2/native_tu15_conditional_contact_process.hpp"\n#include "halo2/native_tu15_wrapped_contact_query.hpp"\n#include "halo2/native_tu15_convex_overlap_query.hpp"')
  driver=driver.replace('unchanged-tu15-native-wrapped-contact-process-v1',BACKEND).replace('J::array({46,40,40})','J::parse("'+json.dumps(meta['body_sites'])+'")')
  begin=driver.index('   services.process_child=');end=driver.index('   const auto cw=',begin)
  driver=driver[:begin]+'''   NativeTu15ConditionalContactServices conditional;
   NativeTu15ConvexOverlapQueryServices convex;
   const auto stop=[&](std::uint32_t owner,std::uint32_t receiver,J args){const auto&b=c.at("boundary");check(!b.is_null()&&b.at("owner")==owner&&b.at("receiver")==receiver&&b.at("arguments")==args,name+" actual query dependency ABI differs");boundary_hit=true;throw std::runtime_error("actual-island-owner-boundary");};
   conditional.query=[&](auto receiver,auto owner,auto first,auto second,auto configuration,auto output,auto&){if(owner.identity==0x8eb50u){native_tu15_ignore_contact_process();return;}stop(owner.identity,receiver.identity,J::array({first.identity,second.identity,configuration.identity,output.identity}));};
   services.process_child=[&](const auto&call,auto&state){check(call.owner.identity==0x2f0f10u,"Wrong reversed owner");if(c.at("kind")=="reversed_empty"){native_tu15_process_conditional_contact_agent(m,call.receiver,{call.first.identity},call.second,call.configuration,call.output,{{c.at("body")},{c.at("collector")}},state,conditional,observer);return;}stop(call.owner.identity,call.receiver.identity,J::array({call.first.identity,call.second.identity,call.configuration.identity,call.output.identity}));};
   convex.geometry.solve_simplex=[&](auto simplex,auto first,auto second,auto relative,auto cache,auto plane,auto&)->std::uint32_t{stop(0x30e930u,simplex,J::array({first.identity,second.identity,relative,cache,plane}));return 0;};
''' +driver[end:]
  needle='native_tu15_process_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("proxy")},c.at("geometry")},fp,services,observer,&stats);'
  assert driver.count(needle)==1
  driver=driver.replace(needle,'''const auto kind=c.at("kind").get<std::string>();
    if(kind=="reversed"||kind=="reversed_empty")native_tu15_process_reversed_query_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},fp,services,observer);
    else if(kind=="conditional"||kind=="conditional_empty")native_tu15_process_conditional_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("body")},{c.at("collector")}},fp,conditional,observer);
    else if(kind=="wrapped")native_tu15_query_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("proxy")},c.at("geometry")},fp,conditional,observer);
    else if(kind=="convex")native_tu15_query_convex_overlap(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{c.at("convex")},fp,0,convex,observer);
    else if(kind=="collector")native_tu15_mark_contact_query_hit(m,{c.at("agent")},observer);
    else if(kind=="noop")native_tu15_ignore_contact_process();
    else check(false,"Unknown query fixture");''')
  (project/'driver.cpp').write_text(driver)
  variants={}
  for label,unit,before,after in(
   ('wrong_reversal','native_tu15_reversed_query_process.cpp','{0x2f0f10u},{second.identity}','{0x2f0f10u},{first.identity}'),
   ('wrong_wrapped_shape','native_tu15_wrapped_contact_query.cpp','m.word(shape+12u)','m.word(shape+16u)'),
   ('wrong_simplex_flag','native_tu15_convex_overlap_query.cpp','p+0x7cu,1u,1u','p+0x7cu,0u,1u')):
   text=(project/unit).read_text();assert text.count(before)==1
   (project/(label+'.cpp')).write_text(text.replace(before,after));variants[label]=unit
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(w in r.stderr for w in ("differs", "differ")),(r.returncode,r.stdout,r.stderr)\nprint("Original query rejects mutation:",r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(query_parents LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  objects={}
  for source in(*UNITS,*[name+'.cpp'for name in variants]):
   target='object_'+str(len(objects));objects[source]=target
   cmake+=f'add_library({target} OBJECT {source})\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for label in('native',*variants):
   object_names=[objects[label+'.cpp'if variants.get(label)==unit else unit]for unit in UNITS]
   cmake+=f'add_executable(query_{label} $<TARGET_OBJECTS:driver> '+' '.join('$<TARGET_OBJECTS:'+name+'>'for name in object_names)+')\n'
   command=''if label=='native'else f'"{sys.executable}" "{project}/negative.py" '
   cmake+=f'add_test(NAME query_{label} COMMAND {command}$<TARGET_FILE:query_{label}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake)
  results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','query_'+label]for label in('native',*variants)),[]),'--filter','^query_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native query parents differ'
  files=[*[ROOT/'src/physics'/name for name in UNITS],*[ROOT/'include/halo2'/name.replace('.cpp','.hpp')for name in UNITS],Path(__file__),ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp',ROOT/'tools/validate_native_island_agent_phase_tu15.py',ROOT/'tools/observe_native_constraint_step_tu15.cpp']
  report.update(passed=True,original_sha256=NEW_PIN,body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],profiles=len(rows),genuine_boundary_profiles=sum(c['boundary']is not None for c in rows),fault_or_observer_profiles=sum('fault'in c or'observer_fault'in c for c in rows),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),compiler_results=results,negative_controls=list(variants),source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},scope=__doc__,remaining=['Positive appended-contact reversal loop','Full query after genuine30E930 return','Actual input/World production composition'])
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
