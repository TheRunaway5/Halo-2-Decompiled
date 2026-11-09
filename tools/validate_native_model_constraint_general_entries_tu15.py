#!/usr/bin/env python3
"""Real TU1.5 general20D8B0/22E0D0 DATA, alias, fault and observer proof."""
import argparse,copy,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
STAGE=ROOT/'analysis/physics/native-model-constraint-general-entries-stage-20261103/staged'
REQUEST='src/physics/native_tu15_body_constraint_update.cpp'
LOOKUP='src/physics/native_tu15_model_constraint_lookup.cpp'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,d):Path(p).write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')

def cases():
 u=base.UI;rows=[]
 for mode in (0,1,255):
  for descriptor in (0,u+0x6200):
   for bits in (0,0x3f800000,0x7f812345,0xffffffff):
    words=[(0x6000+i*4,(bits^i*0x10001)&0xffffffff)for i in range(3)]+[(0x6100+i*4,(0x80000000^(bits+i))&0xffffffff)for i in range(3)]+[(0x6200,0x1234ffff)]
    rows.append(dict(name=f'request_{mode}_{descriptor:x}_{bits:x}',entry=0x20d8b0,eax=u+0x5000,edx=mode,receiver=u+0x6100,
      arguments=[0x12345678,0x87654321,0xfffe,0xabcdef01,0x76543210,0x8001,u+0x6000,0x87654321,descriptor],user_words=words))
 seed=next(r for r in rows if r['name']==f'request_1_{u+0x6200:x}_3f800000')
 for target in range(0,64,2):
  for kind in ('position','forward','descriptor'):
   row=copy.deepcopy(seed);row['name']=f'alias_{kind}_{target}'
   if kind=='position':row['arguments'][6]=row['eax']+target
   elif kind=='forward':row['receiver']=row['eax']+target
   else:row['arguments'][8]=row['eax']+target
   rows.append(row)
 def mutation(pc,p,value):return dict(instruction=pc,address=p,written=struct.pack('<I',value).hex())
 for name,pc,p in (('position_before',0x20d8b0,u+0x6000),('position_middle',0x20d8eb,u+0x6004),('forward_last',0x20d903,u+0x6108),('descriptor_late',0x20d91a,u+0x6200)):
  row=copy.deepcopy(seed);row.update(name='observer_'+name,mutations=[mutation(pc,p,0x10203040)]);rows.append(row)
 # Source components and header identities remain ordinary DATA; neither
 # a biped classification nor a visibility result is supplied to the kernel.
 for first in (0xffffffff,1):
  for second in (0xffffffff,2):
   for type_a in (0,1,32,255):
    for type_b in (0,1,32,255):
     for flags in (0,0x800,0x80000000,0x80000800):
      words=[(0x1044,u+0x4000),(0x2044,u+0x3000),
       (0x3000+160+8,3),(0x3000+320+8,5),(0x3000+160+4,flags),(0x3000+320+4,flags^0x800),
       (0x4000+3*12,(type_a<<24)|0x1234),(0x4000+5*12,(type_b<<24)|0x5678)]
      rows.append(dict(name=f'visible_{first:x}_{second:x}_{type_a}_{type_b}_{flags:x}',entry=0x22e0d0,eax=first,edx=0,receiver=0,arguments=[second],user_words=words))
 return rows

