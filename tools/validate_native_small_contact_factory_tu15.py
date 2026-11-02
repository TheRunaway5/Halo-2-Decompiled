#!/usr/bin/env python3
"""Qualify actual2F9990 with unchanged i386 and the genuine native allocator.

First three ordinary formals are intentionally unread by this source owner.
Only the fourth is consumed after real allocation. Compare integer/FP state,
all pool banks, ordered retained reads/stores and real allocator requests.
No allocator outcome, original AFTER bank or emulated instruction is supplied.
"""
import argparse,hashlib,json,os,re,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,d):p.write_text(json.dumps(d,indent=2)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--handoff',action='store_true',help='Qualify the actual2F7293 cdecl caller adapter as well');p.add_argument('--bound-pool',action='store_true',help='Use the production native pool binding; callback journal remains covered by the direct component proof');a=p.parse_args();assert not a.bound_pool or a.handoff;a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 report=dict(passed=False,scope=__doc__,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);banks,provenance=base.reference_inputs(image)
  base.OWNERS=(0x2f9990,0x149da0,0x233250)
  cases=[]
  def add(name,user=0x12345678,**more):cases.append(dict(name=name,entry=0x2f9990,receiver=0xbad0bad0,arguments=[0xffffffff,0x7f800001,0xdeadbeef,user],retained_arguments=True,**more))
  for value in(0,1,0xffffffff,0x80000000,0x7f800001,0x7fc12345):add('user_'+hex(value),value)
  # The original wrapper varies x87 rounding/status and SSE rounding. This
  # integer owner preserves those fields and the host FP environment.
  for n in range(16):add('controls_'+str(n))
  fixture,authority=base.original(image,banks,cases,scratch)
  allocation=cases[0]['return_word'];assert allocation
  def fault(name,address,size=4,write=False):add('fault_'+name,fault=dict(address=address,bytes=size,occurrence=1,write=write))
  fault('allocator_global',0x48c938);fault('allocator_table',0x486080);fault('allocator_method',0x45f4dc);fault('late_formal',base.SP+16)
  for name,offset,size in(('size',4,2),('user',8,4),('table',0,4),('sentinel',12,2)):fault(name,allocation+offset,size,True)
  mutation=lambda pc,value:dict(instruction=pc,address=base.SP+16,written=struct.pack('<I',value).hex())
  add('source_current_formal_after_allocation',mutations=[mutation(0x23328f,0x87654321)])
  add('cached_formal_after_size',mutations=[mutation(0x2f99a3,0xaabbccdd)])
  # Each independent ordinary caller starts from the same real BEFORE pool.
  # Rebuild all fixtures together so the original bridge varies FP controls
  # across the complete case set and retains the declared formal stack bytes.
  shutil.rmtree(scratch/'original');shutil.rmtree(fixture)
  fixture,authority=base.original(image,banks,cases,scratch)
  input_path=fixture/'original.json';save(input_path,dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-small-contact-factory-v1',cases=cases))
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text()
  driver='#include "halo2/native_tu15_contact_agent_factory.hpp"\n'+driver
  driver=driver.replace('unchanged-tu15-physical-array-and-real-pool-v1','unchanged-tu15-physical-small-contact-factory-v1')
  needle='if(entry==0x2e3250u)native_tu15_grow_array(array,args[1],fp,growth,&activity);';assert driver.count(needle)==1
  driver=driver.replace(needle,'''if(entry==0x2f9990u){
          const auto user=[&]{std::array<std::byte,4>b{};memory.read(0x200f010u,b);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;};
          result=native_tu15_create_contact_agent(memory,NativeTu15ContactAgentFactory::owner_2f9990,NativeTu15ContactAgentFactoryArguments{user},fp,growth,observe).identity;
        }else '''+needle)
  driver=driver.replace('c.at("entry")==0x233250u||','c.at("entry")==0x2f9990u||c.at("entry")==0x233250u||')
  if a.handoff:
   driver='#include "halo2/native_tu15_small_contact_factory_handoff.hpp"\n'+driver
   driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&p!=0x200f000u)reads.push_back')
   marker='result=native_tu15_create_contact_agent(memory,NativeTu15ContactAgentFactory::owner_2f9990,NativeTu15ContactAgentFactoryArguments{user},fp,growth,observe).identity;'
   assert driver.count(marker)==1
   driver=driver.replace(marker,'''(void)user;
          memory.banks.emplace(0x200f000u,std::vector<std::byte>{std::byte{0x95},std::byte{0x72},std::byte{0x2f},std::byte{0}});
          SourcePhysicsContactRuntimeAbi abi{};abi.gp={11u,22u,33u,44u,0x200f000u,55u,66u,77u};abi.eflags=0x202u;abi.fs_base=0x12340000u;abi.x87_status=fp.status;abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};const auto before=abi.gp;
          NativeTu15SmallContactFactoryActivity calls;
          check(native_tu15_handoff_small_contact_factory(memory,{0x2f7293u,0x2f9990u,false},abi,{0x2000000u,0x2010000u,true},growth,observe,&calls),"Small factory caller was not owned");
          result=abi.gp[0];auto expected=before;expected[0]=result;expected[4]+=4u;
          check(abi.gp==expected&&abi.eflags==0x202u&&abi.fs_base==0x12340000u,"Small factory cdecl result/control differs");
          check(calls.attempts==1&&calls.completed==1&&calls.refusals==0,"Small factory handoff activity differs");
          fp={abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};''')
   marker='check(cases>20,"Insufficient physical array coverage");';assert driver.count(marker)==1
   controls='''
  for(unsigned input=0;input<11;++input){
    Memory memory;memory.banks.emplace(0x200f000u,std::vector<std::byte>{std::byte{0x95},std::byte{0x72},std::byte{0x2f},std::byte{0}});
    SourcePhysicsContactRuntimeAbi a{};a.gp[4]=0x200f000u;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27fu,0x1f80u};
    SourcePhysicsContactRuntimeCall c{0x2f7293u,0x2f9990u,false};NativeTu15SmallContactFactoryPolicy p{0x2000000u,0x2010000u,true};bool trace=true;
    switch(input){case 0:c.instruction++;break;case 1:c.tail=true;break;case 2:memory.banks.at(0x200f000u)[0]=std::byte{0x96};break;case 3:p.observers_read_only=false;break;case 4:p.first_trace_active=&trace;break;case 5:p.second_trace_active=&trace;break;case 6:a.x87_depth=1;break;case 7:a.floating_point.reset();break;case 8:a.active_mmx=SourcePhysicsMmxAlias{};break;case 9:p.stack_first=0x200f001u;break;case 10:p.stack_end=0x200f013u;break;}
    const auto before=a.gp;const auto data=memory.banks;bool entered{},refused{};NativeTu15ArrayGrowthServices allocation;allocation.allocate=[&](const auto&,auto&){entered=true;return 0u;};NativeTu15SmallContactFactoryActivity activity;
    try{native_tu15_handoff_small_contact_factory(memory,c,a,p,allocation,{},&activity);}catch(const std::runtime_error&){refused=true;}
    check(refused&&!entered&&a.gp==before&&memory.banks==data&&activity.attempts==1&&activity.refusals==1&&!activity.completed,"Invalid small factory caller entered work");
  }
'''
   driver=driver.replace(marker,controls+marker)
  if a.bound_pool:
   driver='#include "halo2/native_tu15_pool_array_services.hpp"\n'+driver
   driver=driver.replace('NativeTu15SmallContactFactoryActivity calls;', 'NativeTu15PoolArrayServiceActivity pool_activity;const auto production=native_tu15_make_pool_array_services(memory,observe,&pool_activity);NativeTu15SmallContactFactoryActivity calls;')
   driver=driver.replace('0x2010000u,true},growth,observe,&calls)', '0x2010000u,true},production,observe,&calls)')
   marker='check(calls.attempts==1&&calls.completed==1&&calls.refusals==0,"Small factory handoff activity differs");';assert driver.count(marker)==1
   driver=driver.replace(marker,marker+'check(pool_activity.allocation_requests==1&&pool_activity.pool_allocations==1&&!pool_activity.refusals,"Production pool factory ownership differs");')
   marker='check(calls==c.at("allocator_calls"),name+" actual allocator request order differs");';assert driver.count(marker)==1;driver=driver.replace(marker,'// Direct factory/handoff mode checks the callback journal; this mode checks production native pool ownership and its full operands/stores/banks.')
  project=scratch/'project';project.mkdir();(project/'include').mkdir();(project/'driver.cpp').write_text(driver)
  units=('native_tu15_contact_agent_factory.cpp','native_tu15_array_allocator.cpp','native_tu15_array_growth.cpp')
  pending=[ROOT/'src/physics'/n for n in units];pending.append(ROOT/'include/halo2/native_tu15_contact_agent_factory.hpp');pins={}
  if a.handoff:pending.append(ROOT/'include/halo2/native_tu15_small_contact_factory_handoff.hpp')
  if a.bound_pool:pending.append(ROOT/'include/halo2/native_tu15_pool_array_services.hpp')
  while pending:
   source=pending.pop();name=str(source.relative_to(ROOT))
   if name in pins:continue
   pins[name]=sha(source);target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target);assert sha(target)==pins[name]
   for include in re.findall(r'^#include "([^"]+)"',source.read_text(),re.M):
    child=ROOT/'include'/include if include.startswith('halo2/')else source.parent/include
    if child.is_file():pending.append(child)
  # The generated driver uses this public header directly as well.
  shutil.copyfile(ROOT/'tests/native_tu15_array_growth_tests.cpp',scratch/'input-driver.cpp')
  kernel=project/'src/physics/native_tu15_contact_agent_factory.cpp';text=kernel.read_text()
  mutations={
   'wrong_allocation_size':('owner,16u,0x1au','owner,32u,0x1au'),
   'wrong_size_word':('p+4u,0x10u,2','p+4u,0x20u,2'),
   'wrong_sentinel':('0x2f99b2u,p+0xcu,0xffffu,2','0x2f99b2u,p+0xcu,0u,2'),
   'early_formal':('const NativeTu15ContactAgent agent{s.allocate({allocator,owner,16u,0x1au},fp)};\n  if(!arguments.user_data)', 'const auto early_user=arguments.user_data();\n  const NativeTu15ContactAgent agent{s.allocate({allocator,owner,16u,0x1au},fp)};\n  if(!arguments.user_data)'),
  }
  for name,(old,new)in mutations.items():
   assert text.count(old)==1,name;variant=text.replace(old,new)
   if name=='early_formal':variant=variant.replace('const auto user_data=arguments.user_data(),p=agent.identity;','const auto user_data=early_user,p=agent.identity;',1)
   (project/(name+'.cpp')).write_text(variant)
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(s in r.stderr for s in ("array read order differs","array store differs","allocator request order differs","source array completion differs","allocation result differs","complete array/pool bank differs")),r.stderr\nprint("Original factory rejects semantic mutation")\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(SmallContactFactory LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(driver OBJECT driver.cpp)\nadd_library(dependencies OBJECT src/physics/native_tu15_array_allocator.cpp src/physics/native_tu15_array_growth.cpp)\nforeach(target driver dependencies)\n target_include_directories(${target} PRIVATE include)\n target_compile_features(${target} PRIVATE cxx_std_20)\n target_compile_options(${target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nendforeach()\ntarget_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)\n'
  for name in('native',*mutations):
   source='src/physics/native_tu15_contact_agent_factory.cpp'if name=='native'else name+'.cpp'
   cmake+=f'add_executable(factory_{name} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> {source})\ntarget_include_directories(factory_{name} PRIVATE include src/physics)\ntarget_compile_features(factory_{name} PRIVATE cxx_std_20)\n'
   prefix=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" '
   cmake+=f'add_test(NAME small_factory_{name} COMMAND {prefix}$<TARGET_FILE:factory_{name}> "{input_path}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1','--target','factory_native',*sum((['--target','factory_'+n]for n in mutations),[]),'--filter','^small_factory_']
   r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-runner.log')).write_text(r.stdout+r.stderr);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,r.stdout[-2000:]+r.stderr[-2000:]
  assert all(sha(ROOT/n)==pin for n,pin in pins.items()),'Source changed during qualification'
  report.update(passed=True,cases_per_compiler=len(cases),fault_prefixes=sum(c.get('declared_fault',False)for c in cases),observer_changes=2,mutants_per_compiler=list(mutations),results=results,source_hashes=pins,original_authority=authority,allocator_provenance=provenance,all_original_instructions_unchanged=True,handoff_compared=a.handoff,invalid_handoff_controls=11 if a.handoff else 0,production_pool_binding=a.bound_pool,allocator_callback_journal_compared=not a.bound_pool,full_game_closure_claimed=False)
 except BaseException as error:report['error']=str(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
