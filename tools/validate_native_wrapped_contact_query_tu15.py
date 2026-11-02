#!/usr/bin/env python3
"""Compare reversed query/collector DATA and resource cleanup with original TU1.5.

Closed child queries execute genuine8EB50, not a supplied collision outcome.
Collector controls execute actual30BA80 or stop at its authentic entry to check
the swapped ABI. Fault prefixes execute37E870 only when the live original
registration state selects that action. No Xbox exception dispatcher is used.
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
AGENT,FIRST,SECOND,SHAPE,TRANSFORM,CHILD,TABLE,COLLECTOR,DESTINATION=map(lambda n:BASE+n,(0x100,0x200,0x300,0x400,0x500,0x600,0x700,0x800,0x900))
OWNERS=(0x2f73c0,0x2f6f40,0x2e44d0,0x30b960,0x30ba80,0x8eb50,0x37e870,0x2f0dd0)
UNITS=('native_tu15_wrapped_contact_query.cpp','native_tu15_geometry.cpp','native_tu15_contact_pair_collectors.cpp','native_tu15_convex_overlap_query.cpp','native_tu15_inverse_geometry.cpp')
BACKEND='unchanged-tu15-reversed-wrapped-query-collectors-v1'
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,d):p.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')

def metadata(image):
 code=_Code(image,'.text');names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites={};counts=[];hashes={}
 for entry in OWNERS:
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   ins=code.at(pc);listing.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n');ops=[]
   for op in ins.operands:
    if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue
    if op.mem.segment:assert ins.reg_name(op.mem.segment)=='fs';continue
    access=CS_AC_WRITE if ins.mnemonic in('fst','fstp')else op.access
    if ins.mnemonic in('cmp','test'):access|=CS_AC_READ
    base,index=ins.reg_name(op.mem.base),ins.reg_name(op.mem.index)
    ops.append(dict(base=names.get(base,-1),index=names.get(index,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites[pc]=dict(instruction=pc,operands=ops)
  hashes[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts[:6]==[25,46,40,8,2,1],counts
 handler=code.at(0x37e878);assert handler.mnemonic=='mov'and handler.op_str.startswith('eax, ')
 info=int(handler.op_str.split(', ')[1],0);word=lambda p:int.from_bytes(image.read(p,4),'little')
 assert word(info)==0x19930520 and word(info+4)==1
 action=word(info+8);assert word(action)==0xffffffff and word(action+4)==0x37e870
 return dict(sites=list(sites.values()),body_sites=counts,listing_sha256=hashes,regions=[],boundaries=[],unwind_info=info)

def cases():
 rows=[]
 def make(kind,controls=None,changes=(),seed=False):
  rng=random.Random(len(rows));data=bytearray(rng.randbytes(0x1000));put=lambda p,n:struct.pack_into('<I',data,p-BASE,n&0xffffffff)
  put(AGENT+12,CHILD);put(CHILD,TABLE);put(TABLE+4,0x8eb50)
  for body in(FIRST,SECOND):put(body,SHAPE);put(body+8,TRANSFORM)
  put(SHAPE+12,SHAPE+0x80);put(COLLECTOR+8,DESTINATION);put(DESTINATION,TABLE+0x20);put(TABLE+0x24,0x30ba80)
  for p in range(TRANSFORM,TRANSFORM+0x60,4):put(p,struct.unpack('<I',struct.pack('<f',rng.uniform(-5,5)))[0])
  for p in range(SHAPE+16,SHAPE+80,4):put(p,struct.unpack('<I',struct.pack('<f',rng.uniform(-5,5)))[0])
  for p,n in changes:put(p,n)
  proxy,geometry,local=physical.SP-0xa0,physical.SP-0x90,physical.SP-24
  banks=[dict(address=BASE,before=data.hex())]
  if kind=='reversed':banks += [dict(address=proxy,before=rng.randbytes(112).hex()),dict(address=local,before=rng.randbytes(12).hex())]
  entry=0x2f73c0 if kind=='reversed'else 0x30ba80 if kind=='mark'else 0x30b960
  args=[FIRST,SECOND,BASE+0xa00,COLLECTOR]if kind=='reversed'else[FIRST,SECOND]
  rows.append(dict(name=('fault_seed_'if seed else'')+kind+'_'+str(len(rows)),kind=kind,entry=entry,receiver=AGENT if kind=='reversed'else COLLECTOR,agent=AGENT if kind=='reversed'else COLLECTOR,first=FIRST,second=SECOND,configuration=BASE+0xa00,output=COLLECTOR,proxy=proxy,geometry=geometry,collector=local,arguments=args,banks=banks,before_fp=controls or[0x27f,0x453f,0x1f80],stop_pc=physical.STOP,boundary=None,declared_boundary=0x30ba80 if kind=='forward_prefix'else None))
 for precision in(0,2,3):
  for rounding in range(4):
   for mode in(0,0x40,0x8000,0x8040):make('reversed',[0x7f|(precision<<8)|(rounding<<10),0x453f,0x1f80|(rounding<<13)|mode])
 for p in(SHAPE+16,SHAPE+0x2c,TRANSFORM+0x20,TRANSFORM+0x4c):
  for value in(0x80000000,1,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make('reversed',changes=((p,value),))
 for kind in('mark','forward','forward_prefix'):
  for n in range(4):make(kind)
 for kind in('reversed','mark','forward'):make(kind,seed=True)
 return rows

def observer_source():
 text=ORIGINAL_OBSERVER()
 old='if(pc==stop||std::find(m.at("boundaries").begin(),m.at("boundaries").end(),pc)!=m.at("boundaries").end())break;'
 assert text.count(old)==1
 text=text.replace('unsigned count=0;','unsigned count=0;bool composed{};J source_actions=J::array();')
 text=text.replace(old,r'''if(pc==stop||std::find(m.at("boundaries").begin(),m.at("boundaries").end(),pc)!=m.at("boundaries").end()){
      if(m.value("compose_unwind",false)&&!composed){composed=true;
        const auto word=[&](std::uint32_t p){const auto b=memory(child.pid,p,4);std::uint32_t n{};std::memcpy(&n,b.data(),4);return n;};
        const auto node=word(0x4000000u);check(node!=0,"Missing source query registration");
        check(word(node+4u)==0x37e878u,"Unexpected source query handler");
        const auto state=word(node+8u);check(state==0u||state==0xffffffffu,"Unexpected source query state");
        if(state==0u){
          source_actions.push_back(J{{"handler",0x37e878u},{"state",0},{"entry",0x37e870u},{"frame",node+12u}});
          const std::uintptr_t call_sp=0x2000ff0u;
          const auto old_word=std::uint64_t(request(PTRACE_PEEKDATA,child.pid,reinterpret_cast<void*>(call_sp)));
          const auto result=(old_word&0xffffffff00000000ull)|std::uint32_t(stop);
          request(PTRACE_POKEDATA,child.pid,reinterpret_cast<void*>(call_sp),reinterpret_cast<void*>(result));
          auto call=regs;call.rip=0x37e870u;call.rbp=node+12u;call.rsp=call_sp;
          request(PTRACE_SETREGS,child.pid,nullptr,&call);continue;
        }
      }break;
    }''')
 return text.replace('{"timestamps",timestamps}', '{"timestamps",timestamps},{"source_unwind_actions",source_actions}')

ORIGINAL_OBSERVER=physical.observer_source

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--binding',action='store_true');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],outcomes_supplied=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image)
  def boundary(c,d):return None if d['actual_stop']==physical.STOP else dict(owner=d['actual_stop'],receiver=d['boundary_gp'][1],arguments=d['boundary_stack'][1:3])
  physical.boundary=boundary;physical.observer_source=observer_source
  rows=[]
  def original(c):return physical.physical(image,dict(meta,boundaries=[]if c['declared_boundary']is None else[c['declared_boundary']],compose_unwind=c.get('compose_unwind',False)),[c],scratch)
  for c in cases():rows+=original(c)
  prefixes=[];code=_Code(image,'.text')
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   for category in('reads','writes'):
    seen=set();occurrences={}
    for event in seed[category]:
     key=event['address'],event['bytes']if category=='reads'else len(event['written'])//2;occurrences[key]=occurrences.get(key,0)+1
     if event['instruction']in seen:continue
     seen.add(event['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(event['instruction']),stop_pc=event['instruction'],compose_unwind=seed['kind']=='reversed',fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='read'if category=='reads'else'write'));prefixes.append(c)
   seen=set()
   for ordinal,event in enumerate(seed['writes']):
    if event['instruction']in seen:continue
    seen.add(event['instruction'])
    c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(ordinal),stop_pc=event['instruction']+code.at(event['instruction']).size,compose_unwind=seed['kind']=='reversed',observer_fault=ordinal+1);prefixes.append(c)
  for c in prefixes:rows+=original(c)
  save(a.output/'source-cleanup-actions.json',[dict(name=c['name'],stop_pc=c['stop_pc'],actions=c.get('source_unwind_actions',[]),stores=[e['instruction']for e in c['writes']])for c in rows if c.get('compose_unwind')])
  fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for name in(*UNITS,'native_tu15_contact_unwind_internal.hpp'):shutil.copyfile(ROOT/'src/physics'/name,project/name)
  driver=(ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_wrapped_contact_process.hpp"','#include "halo2/native_tu15_wrapped_contact_query.hpp"\n#include "halo2/native_tu15_contact_pair_collectors.hpp"')
  driver=driver.replace('unchanged-tu15-native-wrapped-contact-process-v1',BACKEND).replace('J::array({46,40,40})','J::parse("'+json.dumps(meta['body_sites'])+'")')
  driver=driver.replace('std::to_string(expected.size()));for(std::size_t n=0;n<stores.size();++n)', 'std::to_string(expected.size())+" original="+expected.dump());for(std::size_t n=0;n<stores.size();++n)')
  start=driver.index('   services.process_child=');stop=driver.index('   const auto cw=',start)
  driver=driver[:start]+'''   NativeTu15ConditionalContactServices query;
   query.query=[&](auto,auto owner,auto,auto,auto,auto,auto&){check(owner.identity==0x8eb50u,"Wrong genuine no-op query");};
   NativeTu15ContactPairCollectorServices collectors;
   collectors.emit_pair=[&](auto receiver,auto owner,auto first,auto second,auto&){check(owner.identity==0x30ba80u,"Wrong actual collector owner");const auto&b=c.at("boundary");if(!b.is_null()){check(b.at("owner")==owner.identity&&b.at("receiver")==receiver.identity&&b.at("arguments")==J::array({first.identity,second.identity}),name+" reversed collector ABI differs");boundary_hit=true;throw std::runtime_error("actual-island-owner-boundary");}native_tu15_mark_contact_pair(m,receiver,observer);};
''' +driver[stop:]
  old='native_tu15_process_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("proxy")},c.at("geometry")},fp,services,observer,&stats);'
  assert driver.count(old)==1
  driver=driver.replace(old,'''if(c.at("kind")=="reversed")native_tu15_query_reversed_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("collector")},{{c.at("proxy")},c.at("geometry")}},fp,query,observer);
    else if(c.at("kind")=="mark")native_tu15_mark_contact_pair(m,{c.at("agent")},observer);
    else native_tu15_forward_reversed_contact_pair(m,{c.at("agent")},{c.at("first")},{c.at("second")},fp,collectors);''')
  if a.binding:
   driver='#include "halo2/native_tu15_overlap_query_binding.hpp"\n'+driver
   old='''if(c.at("kind")=="reversed")native_tu15_query_reversed_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("collector")},{{c.at("proxy")},c.at("geometry")}},fp,query,observer);
    else if(c.at("kind")=="mark")native_tu15_mark_contact_pair(m,{c.at("agent")},observer);
    else native_tu15_forward_reversed_contact_pair(m,{c.at("agent")},{c.at("first")},{c.at("second")},fp,collectors);'''
   assert driver.count(old)==1
   driver=driver.replace(old,'''NativeTu15OverlapQueryBinding binding(m,{0x200ee00u,512u},{},observer,nullptr,
      [&](auto receiver,auto owner,auto first,auto second){const auto&b=c.at("boundary");if(b.is_null()||owner.identity!=b.at("owner"))return;check(b.at("receiver")==receiver.identity&&b.at("arguments")==J::array({first.identity,second.identity}),name+" binding pair ABI differs");boundary_hit=true;throw std::runtime_error("actual-island-owner-boundary");});
    if(c.at("kind")=="reversed")binding.query({c.at("agent")},{0x2f73c0u},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},fp);
    else binding.collect({c.at("agent")},{c.at("kind")=="mark"?0x30ba80u:0x30b960u},{c.at("first")},{c.at("second")},fp);''')
  (project/'driver.cpp').write_text(driver)
  variants={}
  for label,unit,before,after in(
   ('wrong_swap',UNITS[0],'memory,agent,second,first,configuration,','memory,agent,first,second,configuration,'),
   ('wrong_collector_swap',UNITS[2],'owner,second,first,fp','owner,first,second,fp'),
   ('missing_cleanup',UNITS[0],'cleanup.arm(); // Source state0','// Source state0'),
   ('wrong_flag',UNITS[2],'std::byte{1}','std::byte{0}')):
   text=(project/unit).read_text();assert text.count(before)==1;(project/(label+'.cpp')).write_text(text.replace(before,after));variants[label]=unit
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(w in r.stderr for w in ("differs", "differ")),(r.returncode,r.stdout,r.stderr)\nprint("Original reversed query rejects mutation:",r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(reversed_query LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';objects={}
  for source in(*UNITS,*[name+'.cpp'for name in variants]):
   target='object_'+str(len(objects));objects[source]=target;cmake+=f'add_library({target} OBJECT {source})\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for label in('native',*variants):
   names=[objects[label+'.cpp'if variants.get(label)==unit else unit]for unit in UNITS];cmake+=f'add_executable(query_{label} $<TARGET_OBJECTS:driver> '+' '.join('$<TARGET_OBJECTS:'+name+'>'for name in names)+')\n';command=''if label=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME query_{label} COMMAND {command}$<TARGET_FILE:query_{label}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','query_'+label]for label in('native',*variants)),[]),'--filter','^query_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native reversed query differs'
  files=[*[ROOT/'src/physics'/name for name in(*UNITS,'native_tu15_contact_unwind_internal.hpp')],ROOT/'include/halo2/native_tu15_wrapped_contact_query.hpp',ROOT/'include/halo2/native_tu15_contact_pair_collectors.hpp',Path(__file__),ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp',ROOT/'tools/observe_native_constraint_step_tu15.cpp',ROOT/'tools/validate_native_island_agent_phase_tu15.py']
  if a.binding:files.append(ROOT/'include/halo2/native_tu15_overlap_query_binding.hpp')
  report.update(passed=True,typed_binding=a.binding,original_sha256=NEW_PIN,body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],cases=len(rows),fault_or_observer_cases=len(prefixes),source_cleanup_actions=sum(len(c.get('source_unwind_actions',[]))for c in rows),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),compiler_results=results,negative_controls=list(variants),source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},scope=__doc__,production_native_only_closure=False)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
