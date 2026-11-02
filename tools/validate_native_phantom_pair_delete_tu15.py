#!/usr/bin/env python3
"""Physical41-site320040 + authentic2E4D60 RET4; declared semantic pair DATA."""
import argparse,hashlib,inspect,json,os,shutil,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def save(path,value):path.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')
def cases():
 U=base.UI;rows=[]
 def add(name,a,b,**more):
  words=[(0,U+0x110),(4,U+0x210),(0x118,a),(0x218,b),(0x318,2),(0x120,U+0x400),(0x220,U+0x440),(0x320,U+0x480),(0x400,U+0x500),(0x440,U+0x540),(0x480,U+0x580),(0x520,0x2e4d60),(0x560,0x2e4d60),(0x5a0,0x2e4d60)]
  rows.append(dict(name=name,entry=0x320040,receiver=0x915327c1,arguments=[U],user_words=words,**more))
 for a in(0,1,2,3,0xffffffff):
  for b in(0,1,2,3,0xffffffff):add(f'types_{a:x}_{b:x}',a,b)
 for name,address in [('pair_first',U),('first_type',U+0x118),('second_payload',U+4),('first_phantom',U+0x120),('first_vtable',U+0x400),('first_owner',U+0x520),('second_type',U+0x218),('second_owner',U+0x560)]:
  add('fault_'+name,2,2,fault=dict(address=address,bytes=4,occurrence=1,write=False))
 for name,changes in [('second_becomes_body',[(U+0x218,1)]),('second_retargeted',[(U+4,U+0x310)]),('first_retargeted',[(U,U+0x310)]),('second_becomes_phantom',[(U+0x218,2)])]:
  add(name,2,1 if name=='second_becomes_phantom'else 2,mutations=[dict(instruction=0x2e4d60,address=p,written=v.to_bytes(4,'little').hex())for p,v in changes])
 add('unbound_first_entry_prefix',2,2,required_owner=True,stop_boundary=0x2e4d60)
 add('unbound_tail_entry_prefix',1,2,required_owner=True,stop_boundary=0x2e4d60)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();files=['include/halo2/native_tu15_phantom_pair_delete.hpp','src/physics/native_tu15_phantom_pair_delete.cpp','tools/validate_native_phantom_pair_delete_tu15.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp'];pins={f:sha(ROOT/f)for f in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],actual_live_phantom_input_claimed=False,supplied_original_outcomes=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x320040,0x2e4d60)
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();needle='if(pc==stop)break;';assert observer.count(needle)==1;record='''if(pc==0x2e4d60u){const auto raw=memory(child.pid,std::uint32_t(regs.rsp)+4u,4);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(raw[n])<<(8u*n);allocator_calls.push_back(J{{"owner",pc},{"receiver",std::uint32_t(regs.rcx)},{"other",value}});}''';observer=observer.replace(needle,record+needle);observer=observer.replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}');observer=observer.replace('pc==mutation.at("instruction")&&!pending.empty()','pc==mutation.at("instruction")&&(!pending.empty()||pc==0x2e4d60u)');observer_source=scratch/'observer.cpp';observer_source.write_text(observer)
  source=inspect.getsource(base.original);needle="str(ROOT/'tools/observe_native_array_growth_tu15.cpp')";assert source.count(needle)==1;source=source.replace(needle,'str(observer_source)').replace('hex(STOP)],','hex(case.get(\'stop_boundary\',STOP))],');environment=dict(base.__dict__,observer_source=observer_source);exec(source,environment);all_cases=cases();fixture,authority=environment['original'](image,{},all_cases,scratch);save(fixture/'original.json',dict(original_sha256=NEW_PIN,cases=all_cases))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(ROOT/'src/physics/native_tu15_phantom_pair_delete.cpp',project/'kernel.cpp')
  test=r'''#include "halo2/native_tu15_phantom_pair_delete.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
using namespace halo2;using J=nlohmann::json;
void check(bool x,const std::string&s){if(!x)throw std::runtime_error(s);}
struct Fault{};struct Required{};
std::vector<std::byte>file(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);std::vector<char>v((std::istreambuf_iterator<char>(f)),{});std::vector<std::byte>b(v.size());std::transform(v.begin(),v.end(),b.begin(),[](auto x){return std::byte(std::uint8_t(x));});return b;}
struct Memory:SourcePhysicsContactMemory{std::map<std::uint32_t,std::vector<std::byte>>banks;mutable J reads=J::array();J fault;mutable unsigned count{};void read(std::uint32_t p,std::span<std::byte>b)const override{if(!fault.is_null()&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++count==fault.at("occurrence"))throw Fault{};reads.push_back(J::array({p,b.size()}));auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unbacked phantom read");--i;check(std::uint64_t(p-i->first)+b.size()<=i->second.size(),"Unbacked phantom extent");std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unbacked phantom mutation");--i;std::copy(b.begin(),b.end(),i->second.begin()+p-i->first);}};
std::uint16_t sw(){std::uint16_t v;asm volatile("fnstsw %0":"=a"(v));return v;}std::uint16_t cw(){std::uint16_t v;asm volatile("fnstcw %0":"=m"(v));return v;}
int main(int argc,char**argv)try{check(argc==2,"Supply physical fixture");const auto p=std::filesystem::path(argv[1]);std::ifstream f(p);auto data=J::parse(f);unsigned cases{},events{},calls{},faults{},mutations{},prefixes{};for(const auto&c:data.at("cases")){Memory m;for(const auto&b:c.at("banks"))m.banks.emplace(b.at("address"),file(p.parent_path()/b.at("before_file").get<std::string>()));m.fault=c.value("fault",J{});NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15PhantomPairDeleteServices services;J callbacks=J::array();std::set<unsigned>applied;services.remove_overlap=[&](auto self,auto owner,auto other,auto&){callbacks.push_back(J{{"owner",owner.identity},{"receiver",self},{"other",other.identity}});check(owner.identity==0x2e4d60,"Unqualified native phantom callback");if(c.value("required_owner",false))throw Required{};native_tu15_ignore_removed_phantom_overlap();unsigned n{};for(const auto&mutation:c.value("mutations",J::array())){if(applied.insert(n).second){const auto s=mutation.at("written").get<std::string>();std::vector<std::byte>b;for(unsigned k=0;k<s.size();k+=2)b.push_back(std::byte(std::stoul(s.substr(k,2),nullptr,16)));m.write(mutation.at("address"),b);}++n;}};const auto hsw=sw(),hcw=cw();const auto hmx=_mm_getcsr();bool failed=false,required=false;try{native_tu15_delete_phantom_pair(m,c.at("arguments")[0],fp,services);}catch(Fault&){failed=true;}catch(Required&){required=true;}check(failed==c.at("declared_fault")&&required==c.value("required_owner",false),c.at("name").get<std::string>()+" completion differs");if(!failed&&!required)check(c.at("after_esp")==0x200f008u,"Original phantom RET4/ordinary tail return stack differs");check(hsw==sw()&&hcw==cw()&&hmx==_mm_getcsr(),"Phantom callback leaked host FP");check(J::array({fp.control_word,fp.status,fp.mxcsr})==c.at("after_fp"),"Phantom completeFP differs");check(callbacks==c.at("allocator_calls"),"Phantom callback identities/arguments differ");J reads=J::array();for(const auto&r:c.at("reads"))reads.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=reads){std::size_t n{};while(n<m.reads.size()&&n<reads.size()&&m.reads[n]==reads[n])++n;throw std::runtime_error(c.at("name").get<std::string>()+" read order differs at "+std::to_string(n));}check(c.at("writes").empty(),"Unexpected persistent original phantom store");for(const auto&b:c.at("banks"))check(m.banks.at(b.at("address"))==file(p.parent_path()/b.at("after_file").get<std::string>()),"Complete phantom DATA bank differs");check(applied.size()==c.at("mutations_applied"),"Phantom mutation lifetime differs");++cases;events+=reads.size();calls+=callbacks.size();faults+=failed;prefixes+=required;mutations+=applied.size();}check(cases>30,"Insufficient phantomcases");std::cout<<J{{"passed",true},{"cases",cases},{"ordered_reads",events},{"genuine_callbacks",calls},{"fault_prefixes",faults},{"unbound_callback_prefixes",prefixes},{"declared_input_mutations",mutations}}.dump()<<'\n';return0;}catch(std::exception&e){std::cerr<<e.what()<<'\n';return1;}
'''.replace('return0;','return 0;').replace('return1;','return 1;');(project/'test.cpp').write_text(test);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(phantom_delete LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(phantom test.cpp kernel.cpp)\ntarget_include_directories(phantom PRIVATE include)\ntarget_compile_features(phantom PRIVATE cxx_std_20)\ntarget_link_libraries(phantom PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME phantom_delete COMMAND phantom "'+str(fixture/'original.json')+'")\n')
  results=[];commands=[]
  def run(label,compiler):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','phantom','--filter','^phantom_delete$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];commands.append(command);r=subprocess.run(command);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
  for compiler in a.compilers:assert run(compiler,compiler)==0,'Native phantomdelete differs'
  kernel=project/'kernel.cpp';original=kernel.read_text();mutants=[('first_type','first+8u)==2u','first+8u)==1u'),('skip_second','if(word(memory,second+8u)==2u)','if(false)'),('wrong_other','{other},fp);','{other+4u},fp);')]
  for name,before,after in mutants:
   assert original.count(before)>0;kernel.write_text(original.replace(before,after));assert run('negative-'+name,a.compilers[0])!=0,'Original failed to reject '+name;kernel.write_text(original)
  report.update(passed=all(sha(ROOT/f)==v for f,v in pins.items()),authority=authority,original_sha256=NEW_PIN,original_cases=len(all_cases),original_instructions=sum(c['original_instructions']for c in all_cases),ordered_reads=sum(len(c['reads'])for c in all_cases),genuine_callbacks=sum(len(c['allocator_calls'])for c in all_cases),fault_prefixes=sum(c['declared_fault']for c in all_cases),unbound_callback_prefixes=sum(c.get('required_owner',False)for c in all_cases),declared_input_mutations=sum(c['mutations_applied']for c in all_cases),commands=commands,results=results,negative_controls=[m[0]for m in mutants],executed_source_sites=sorted({pc for c in all_cases for pc in c['executed_sites']}),scope='Declared valid model pairs/type2 phantom dispatch with authentic original RET4 no-op; mutable current DATA controls after first genuine callback; exact local liveness/ordinaryRET4, not actual live nestedowner admission')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
