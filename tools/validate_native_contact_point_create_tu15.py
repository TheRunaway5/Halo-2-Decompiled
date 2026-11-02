#!/usr/bin/env python3
"""Physical TU1.5 manager point creation with guarded event DATA and native children."""
import argparse,copy,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from generate_physics_live_support_tu15 import complete_body
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
UNITS=('native_tu15_contact_point_create.cpp','native_tu15_contact_point_storage.cpp','native_tu15_contact_point_ids.cpp','native_tu15_contact_creation_listeners.cpp','native_tu15_contact_constraint_registration.cpp','native_tu15_contact_collector_info.cpp','native_tu15_pair_creation_queues.cpp','native_tu15_array_growth.cpp','native_tu15_array_reserve.cpp','native_tu15_array_allocator.cpp','native_tu15_island_partition_metadata.cpp','native_tu15_contact_body_update.cpp')
MANAGER,FIRST,SECOND,BODY1,BODY2,MOTION1,MOTION2,WORLD,ISLAND,PACKET,CONTEXT,IDS,POINTS,PROPS,TABLE=(base.UI+p for p in (0x2000,0x400,0x500,0x6000,0x6500,0x7000,0x7800,0x3000,0x4000,0x1000,0x1100,0x8000,0x8800,0x9000,0x300))
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def cases():
 rows=[]
 for count in (0,1,4,251):
  for listeners in (0,2):
   for parameter in (0.,.25,-.5):
    points=min(count,2) if count<251 else 0;c=MANAGER+16
    data={MANAGER:0x41f124,MANAGER+8:WORLD,c:0x41f478,c+4:0x00021234,c+12:BODY1,c+16:BODY2,c+20:ISLAND,c+0x20:IDS,c+0x24:points,c+0x28:0x80000008,c+0x34:POINTS,c+0x38:points,c+0x3c:0x80000008,c+0x50:0,c+0x90:PROPS,c+0x94:count,c+0x98:0x80000008,FIRST+12:0,FIRST+32:BODY1,SECOND+12:0,SECOND+32:BODY2,BODY1+0x3c:MOTION1,BODY2+0x3c:MOTION2,BODY1+0x44:ISLAND,BODY2+0x44:ISLAND,WORLD+0x2c:base.UI+0x4300,WORLD+0x60:0,WORLD+0x64:0,WORLD+0xb4:base.UI+0xa000,WORLD+0xb8:listeners,ISLAND:0x41f080,ISLAND+8:0,ISLAND+12:0,ISLAND+16:0,ISLAND+24:0,ISLAND+0x28:0x100,ISLAND+0x58:base.UI+0xa100,ISLAND+0x5c:0,ISLAND+0x60:0x80000008,MOTION1:TABLE,MOTION2:TABLE,TABLE+0x58:0x323a10,CONTEXT+0x30:bits(.5)}
    for n in range(4):data[IDS+n*4]=0x04030201
    for n in range(2):
     listener=base.UI+0xa300+n*16;data[base.UI+0xa000+n*4]=listener;data[listener]=base.UI+0xa400;data[base.UI+0xa400]=0x2e4d60
    for n,b in enumerate((BODY1,BODY2)):
     data[b+0x60]=base.UI+0xa000;data[b+0x64]=listeners;data[b+0x4c]=bits(.5+n*.5);data[b+0x50]=bits(.25+n*.25)
    for n,m in enumerate((MOTION1,MOTION2)):
     data[m+0x2c]=bits(1.+n)
     for offset,vector in ((0x40,(parameter+n,.25,-.125,0.)),(0x50,(.1,.2,.3,0.)),(0x70,(float(n),.5,-.25,0.))):
      for lane,v in enumerate(vector):data[m+offset+lane*4]=bits(v)
    for offset,vector in ((0,(1.,2.,3.,.125)),(0x10,(1.,0.,0.,0.))):
     for lane,v in enumerate(vector):data[PACKET+offset+lane*4]=bits(v)
    rows.append(dict(name=f'create_{count}_{listeners}_{parameter}',entry=0x322d20,receiver=MANAGER,arguments=[FIRST,SECOND,CONTEXT,PACKET],user_words=[(p-base.UI,v)for p,v in data.items()]))
 seed=next(r for r in rows if r['name']=='create_1_2_0.25')
 for name in ('payload_chains','first_body_only','second_body_only','negative_material','nan_material'):
  row=copy.deepcopy(seed);row['name']=name;words=dict(row['user_words'])
  if name=='payload_chains':words.update({FIRST+12-base.UI:base.UI+0x550,0x550+12:0,0x550+32:BODY1,SECOND+12-base.UI:base.UI+0x580,0x580+12:base.UI+0x5a0,0x5a0+12:0,0x5a0+32:BODY2})
  elif name=='first_body_only':words[BODY2+0x64-base.UI]=0
  elif name=='second_body_only':words[BODY1+0x64-base.UI]=0
  elif name=='negative_material':words[BODY1+0x4c-base.UI]=bits(-1.)
  else:words[BODY2+0x50-base.UI]=0x7f812345
  row['user_words']=list(words.items());rows.append(row)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],allocation_outcomes_supplied=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);banks,provenance=base.reference_inputs(image);base.OWNERS=(0x322d20,0x325f80,0x325f20,0x3200b0,0x2e31e0,0x317350,0x320e40,0x325e90,0x317200,0x322360,0x322440,0x31fe70,0x323a10,0x2e4d60,*base.OWNERS);banks.update({p:image.read(p,n)for p,n in ((0x41f080,20),(0x41f124,20),(0x41f478,44),(0x41f118,4),(0x46a0d0,4),(0x46a35c,4),(0x46a16c,4))})
  metadata=inspect.getsource(base.metadata).replace('read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)','read=(ins.mnemonic not in("fst","fstp"))and(bool(op.access&CS_AC_READ)or ins.mnemonic in("test","cmp")),write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in("fst","fstp")');namespace=dict(base.__dict__);exec(metadata,namespace);base.metadata=namespace['metadata']
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}')
  observer=observer.replace('J writes=J::array(),reads=J::array(),','J listener_calls=J::array();J writes=J::array(),reads=J::array(),')
  observer=observer.replace('    const bool repeated_empty=','''    if(pc==0x2e4d60u){const auto raw=memory(child.pid,registers[4]+4u,4);std::uint32_t event{};for(unsigned k=0;k<4;++k)event|=std::to_integer<unsigned>(raw[k])<<(8u*k);listener_calls.push_back(J{{"owner",pc},{"receiver",registers[1]},{"id",hex(memory(child.pid,event,2))},{"fields",hex(memory(child.pid,event+4u,32))}});}
    const bool repeated_empty=''').replace('{"initial_banks",before}','{"listener_calls",listener_calls},{"initial_banks",before}');(scratch/'observer.cpp').write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')").replace('case.get("count",0)*case["arguments"][1]','case.get("count",0)*case.get("prepare_stride",1)')
  anchor="        assembly.append(f'''.code32";assert source.count(anchor)==1
  prepare='''        if case.get('owned_arrays'):
            prepare=f'mov ${SP-0x100},%esp\\n'
            for header,data,count,stride in case['owned_arrays']:
                prepare+=f'push $0x12\\n push ${count*stride}\\n mov $0x486080,%ecx\\n call 0x149da0\\n mov %eax,{header}\\n mov %eax,%edi\\n mov ${data},%esi\\n mov ${count*stride},%ecx\\n rep movsb\\n'
'''
  source=source.replace(anchor,prepare+anchor);ns=dict(base.__dict__);exec(source,ns)
  rows=cases();fixture,authority=ns['original'](image,banks,rows,scratch);assert all(c['after_esp']==base.SP+20 for c in rows)
  normal_rows=list(rows);fault_rows=[];seen=set()
  for original in(next(c for c in rows if c['name']==name)for name in ('create_0_2_0.25','create_4_2_-0.5')):
   for kind in ('reads','writes'):
    occurrences={}
    for event in original[kind]:
     address=event['address'];width=event.get('bytes',len(event.get('written',''))//2);pair=(address,width);occurrences[pair]=occurrences.get(pair,0)+1;signature=(event['instruction'],kind)
     if signature in seen or not 0x322d20<=event['instruction']<=0x322f62:continue
     seen.add(signature);row={k:copy.deepcopy(original[k])for k in ('entry','receiver','arguments','user_words')};row.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=address,bytes=width,occurrence=occurrences[pair],write=kind=='writes'));fault_rows.append(row)
  fault_root=scratch/'faults';fault_root.mkdir();shutil.copyfile(scratch/'observer.cpp',fault_root/'observer.cpp');fault_fixture,_=ns['original'](image,banks,fault_rows,fault_root)
  for row in fault_rows:
   for bank in row['banks']:
    for key in ('before_file','after_file'):bank[key]=str(fault_fixture/bank[key])
  rows+=fault_rows;(fixture/'original.json').write_text(json.dumps(dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows)))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for unit in UNITS:shutil.copyfile(ROOT/'src/physics'/unit,project/unit)
  shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_contact_point_create.hpp"\n#include "halo2/native_tu15_contact_point_storage.hpp"\n#include "halo2/native_tu15_contact_constraint_registration.hpp"\n#include "halo2/native_tu15_contact_collector_info.hpp"\n#include "halo2/native_tu15_contact_body_update.hpp"\n#include "halo2/native_tu15_island_partition_metadata.hpp"\n'+driver
  driver=driver.replace('      if(read_pass&&!c.value("mutations",J::array()).empty())continue;','      if(read_pass&&!c.value("mutations",J::array()).empty())continue;\n      if(!read_pass&&c.contains("fault")&&!c.at("fault").value("write",false))continue;')
  driver=driver.replace('struct Fault{};struct Required{};','constexpr std::uint32_t PRIVATE=0x6000000u;bool private_data(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=4096;}struct Fault{};struct Required{};')
  driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!private_data(p,out.size()))reads.push_back')
  driver=driver.replace('      J actual_stores=J::array(),calls=J::array();','      memory.banks.emplace(PRIVATE,std::vector<std::byte>(4096,read_pass?std::byte(0x5a):std::byte(0xa5)));J actual_stores=J::array(),calls=J::array();').replace('observe=[&](const auto&event){actual_stores.push_back','observe=[&](const auto&event){if(private_data(event.address,event.written.size()))return;actual_stores.push_back')
  driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','NativeTu15ContactPointCreateStats activity;')
  driver=driver.replace('bool log_reads{};','bool log_reads{};mutable J actual_writes=J::array();')
  driver=driver.replace('const auto i=at(p,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);','const auto i=at(p,in.size());if(!private_data(p,in.size())){const auto begin=i->second.begin()+p-i->first;const std::vector<std::byte>before(begin,begin+in.size());actual_writes.push_back(J{{"address",p},{"before",hex(before)},{"written",hex(in)}});}std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);')
  driver=driver.replace('  void write(std::uint32_t p,','  void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}:REPLACE:  void write(std::uint32_t p,').replace(':REPLACE:','\n')
  driver=driver.replace('if(actual_stores!=c.at("writes"))','J projected=J::array();for(auto event:c.at("writes")){event.erase("instruction");projected.push_back(event);}actual_stores=memory.actual_writes;if(actual_stores!=projected)').replace('actual_stores[i]==c.at("writes")[i]','actual_stores[i]==projected[i]')
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
  driver=driver[:begin]+'''      NativeTu15ContactPointCreateServices services;NativeTu15ContactConstraintRegistrationServices registration;registration.allocation=growth;
      registration.get_info=[&](auto constraint,auto owner,auto info,auto&){if(owner.identity!=0x325e90u)throw Required{};native_tu15_get_contact_collector_info(memory,constraint,{info});};
      registration.add_info=[&](auto island,auto owner,auto,auto info,auto&){if(owner.identity!=0x3200b0u)throw Required{};native_tu15_add_island_constraint_info(memory,{island},{info});};
      services.register_constraint=[&](auto&view,auto world,auto collector,auto info,auto&f){auto scoped=registration;scoped.get_info=[&](auto constraint,auto owner,auto packet,auto&){if(owner.identity!=0x325e90u)throw Required{};native_tu15_get_contact_collector_info(view,constraint,{packet});};scoped.add_info=[&](auto island,auto owner,auto,auto packet,auto&){if(owner.identity!=0x3200b0u)throw Required{};native_tu15_add_island_constraint_info(view,{island},{packet});};native_tu15_register_world_contact_constraint(view,world,collector,info,f,scoped);};
      services.allocate_point=[&](auto&view,auto collector,auto point,auto properties,auto info,auto&f){NativeTu15ContactPointStorageServices storage;storage.allocation=growth;storage.add_info=[&](auto island,auto owner,auto,auto packet,auto&){if(owner.identity!=0x3200b0u)throw Required{};native_tu15_add_island_constraint_info(view,{island},{packet});};return native_tu15_allocate_contact_point_storage(view,collector,point,properties,{info},f,storage);};
      services.point_velocity=[&](auto&view,auto motion,auto owner,auto packet,auto output,auto&f){if(owner.identity!=0x323a10u)throw Required{};native_tu15_contact_motion_point_velocity(view,motion,packet,output,f);};
      J listener_calls=J::array();const auto notify=[&](auto&view,auto owner,auto event,auto&f,bool world){NativeTu15ContactCreationListenerServices listeners;listeners.notify=[&](auto listener,auto target,auto argument,auto&){std::array<std::byte,2>id{};std::array<std::byte,32>fields{};view.read(argument,id);view.read(argument+4u,fields);listener_calls.push_back(J{{"owner",target.identity},{"receiver",listener},{"id",hex(id)},{"fields",hex(fields)}});if(target.identity!=0x2e4d60u)throw Required{};};if(world)native_tu15_notify_world_contact_creation(view,owner,event,f,listeners);else native_tu15_notify_body_contact_creation(view,owner,event,f,listeners);};
      services.notify_world=[&](auto&view,auto owner,auto event,auto&f){notify(view,owner,event,f,true);};
      services.notify_body=[&](auto&view,auto owner,auto event,auto&f){notify(view,owner,event,f,false);};
      try{const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();result=native_tu15_create_contact_point(memory,c.at("receiver"),{args[0]},{args[1]},{args[2]},args[3],{PRIVATE},fp,0,services,{},&activity);
'''+driver[end:]
  needle='      check(listener_calls==c.at("listener_calls"),name+" initialized event fields/listener sequence differs");\n      check(activity.unprovided_event_reads==0,name+" native event read unprovided bytes");\n      check(calls==c.at("allocator_calls"),';driver=driver.replace(needle,'      if(!failed&&!required)check(result==(c.at("return_word").get<std::uint32_t>()&0xffffu),name+" created contact ID differs");\n'+needle)
  guard_model='''      if(!read_pass&&name=="create_1_2_0.25"){
        const auto reset=[&]{memory.banks.clear();for(const auto&bank:c.at("banks"))memory.banks.emplace(bank.at("address"),file(path.parent_path()/bank.at("before_file").get<std::string>()));memory.banks.emplace(PRIVATE,std::vector<std::byte>(4096,std::byte(0x7d)));memory.reads=J::array();memory.actual_writes=J::array();memory.fault=J{};memory.log_reads=true;fp={c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};};
        const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
        reset();NativeTu15ContactPointCreateStats raw;bool refused=false;try{native_tu15_create_contact_point(memory,c.at("receiver"),{args[0]},{args[1]},{args[2]},args[3],{PRIVATE},fp,0,services,[](const auto&){},&raw);}catch(const std::runtime_error&e){refused=std::string(e.what()).find("raw private")!=std::string::npos;}
        check(refused&&raw.refusals==1&&memory.reads.empty()&&memory.actual_writes.empty(),"Raw private observer guard differs");
        const auto saved=services.notify_world;
        for(unsigned offset:{2u,0x10u}){reset();NativeTu15ContactPointCreateStats guard;services.notify_world=[&](auto&view,auto,auto event,auto&){std::array<std::byte,1>b{};view.read(event+offset,b);};refused=false;try{native_tu15_create_contact_point(memory,c.at("receiver"),{args[0]},{args[1]},{args[2]},args[3],{PRIVATE},fp,0,services,{},&guard);}catch(const std::runtime_error&e){refused=std::string(e.what()).find("unprovided DATA")!=std::string::npos;}check(refused&&guard.refusals==1&&guard.unprovided_event_reads==1&&guard.completed==0,"Unprovided event guard differs");}
        services.notify_world=saved;check(host_sw==status()&&host_cw==control()&&host_mx==_mm_getcsr(),"Guard model leaked host FP");
      }
'''
  driver=driver.replace('    }\n    ++cases;faults+=',guard_model+'    }\n    ++cases;faults+=')
  (project/'driver.cpp').write_text(driver);source=(project/UNITS[0]).read_text();mutants={}
  for name,before,after in (('wrong_material','properties+8u);','properties+12u);'),('wrong_relative','subtract(second_velocity,first_velocity)','subtract(first_velocity,second_velocity)'),('wrong_limit','initial_count)>250','initial_count)>252')):
   assert source.count(before)==1;mutants[name]=source.replace(before,after);(project/(name+'.cpp')).write_text(mutants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differs" in r.stderr,(r.returncode,r.stdout,r.stderr)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(contact_ids LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(common OBJECT driver.cpp '+' '.join(UNITS[1:])+')\ntarget_include_directories(common PRIVATE include)\ntarget_compile_features(common PRIVATE cxx_std_20)\ntarget_compile_options(common PRIVATE -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(common PRIVATE nlohmann_json::nlohmann_json)\n'
  for mode in ('native',*mutants):
   unit=UNITS[0]if mode=='native'else mode+'.cpp';target='ids_'+mode;wrapper=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" '
   cmake+=f'add_executable({target} $<TARGET_OBJECTS:common> {unit})\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_link_options({target} PRIVATE -Wl,--gc-sections)\ntarget_compile_options({target} PRIVATE -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME native_ids_{mode} COMMAND {wrapper}$<TARGET_FILE:{target}> "{fixture}/original.json")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in ('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','ids_'+mode]for mode in ('native',*mutants)),[]),'--filter','^native_ids_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native manager point creation differs'
  sites={pc for c in rows for pc in c['executed_sites']if 0x322d20<=pc<=0x322f62};report.update(passed=True,profiles=len(rows),full_returns=len(normal_rows),operand_fault_prefixes=len(fault_rows),negative_controls=list(mutants),parent_sites_covered=len(sites),unexecuted_parent_sites=sorted(set(complete_body(_Code(image,'.text'),0x322d20).code)-sites),source_authority=authority,input_provenance=provenance,results=results,original_sha256=NEW_PIN,ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),source_hashes={n:sha(ROOT/n)for n in ('include/halo2/native_tu15_contact_point_create.hpp',*[f'src/physics/{u}'for u in UNITS])},guard_models=['raw observer refuses before any DATA read/store','event padding2byte guard','current-body field before genuine world writer guard','normal initialized event projection excludes padding'],scope='Declared coherent manager/payload/body/motion/point DATA with sole native registration/storage/velocity/listener children. Complete retained bytes/ordered reads/ordered retained write address-before-data/FP/AX point ID/RET16 compared. Event padding2bytes remains unprovided with same guarded Memory; SourcePC/private raw store observers explicitly unqualified/refused; no World/live claim.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
