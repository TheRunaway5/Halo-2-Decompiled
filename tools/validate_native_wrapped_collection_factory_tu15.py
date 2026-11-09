#!/usr/bin/env python3
"""Compare BV->collection constructors with unchanged TU1.5 and native pools.

Uses authenticated original dispatch rows and actual initialized pool BEFORE
bytes. No allocation pointer, factory result or AFTER state is supplied.
"""
import argparse,copy,hashlib,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_array_growth_tu15 as pools
import validate_native_island_agent_phase_tu15 as physical
import validate_native_sphere_overlap_tu15 as sphere
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE=0x6000000
FIRST,SECOND,WRAPPER,CHILD,OTHER,CONTEXT,DISPATCH=[BASE+n for n in(0x100,0x200,0x300,0x400,0x500,0x600,0x800)]
DESCRIPTOR=physical.SP-0x40
BACKEND='unchanged-tu15-wrapped-collection-factory-v1'
HEADER='include/halo2/native_tu15_wrapped_factory_binding.hpp'
CENSUS=ROOT/'build/test-results/engine-crash-audit-20261102/claude-physics-dispatch/evidence/dispatch-table-census.json'
OWNERS=(0x2f0eb0,0x2f0df0,0x303c10,0x3040d0,0x2f3620,0x2f0380,0x149da0,0x149e00,0x233250,0x233420,0xa9520,0xadb50,0x37e400,0x2f7a10,0x37e4c0,0x37ed40)
UNITS=('native_tu15_wrapped_contact_agent.cpp','native_tu15_convex_agent_factory.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_geometry.cpp','native_tu15_inverse_geometry.cpp','native_tu15_contact_cache.cpp','native_tu15_contact_shape.cpp','native_tu15_activation_services.cpp','native_tu15_array_growth.cpp','native_tu15_array_allocator.cpp','native_tu15_agent_cleanup_children.cpp','native_tu15_agent_cleanup.cpp','native_tu15_phantom_callback_agent.cpp','native_tu15_gameplay_phantom_enter.cpp','native_tu15_gameplay_phantom_membership.cpp')
PATCH_OLD='''            }else if(target.identity==0x2f9990u){
                result=native_tu15_create_contact_agent(memory,NativeTu15ContactAgentFactory::owner_2f9990,
                    NativeTu15ContactAgentFactoryArguments{input.user_data},fp,allocation,observe);
'''
PATCH_NEW='''            }else if(target.identity==0x2f9990u||target.identity==0x303c10u||target.identity==0x3040d0u){
                const auto kind=target.identity==0x303c10u?NativeTu15ContactAgentFactory::owner_303c10:
                    target.identity==0x3040d0u?NativeTu15ContactAgentFactory::owner_3040d0:
                    NativeTu15ContactAgentFactory::owner_2f9990;
                result=native_tu15_create_contact_agent(memory,kind,
                    NativeTu15ContactAgentFactoryArguments{input.user_data},fp,allocation,observe);
'''
def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2)+'\n')
def metadata(image):
 code=_Code(image,'.text');regs={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites={};counts={};pins={};actions={}
 for owner in OWNERS:
  body=complete_body(code,owner);counts[hex(owner)]=len(body.code);listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for o in i.operands:
    if o.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    if o.mem.segment:assert i.reg_name(o.mem.segment)=='fs';continue
    access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else o.access
    if i.mnemonic in('cmp','test'):access|=CS_AC_READ
    ops.append(dict(base=regs.get(i.reg_name(o.mem.base),-1),index=regs.get(i.reg_name(o.mem.index),-1),scale=o.mem.scale,displacement=o.mem.disp&0xffffffff,bytes=o.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites[pc]=dict(instruction=pc,operands=ops,timestamp=i.mnemonic=='rdtsc')
  pins[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 word=lambda p:int.from_bytes(image.read(p,4),'little')
 for handler in(0x37e4cb,0x37e408,0x37ed4b):
  i=code.at(handler);assert i.mnemonic=='mov'and i.op_str.startswith('eax, ');info=int(i.op_str.split(', ')[1],0);assert word(info)==0x19930520 and word(info+4)==1;table=word(info+8);assert word(table)==0xffffffff;action=word(table+4);assert action in OWNERS;actions[str(handler)]=action
 return dict(sites=list(sites.values()),body_sites=counts,listing_sha256=pins,regions=[],boundaries=[],unwind_actions=actions)
def before_inputs(image):
 banks,proof=pools.reference_inputs(image)
 def read(p,n):return next(raw[p-a:p-a+n]for a,raw in banks.items()if a<=p and p+n<=a+len(raw))
 pool=proof['initial_pool'];selected={a:raw for a,raw in banks.items()if a!=0x10060000};selected[pool]=read(pool,0x340);selected[pool+0xf0340]=read(pool+0xf0340,0xc0);slices=[]
 def keep(p,n):
  data=read(p,n);selected[p]=data;slices.append(dict(address=p,bytes=n,sha256=hashlib.sha256(data).hexdigest()))
 for size in(20,64):
  index=struct.unpack('<b',read(pool+0xac+size,1))[0];head=struct.unpack('<I',read(pool+0x24+index*4,4))[0];stride=struct.unpack('<I',read(pool+0x68+index*4,4))[0]
  for _ in range(3):
   if not head:break
   keep(head,stride);head=struct.unpack('<I',read(head,4))[0]
 cursor=struct.unpack('<I',read(pool+0x20,4))[0];end=struct.unpack('<I',read(pool+0x1c,4))[0]
 if cursor<end:keep(cursor,min(1024,end-cursor))
 pages=struct.unpack('<I',read(pool+0xf0340,4))[0]
 if pages<0x76:keep(pool+0x4340+(pages<<13),1024)
 # Merge exact overlapping/adjacent authentic slices; never fill missing bytes.
 merged={}
 for a,raw in sorted(selected.items()):
  if merged and a<=last+len(merged[last]):
   overlap=max(0,last+len(merged[last])-a);assert merged[last][a-last:a-last+min(overlap,len(raw))]==raw[:min(overlap,len(raw))];merged[last]+=raw[overlap:]
  else:merged[a]=raw;last=a
 proof.update(consumed_before_slices=slices,retained_input_bytes=sum(map(len,merged.values())))
 return merged,proof

def cases(image,before):
 census=json.loads(CENSUS.read_text());assert census['xbe_sha256']==NEW_PIN and census['bank_hashes_verified'];table=[[int(x,0)for x in r]for r in census['create_table']];assert table[23][20]==0x303c10 and table[20][23]==0x3040d0
 classes={t['type']:int(t['vtables'][0],0)for t in census['shape_vtable_candidates']if t['get_type_owner']in('0xa9520','0xadb50')};assert set(classes)=={20,23};rows=[]
 def add(name,reverse=False,fp=None,user=0xdeadbeef,seed=False):
  rng=random.Random(len(rows));banks={a:bytearray(raw)for a,raw in before.items()};banks[BASE]=bytearray(rng.randbytes(0x2000));banks[DESCRIPTOR]=bytearray(rng.randbytes(16))
  for vt in classes.values():banks[vt]=bytearray(image.read(vt,24))
  def put(p,n):
   lo=next(a for a,b in banks.items()if a<=p and p+4<=a+len(b));struct.pack_into('<I',banks[lo],p-lo,n)
  for p,n in((FIRST,WRAPPER),(FIRST+4,0x11110000),(FIRST+8,BASE+0x1800),(SECOND,OTHER),(WRAPPER+12,CHILD),(CHILD,classes[20 if reverse else 23]),(OTHER,classes[23 if reverse else 20]),(CONTEXT,DISPATCH)):put(p,n)
  for a,row in enumerate(table):
   for b,n in enumerate(row):put(DISPATCH+0x18c+(a*32+b)*4,n)
  rows.append(dict(name=('fault_seed_'if seed else'')+name,entry=0x2f0eb0,owner=0x3040d0 if reverse else 0x303c10,receiver=0,arguments=[FIRST,SECOND,CONTEXT,user],first=FIRST,second=SECOND,context=CONTEXT,user_data=user,data=DESCRIPTOR,collector=0,banks=[dict(address=a,before=b.hex())for a,b in sorted(banks.items())],before_fp=fp or[0x27f,0x453f,0x1f80],stop_pc=physical.STOP,boundary=None))
 for reverse in(False,True):
  for rounding in range(4):
   for sse in(0,0x40,0x8000,0x8040):add(f'factory_{reverse}_{rounding}_{sse}',reverse,[0x27f|(rounding<<10),0x453f,0x1f80|(rounding<<13)|sse])
  for user in(0,0xffffffff,0x80000000,0x7f800001,0x7fc12345):add(f'user_{reverse}_{user:x}',reverse,user=user)
  add(f'armed_{reverse}',reverse,seed=True)
 return rows

def observer_source():
 text=sphere.observer_source();old='if(pc==stop||std::find(m.at("boundaries").begin(),m.at("boundaries").end(),pc)!=m.at("boundaries").end())break;';assert old in text
 # The descriptor is ordinary constructor DATA, but its original stack address
 # earlier overlaps private allocator arguments/saved registers. Observe its
 # four semantic publications, not those CPU-frame aliases. Final sixteen
 # bytes remain in the full retained-bank comparison.
 needle='if(!retained(p,n))continue;'
 assert text.count(needle)==1
 text=text.replace(needle,needle+f'if(p>={DESCRIPTOR}u&&std::uint64_t(p)+n<={DESCRIPTOR+16}u&&pc!=0x2f0e2cu&&pc!=0x2f0e30u&&pc!=0x2f0e3du&&pc!=0x2f0e41u)continue;')
 text=text.replace('unsigned count=0;J visited=J::array();','unsigned count=0;J visited=J::array(),source_actions=J::array();bool composed{};std::size_t action_index{};')
 text=text.replace(old,r'''if(pc==stop||std::find(m.at("boundaries").begin(),m.at("boundaries").end(),pc)!=m.at("boundaries").end()){
 if(m.value("compose_unwind",false)&&!composed){composed=true;
  const auto word=[&](std::uint32_t p){const auto b=memory(child.pid,p,4);std::uint32_t n{};std::memcpy(&n,b.data(),4);return n;};
  auto node=word(0x4000000u);unsigned depth{};
  while(node){check(++depth<=3,"Unexpected constructor source frame depth");const auto handler=word(node+4u),state=word(node+8u);const auto key=std::to_string(handler);check(m.at("unwind_actions").contains(key),"Unqualified constructor source handler");check(state==0u||state==0xffffffffu,"Unqualified constructor source state");if(state==0u)source_actions.push_back(J{{"handler",handler},{"state",state},{"entry",m.at("unwind_actions").at(key)},{"frame",node+12u}});node=word(node);}
 }
 if(action_index<source_actions.size()){
  const auto&action=source_actions.at(action_index++);const std::uintptr_t call_sp=0x2000ff0u;const auto previous=std::uint64_t(request(PTRACE_PEEKDATA,child.pid,reinterpret_cast<void*>(call_sp)));request(PTRACE_POKEDATA,child.pid,reinterpret_cast<void*>(call_sp),reinterpret_cast<void*>((previous&0xffffffff00000000ull)|std::uint32_t(stop)));auto call=regs;call.rip=action.at("entry").get<std::uint32_t>();call.rbp=action.at("frame").get<std::uint32_t>();call.rsp=call_sp;request(PTRACE_SETREGS,child.pid,nullptr,&call);continue;
 }break;
}''')
 return text.replace('{"timestamps",timestamps}', '{"timestamps",timestamps},{"source_unwind_actions",source_actions}')

def driver_source():
 text=(ROOT/'tests/native_tu15_sphere_overlap_tests.cpp').read_text().replace('#include "halo2/native_tu15_convex_overlap_query.hpp"','#include "halo2/native_tu15_wrapped_factory_binding.hpp"\n#include "halo2/native_tu15_array_allocator.hpp"').replace('unchanged-tu15-sphere-overlap-v1',BACKEND)
 start=text.index(' for(unsigned depth=1;');end=text.index(' for(const auto&c:fixture.at("cases"))',start)
 text=text[:start]+text[end:]
 text=text.replace('NativeTu15ConvexOverlapQueryServices services;','')
 start=text.index('  services.geometry.timestamp_counter=');end=text.index('  const auto cw=',start)
 text=text[:start]+'''  NativeTu15ArrayAllocatorMemory routing(m);NativeTu15ArrayAllocatorServices owners;
  owners.allocate_owner=[&](const auto&r,auto&){check(r.owner==0x233250u,"Unqualified native pool allocation owner");NativeTu15PoolMemory pool(m,r.receiver,observe);return native_tu15_pool_allocate(pool,r.bytes);};
  owners.release_owner=[&](const auto&r,auto&){check(r.owner==0x233420u,"Unqualified native pool release owner");NativeTu15PoolMemory pool(m,r.receiver,observe);check(native_tu15_pool_release(pool,r.data,r.bytes),"Unqualified native large release");};
  NativeTu15ArrayGrowthServices allocation;
  allocation.allocate=[&](const auto&r,auto&state){check(r.owner==0x149da0u,"Unqualified native allocation router");return native_tu15_route_array_allocation(routing,r.bytes,r.memory_class,state,owners);};
  allocation.release=[&](const auto&r,auto&state){check(r.owner==0x149e00u,"Unqualified native release router");native_tu15_route_array_release(routing,r.data,r.bytes,r.memory_class,state,owners);};
  std::uint32_t result{};NativeTu15WrappedFactoryBindingActivity activity;
''' +text[end:]
 start=text.index('#if defined(HALO2_SPHERE_QUERY_BINDING)',text.index('  try{'));end=text.index('#endif',start)+len('#endif')
 text=text[:start]+'''   const NativeTu15WrappedContactArguments input{[&]{return c.at("first").get<std::uint32_t>();},[&]{return c.at("second").get<std::uint32_t>();},[&]{return c.at("context").get<std::uint32_t>();},[&]{return c.at("user_data").get<std::uint32_t>();}};
   result=native_tu15_create_bound_wrapped_contact_factory(m,{0x2f0eb0u},input,{c.at("data"),1024u},fp,0,allocation,observe,&activity).identity;
''' +text[end:]
 text=text.replace('check(failed==fault_case', 'check((fault_case||c.contains("observer_fault")||result==c.at("boundary_gp")[0])&&activity.unqualified_owners==0,name+" factory result differs");check(failed==fault_case')
 text=text.replace('name+" store count differs"','name+" store count differs native="+std::to_string(stores.size())+" original="+std::to_string(expected.size())')
 text=text.replace('name+" store differs at "+std::to_string(n)', 'name+" store differs at "+std::to_string(n)+" native="+J{{"pc",a.instruction},{"address",a.address},{"before",hex(a.before)},{"written",hex(a.written)}}.dump()+" original="+b.dump()')
 text=text.replace('hex(a.before)==b.at("before").get<std::string>()', '(a.address>=c.at("data").get<std::uint32_t>()&&std::uint64_t(a.address)+a.before.size()<=c.at("data").get<std::uint32_t>()+16u||hex(a.before)==b.at("before").get<std::string>())')
 original_bank_check='for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" bank differs");'
 semantic_bank_check='''for(const auto&b:c.at("banks")){
   auto expected=bytes(b.at("after"));
   if(b.at("address")==c.at("data")){
    auto initial=bytes(b.at("before"));std::vector<bool>published(expected.size());
    for(const auto&e:c.at("writes")){const auto address=e.at("address").get<std::uint32_t>();
     if(address>=c.at("data").get<std::uint32_t>()&&address+e.at("written").get<std::string>().size()/2<=c.at("data").get<std::uint32_t>()+16u)
      for(unsigned n=0;n<e.at("written").get<std::string>().size()/2;++n)published[address-c.at("data").get<std::uint32_t>()+n]=true;
    }
    for(unsigned n=0;n<expected.size();++n)if(!published[n])expected[n]=initial[n];
   }
   check(m.banks.at(b.at("address"))==expected,name+" bank differs at "+std::to_string(b.at("address").get<std::uint32_t>()));
  }'''
 assert original_bank_check in text
 text=text.replace(original_bank_check,semantic_bank_check)
 guards=''' for(auto target:{0u,0x303c11u,0x3040d1u,0xffffffffu}){
  Memory m;m.log_reads=true;NativeTu15PhysicsFrameFp fp{0x27f,0x453f,0x1f80};NativeTu15ArrayGrowthServices allocation;bool called{},refused{};
  allocation.allocate=[&](const auto&,auto&){called=true;return 0u;};NativeTu15WrappedContactArguments input;
  try{native_tu15_create_bound_wrapped_contact_factory(m,{target},input,{0x1000u,1024u},fp,0,allocation);}catch(const std::runtime_error&){refused=true;}
  check(refused&&!called&&m.reads.empty()&&m.banks.empty(),"Unknown wrapped child owner entered allocation or DATA");
 }
'''
 text=text.replace(' std::cout<<fixture.at("cases").size()',guards+' std::cout<<fixture.at("cases").size()')
 return text

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--no-prefixes',action='store_true');a=ap.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],outcomes_supplied=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);before,proof=before_inputs(image);save(a.output/'before-input-provenance.json',proof);physical.observer_source=observer_source;physical.boundary=lambda c,d:None;rows=physical.physical(image,meta,cases(image,before),scratch);assert all(c['boundary_gp'][4]==physical.SP+4 for c in rows);prefixes=[];code=_Code(image,'.text')
  if not a.no_prefixes:
   for seed in(c for c in rows if c['name'].startswith('fault_seed_')):
    for category in('reads','writes'):
     seen=set();occ={}
     for e in seed[category]:
      key=e['address'],e['bytes']if category=='reads'else len(e['written'])//2;occ[key]=occ.get(key,0)+1
      if e['instruction']in seen:continue
      seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(e['instruction']),stop_pc=e['instruction'],compose_unwind=True,fault=dict(address=key[0],bytes=key[1],occurrence=occ[key],access='read'if category=='reads'else'write'));prefixes.append(c)
    seen=set()
    for n,e in enumerate(seed['writes']):
     if e['instruction']in seen:continue
     seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(n),stop_pc=e['instruction']+code.at(e['instruction']).size,compose_unwind=True,observer_fault=n+1);prefixes.append(c)
   for c in prefixes:rows+=physical.physical(image,dict(meta,compose_unwind=True),[c],scratch)
  fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');baseline=project/'baseline/halo2';baseline.mkdir(parents=True);text=(ROOT/HEADER).read_text();assert (text.count(PATCH_OLD),text.count(PATCH_NEW))in((1,0),(0,1));old=text.replace(PATCH_NEW,PATCH_OLD)if PATCH_NEW in text else text;current=text.replace(PATCH_OLD,PATCH_NEW);(baseline/Path(HEADER).name).write_text(old);(project/'include/halo2'/Path(HEADER).name).write_text(current);pins={HEADER:sha(ROOT/HEADER)}
  for n in(*UNITS,'native_tu15_contact_unwind_internal.hpp'):
   src=ROOT/'src/physics'/n;shutil.copyfile(src,project/n);pins[str(src.relative_to(ROOT))]=sha(project/n)
  (project/'driver.cpp').write_text(driver_source());(project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "qualified child factory owner=3161104"in r.stderr,(r.returncode,r.stderr)\nprint("Baseline RED: closed wrapped constructor rejects actual303C10")\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(wrapped_collection LANGUAGES CXX)\ninclude(CTest)\nadd_library(deps OBJECT '+' '.join(UNITS)+')\ntarget_include_directories(deps PRIVATE include)\ntarget_compile_features(deps PRIVATE cxx_std_20)\ntarget_compile_options(deps PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for name in('baseline','candidate'):
   cmake+=f'add_executable(factory_{name} driver.cpp $<TARGET_OBJECTS:deps>)\ntarget_include_directories(factory_{name} PRIVATE '+('baseline 'if name=='baseline'else'')+f'include)\ntarget_compile_features(factory_{name} PRIVATE cxx_std_20)\n';cmd=f'"{sys.executable}" "{project}/negative.py" 'if name=='baseline'else'';cmake+=f'add_test(NAME factory_{name} COMMAND {cmd}$<TARGET_FILE:factory_{name}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1','--target','factory_baseline','--target','factory_candidate','--filter','^factory_'];r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-runner.log')).write_text(r.stdout+r.stderr);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,r.stdout[-1800:]+r.stderr[-1800:]
  pins[str(Path(__file__).relative_to(ROOT))]=sha(__file__);assert all(sha(ROOT/p)==h for p,h in pins.items());visited={pc for c in rows for pc in c['visited']};report.update(passed=True,source_hashes=pins,original_sha256=NEW_PIN,dispatch_census_sha256=sha(CENSUS),body_coverage={hex(n):dict(covered=sum(pc in visited for pc in complete_body(code,n).code),sites=meta['body_sites'][hex(n)])for n in OWNERS},listing_sha256=meta['listing_sha256'],profiles=len(rows),fault_prefixes=sum('fault'in c for c in rows),observer_fault_prefixes=sum('observer_fault'in c for c in rows),compiler_results=results,baseline_red='Current closed wrapped factory rejects qualified actual303C10',private_candidate_green=True,production_changed=False)
 except BaseException as e:report['error']=str(e);raise
 finally:save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
