#!/usr/bin/env python3
"""Declared original1CE1E0/1D76B0 DATA, source-order and attributed-store proof."""
import argparse,copy,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-contact-body-creation-semantic-draft-20261102/staged'
FROZEN=ROOT/'analysis/physics/native-contact-shared-observed-union-stage-20261102/staged'
UNIT='src/physics/native_tu15_created_contact_row.cpp'
HEADER='include/halo2/native_tu15_created_contact_row.hpp'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,d):Path(p).write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
def cases():
 u=base.UI;rows=[]
 for length in range(5):
  for marker in range(-1,length):
   words=[]
   for i in range(length+1):
    node=u+0x100+i*0x40;table=u+0x800+i*0x10
    words += [(node+12-u,node+0x40 if i<length else 0),(node+4-u,0x87650000+i),(node-u,table),(table+8-u,0xcabcabb0 if i==marker+1 else 0)]
   rows.append(dict(name=f'key_{length}_{marker}',entry=0x1ce1e0,receiver=0,eax=0,edx=u+0x100,arguments=[],user_words=words))
 for selector in (0xffffffff,0,0x40000002,0x60000002,0x80000002,0xc0000002):
  for count in (-1,0,1,3):
   for second_key in (0x2000,0x2001,0x2002):
    for coefficient in (0,0x3f800000,0x7f812345):
     first,second,row=u+0x2000,u+0x2200,u+0x1000
     words=[(0x2030,u+0x3000),(0x2034,count&0xffffffff),(0x2230,u+0x3100),(0x2234,count&0xffffffff)]
     for i,key in enumerate((0x2001,0x2002,0x2002)):
      words += [(0x3000+i*8,key),(0x3004+i*8,0x101+i),(0x3100+i*8,second_key if i==0 else key),(0x3104+i*8,0 if i==0 else 0x202+i)]
     words += [(0x4000+i*4,0xaabb0000+i)for i in range(8)]
     args=[selector,first,second,0x12345678,0xabcdef21,coefficient,0x80000000,0x1234,0xabcd]
     rows.append(dict(name=f'row_{selector:x}_{count}_{second_key:x}_{coefficient:x}',entry=0x1d76b0,receiver=0x87654321,eax=row,edx=0x23456789,arguments=args,user_words=words))
 for alias,at in (('second_body',u+0x2200),('first_properties',u+0x3000),('second_properties',u+0x3100)):
  row=copy.deepcopy(next(r for r in rows if r['entry']==0x1d76b0 and r['arguments'][0]==0xffffffff and r['arguments'][5]==0 and '_3_2002_'in r['name']))
  row.update(name='row_alias_'+alias,eax=at);rows.append(row)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);inputs={str(STAGE/p):sha(STAGE/p)for p in (UNIT,HEADER)}
 reused=('src/physics/native_tu15_gameplay_phantom_membership.cpp','include/halo2/native_tu15_gameplay_phantom_membership.hpp')
 inputs.update({str(ROOT/p):sha(ROOT/p)for p in (*reused,'tools/validate_native_created_contact_row_tu15.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp','tools/run_test.py','tools/run_tests.py')})
 report=dict(passed=False,owned_scratch=str(scratch),input_pins=inputs,retained_large_artifacts=[],native_admission=False,genuine_accepted_entry=False)
 try:
  for path in inputs:
   path=Path(path);target=a.output/'source'/path.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
  contexts=[]
  for compiler in ('g++','clang++'):
   command=[compiler,'-std=c++20','-fsyntax-only','-I'+str(STAGE/'include'),'-I'+str(FROZEN/'include'),'-I'+str(ROOT/'include'),'-I'+str(FROZEN/'src/physics'),str(STAGE/UNIT)]
   r=subprocess.run(command,capture_output=True,text=True);(a.output/('context-'+compiler+'.log')).write_text((r.stdout+r.stderr)[-65536:]);contexts.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Private row context compile failed'
  report['contexts']=contexts
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x1ce1e0,0x1d76b0)
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}');(scratch/'observer.cpp').write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')")
  source=source.replace(" mov ${case['receiver']},%ecx\n int3"," mov ${case['receiver']},%ecx\n mov ${case['eax']},%eax\n mov ${case['edx']},%edx\n int3")
  ns=dict(base.__dict__);exec(source,ns);rows=cases();banks={0x52be70:struct.pack('<I',base.UI+0x4000)}
  fixture,authority=ns['original'](image,banks,rows,scratch);normal=list(rows)
  for r in normal:assert r['after_esp']==base.SP+(40 if r['entry']==0x1d76b0 else 4)
  faults=[];seen=set()
  for original in normal:
   for kind in ('reads','writes'):
    occurrences={}
    for event in original[kind]:
     n=event.get('bytes',len(event.get('written',''))//2);pair=(event['address'],n);occurrences[pair]=occurrences.get(pair,0)+1;signature=(event['instruction'],kind)
     if signature in seen:continue
     seen.add(signature);r={k:copy.deepcopy(original[k])for k in ('entry','receiver','arguments','eax','edx','user_words')};r.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=event['address'],bytes=n,occurrence=occurrences[pair],write=kind=='writes'));faults.append(r)
  d=scratch/'faults';d.mkdir();shutil.copyfile(scratch/'observer.cpp',d/'observer.cpp');ff,_=ns['original'](image,banks,faults,d)
  for r in faults:
   for b in r['banks']:
    for k in ('before_file','after_file'):b[k]=str(ff/b[k])
  rows+=faults;save(fixture/'original.json',dict(image_sha256=NEW_PIN,cases=rows))
  project=scratch/'project';shutil.copytree(FROZEN,project)
  for path in (UNIT,HEADER):shutil.copyfile(STAGE/path,project/path)
  for path in reused:shutil.copyfile(ROOT/path,project/path)
  prefix=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text().split('int main(int argc,char**argv)')[0]
  driver='#include "halo2/native_tu15_created_contact_row.hpp"\n'+prefix+r'''
int main(int argc,char**argv)try{
 check(argc==2||argc==4,"Supply row fixture");const bool mutant=argc==4;unsigned completed{},data_changes{},result_changes{};const auto path=std::filesystem::path(argv[1]);std::ifstream input(path);const auto fixture=J::parse(input);
 for(const auto&c:fixture.at("cases"))for(bool read_pass:{false,true}){
  if(mutant&&(read_pass||c.contains("fault")))continue;
  if(!read_pass&&c.contains("fault")&&!c.at("fault").at("write").get<bool>())continue;
  Memory memory;for(const auto&b:c.at("banks"))memory.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));memory.fault=c.value("fault",J{});memory.log_reads=read_pass;
  J stores=J::array();SourcePhysicsContactRecordObserver observer;if(!read_pass)observer=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
  const auto sw=status(),cw=control();const auto mx=_mm_getcsr();bool failed=false;std::uint32_t result{};
  try{if(c.at("entry")==0x1ce1e0u)result=native_tu15_created_contact_object_key(memory,c.at("edx"));
   else{const auto&a=c.at("arguments");native_tu15_initialize_created_contact_row(memory,{c.at("eax"),c.at("receiver"),c.at("edx"),a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7].get<std::uint16_t>(),a[8].get<std::uint16_t>()},observer);}
  }catch(const Fault&){failed=true;}
  const auto name=c.at("name").get<std::string>();check(failed==c.value("declared_fault",false),name+" completion differs");check(sw==status()&&cw==control()&&mx==_mm_getcsr(),name+" hostFP differs");check(c.at("before_fp")==c.at("after_fp"),name+" sourceFP unexpectedly changes");
  if(mutant){++completed;bool changed=false;for(const auto&b:c.at("banks"))changed|=memory.banks.at(b.at("address"))!=file(path.parent_path()/b.at("after_file").get<std::string>());data_changes+=changed;result_changes+=c.at("entry")==0x1ce1e0u&&result!=c.at("return_word");continue;}
  if(!failed&&c.at("entry")==0x1ce1e0u)check(result==c.at("return_word"),name+" key result differs");
  if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));check(memory.reads==expected,name+" ordered reads differ");}
  else check(stores==c.at("writes"),name+" attributed stores differ");
  for(const auto&b:c.at("banks"))check(memory.banks.at(b.at("address"))==file(path.parent_path()/b.at("after_file").get<std::string>()),name+" complete bank differs");
 }
 if(mutant){check(completed>200,"Insufficient completed mutant cases");check(std::string(argv[3])=="key_parent"?result_changes>0:data_changes>0,"Mutation did not alter actual output");std::filesystem::create_directories("evidence");std::ofstream output(std::string("evidence/")+argv[3]+".json");output<<J{{"passed",true},{"mutation",argv[3]},{"full_returns",completed},{"data_mismatches",data_changes},{"result_mismatches",result_changes}}.dump()<<'\n';check(bool(output),"Cannot retain mutation counts");}
 else std::cout<<"Created contact row and key match original DATA/order/FP\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''
  (project/'driver.cpp').write_text(driver)
  changes=(('key_parent','m.word(table+8u)==0xcabcabb0u','m.word(table+8u)==0xcabcabb1u'),('point_width','narrow(memory,observer,0x1d76c3u,p,r.point,2u);','narrow(memory,observer,0x1d76c3u,p,r.point,4u);'),('second_part','p+0x46u,second_part,1u);','p+0x46u,second_part^1u,1u);'))
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(createdRow LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(common OBJECT driver.cpp src/physics/native_tu15_gameplay_phantom_membership.cpp)\ntarget_include_directories(common PRIVATE include src/physics)\ntarget_compile_features(common PRIVATE cxx_std_20)\ntarget_compile_options(common PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_libraries(common PRIVATE nlohmann_json::nlohmann_json)\n'
  targets=[('row',UNIT,'')]
  for name,before,after in changes:
   text=(project/UNIT).read_text();assert text.count(before)==1;path='mutant_'+name+'.cpp';(project/path).write_text(text.replace(before,after,1));targets.append(('row_'+name,path,' --mutant '+name))
  for name,path,arguments in targets:
   cmake+=f'add_executable({name} $<TARGET_OBJECTS:common> {path})\ntarget_include_directories({name} PRIVATE include src/physics)\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_options({name} PRIVATE -Wl,--gc-sections)\nadd_test(NAME created_{name} COMMAND {name} "{fixture/"original.json"}"{arguments})\n'
  cmake+='add_custom_target(created_rows_all DEPENDS '+' '.join(t[0]for t in targets)+')\n';(project/'CMakeLists.txt').write_text(cmake)
  compiled={str(p.relative_to(project)):sha(p)for p in project.rglob('*')if p.is_file()};results=[]
  for compiler in ('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','created_rows_all','--filter','^created_row','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--retain-report','evidence/'+c[0]+'.json']for c in changes),[])];r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'New created contact row/key differs'
  assert all(sha(p)==h for p,h in inputs.items())and all(sha(project/p)==h for p,h in compiled.items())
  executed={pc for r in rows for pc in r['executed_sites']};code=base._Code(image,'.text')
  report.update(passed=True,profiles=len(rows),full_returns=len(normal),fault_prefixes=len(faults),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),authority=authority,source_body_coverage={hex(o):{'all':len(base.complete_body(code,o).code),'executed':len(set(base.complete_body(code,o).code)&executed),'missing_sites':sorted(set(base.complete_body(code,o).code)-executed)}for o in base.OWNERS},compiled_source_pins=compiled,results=results,negative_controls=[c[0]for c in changes],scope=__doc__,source_inputs_unchanged=True)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()
