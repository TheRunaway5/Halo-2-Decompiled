#!/usr/bin/env python3
"""Declared unchanged-TU1.5 pure184D30/184D80 proof; no live admission."""
import argparse
import copy
import hashlib
import inspect
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ROOT=Path(__file__).resolve().parents[1]
FROZEN=ROOT/'analysis/physics/native-contact-shared-observed-union-stage-20261102/staged'
SOURCE=ROOT/'analysis/physics/native-contact-body-creation-semantic-draft-20261102/staged'
UNIT='src/physics/native_tu15_contact_body_creation.cpp'
HEADER='include/halo2/native_tu15_contact_body_creation.hpp'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,d):Path(p).write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def cases():
 u=base.UI;rows=[]
 values=[0,0x80000000,bits(.0001),bits(.01),bits(.1),bits(1.),bits(2.),bits(10.),bits(100.),bits(10000.),0x7fc12345,0x7f812345,0x7f800000,0xff800000,1,0x80000001]
 pairs=[(a,b)for a in values for b in values if a in (0,bits(1.),0x7f812345)or b in (0,bits(1.),bits(10000.))]
 for owner in (0x184d30,0x184d80):
  for index,(a,b)in enumerate(pairs):
   for rounding in range(4):
    first,second=u+0x100,u+0x200;out0,out1=u+0x500,u+0x504
    words=[(0x13c,u+0x300),(0x23c,u+0x400),(0x32c,a),(0x42c,b)]
    row=dict(name=f'weights_{owner:x}_{index}_{rounding}',entry=owner,
     receiver=out1 if owner==0x184d80 else 0,arguments=[second]if owner==0x184d80 else [],
     first=first,second=second,out0=out0,out1=out1,eax=first if owner==0x184d80 else 0,
     edx=out0,first_bits=a,second_bits=b,user_words=words,
     fp_profile=[0x27f|(rounding<<10),0x20,0x1f80|(rounding<<13)])
    for policy in (0,0x3d800000):
     controlled=copy.deepcopy(row);controlled['name']+=f'_policy{policy:x}'
     controlled['global_words']=[(0x55d700,policy)];rows.append(controlled)
  if owner==0x184d80:
   for alias in ('same_output','first_motion_field','late_limit_constant'):
    row=copy.deepcopy(next(r for r in rows if r['entry']==owner and r['first_bits']==bits(1.)and r['second_bits']==bits(10000.)))
    row['name']='weights_alias_'+alias
    if alias=='same_output':row['out1']=row['out0'];row['receiver']=row['out1']
    elif alias=='first_motion_field':row['out0']=u+0x13c;row['edx']=row['out0']
    else:row['out0']=0x55d700;row['edx']=row['out0']
    rows.append(row)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();inputs={n:(SOURCE/n if n in (UNIT,HEADER)else ROOT/n)for n in (UNIT,HEADER,'tools/validate_native_contact_body_weights_tu15.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp','tools/run_test.py','tools/run_tests.py')};pins={str(p.relative_to(ROOT)):sha(p)for p in inputs.values()};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],genuine_accepted_body_entry=False)
 try:
  for path in inputs.values():
   target=a.output/'source'/path.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
  banks={p:image.read(p,4)for p in (0x46a180,0x46a0d0,0x55d700,0x4500b4)}
  base.OWNERS=(0x184d30,0x184d80)
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}')
  observer=observer.replace('bool declared_fault=false;','bool declared_fault=false,observer_stopped=false;')
  observer=observer.replace('    unsigned mutation_index=0;', '''    if(m.contains("observer_stop")&&writes.size()==m.at("observer_stop").get<unsigned>()){
      observer_stopped=true;request(PTRACE_GETREGS,child.pid,nullptr,&regs);break;
    }
    unsigned mutation_index=0;''')
  observer=observer.replace('check(declared_fault||std::uint32_t(regs.rip)==stop,','check(declared_fault||observer_stopped||std::uint32_t(regs.rip)==stop,')
  observer=observer.replace('{"declared_fault",declared_fault}', '{"declared_fault",declared_fault},{"observer_stopped",observer_stopped}')
  (scratch/'observer.cpp').write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')")
  old='fp=[0x27f|((index%4)<<10),((index%8)<<11)|((index%4)<<8)|(index%32),0x1f80|((index%4)<<13)]';assert source.count(old)==1;source=source.replace(old,"fp=case['fp_profile']")
  old=" mov ${case['receiver']},%ecx\n int3";assert source.count(old)==1
  source=source.replace(old,"{chr(10).join(f' mov ${v},0x{p:x}' for p,v in case.get('global_words',[]))}\n mov ${case['receiver']},%ecx\n mov ${case['eax']},%eax\n mov ${case['edx']},%edx\n mov ${case['first_bits']},%ebx\n movd %ebx,%xmm0\n mov ${case['second_bits']},%ebx\n movd %ebx,%xmm1\n int3")
  source=source.replace("('fault','mutations')", "('fault','mutations','observer_stop')")
  env=dict(base.__dict__,scratch=scratch);exec(source,env);rows=cases();fixture,authority=env['original'](image,banks,rows,scratch)
  for row in rows:assert row['after_esp']==base.SP+4
  normal=list(rows);faults=[];seen=set()
  for original in rows:
   for kind in ('reads','writes'):
    occurrences={}
    for event in original[kind]:
     n=event.get('bytes',len(event.get('written',''))//2);pair=(event['address'],n);occurrences[pair]=occurrences.get(pair,0)+1
     signature=(event['instruction'],kind)
     if signature in seen:continue
     seen.add(signature);row={k:copy.deepcopy(v)for k,v in original.items()if k in ('entry','receiver','arguments','first','second','out0','out1','eax','edx','first_bits','second_bits','user_words','global_words','fp_profile')}
     row.update(name=f'weights_fault_{kind}_{event["instruction"]:x}',fault=dict(address=event['address'],bytes=n,occurrence=occurrences[pair],write=kind=='writes'));faults.append(row)
  fault_root=scratch/'faults';fault_root.mkdir();shutil.copyfile(scratch/'observer.cpp',fault_root/'observer.cpp')
  fenv=dict(base.__dict__,scratch=fault_root);exec(source,fenv);ff,_=fenv['original'](image,banks,faults,fault_root)
  for row in faults:
   for bank in row['banks']:
    for k in ('before_file','after_file'):bank[k]=str(ff/bank[k])
  stops=[];seen_stores=set()
  for original in normal:
   for index,event in enumerate(original['writes']):
    if event['instruction'] in seen_stores:continue
    seen_stores.add(event['instruction'])
    row={k:copy.deepcopy(v)for k,v in original.items()if k in ('entry','receiver','arguments','first','second','out0','out1','eax','edx','first_bits','second_bits','user_words','global_words','fp_profile')}
    row.update(name=f'weights_after_store_{event["instruction"]:x}',observer_stop=index+1);stops.append(row)
  stop_root=scratch/'stops';stop_root.mkdir();shutil.copyfile(scratch/'observer.cpp',stop_root/'observer.cpp')
  senv=dict(base.__dict__,scratch=stop_root);exec(source,senv);sf,_=senv['original'](image,banks,stops,stop_root)
  for row in stops:
   assert row['observer_stopped'] and not row['declared_fault']
   for bank in row['banks']:
    for k in ('before_file','after_file'):bank[k]=str(sf/bank[k])
  rows+=faults+stops;save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows))
  project=scratch/'project';shutil.copytree(FROZEN,project)
  for n in (UNIT,HEADER):shutil.copyfile(SOURCE/n,project/n)
  template=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();prefix=template.split('int main(int argc,char**argv)')[0]
  driver='#include "halo2/native_tu15_contact_body_creation.hpp"\n'+prefix+r'''
struct ObserverStop{};
int main(int argc,char**argv)try{
 check(argc==2||argc==4,"Supply original weight fixture");const bool mutant=argc==4;const auto path=std::filesystem::path(argv[1]);std::ifstream input(path);const auto fixture=J::parse(input);
 unsigned completed{},data_changes{},result_changes{},fp_changes{};
 for(const auto&c:fixture.at("cases"))for(bool read_pass:{false,true}){
  if(mutant&&(read_pass||c.contains("fault")||c.contains("observer_stop")))continue;
  if(read_pass&&c.contains("observer_stop"))continue;
  if(!read_pass&&c.contains("fault")&&!c.at("fault").at("write").get<bool>())continue;
  Memory memory;for(const auto&b:c.at("banks"))memory.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));memory.fault=c.value("fault",J{});memory.log_reads=read_pass;
  NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};J stores=J::array();SourcePhysicsContactRecordObserver observer;
  if(!read_pass)observer=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});if(c.contains("observer_stop")&&stores.size()==c.at("observer_stop").get<unsigned>())throw ObserverStop{};};
  const auto sw=status(),cw=control();const auto mx=_mm_getcsr();bool failed=false,result=false,observer_stopped=false;
  try{if(c.at("entry")==0x184d30u)result=native_tu15_contact_mass_ratio_requires_scaling(memory,c.at("first_bits"),c.at("second_bits"),fp);
   else native_tu15_contact_body_mass_scales(memory,c.at("first"),c.at("second"),c.at("out0"),c.at("out1"),fp,0,observer);
  }catch(const Fault&){failed=true;}catch(const ObserverStop&){observer_stopped=true;}
  const auto name=c.at("name").get<std::string>();check(failed==c.value("declared_fault",false),name+" completion differs");
  check(observer_stopped==c.value("observer_stopped",false),name+" observer boundary differs");
  check(status()==sw&&control()==cw&&_mm_getcsr()==mx,name+" host FP differs");
  if(mutant){
   ++completed;bool changed=false;for(const auto&b:c.at("banks"))changed|=memory.banks.at(b.at("address"))!=file(path.parent_path()/b.at("after_file").get<std::string>());data_changes+=changed;
   result_changes+=c.at("entry")==0x184d30u&&result!=bool(c.at("return_word").get<std::uint32_t>()&255u);
   fp_changes+=J::array({fp.control_word,fp.status,fp.mxcsr})!=c.at("after_fp");continue;
  }
  check(J::array({fp.control_word,fp.status,fp.mxcsr})==c.at("after_fp"),name+" Source FP differs");
  if(!failed&&c.at("entry")==0x184d30u)check(result==bool(c.at("return_word").get<std::uint32_t>()&255u),name+" boolean result differs");
  if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));check(memory.reads==expected,name+" ordered reads differ");}
  else check(stores==c.at("writes"),name+" attributed stores differ");
  for(const auto&b:c.at("banks"))check(memory.banks.at(b.at("address"))==file(path.parent_path()/b.at("after_file").get<std::string>()),name+" complete bank differs");
 }
 if(mutant){check(completed>600,"Too few completed mutant profiles");check(std::string(argv[3])=="ratio"?result_changes>0:data_changes>0,"Mutation failed to change actual result/DATA");std::cout<<J{{"mutation",argv[3]},{"full_returns",completed},{"data_mismatches",data_changes},{"result_mismatches",result_changes},{"fp_mismatches",fp_changes}}.dump()<<'\n';}
 else std::cout<<"Pure contact weights match unchanged original DATA/FP/order\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''
  (project/'driver.cpp').write_text(driver)
  mutations=(('ratio','return!compare(b,a,true).ordered_equal();','return compare(b,a,true).ordered_equal();'),('low_scale','a=divide(a,b);m.word(0x184df8u','a=divide(b,a);m.word(0x184df8u'),('high_scale','b=divide(b,a);m.word(0x184e12u','b=divide(a,b);m.word(0x184e12u'))
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(bodyWeights LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\n'
  units=[('weights',UNIT,'body_weights','')]
  for name,before,after in mutations:
   text=(project/UNIT).read_text();assert text.count(before)==1
   path=f'mutant_{name}.cpp';(project/path).write_text(text.replace(before,after,1))
   units.append(('weights_'+name,path,'body_weights_mutant_'+name,' --mutant '+name))
  for target,unit,test,arguments in units:
   cmake+=f'add_executable({target} driver.cpp {unit})\ntarget_include_directories({target} PRIVATE include src/physics)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_options({target} PRIVATE -Wl,--gc-sections)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME {test} COMMAND {target} "{fixture/"original.json"}"{arguments})\n'
  cmake+='add_custom_target(body_weights_all DEPENDS '+' '.join(u[0]for u in units)+')\n';(project/'CMakeLists.txt').write_text(cmake)
  compiled={str(p.relative_to(project)):sha(p)for p in project.rglob('*')if p.is_file()};results=[]
  for compiler in ('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','body_weights_all','--filter','^body_weights','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Pure body weights differ; retain failure diagnostics'
  assert all(sha(ROOT/n)==h for n,h in pins.items())and all(sha(project/n)==h for n,h in compiled.items())
  executed={pc for r in rows for pc in r['executed_sites']};code=base._Code(image,'.text')
  report.update(passed=True,source_inputs_unchanged=True,profiles=len(rows),full_returns=len(normal),fault_prefixes=len(faults),observer_stop_prefixes=len(stops),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),authority=authority,source_body_coverage={hex(o):{'all':len(base.complete_body(code,o).code),'executed':len(set(base.complete_body(code,o).code)&executed),'missing_sites':sorted(set(base.complete_body(code,o).code)-executed)}for o in base.OWNERS},results=results,compiled_source_pins=compiled,negative_controls=[m[0]for m in mutations],policy_data_controls={'0x55d700':[0,0x3d800000],'authentic_runtime_census':'analysis/physics/native-contact-body-creation-before-census-20261102-second/report.json','readonly_4500b4_unchanged':True},scope=__doc__)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()