def driver():
 prefix=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text().split('int main(int argc,char**argv)')[0]
 return '#include "halo2/native_tu15_model_constraint_lookup.hpp"\n'+prefix+r'''
struct RequestData final:NativeTu15ModelConstraintRequestData{
 Memory&memory;std::uint32_t position,forward,descriptor;
 RequestData(Memory&m,std::uint32_t p,std::uint32_t f,std::uint32_t d):memory(m),position(p),forward(f),descriptor(d){}
 std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b;memory.read(p,b);std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
 std::uint32_t position_word(unsigned n)const override{return word(position+n*4u);}
 std::uint32_t forward_word(unsigned n)const override{return word(forward+n*4u);}
 std::uint32_t descriptor_word()const override{check(descriptor!=0,"Null descriptor was read");return word(descriptor);}
};
int main(int argc,char**argv)try{
 check(argc==2||argc==4,"Supply general request/visibility fixture");const bool mutant=argc==4;unsigned completed{},data_changes{},result_changes{},cases{},read_count{},stores_count{};
 const auto path=std::filesystem::path(argv[1]);std::ifstream input(path);const auto fixture=J::parse(input);
 for(const auto&c:fixture.at("cases"))for(bool read_pass:{false,true}){
  if(mutant&&(read_pass||c.contains("fault")||c.contains("mutations")))continue;
  if(read_pass&&c.contains("mutations"))continue;
  if(!read_pass&&c.contains("fault")&&!c.at("fault").at("write").get<bool>())continue;
  Memory memory;for(const auto&b:c.at("banks"))memory.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));memory.fault=c.value("fault",J{});memory.log_reads=read_pass;
  J stores=J::array();std::set<unsigned>applied;SourcePhysicsContactRecordObserver observer;
  if(!read_pass)observer=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});unsigned index{};for(const auto&m:c.value("mutations",J::array())){if(m.at("instruction")==e.instruction&&applied.insert(index).second){const auto bytes=unhex(m.at("written"));memory.write(m.at("address"),bytes);}++index;}};
  const auto sw=status(),cw=control();const auto mx=_mm_getcsr();bool failed{};std::uint32_t result{};
  try{const auto&a=c.at("arguments");if(c.at("entry")==0x20d8b0u){
    NativeTu15ModelConstraintRequestFields fields;
    fields.mode=std::uint8_t(c.at("edx").get<std::uint32_t>());fields.first_datum=a[0];fields.first_slot=a[1];fields.first_node=a[2].get<std::uint16_t>();
    fields.second_datum=a[3];fields.second_slot=a[4];fields.second_node=a[5].get<std::uint16_t>();fields.constraint_class=a[7];fields.has_descriptor=a[8].get<std::uint32_t>()!=0;
    RequestData data(memory,a[6],c.at("receiver"),a[8]);native_tu15_initialize_model_constraint_request(memory,{c.at("eax")},fields,data,observer);
   }else{NativeTu15ModelConstraintLookupStats stats;result=native_tu15_model_constraint_bodies_visible(memory,c.at("eax"),a[0],&stats);check(stats.body_visibility_queries==1,"Visibility query counter differs");}
  }catch(const Fault&){failed=true;}
  const auto name=c.at("name").get<std::string>();check(failed==c.value("declared_fault",false),name+" general completion differs");
  check(sw==status()&&cw==control()&&mx==_mm_getcsr(),name+" hardware FP leaked");check(c.at("before_fp")==c.at("after_fp"),name+" original integer FP changed");
  if(mutant){++completed;bool changed{};for(const auto&b:c.at("banks"))changed|=memory.banks.at(b.at("address"))!=file(path.parent_path()/b.at("after_file").get<std::string>());data_changes+=changed;result_changes+=c.at("entry")==0x22e0d0u&&result!=c.at("return_word");continue;}
  if(!failed&&c.at("entry")==0x22e0d0u)check(result==c.at("return_word"),name+" visibility result differs");
  if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));
   if(memory.reads!=expected){std::size_t n{};while(n<memory.reads.size()&&n<expected.size()&&memory.reads[n]==expected[n])++n;throw std::runtime_error(name+" general read order differs at "+std::to_string(n)+" native="+(n<memory.reads.size()?memory.reads[n].dump():"end")+" original="+(n<expected.size()?expected[n].dump():"end"));}read_count+=memory.reads.size();}
  else{check(stores==c.at("writes"),name+" general attributed stores differ");check(applied.size()==c.value("mutations",J::array()).size(),name+" observer mutation phase missing");stores_count+=stores.size();}
  for(const auto&b:c.at("banks"))check(memory.banks.at(b.at("address"))==file(path.parent_path()/b.at("after_file").get<std::string>()),name+" complete general bank differs");++cases;
 }
 if(mutant){check(completed>200,"Insufficient completed general mutant profiles");check(data_changes+result_changes>0,"Mutant did not change actual result/DATA");std::filesystem::create_directories("evidence");std::ofstream out(std::string("evidence/")+argv[3]+".json");out<<J{{"passed",true},{"completed",completed},{"data_mismatches",data_changes},{"result_mismatches",result_changes},{"refusals",0}}.dump()<<'\n';check(bool(out),"Cannot retain general mutant result");}
 else std::cout<<J{{"passed",true},{"cases",cases},{"ordered_reads",read_count},{"ordered_stores",stores_count}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 files=[*STAGE.rglob('*.hpp'),STAGE/REQUEST,STAGE/LOOKUP,Path(__file__),ROOT/'tools/validate_native_array_growth_tu15.py',ROOT/'tools/observe_native_array_growth_tu15.cpp',ROOT/'tests/native_tu15_array_growth_tests.cpp'];pins={str(f.relative_to(ROOT)):sha(f)for f in files}
 report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],native_admission_added=False,source_outcomes_supplied=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x20d8b0,0x22e0d0)
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}');(scratch/'observer.cpp').write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')")
  source=source.replace(" mov ${case['receiver']},%ecx\n int3"," mov ${case['receiver']},%ecx\n mov ${case['eax']},%eax\n mov ${case['edx']},%edx\n int3")
  ns=dict(base.__dict__);exec(source,ns);original=ns['original'];rows=cases();banks={0x4ecbd0:struct.pack('<I',base.UI+0x1000),0x52be5c:struct.pack('<I',base.UI+0x2000)}
  fixture,authority=original(image,banks,rows,scratch);normal=list(rows)
  for r in normal:assert r['after_esp']==base.SP+(40 if r['entry']==0x20d8b0 else 8)
  faults=[];seen=set()
  for sample in normal:
   for kind in ('reads','writes'):
    occurrences={}
    for event in sample[kind]:
     width=event.get('bytes',len(event.get('written',''))//2);key=(event['address'],width);occurrences[key]=occurrences.get(key,0)+1
     if(event['instruction'],kind)in seen:continue
     seen.add((event['instruction'],kind));row={k:copy.deepcopy(v)for k,v in sample.items()if k in ('name','entry','receiver','arguments','eax','edx','user_words')};row.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=event['address'],bytes=width,occurrence=occurrences[key],write=kind=='writes'));faults.append(row)
  fs=scratch/'faults';fs.mkdir();shutil.copyfile(scratch/'observer.cpp',fs/'observer.cpp');ff,_=original(image,banks,faults,fs)
  for r in faults:
   assert r['declared_fault']
   for b in r['banks']:
    for key in ('before_file','after_file'):b[key]=str(ff/b[key])
  rows+=faults;save(fixture/'original.json',dict(image_sha256=NEW_PIN,cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for f in STAGE.rglob('*.hpp'):shutil.copyfile(f,project/'include/halo2'/f.name)
  shutil.copyfile(STAGE/REQUEST,project/'request.cpp');shutil.copyfile(STAGE/LOOKUP,project/'lookup.cpp');(project/'driver.cpp').write_text(driver())
  changes=(('null_descriptor','request.cpp','store(0x20d929u,p+0x3cu,0xffffu,2)','store(0x20d929u,p+0x3cu,0xfffeu,2)'),('descriptor_width','request.cpp','store(0x20d935u,p+0x3au,data.descriptor_word())','store(0x20d935u,p+0x3au,data.descriptor_word(),2)'),('visibility_flag','lookup.cpp','&0x800u','&0x400u'))
  targets=[('general','request.cpp','lookup.cpp','')]
  for name,file,before,after in changes:
   text=(project/file).read_text();assert text.count(before)==(2 if name=='visibility_flag'else 1);target='mutant_'+name+'.cpp';(project/target).write_text(text.replace(before,after));targets.append(('general_'+name,target if file=='request.cpp'else'request.cpp',target if file=='lookup.cpp'else'lookup.cpp',' --mutant '+name))
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(generalEntries LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)\n'
  for name,request,lookup,argv in targets:
   cmake+=f'add_executable({name} $<TARGET_OBJECTS:driver> {request} {lookup})\ntarget_include_directories({name} PRIVATE include)\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_options({name} PRIVATE -Wl,--gc-sections)\nadd_test(NAME {name} COMMAND {name} "{fixture/"original.json"}"{argv})\n'
  cmake+='add_custom_target(general_all DEPENDS '+' '.join(t[0]for t in targets)+')\n';(project/'CMakeLists.txt').write_text(cmake);compiled={str(f.relative_to(project)):sha(f)for f in project.rglob('*')if f.is_file()}
  for compiler in ('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','general_all','--filter','^general','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--retain-report','evidence/'+name+'.json']for name,_,_,_ in changes),[])]
   r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-console.log')).write_text((r.stdout+r.stderr)[-65536:]);report.setdefault('results',[]).append(dict(compiler=compiler,command=command,exit_code=r.returncode));assert r.returncode==0,(r.stdout+r.stderr)[-6000:]
  assert all(sha(ROOT/f)==h for f,h in pins.items())and all(sha(project/f)==h for f,h in compiled.items())
  code=_Code(image,'.text');executed={pc for row in rows for pc in row['executed_sites']}
  report.update(passed=True,profiles=len(rows),full_returns=len(normal),fault_prefixes=len(faults),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),source_body_coverage={hex(o):dict(total=len(base.complete_body(code,o).code),executed=len(set(base.complete_body(code,o).code)&executed))for o in base.OWNERS},authority=authority,compiled_source_pins=compiled,reference_runner_sha256=hashlib.sha256(source.encode()).hexdigest(),semantic_mutants=[c[0]for c in changes],source_inputs_unchanged=True,scope=__doc__)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(f.stat().st_blocks*512 for f in scratch.rglob('*')if f.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()
