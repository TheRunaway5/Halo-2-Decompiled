#!/usr/bin/env python3
"""Physical source proof of actual contact-agent cleanup/destructor closure."""
import argparse,hashlib,inspect,json,os,shutil,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
OWNERS=(0x2f82d0,0x2f6c80,0x2f8090,0x3051a0,0x2f35f0,0x2fc540,0x30ca60,0x2f7e90,0x2f5b30,0x2f3580,0x2f7a10,0x31d400,0x2e4d60,0x149da0,0x149e00,0x233250,0x233420)
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def cases():
 U=base.UI;rows=[]
 def add(name,entry,args,**extra):
  words=[(0x100,U+0x800),(0x104,256),(0x108,U+0x600),(0x10c,U+0x300),(0x110,0),(0x114,0x80000000),(0x118,0),(0x300,U+0x820),(0x600,U+0x840),(0x800,0x2e4d60),(0x820,0x2e4d60),(0x834,0x31d400),(0x848,0x2e4d60),(0x900,0x2e4d60)]
  rows.append(dict(name=name,entry=entry,receiver=U+0x100,arguments=args,user_words=words,**extra))
 for n in(0,1,2,5):
  add(f'cached_points_{n}',0x2f82d0,[],points=n);rows[-1]['user_words'] +=[(0x118,n),*[(0x130+i*48+0x1c,0x8000+i*31)for i in range(n)]]
 add('wrapped_leaf',0x2f6c80,[])
 for owner in(0x2f8090,0x3051a0,0x2f35f0,0x2fc540):
  for flags in(0,1,2,3,0x100,0x101,0xffffffff):add(f'destructor_{owner:x}_flags{flags:x}',owner,[flags],owned_object=bool(flags&1))
 for owner in(0x2f8090,0x3051a0,0x2f35f0):
  for cap in(1,3,8):add(f'destructor_owned_{owner:x}_{cap}',owner,[1],owned_object=True,backing_bytes=cap*(4 if owner==0x2f8090 else 12));rows[-1]['user_words'] +=[(0x114,cap)]
 for cap in(0,1,3,8):
  add(f'collection64_{cap}',0x2f35f0,[1],owned_object=True,object_bytes=64,
      backing_bytes=max(1,cap*12));rows[-1]['user_words'] +=[(0x104,64),(0x114,cap)]
 for owner in(0x2f7e90,0x2f3580):
  add(f'base_borrowed_{owner:x}',owner,[])
  for cap in(0,1,3):add(f'base_owned_{owner:x}_{cap}',owner,[],backing_bytes=max(1,cap*(12 if owner==0x2f3580 else 4)));rows[-1]['user_words'] +=[(0x114,cap)]
 add('bare_base',0x2f7a10,[])
 for name,address,size,writing in (('capacity',U+0x114,4,False),
     ('initial_table',U+0x100,4,True),('size',U+0x104,2,False),
     ('allocator',0x48c938,4,False)):
  add('collection_fault_'+name,0x2f35f0,[1],
      fault=dict(address=address,bytes=size,occurrence=1,write=writing))
 for name,entry,address,size,occurrence in [('wrapper_child',0x2f6c80,U+0x10c,4,1),('wrapper_child_table',0x2f6c80,U+0x300,4,1),('wrapper_child_owner',0x2f6c80,U+0x834,4,1),('wrapper_self_table',0x2f6c80,U+0x100,4,1),('cached_manager',0x2f82d0,U+0x108,4,1),('cached_count',0x2f82d0,U+0x118,4,1),('capacity',0x2f7e90,U+0x114,4,1)]:add('fault_'+name,entry,[],fault=dict(address=address,bytes=size,occurrence=occurrence,write=False))
 add('point_count_current',0x2f82d0,[],mutations=[dict(instruction=0x2e4d60,address=U+0x118,written=(0).to_bytes(4,'little').hex())]);rows[-1]['user_words'] +=[(0x118,4)]
 add('wrapper_self_table_current',0x2f6c80,[],mutations=[dict(instruction=0x31d400,address=U+0x100,written=(U+0x900).to_bytes(4,'little').hex())])
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();files=['include/halo2/native_tu15_agent_cleanup_children.hpp','src/physics/native_tu15_agent_cleanup_children.cpp','include/halo2/native_tu15_agent_cleanup.hpp','src/physics/native_tu15_agent_cleanup.cpp','src/physics/native_tu15_contact_unwind_internal.hpp','src/physics/native_tu15_array_allocator.cpp','src/physics/native_tu15_array_growth.cpp','tools/validate_native_agent_cleanup_children_tu15.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp'];pins={f:sha(ROOT/f)for f in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],supplied_original_outcomes=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);banks,inputs=base.reference_inputs(image);base.OWNERS=OWNERS
  metadata=inspect.getsource(base.metadata);needle="assert not op.mem.segment or ins.reg_name(op.mem.segment)=='es'";assert metadata.count(needle)==1;metadata=metadata.replace(needle,"if op.mem.segment and ins.reg_name(op.mem.segment)=='fs':continue\n                    "+needle);exec(metadata,base.__dict__)
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();needle='if(pc==stop)break;';record='''if(pc==0x31d400u)allocator_calls.push_back(J{{"kind","cleanup"},{"owner",pc},{"receiver",std::uint32_t(regs.rcx)}});if(pc==0x2e4d60u){const auto raw=memory(child.pid,std::uint32_t(regs.rsp)+4u,4);std::uint32_t argument{};for(unsigned n=0;n<4;++n)argument|=std::to_integer<unsigned>(raw[n])<<(8u*n);allocator_calls.push_back(J{{"kind","ret4"},{"owner",pc},{"receiver",std::uint32_t(regs.rcx)},{"argument",argument}});}''';assert observer.count(needle)==1;observer=observer.replace(needle,record+needle).replace('pc==mutation.at("instruction")&&!pending.empty()','pc==mutation.at("instruction")&&(!pending.empty()||pc==0x31d400u||pc==0x2e4d60u)');observer_source=scratch/'observer.cpp';observer_source.write_text(observer)
  source=inspect.getsource(base.original);needle="str(ROOT/'tools/observe_native_array_growth_tu15.cpp')";assert source.count(needle)==1;source=source.replace(needle,'str(observer_source)');begin=source.index("        prepare=''\n");end=source.index('        initial_words=',begin);source=source[:begin]+'''        prepare=''
        if case.get('backing_bytes'):
            prepare+=f'mov ${SP-0x100},%esp\\n push $0x12\\n push ${case["backing_bytes"]}\\n mov $0x486080,%ecx\\n call 0x149da0\\n mov %eax,{UI+0x10c}\\n'
        if case.get('owned_object'):
            prepare+=f'mov ${SP-0x100},%esp\\n push $0x1a\\n push ${case.get("object_bytes",256)}\\n mov $0x486080,%ecx\\n call 0x149da0\\n mov %eax,{UI+0x80}\\n mov %eax,%edi\\n mov ${UI+0x100},%esi\\n mov ${case.get("object_bytes",256)},%ecx\\n rep movsb\\n'
'''+source[end:];source=source.replace("sections=['.image", "sections=['.fs 0x71000000 : { *(.fs) }','.image");source=source.replace("assembly=[f'.section .image", "assembly=['.section .fs,\\\"aw\\\"\\n.zero 4096',f'.section .image")
  source=source.replace(' cld\n fninit',''' cld
 mov $123,%eax
 mov $1,%ebx
 mov $descriptor,%ecx
 mov $16,%edx
 int $0x80
 mov $7,%eax
 mov %ax,%fs
 fninit''');source=source.replace("mov ${case['receiver']},%ecx", "mov {'%d'% (UI+0x80) if case.get('owned_object') else '$%d'%case['receiver']},%ecx");source=source.replace('entry: .long', 'descriptor:.long 0,0x71000000,0xfffff,0x51\nentry: .long');needle="case.update(observed)";assert source.count(needle)==1;source=source.replace(needle,needle+"\n        if case.get('owned_object'):case['receiver']=int.from_bytes(bytes.fromhex(before[UI])[0x80:0x84],'little')");environment=dict(base.__dict__,observer_source=observer_source);exec(source,environment);all_cases=cases();fixture,authority=environment['original'](image,banks,all_cases,scratch);save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=all_cases));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for name in('native_tu15_agent_cleanup_children.cpp','native_tu15_agent_cleanup.cpp','native_tu15_array_allocator.cpp','native_tu15_array_growth.cpp','native_tu15_contact_unwind_internal.hpp'):shutil.copyfile(ROOT/'src/physics'/name,project/name)
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_agent_cleanup_children.hpp"\n'+driver;driver=driver.replace('NativeTu15ArrayGrowthMemory array(memory,{c.value("header",0u)},observe);','');needle='      const auto host_sw=';at=driver.index(needle)
  extra='''      NativeTu15AgentCleanupChildrenServices services;services.allocation=growth;
      const auto mutate=[&](std::uint32_t owner){unsigned n{};for(const auto&change:c.value("mutations",J::array())){if(change.at("instruction")==owner&&applied.insert(n).second){memory.write(change.at("address"),unhex(change.at("written")));++mutations;}++n;}};
      services.agents.cleanup_child=[&](auto child,auto owner,auto&){check(owner.identity==0x31d400u,"Unqualified native cleanup child");calls.push_back(J{{"kind","cleanup"},{"owner",owner.identity},{"receiver",child.identity}});mutate(owner.identity);};
      services.agents.dispose=[&](auto object,auto owner,auto flag,auto&){check(owner.identity==0x2e4d60u,"Unqualified native deleting callback");calls.push_back(J{{"kind","ret4"},{"owner",owner.identity},{"receiver",object},{"argument",flag}});mutate(owner.identity);};
      services.remove_point=[&](auto manager,auto owner,auto point,auto&){check(owner.identity==0x2e4d60u,"Unqualified native point removal");calls.push_back(J{{"kind","ret4"},{"owner",owner.identity},{"receiver",manager},{"argument",point}});mutate(owner.identity);};
''';driver=driver[:at]+extra+driver[at:];begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin);driver=driver[:begin]+'''      try{const auto entry=c.at("entry").get<std::uint32_t>(),receiver=c.at("receiver").get<std::uint32_t>();
        if(entry==0x2f82d0u)native_tu15_cleanup_cached_contact_agent(memory,{receiver},fp,services,observe);
        else if(entry==0x2f6c80u)native_tu15_cleanup_wrapped_contact_agent(memory,{receiver},fp,services);
        else if(entry==0x2f7a10u)native_tu15_destroy_contact_agent_base(memory,{receiver},observe);
        else if(entry==0x2f7e90u||entry==0x2f3580u)native_tu15_destroy_contact_agent_storage(memory,entry==0x2f3580u,{receiver},fp,growth,observe);
        else result=native_tu15_destroy_contact_agent(memory,entry==0x2f8090u?NativeTu15AgentDestructor::owner_2f8090:entry==0x3051a0u?NativeTu15AgentDestructor::owner_3051a0:entry==0x2f35f0u?NativeTu15AgentDestructor::owner_2f35f0:NativeTu15AgentDestructor::owner_2fc540,{receiver},c.at("arguments")[0],fp,services,observe).identity;
'''+driver[end:];driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','');needle='      check(calls==';driver=driver.replace(needle,'      if(!failed&&(c.at("entry")==0x2f8090u||c.at("entry")==0x3051a0u||c.at("entry")==0x2f35f0u||c.at("entry")==0x2fc540u))check(result==c.at("return_word"),name+" scalar deleting destructor result differs");\n'+needle);(project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(agent_cleanup_children LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(cleanup driver.cpp native_tu15_agent_cleanup_children.cpp native_tu15_agent_cleanup.cpp native_tu15_array_allocator.cpp native_tu15_array_growth.cpp)\ntarget_include_directories(cleanup PRIVATE include)\ntarget_compile_features(cleanup PRIVATE cxx_std_20)\ntarget_link_libraries(cleanup PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME cleanup_children COMMAND cleanup "'+str(fixture/'original.json')+'")\n');results=[];commands=[]
  def run(label,compiler):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','cleanup','--filter','^cleanup_children$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];commands.append(cmd);r=subprocess.run(cmd);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
  for compiler in a.compilers:assert run(compiler,compiler)==0,'Native cleanup children differ'
  target=project/'native_tu15_agent_cleanup_children.cpp';text=target.read_text();mutants=[('point_stride','count*48u','count*32u'),('borrowed_storage','if(!(capacity&0x80000000u))','if(false)'),('delete_class','agent.identity,size,0x1au','agent.identity,size,0x12u')]
  for name,before,after in mutants:
   assert text.count(before)==1;target.write_text(text.replace(before,after));assert run('negative-'+name,a.compilers[0])==8,'Mutation must reach a completed comparison test: '+name;negative=json.loads((a.output/('negative-'+name)/'report.json').read_text());assert all(step['exit_code']==0 for step in negative['steps'] if step['name'] in ('configure','build')),'Mutation did not compile';assert 'differs' in (a.output/('negative-'+name)/'tests.log').read_text(),'Mutation did not produce a semantic comparator difference: '+name;target.write_text(text)
  executed={pc for c in all_cases for pc in c['executed_sites']};code=base._Code(image,'.text');report.update(passed=all(sha(ROOT/f)==v for f,v in pins.items()),authority=authority,original_sha256=NEW_PIN,input_provenance=inputs,original_cases=len(all_cases),original_instructions=sum(c['original_instructions']for c in all_cases),ordered_reads=sum(len(c['reads'])for c in all_cases),ordered_stores=sum(len(c['writes'])for c in all_cases),genuine_service_entries=sum(len(c['allocator_calls'])for c in all_cases),fault_prefixes=sum(c['declared_fault']for c in all_cases),callback_input_mutations=sum(c['mutations_applied']for c in all_cases),results=results,commands=commands,negative_controls=[m[0]for m in mutants],body_coverage={hex(owner):dict(total=len(base.complete_body(code,owner).code),executed=len(set(base.complete_body(code,owner).code)&executed))for owner in OWNERS},scope='Declared valid objects and genuinely allocated object/backing ownership; native149E00/233420 release, genuine callbackRET stubs; no sourceSEHCPU model. Actual error-unwind future-use not qualified; service owners explicit.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
