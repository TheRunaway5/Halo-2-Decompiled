#!/usr/bin/env python3
"""Physical1D5790/1E15D0 transition DATA and signed byte state proof."""
import argparse,copy,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
STAGE=ROOT/'analysis/physics/native-object-update-stage-v2-20261103/staged'
UNIT='src/physics/native_tu15_object_transition.cpp';HEADER='include/halo2/native_tu15_object_transition.hpp'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,d):Path(p).write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
def packed(current,bound,flags):return current|(bound<<8)|(0x7a<<16)|(flags<<24)
def cases():
 rows=[];u=base.UI
 for current in (0,1,2,126,127,128,129,254,255):
  for bound in (0,1,2,126,127,128,129,254,255):
   for flags in (0,1,2,3,0x80,0x81,0xfe,0xff):
    rows.append(dict(name=f'counter_{current}_{bound}_{flags}',entry=0x1e15d0,receiver=u+0x100,esi=0,arguments=[],user_words=[(0x100,packed(current,bound,flags))]))
 for current in (0,1,127,128,255):
  for bound in (0,1,128):
   for flags in (0,1,2):
    for other in (0,1,2):
     rows.append(dict(name=f'transition_{current}_{bound}_{flags}_{other}',entry=0x1d5790,receiver=0,esi=u+0x200,arguments=[],user_words=[(0x264,packed(current,bound,flags)),(0x260,packed(255-current,2,other))]))
 seed=next(r for r in rows if r['name']=='transition_0_1_1_1')
 for pc,p in ((0x1e15ed,u+0x265),(0x1e15f6,u+0x264),(0x1d57a3,u+0x260),(0x1d57a6,u+0x267)):
  row=copy.deepcopy(seed);row.update(name=f'observer_{pc:x}',mutations=[dict(instruction=pc,address=p,written='01020304')]);rows.append(row)
 return rows
