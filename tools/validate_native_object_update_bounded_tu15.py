#!/usr/bin/env python3
"""SourceBE4A0 selected full native object-update graph and true boundaries."""
import argparse,copy,hashlib,inspect,json,os,re,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
STAGE=ROOT/'analysis/physics/native-object-update-stage-v2-20261103/staged'
OWNERS=(0xbe4a0,0x1d5790,0x1e15d0,0x10dd10,0xd8330,0xd80a0,0xc0910,0xbda50,0xbcfb0,0xbe7e0,0xb9390)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,d):Path(p).write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
def cases():
 u=base.UI;rows=[]
 def create(name,flags=2,publish=False,offset=0xffff,countdown=0,children=0,unit_rows=0,kind=2,game=0,model=0xffffffff,model_count=0,attachment_flags=0,attachment_bytes=0,**extra):
  words={0x1044:u+0x2000,0x3044:u+0xa000,0x7000+0x38:model,0x8000+3*16+8:u+0x7000,
    0x8000+4*16+8:u+0x7800,0x7860:model_count,0x7864:0,0x701c:attachment_flags,0x9008:game,0x900c:0}
  for index in range(1,4):
   body=u+0x2000+index*0x1000
   words.update({0x2000+index*12:(2<<24)|(flags if index==1 else children)<<16|index,
     0x2000+index*12+8:body,0x2600+index*12:(2<<24)|2<<16|index,0x2600+index*12+8:u+0x4000,
     body-u:3,body-u+0xc:0xffffffff,body-u+0x10:0xffffffff,body-u+0x14:0xffffffff,
     body-u+0xaa:kind&255,body-u+0xb0:countdown<<24,body-u+0xb4:0xffffffff,
     body-u+0xc0:4 if publish else 0,body-u+0x124:(attachment_bytes&0xffff)|0xff800000,
     body-u+0x12a:offset,body-u+0x13c:1,body-u+0x260:0x017a0200,body-u+0x264:0x017a0100,
     body-u+0x268:0x12345678,body-u-0x20:0x017a0200,body-u-0x1c:0x017a0100,body-u-0x18:0x12345678})
  if children:
   words[0x3010]=0x56780002;words[0x400c]=0x43210003;words[0x500c]=0xffffffff
  for n in range(4):words[0x6084+n*4]=u+0x6500+n*0x80 if n<unit_rows else 0;words[0x6500+n*0x80+0x40]=0
  words.update(extra.pop('words',{}));row=dict(name=name,entry=0xbe4a0,receiver=0,arguments=[0xabcd0001],user_words=sorted(words.items()),**extra);rows.append(row);return row
 for flags in (0,2,0x22,0x82):
  for publish in (False,True):
   for offset in (0xffff,0x200,0xff80):
    for countdown in (0,1,255):create(f'base_{flags}_{publish}_{offset:x}_{countdown}',flags=flags,publish=publish,offset=offset,countdown=countdown)
 for child in (2,0x22):
  for publish in (False,True):
   for offset in (0xffff,0x200):create(f'children_{child}_{publish}_{offset:x}',children=child,publish=publish,offset=offset)
 for unit_rows in (1,3):
  for kind in (0,2,127,255):create(f'unit_{unit_rows}_{kind}',unit_rows=unit_rows,kind=kind)
 for kind in (0,1,2,32,33):
  for game in (0,1):
   for model,count in ((0xffffffff,0),(4,0),(4,1),(4,0xffffffff)):
    create(f'vitality_{kind}_{game}_{model:x}_{count:x}',kind=kind,game=game,model=model,model_count=count)
 for size in (0,1,12,23):create(f'empty_attachments_{size}',attachment_flags=1,attachment_bytes=size)
 def mutation(pc,p,v,width=4):return dict(instruction=pc,address=p,written=int(v).to_bytes(width,'little').hex())
 create('observer_changed_current_body',offset=0x200,words={0x412a:0},mutations=[mutation(0xbe4e2,u+0x1044,u+0x2600)])
 create('observer_cached_old_header',offset=0x200,words={0x412a:0},mutations=[mutation(0xbe4e2,u+0x1044,u+0x2600),mutation(0x1e15ed,u+0x2000+12+2,6,1)])
 create('observer_late_header_flags',kind=0,game=1,mutations=[mutation(0xd8398,u+0x2000+12+2,6,1)])
 create('boundary_b37a0',words={0x900c:4,0x30d8:1},boundaries=[0xb37a0],required_owner=0xb37a0)
 create('boundary_component',words={0x30b4:1,0x30c0:0x40},boundaries=[0x1d6fb0],required_owner=0x1d6fb0)
 create('boundary_unit_callback',unit_rows=1,words={0x6540:0x10e630},boundaries=[0x10e630],required_owner=0x10e630)
 create('boundary_positive_damage',model=4,model_count=1,words={0x7864:u+0xb000},boundaries=[0xd83a9],required_owner=0xd8330)
 create('boundary_positive_attachments',attachment_flags=1,attachment_bytes=24,boundaries=[0xc0980],required_owner=0xc0910)
 return rows
