#!/usr/bin/env python3
"""Independent physical185A00 material selection with sole native1F66D0."""
import argparse,copy,hashlib,inspect,json,os,re,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
STAGE=ROOT/'analysis/physics/native-contact-material-parent-stage-20261103/staged'
SHARED=ROOT/'analysis/physics/native-contact-shared-observed-union-stage-20261102/staged'
UNIT='src/physics/native_tu15_contact_material.cpp'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,d):Path(p).write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')

def cases():
 u=base.UI;rows=[]
 def add(name,kind=0x31ede0,wrapped=False,body=True,record=0,component=1,object_type=0,alternate=0,material_path='bsp',coefficient=0x3f800001,**more):
  shape=u+0x100;child=u+0x500
  words={0x100:u+0x800,0x814:0xa9520 if wrapped else kind,0x130:child,
    0x500:u+0x900,0x914:kind,0x108:record,0x508:record,
    0x1030:u+0x1200,0x1034:1,0x104c:coefficient,0x1050:0x80000000,
    0x1200:0x2001,0x1204:component,0x2044:u+0x7000,0x3044:u+0x6000,
    0x6008:2,0x6000+160+8:2,0x7000+24:(object_type<<24)|0x1234,0x7000+24+8:u+0x8000,
    0x8000:3,0x8108:alternate<<16,0x4000+3*16+8:u+0x9000,
    0x9280:0x56781234,0x90f0:0xef01abcd,
    0x5150:4,0x5154:u+0xa000,0xa000+0xb4+0x1c:coefficient,0xa000+0xb4+0x20:0x80000000,
    0x5610:u+0xb000,0xb008:1,0x905c:u+0xb100,0xb110:1,
    0x7006:0x4321,0x700c:coefficient,0x7010:0x80000000}
  actual=child if wrapped else shape;words[actual-u+0x18]=0;words[actual-u+0x1c]=3 if material_path=='tag'else 0xffffffff
  words.update(more.pop('words',{}))
  row=dict(name=name,entry=0x185a00,eax=u+0x1000 if body else 0,edx=0,receiver=shape,ebx=u+0xd100,
    arguments=[u+0xd000,u+0xd200],user_words=sorted(words.items()),**more);rows.append(row);return row
 for wrapped in (False,True):
  for kind in (0x31ede0,0xab4b0):
   for coefficient in (0,0x3f800001,0x7f812345,0xffffffff):
    for material_path in ('bsp','tag'):add(f'base_{wrapped}_{kind:x}_{coefficient:x}_{material_path}',kind=kind,wrapped=wrapped,coefficient=coefficient,material_path=material_path)
 for record in (0,1,16,0x80001000):
  for body in (False,True):
   for object_type in (0,12,2):
    for alternate in (0,4):add(f'body_{record:x}_{body}_{object_type}_{alternate}',record=record,body=body,object_type=object_type,alternate=alternate)
 for count,key,component in ((0,0x2001,1),(0xffffffff,0x2001,1),(1,0x2002,1),(1,0x2001,0xffffffff),(2,0x2002,1)):
  add(f'properties_{count:x}_{key:x}_{component:x}',component=component,words={0x1034:count,0x1200:key,0x1208:0x2001,0x120c:component})
 # A valid high record pointer and a normal retained record exercise both
 # sides of the original signed clamp without supplying a material result.
 add('retained_record',record=u+0xc000,words={0xc004:0x43210000,0xc00c:0x7f812345,0xc010:0x80000000})
 for kind in (0x31ede0,0xab4b0):
  seed=next(r for r in rows if r['name']==f'base_False_{kind:x}_3f800001_bsp')
  for which in (0,1,2):
   for target in (u+0x104c,u+0x1050,u+0x810a,u+0xa0d4,u+0x9280,0x55d728):
    row=copy.deepcopy(seed);row['name']=f'alias_{kind:x}_{which}_{target:x}'
    if which==0:row['arguments'][0]=target
    elif which==1:row['ebx']=target
    else:row['arguments'][1]=target
    # A DWORD output cannot fit the separately retained2B default bank.
    if target==0x55d728 and which!=2:continue
    rows.append(row)
 def mutation(pc,p,v):return dict(instruction=pc,address=p,written=struct.pack('<I',v).hex())
 for kind,pc,p in ((0x31ede0,0x185b42,u+0x1050),(0xab4b0,0x185a75,u+0xa0d0),(0x31ede0,0x185b48,u+0x9280)):
  row=copy.deepcopy(next(r for r in rows if r['name']==f'base_False_{kind:x}_3f800001_bsp'));row.update(name=f'observer_{kind:x}_{pc:x}',mutations=[mutation(pc,p,0x3f000001)]);rows.append(row)
 return rows

