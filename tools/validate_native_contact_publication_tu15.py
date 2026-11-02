#!/usr/bin/env python3
"""TU1.5 30CD00 cached matches and genuine manager-create entry prefixes."""
import argparse,copy,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
U=base.UI
FIRST,SECOND,CONTEXT,SETTINGS,MANAGER,COUNT,RECORDS,CONTACT,A,B=(U+p for p in(0x400,0x500,0x600,0x700,0x800,0x900,0x1000,0x2000,0x3000,0x3100))
UNITS=('native_tu15_contact_publication.cpp','native_tu15_contact_reduction.cpp','native_tu15_geometry.cpp')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def cases():
 rows=[]
 for count in(0,1,2,4):
  for selected in(-1,0,count-1):
   for radius in(.01,.5,2.):
    data={FIRST+8:A,SECOND+8:B,CONTEXT+12:SETTINGS,SETTINGS+4:bits(radius),MANAGER:0x41f124,COUNT:count}
    for motion,translation in((A,.25),(B,-.5)):
     for row in range(4):
      for lane in range(4):data[motion+32+row*16+lane*4]=bits(translation if row==3 and lane==0 else float(row==lane))
    for n in range(count):
     for block,vector in enumerate(((float(n),0.,0.,.5),(1.,2.,3.,.25),(1.,0.,0.,0.))):
      for lane,value in enumerate(vector):data[RECORDS+n*48+block*16+lane*4]=bits(value)
     data[RECORDS+n*48+28]=0x3f000100+n
    x=99. if selected<0 else float(selected)
    for offset,vector in((0x10,(x,0.,0.,.125)),(0x30,(.5,.25,.125,0.)),(0x40,(1.,0.,0.,0.))):
     for lane,value in enumerate(vector):data[CONTACT+offset+lane*4]=bits(value)
    data[CONTACT+0x20]=bits(.2);data[CONTACT+0x24]=bits(.3)
    prefix=count==0 or selected<0
    row=dict(name=f'publish_{count}_{selected}_{radius}',entry=0x30cd00,receiver=0,arguments=[FIRST,SECOND,CONTEXT,CONTACT,MANAGER,RECORDS,COUNT],user_words=[(p-U,v)for p,v in data.items()])
    if prefix:row.update(stop_boundary=0x322d20,required_owner=True)
    rows.append(row)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x30cd00,0x2e4690)
  source=inspect.getsource(base.metadata).replace('read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)','read=(ins.mnemonic not in("fst","fstp"))and(bool(op.access&CS_AC_READ)or ins.mnemonic in("test","cmp")),write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in("fst","fstp")');ns=dict(base.__dict__);exec(source,ns);base.metadata=ns['metadata']
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}')
  needle='  std::ofstream output(argv[3]);';extra='''  J point_request=nullptr;
  if(std::uint32_t(regs.rip)==0x322d20u){const auto argument=[&](unsigned n){const auto raw=memory(child.pid,std::uint32_t(regs.rsp)+4u+n*4u,4);std::uint32_t value{};for(unsigned k=0;k<4;++k)value|=std::to_integer<unsigned>(raw[k])<<(8u*k);return value;};
    point_request=J{{"owner",0x322d20u},{"manager",std::uint32_t(regs.rcx)},{"first",argument(0)},{"second",argument(1)},{"context",argument(2)},{"packet",hex(memory(child.pid,argument(3),32))}};}
'''
  assert observer.count(needle)==1;observer=observer.replace(needle,extra+needle).replace('{"initial_banks",before}','{"point_request",point_request},{"initial_banks",before}');(scratch/'observer.cpp').write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')").replace('hex(STOP)]','hex(case.get("stop_boundary",STOP))]');ns=dict(base.__dict__);exec(source,ns)
  banks={0x41c500:image.read(0x41c500,16),0x46a560:image.read(0x46a560,4),0x41f124:image.read(0x41f124,20)};rows=cases();fixture,authority=ns['original'](image,banks,rows,scratch);assert all(c['after_esp']==base.SP+4 for c in rows if not c.get('required_owner'))
  full_rows=list(rows);fault_rows=[];seen=set()
  for original in(next(c for c in rows if c['name']==name)for name in('publish_2_0_0.5','publish_4_-1_0.5','publish_0_-1_0.5')):
   for kind in ('reads','writes'):
    occurrences={}
    for event in original[kind]:
     address=event['address'];width=event.get('bytes',len(event.get('written',''))//2);pair=(address,width);occurrences[pair]=occurrences.get(pair,0)+1;signature=(event['instruction'],kind)
     if signature in seen:continue
     seen.add(signature);row={k:copy.deepcopy(original[k])for k in ('entry','receiver','arguments','user_words')};row.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=address,bytes=width,occurrence=occurrences[pair],write=kind=='writes'));fault_rows.append(row)
  prefix_root=scratch/'faults';prefix_root.mkdir();shutil.copyfile(scratch/'observer.cpp',prefix_root/'observer.cpp');fault_fixture,_=ns['original'](image,banks,fault_rows,prefix_root)
  for row in fault_rows:
   for bank in row['banks']:
    for key in ('before_file','after_file'):bank[key]=str(fault_fixture/bank[key])
  rows+=fault_rows
  (fixture/'original.json').write_text(json.dumps(dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows)))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for unit in UNITS:shutil.copyfile(ROOT/'src/physics'/unit,project/unit)
  shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_contact_publication.hpp"\n'+driver;driver=driver.replace('struct Fault{};struct Required{};','constexpr std::uint32_t PRIVATE=0x6000000u;bool private_data(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=4096;}struct Fault{};struct Required{};')
  driver=driver.replace('  void write(std::uint32_t p,','  void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}\n  void write(std::uint32_t p,').replace('if(log_reads)reads.push_back','if(log_reads&&!private_data(p,out.size()))reads.push_back')
  driver=driver.replace('      J actual_stores=J::array(),calls=J::array();','      memory.banks.emplace(PRIVATE,std::vector<std::byte>(4096,std::byte(0xa5)));J actual_stores=J::array(),calls=J::array();').replace('observe=[&](const auto&event){actual_stores.push_back','observe=[&](const auto&event){if(private_data(event.address,event.written.size()))return;actual_stores.push_back')
  driver=driver.replace('      if(read_pass&&!c.value("mutations",J::array()).empty())continue;','      if(read_pass&&!c.value("mutations",J::array()).empty())continue;\n      if(!read_pass&&c.contains("fault")&&!c.at("fault").value("write",false))continue;')
  begin=driver.index('      NativeTu15ArrayGrowthMemory array');end=driver.index('      const auto host_sw=',begin);driver=driver[:begin]+'''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
'''+driver[end:];driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','NativeTu15ContactPublicationStats activity;')
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin);driver=driver[:begin]+'''      NativeTu15ContactPublicationServices services;J request=nullptr;
      services.create_point=[&](auto manager,auto owner,auto first,auto second,auto context,auto packet,auto&)->std::uint16_t{std::array<std::byte,32>b{};memory.read(packet,b);request=J{{"manager",manager},{"owner",owner.identity},{"first",first.identity},{"second",second.identity},{"context",context.identity},{"packet",hex(b)}};throw Required{};};
      try{const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();native_tu15_publish_contact(memory,{args[0]},{args[1]},{args[2]},{args[3]},args[4],args[5],args[6],{PRIVATE},fp,0,services,observe,&activity);
'''+driver[end:];driver=driver.replace('      check(calls==c.at("allocator_calls"),','      check(request==c.at("point_request"),name+" genuine point request differs");\n      check(calls==c.at("allocator_calls"),')
  (project/'driver.cpp').write_text(driver);text=(project/UNITS[0]).read_text();variants={}
  for name,needle,replacement in(('wrong_match','if(odd(compare_top_pop()))','if(!odd(compare_top_pop()))'),('wrong_weight','m.word(0x30cdf9u,target+0xcu,weight);','m.word(0x30cdf9u,target+0xcu,weight^1u);'),('wrong_preserved_id','push(m.word(target+0x1cu));','push(0x3f000000u);')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differs" in r.stderr,(r.returncode,r.stdout,r.stderr)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(publication LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(common OBJECT driver.cpp '+' '.join(UNITS[1:])+')\ntarget_include_directories(common PRIVATE include)\ntarget_compile_features(common PRIVATE cxx_std_20)\ntarget_compile_options(common PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(common PRIVATE nlohmann_json::nlohmann_json)\n'
  for mode in('native',*variants):
   source=UNITS[0]if mode=='native'else mode+'.cpp';target='publish_'+mode;wrapper=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" '
   cmake+=f'add_executable({target} $<TARGET_OBJECTS:common> {source})\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME native_publish_{mode} COMMAND {wrapper}$<TARGET_FILE:{target}> "{fixture}/original.json")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','publish_'+mode]for mode in('native',*variants)),[]),'--filter','^native_publish_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native contact publication differs'
  sites={pc for c in rows for pc in c['executed_sites']if 0x30cd00<=pc<=0x30cf31};report.update(passed=True,profiles=len(rows),full_cached_match_returns=sum(not c.get('required_owner',False)for c in full_rows),genuine_point_creation_prefixes=sum(c.get('required_owner',False)for c in full_rows),operand_fault_prefixes=len(fault_rows),negative_controls=list(variants),parent_sites_covered=len(sites),source_authority=authority,results=results,original_sha256=NEW_PIN,ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),source_hashes={n:sha(ROOT/n)for n in('include/halo2/native_tu15_contact_publication.hpp',*[f'src/physics/{unit}'for unit in UNITS],'src/physics/native_tu15_joint_constraint_numeric_internal.hpp')},scope='Declared coherent current contact/cache/transform DATA, native2E4690 composition, complete cached-match return or genuine322D20 entry. Compares all persistent banks/operands/stores/FP and fully source-initialized typed manager packet32B; no child result or private GP frame supplied. Point-created suffix/reduction/fullsolver/production/live remain open.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