def driver():
 prefix=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text().split('int main(int argc,char**argv)')[0]
 return '#include "halo2/native_tu15_object_update_binding.hpp"\n'+prefix+r'''
int main(int argc,char**argv)try{
 check(argc==2||argc==4,"Supply object update fixture");const bool mutant=argc==4;unsigned completed{},changes{},profiles{};const auto path=std::filesystem::path(argv[1]);std::ifstream in(path);const auto fixture=J::parse(in);
 for(const auto&c:fixture.at("cases"))for(bool read_pass:{false,true}){
  if(mutant&&(read_pass||c.contains("fault")||c.contains("mutations")||c.contains("required_owner")))continue;if(read_pass&&c.contains("mutations"))continue;
  if(!read_pass&&c.contains("fault")&&!c.at("fault").at("write").get<bool>())continue;
  Memory memory;for(const auto&b:c.at("banks"))memory.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));memory.fault=c.value("fault",J{});memory.log_reads=read_pass;J stores=J::array();std::set<unsigned>applied;SourcePhysicsContactRecordObserver observer;
  if(!read_pass)observer=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});unsigned n{};for(const auto&m:c.value("mutations",J::array())){if(m.at("instruction")==e.instruction&&applied.insert(n).second){auto bytes=unhex(m.at("written"));memory.write(m.at("address"),bytes);}++n;}};
  NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};const auto sw=status(),cw=control();const auto mx=_mm_getcsr();bool failed{},required{},result{};
  NativeTu15ObjectUpdateStats stats;NativeTu15ObjectUpdateChildStats children;NativeTu15ObjectPublicationStats publication;
  auto services=native_tu15_make_bounded_object_update_services(memory,observer,{},&children,&publication);
  try{result=native_tu15_update_object(memory,{c.at("arguments")[0]},fp,services,observer,&stats);}catch(const Fault&){failed=true;}catch(const std::runtime_error&e){const auto owner=c.value("required_owner",0u);if(owner&&std::string(e.what()).find("requires owner="+std::to_string(owner))!=std::string::npos)required=true;else throw;}
  const auto name=c.at("name").get<std::string>();check(failed==c.value("declared_fault",false)&&required==c.contains("required_owner"),name+" object update completion differs");check(sw==status()&&cw==control()&&mx==_mm_getcsr(),name+" hardwareFP leaked");check(J::array({fp.control_word,fp.status,fp.mxcsr})==c.at("after_fp"),name+" SourceFP differs");
  if(mutant){++completed;bool changed=result!=(c.at("return_word").get<std::uint32_t>()&255u);for(const auto&b:c.at("banks"))changed|=memory.banks.at(b.at("address"))!=file(path.parent_path()/b.at("after_file").get<std::string>());changes+=changed;continue;}
  if(!failed&&!required)check(result==(c.at("return_word").get<std::uint32_t>()&255u),name+" boolean result differs");
  if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(memory.reads!=expected){std::size_t n{};while(n<memory.reads.size()&&n<expected.size()&&memory.reads[n]==expected[n])++n;throw std::runtime_error(name+" object read order differs at "+std::to_string(n)+" native="+(n<memory.reads.size()?memory.reads[n].dump():"end")+" original="+(n<expected.size()?expected[n].dump():"end"));}}
  else{check(stores==c.at("writes"),name+" SourcePC stores differ");check(applied.size()==c.value("mutations",J::array()).size(),name+" observer phase missing");}
  for(const auto&b:c.at("banks"))check(memory.banks.at(b.at("address"))==file(path.parent_path()/b.at("after_file").get<std::string>()),name+" complete object bank differs");++profiles;
 }
 if(mutant){check(completed>100&&changes>0,"Object mutant didnotchange completedDATA");std::filesystem::create_directories("evidence");std::ofstream out(std::string("evidence/")+argv[3]+".json");out<<J{{"passed",true},{"completed",completed},{"changed_results_or_banks",changes},{"refusals",0}}.dump()<<'\n';check(bool(out),"Cannot retain object mutant");}else check(profiles>100,"Insufficient object update profiles");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();files=[*STAGE.rglob('*.cpp'),*STAGE.rglob('*.hpp'),ROOT/'src/physics/native_tu15_object_publication.cpp',ROOT/'src/physics/native_tu15_activation_services.cpp',Path(__file__),ROOT/'tools/validate_native_array_growth_tu15.py',ROOT/'tools/observe_native_array_growth_tu15.cpp',ROOT/'tests/native_tu15_array_growth_tests.cpp'];pins={str(p.relative_to(ROOT)):sha(p)for p in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],source_outcomes_supplied=False,native_admission_added=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=OWNERS
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();observer=observer.replace('unsigned count=0,fault_count=0;bool declared_fault=false;','unsigned count=0,fault_count=0;std::uint32_t boundary{};bool declared_fault=false;')
  needle='const auto site=sites.find(pc);';assert observer.count(needle)==1;observer=observer.replace(needle,'if(m.contains("boundaries")){for(const auto&candidate:m.at("boundaries"))if(pc==candidate)boundary=pc;if(boundary)break;}'+needle)
  observer=observer.replace('check(declared_fault||std::uint32_t(regs.rip)==stop,','check(declared_fault||boundary||std::uint32_t(regs.rip)==stop,').replace('{"initial_banks",before}', '{"boundary",boundary},{"initial_banks",before}');(scratch/'observer.cpp').write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')").replace("('fault','mutations')","('fault','mutations','boundaries')");ns=dict(base.__dict__);exec(source,ns);original=ns['original'];rows=cases();table=b''.join(struct.pack('<I',base.UI+0x6000)for _ in range(256));banks={0x4ecbd0:struct.pack('<I',base.UI+0x1000),0x4f0414:struct.pack('<I',base.UI+0x8000),0x4f3218:struct.pack('<I',base.UI+0x9000),0x52be5c:struct.pack('<I',base.UI+0x3000),0x474990:table,0x5020bc:bytes([1])}
  fixture,authority=original(image,banks,rows,scratch);normal=list(rows);faults=[];seen=set()
  for sample in normal:
   if 'required_owner'in sample:assert sample['boundary']in sample['boundaries'];continue
   for kind in ('reads','writes'):
    occurrences={}
    for event in sample[kind]:
     width=event.get('bytes',len(event.get('written',''))//2);key=(event['address'],width);occurrences[key]=occurrences.get(key,0)+1
     if(event['instruction'],kind)in seen:continue
     seen.add((event['instruction'],kind));r={k:copy.deepcopy(v)for k,v in sample.items()if k in ('entry','receiver','arguments','user_words')};r.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=event['address'],bytes=width,occurrence=occurrences[key],write=kind=='writes'));faults.append(r)
  fs=scratch/'faults';fs.mkdir();shutil.copyfile(scratch/'observer.cpp',fs/'observer.cpp');ff,_=original(image,banks,faults,fs)
  for r in faults:
   assert r['declared_fault']
   for b in r['banks']:
    for key in ('before_file','after_file'):b[key]=str(ff/b[key])
  rows+=faults;save(fixture/'original.json',dict(image_sha256=NEW_PIN,cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for h in STAGE.rglob('*.hpp'):shutil.copyfile(h,project/'include/halo2'/h.name)
  for f in files:
   if f.suffix=='.cpp'and f.parent.name=='physics':shutil.copyfile(f,project/f.name)
  (project/'driver.cpp').write_text(driver());variants=(('wrong_transition_offset','NativeTu15ObjectTransition{transition}','NativeTu15ObjectTransition{transition+4u}'),('wrong_countdown','std::uint8_t(countdown-1u)','std::uint8_t(countdown+1u)'),('wrong_active_flag','std::uint8_t(current&0xfdu)','std::uint8_t(current&0xfbu)'))
  text=(project/'native_tu15_object_update.cpp').read_text();targets=[('object_update','native_tu15_object_update.cpp','')]
  for name,before,after in variants:
   assert text.count(before)==1;unit='mutant_'+name+'.cpp';(project/unit).write_text(text.replace(before,after));targets.append(('object_update_'+name,unit,' --mutant '+name))
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(objectUpdate LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(common OBJECT driver.cpp native_tu15_object_transition.cpp native_tu15_object_update_children.cpp native_tu15_object_publication.cpp native_tu15_activation_services.cpp)\ntarget_include_directories(common PRIVATE include)\ntarget_compile_features(common PRIVATE cxx_std_20)\ntarget_compile_options(common PRIVATE -ffunction-sections -fdata-sections)\ntarget_link_libraries(common PRIVATE nlohmann_json::nlohmann_json)\n'
  for name,unit,argv in targets:cmake+=f'add_executable({name} $<TARGET_OBJECTS:common> {unit})\ntarget_include_directories({name} PRIVATE include)\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -ffunction-sections -fdata-sections)\ntarget_link_options({name} PRIVATE -Wl,--gc-sections)\nadd_test(NAME {name} COMMAND {name} "{fixture/"original.json"}"{argv})\n'
  cmake+='add_custom_target(object_update_all DEPENDS '+' '.join(t[0]for t in targets)+')\n';(project/'CMakeLists.txt').write_text(cmake);compiled={str(p.relative_to(project)):sha(p)for p in project.rglob('*')if p.is_file()}
  for compiler in ('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','object_update_all','--filter','^object_update','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--retain-report','evidence/'+name+'.json']for name,_,_ in variants),[])];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler+'-console.log')).write_text((r.stdout+r.stderr)[-65536:]);report.setdefault('results',[]).append(dict(compiler=compiler,command=cmd,exit_code=r.returncode));assert r.returncode==0,(r.stdout+r.stderr)[-6000:]
  assert all(sha(ROOT/p)==h for p,h in pins.items())and all(sha(project/p)==h for p,h in compiled.items())
  code=_Code(image,'.text');executed={pc for r in rows for pc in r['executed_sites']};report.update(passed=True,profiles=len(rows),full_returns=sum(not r.get('boundary')and not r['declared_fault']for r in rows),boundary_profiles=sum(bool(r.get('boundary'))for r in rows),fault_prefixes=len(faults),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),authority=authority,source_body_coverage={hex(o):dict(total=len(base.complete_body(code,o).code),executed=len(set(base.complete_body(code,o).code)&executed))for o in OWNERS},compiled_source_pins=compiled,semantic_mutants=[v[0]for v in variants],source_inputs_unchanged=True,scope=__doc__,remaining=['B37A0','positiveunitcallbacks','positiveD8330damagegraph','positiveC0910attachmentgraph','1D6FB0','livecanonicalwiring'])
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()