def driver():
 prefix=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text().split('int main(int argc,char**argv)')[0]
 return '#include "halo2/native_tu15_object_transition.hpp"\n'+prefix+r'''
int main(int argc,char**argv)try{
 check(argc==2||argc==4,"Supply transition fixture");const bool mutant=argc==4;unsigned completed{},changes{},cases{};const auto path=std::filesystem::path(argv[1]);std::ifstream in(path);const auto fixture=J::parse(in);
 for(const auto&c:fixture.at("cases"))for(bool read_pass:{false,true}){
  if(mutant&&(read_pass||c.contains("fault")||c.contains("mutations")))continue;if(read_pass&&c.contains("mutations"))continue;
  if(!read_pass&&c.contains("fault")&&!c.at("fault").at("write").get<bool>())continue;
  Memory memory;for(const auto&b:c.at("banks"))memory.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));memory.fault=c.value("fault",J{});memory.log_reads=read_pass;
  J stores=J::array();std::set<unsigned>applied;SourcePhysicsContactRecordObserver observer;
  if(!read_pass)observer=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});unsigned n{};for(const auto&m:c.value("mutations",J::array())){if(m.at("instruction")==e.instruction&&applied.insert(n).second){auto bytes=unhex(m.at("written"));memory.write(m.at("address"),bytes);}++n;}};
  const auto sw=status(),cw=control();const auto mx=_mm_getcsr();bool failed{},result{};
  try{if(c.at("entry")==0x1d5790u)result=native_tu15_update_object_transition(memory,{c.at("esi")},observer);else result=native_tu15_advance_object_transition_counter(memory,c.at("receiver"),observer);}catch(const Fault&){failed=true;}
  const auto name=c.at("name").get<std::string>();check(failed==c.value("declared_fault",false),name+" completion differs");check(sw==status()&&cw==control()&&mx==_mm_getcsr(),name+" hardwareFP leaked");check(c.at("before_fp")==c.at("after_fp"),name+" original integer FP changed");
  if(mutant){++completed;bool changed=result!=(c.at("return_word").get<std::uint32_t>()&255u);for(const auto&b:c.at("banks"))changed|=memory.banks.at(b.at("address"))!=file(path.parent_path()/b.at("after_file").get<std::string>());changes+=changed;continue;}
  if(!failed)check(result==(c.at("return_word").get<std::uint32_t>()&255u),name+" boolean result differs");
  if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));check(memory.reads==expected,name+" read order differs");}
  else{check(stores==c.at("writes"),name+" SourcePC stores differ");check(applied.size()==c.value("mutations",J::array()).size(),name+" observer phase missing");}
  for(const auto&b:c.at("banks"))check(memory.banks.at(b.at("address"))==file(path.parent_path()/b.at("after_file").get<std::string>()),name+" whole bank differs");++cases;
 }
 if(mutant){check(completed>500&&changes>0,"Transition mutant did not change completed DATA/result");std::filesystem::create_directories("evidence");std::ofstream out(std::string("evidence/")+argv[3]+".json");out<<J{{"passed",true},{"completed",completed},{"changed_results_or_banks",changes},{"refusals",0}}.dump()<<'\n';check(bool(out),"Cannot retain transition mutant");}else check(cases>500,"Insufficient transition cases");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 files=[STAGE/UNIT,STAGE/HEADER,ROOT/'include/halo2/native_tu15_retained_store_observer.hpp',ROOT/'include/halo2/native_tu15_retained_store_observation.hpp',Path(__file__),ROOT/'tools/validate_native_array_growth_tu15.py',ROOT/'tools/observe_native_array_growth_tu15.cpp',ROOT/'tests/native_tu15_array_growth_tests.cpp'];pins={str(p.relative_to(ROOT)):sha(p)for p in files}
 report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],native_admission_added=False,source_outcomes_supplied=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x1d5790,0x1e15d0)
  source=inspect.getsource(base.original).replace(" mov ${case['receiver']},%ecx\n int3"," mov ${case['receiver']},%ecx\n mov ${case['esi']},%esi\n int3");ns=dict(base.__dict__);exec(source,ns);original=ns['original'];rows=cases();fixture,authority=original(image,{},rows,scratch);normal=list(rows);faults=[];seen=set()
  for sample in normal:
   for kind in ('reads','writes'):
    occurrences={}
    for event in sample[kind]:
     width=event.get('bytes',len(event.get('written',''))//2);key=(event['address'],width);occurrences[key]=occurrences.get(key,0)+1
     if(event['instruction'],kind)in seen:continue
     seen.add((event['instruction'],kind));r={k:copy.deepcopy(v)for k,v in sample.items()if k in ('entry','receiver','esi','arguments','user_words')};r.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=event['address'],bytes=width,occurrence=occurrences[key],write=kind=='writes'));faults.append(r)
  fs=scratch/'faults';fs.mkdir();ff,_=original(image,{},faults,fs)
  for r in faults:
   assert r['declared_fault']
   for b in r['banks']:
    for key in ('before_file','after_file'):b[key]=str(ff/b[key])
  rows+=faults;save(fixture/'original.json',dict(image_sha256=NEW_PIN,cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(STAGE/HEADER,project/'include/halo2'/Path(HEADER).name);shutil.copyfile(STAGE/UNIT,project/'native.cpp');(project/'driver.cpp').write_text(driver())
  variants=(('unsigned_bound','signed_byte(next)<signed_byte(bound)','next<bound'),('count_direction','std::uint8_t(byte(m,p)-1u)','std::uint8_t(byte(m,p)+1u)'),('reset_value','byte(m,observer,0x1d57a3u,p+0x65u,0);','byte(m,observer,0x1d57a3u,p+0x65u,1);'))
  text=(project/'native.cpp').read_text();targets=[('transition','native.cpp','')]
  for name,before,after in variants:
   assert text.count(before)==1;unit='mutant_'+name+'.cpp';(project/unit).write_text(text.replace(before,after));targets.append(('transition_'+name,unit,' --mutant '+name))
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(transitionCounter LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)\n'
  for name,unit,argv in targets:cmake+=f'add_executable({name} $<TARGET_OBJECTS:driver> {unit})\ntarget_include_directories({name} PRIVATE include)\ntarget_compile_features({name} PRIVATE cxx_std_20)\nadd_test(NAME {name} COMMAND {name} "{fixture/"original.json"}"{argv})\n'
  cmake+='add_custom_target(transition_all DEPENDS '+' '.join(t[0]for t in targets)+')\n';(project/'CMakeLists.txt').write_text(cmake);compiled={str(p.relative_to(project)):sha(p)for p in project.rglob('*')if p.is_file()}
  for compiler in ('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','transition_all','--filter','^transition','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--retain-report','evidence/'+name+'.json']for name,_,_ in variants),[])];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler+'-console.log')).write_text((r.stdout+r.stderr)[-65536:]);report.setdefault('results',[]).append(dict(compiler=compiler,command=cmd,exit_code=r.returncode));assert r.returncode==0,(r.stdout+r.stderr)[-6000:]
  assert all(sha(ROOT/p)==h for p,h in pins.items())and all(sha(project/p)==h for p,h in compiled.items())
  code=_Code(image,'.text');executed={pc for r in rows for pc in r['executed_sites']};report.update(passed=True,profiles=len(rows),full_returns=len(normal),fault_prefixes=len(faults),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),authority=authority,source_body_coverage={hex(o):dict(total=len(base.complete_body(code,o).code),executed=len(set(base.complete_body(code,o).code)&executed))for o in base.OWNERS},compiled_source_pins=compiled,semantic_mutants=[v[0]for v in variants],source_inputs_unchanged=True,scope=__doc__)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()