def driver():
 prefix=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text().split('int main(int argc,char**argv)')[0]
 return '#include "halo2/native_tu15_contact_material.hpp"\n'+prefix+r'''
int main(int argc,char**argv)try{
 check(argc==2||argc==4,"Supply material parent fixture");const bool mutant=argc==4;unsigned completed{},changed{},profiles{};const auto path=std::filesystem::path(argv[1]);std::ifstream in(path);const auto fixture=J::parse(in);
 for(const auto&c:fixture.at("cases"))for(bool read_pass:{false,true}){
  if(mutant&&(read_pass||c.contains("fault")||c.contains("mutations")))continue;if(read_pass&&c.contains("mutations"))continue;
  if(!read_pass&&c.contains("fault")&&!c.at("fault").at("write").get<bool>())continue;
  Memory memory;for(const auto&b:c.at("banks"))memory.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));memory.fault=c.value("fault",J{});memory.log_reads=read_pass;
  J stores=J::array();std::set<unsigned>applied;SourcePhysicsContactRecordObserver observer;
  if(!read_pass)observer=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});unsigned index{};for(const auto&m:c.value("mutations",J::array())){if(m.at("instruction")==e.instruction&&applied.insert(index).second){auto bytes=unhex(m.at("written"));memory.write(m.at("address"),bytes);}++index;}};
  const auto sw=status(),cw=control();const auto mx=_mm_getcsr();bool failed{};NativeTu15ContactMaterialStats stats;
  try{const auto&a=c.at("arguments");native_tu15_contact_body_material(memory,c.at("eax"),c.at("receiver"),{a[1],a[0],c.at("ebx")},observer,&stats);}catch(const Fault&){failed=true;}
  const auto name=c.at("name").get<std::string>();check(failed==c.value("declared_fault",false),name+" parent completion differs");check(sw==status()&&cw==control()&&mx==_mm_getcsr(),name+" hardwareFP leaked");check(c.at("before_fp")==c.at("after_fp"),name+" source integer material FP differs");
  if(mutant){++completed;bool difference{};for(const auto&b:c.at("banks"))difference|=memory.banks.at(b.at("address"))!=file(path.parent_path()/b.at("after_file").get<std::string>());changed+=difference;continue;}
  if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(memory.reads!=expected){std::size_t n{};while(n<memory.reads.size()&&n<expected.size()&&memory.reads[n]==expected[n])++n;throw std::runtime_error(name+" material parent read order differs at "+std::to_string(n)+" native="+(n<memory.reads.size()?memory.reads[n].dump():"end")+" original="+(n<expected.size()?expected[n].dump():"end"));}}
  else{check(stores==c.at("writes"),name+" material parent store differs");check(applied.size()==c.value("mutations",J::array()).size(),name+" poststore phase missing");}
  for(const auto&b:c.at("banks"))check(memory.banks.at(b.at("address"))==file(path.parent_path()/b.at("after_file").get<std::string>()),name+" complete material parent bank differs");++profiles;
 }
 if(mutant){check(completed>100&&changed>0,"Material mutation did not alter completed DATA");std::filesystem::create_directories("evidence");std::ofstream out(std::string("evidence/")+argv[3]+".json");out<<J{{"passed",true},{"completed",completed},{"data_mismatches",changed},{"refusals",0}}.dump()<<'\n';check(bool(out),"Cannot retain material mutation");}else check(profiles>100,"Insufficient material parent profiles");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 files=[STAGE/UNIT,STAGE/'src/physics/native_tu15_contact_material_index.cpp',*STAGE.rglob('*.hpp'),SHARED/'src/physics/native_tu15_contact_agent_factory.cpp',SHARED/'src/physics/native_tu15_activation_services.cpp',ROOT/'src/physics/native_tu15_gameplay_phantom_membership.cpp',SHARED/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',Path(__file__),ROOT/'tools/validate_native_array_growth_tu15.py',ROOT/'tools/observe_native_array_growth_tu15.cpp',ROOT/'tests/native_tu15_array_growth_tests.cpp'];pins={str(f.relative_to(ROOT)):sha(f)for f in files}
 report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],native_admission_added=False,source_outcomes_supplied=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x185a00,0x1f66d0,0x1d7530,0xa9520,0xab4b0,0x31ede0)
  source=inspect.getsource(base.original).replace(" mov ${case['receiver']},%ecx\n int3"," mov ${case['receiver']},%ecx\n mov ${case['eax']},%eax\n mov ${case['edx']},%edx\n mov ${case['ebx']},%ebx\n int3")
  ns=dict(base.__dict__);exec(source,ns);original=ns['original'];rows=cases()
  negative=bytearray(32);struct.pack_into('<H',negative,6,0x4321);struct.pack_into('<II',negative,12,0x3f800001,0x80000000)
  banks={0x4ecbd0:struct.pack('<I',base.UI+0x2000),0x52be5c:struct.pack('<I',base.UI+0x3000),0x4f0414:struct.pack('<I',base.UI+0x4000),0x4ecc1c:struct.pack('<I',base.UI+0x5000),0x4ecc18:struct.pack('<I',base.UI+0x5600),0x55d728:struct.pack('<H',1),0x46a0d0:struct.pack('<I',0x3f800000),0x80001000:bytes(negative)}
  fixture,authority=original(image,banks,rows,scratch);normal=list(rows);faults=[];seen=set()
  for sample in normal:
   for kind in ('reads','writes'):
    occurrences={}
    for event in sample[kind]:
     width=event.get('bytes',len(event.get('written',''))//2);key=(event['address'],width);occurrences[key]=occurrences.get(key,0)+1
     if(event['instruction'],kind)in seen:continue
     seen.add((event['instruction'],kind));row={k:copy.deepcopy(v)for k,v in sample.items()if k in ('entry','receiver','arguments','eax','edx','ebx','user_words')};row.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=event['address'],bytes=width,occurrence=occurrences[key],write=kind=='writes'));faults.append(row)
  fs=scratch/'faults';fs.mkdir();ff,_=original(image,banks,faults,fs)
  for r in faults:
   assert r['declared_fault']
   for b in r['banks']:
    for key in ('before_file','after_file'):b[key]=str(ff/b[key])
  rows+=faults;save(fixture/'original.json',dict(image_sha256=NEW_PIN,cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for root in (SHARED/'include',STAGE/'include'):
   for h in root.rglob('*.hpp'):shutil.copyfile(h,project/'include/halo2'/h.name)
  for f in files:
   if f.suffix=='.cpp'and f.parent.name=='physics':shutil.copyfile(f,project/f.name)
  shutil.copyfile(SHARED/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')
  todo=list(project.glob('*.cpp'))+list(project.glob('*.hpp'));seen=set()
  while todo:
   f=todo.pop()
   if f in seen:continue
   seen.add(f)
   for name in re.findall(r'#include\s+"([^"/\n]+\.hpp)"',f.read_text()):
    target=project/name
    if target.exists():continue
    origin=SHARED/'src/physics'/name
    if not origin.exists():origin=ROOT/'src/physics'/name
    assert origin.is_file(),name;shutil.copyfile(origin,target);todo.append(target);pins[str(origin.relative_to(ROOT))]=sha(origin)
  (project/'driver.cpp').write_text(driver())
  variants=(('wrong_default','out.second_scalar,0u','out.second_scalar,1u'),('wrong_alternate','((flags>>2u)&1u)!=0','((flags>>1u)&1u)!=0'),('wrong_record_word','record+6u,2u','record+4u,2u'))
  original_cpp=(project/'native_tu15_contact_material.cpp').read_text();targets=[('material_parent','native_tu15_contact_material.cpp','')]
  for name,before,after in variants:
   assert before in original_cpp;path='mutant_'+name+'.cpp';(project/path).write_text(original_cpp.replace(before,after));targets.append(('material_parent_'+name,path,' --mutant '+name))
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(materialParent LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(common OBJECT driver.cpp native_tu15_contact_material_index.cpp native_tu15_contact_agent_factory.cpp native_tu15_activation_services.cpp native_tu15_gameplay_phantom_membership.cpp)\ntarget_include_directories(common PRIVATE include .)\ntarget_compile_features(common PRIVATE cxx_std_20)\ntarget_compile_options(common PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_libraries(common PRIVATE nlohmann_json::nlohmann_json)\n'
  for name,unit,argv in targets:
   cmake+=f'add_executable({name} $<TARGET_OBJECTS:common> {unit})\ntarget_include_directories({name} PRIVATE include .)\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_options({name} PRIVATE -Wl,--gc-sections)\nadd_test(NAME {name} COMMAND {name} "{fixture/"original.json"}"{argv})\n'
  cmake+='add_custom_target(material_parent_all DEPENDS '+' '.join(t[0]for t in targets)+')\n';(project/'CMakeLists.txt').write_text(cmake);compiled={str(f.relative_to(project)):sha(f)for f in project.rglob('*')if f.is_file()}
  for compiler in ('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','material_parent_all','--filter','^material_parent','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--retain-report','evidence/'+name+'.json']for name,_,_ in variants),[])];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler+'-console.log')).write_text((r.stdout+r.stderr)[-65536:]);report.setdefault('results',[]).append(dict(compiler=compiler,command=cmd,exit_code=r.returncode));assert r.returncode==0,(r.stdout+r.stderr)[-6000:]
  assert all(sha(ROOT/f)==h for f,h in pins.items())and all(sha(project/f)==h for f,h in compiled.items())
  code=_Code(image,'.text');executed={pc for row in rows for pc in row['executed_sites']};report.update(passed=True,profiles=len(rows),full_returns=len(normal),fault_prefixes=len(faults),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),authority=authority,source_body_coverage={hex(o):dict(total=len(base.complete_body(code,o).code),executed=len(set(base.complete_body(code,o).code)&executed))for o in base.OWNERS},compiled_source_pins=compiled,semantic_mutants=[v[0]for v in variants],source_inputs_unchanged=True,scope=__doc__)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(f.stat().st_blocks*512 for f in scratch.rglob('*')if f.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()
