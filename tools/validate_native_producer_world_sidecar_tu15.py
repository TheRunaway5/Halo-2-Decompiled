#!/usr/bin/env python3
"""Owned Host sidecar controls at the exact World identity, no bank exclusion."""
from pathlib import Path
import hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 stage=ROOT/'analysis/physics/native-contact-producer-world-stage-v5-20261102';owned=Path(os.environ['HALO2_TEST_SCRATCH']);out=ROOT/'analysis/physics/native-contact-producer-world-sidecar-20261102-first';out.mkdir(exist_ok=False);report=dict(passed=False,owned_scratch=str(owned),retained_large_artifacts=[],results=[])
 try:
  header='include/halo2/native_tu15_contact_candidate_private_data.hpp';original=(stage/'staged'/header).read_text();pin=sha(stage/'staged'/header)
  cpp=r'''#include "halo2/native_tu15_contact_candidate_private_data.hpp"
#include <type_traits>
using namespace halo2;
void check(bool x){if(!x)throw std::runtime_error("World sidecar control differs");}
struct Backing:SourcePhysicsContactMemory{mutable unsigned reads{};unsigned writes{};std::vector<std::byte> bytes=std::vector<std::byte>(0x100000,std::byte{0xa5});
 void read(std::uint32_t p,std::span<std::byte> out)const override{check(p>=0x60000000u&&std::uint64_t(p)+out.size()<=0x60100000u);++reads;std::copy_n(bytes.begin()+p-0x60000000u,out.size(),out.begin());}
 void write(std::uint32_t p,std::span<const std::byte> in)override{check(p>=0x60000000u&&std::uint64_t(p)+in.size()<=0x60100000u);++writes;std::copy(in.begin(),in.end(),bytes.begin()+p-0x60000000u);}};
int main(){
 static_assert(!std::is_copy_constructible_v<NativeTu15ContactCandidatePrivateData> && !std::is_move_constructible_v<NativeTu15ContactCandidatePrivateData>);
 constexpr auto p=0x600ce000u;Backing bank;const auto before=bank.bytes;
 const std::array<NativeTu15ContactPrivateRange,1> own{{{p,0x10000u}}};
 const std::array<NativeTu15ContactPrivateRange,2> retained{{{0x600df000u,0x10000u},{0x600ef000u,0x1000u}}};
 NativeTu15ContactCandidatePrivateData view(bank,own,retained);
 std::array<std::byte,16> input;for(unsigned i=0;i<16;++i)input[i]=std::byte(i+1);std::array<std::byte,16> output;
 bool refused=false;try{view.read(p,output);}catch(const std::runtime_error&){refused=true;}check(refused&&!bank.reads&&!bank.writes&&bank.bytes==before);
 view.write(p,input);view.read(p,output);check(output==input&&!bank.reads&&!bank.writes&&bank.bytes==before);
 unsigned callbacks{};SourcePhysicsContactRecordObserver observer=[&](const auto&){++callbacks;};
 check(view.copy_defined_data(0,p+16u,p,16u,observer));view.read(p+16u,output);check(output==input&&!callbacks&&bank.bytes==before);
 view.withhold_defined_data(p+4u,4u);check(!view.data_is_defined(p,16u));check(view.copy_defined_data(0,p+32u,p,16u,observer));check(!view.data_is_defined(p+32u,16u));
 for(unsigned mode=0;mode<5;++mode){refused=false;try{
  if(mode==0)view.read(p+32u,output);if(mode==1)view.require_retained_extent(p,16u);
  if(mode==2)(void)view.copy_defined_data(0,0x600e1000u,p+32u,16u,observer);
  if(mode==3)(void)view.observes_retained_store(0x2f3d6bu,p,4u);
  if(mode==4)view.read(p-1u,std::span(output).first(2));
 }catch(const std::runtime_error&){refused=true;}check(refused&&!bank.reads&&!bank.writes&&!callbacks&&bank.bytes==before);}
 refused=false;try{const std::array<NativeTu15ContactPrivateRange,1> alias{{{p+16u,4u}}};NativeTu15ContactCandidatePrivateData bad(bank,own,alias);}catch(const std::runtime_error&){refused=true;}check(refused&&!bank.reads&&!bank.writes&&bank.bytes==before);
 // Retained writes deliberately delegate. Private stores did not touch ANY
 // byte of the complete captured-size1MiB underlying bank above.
 view.write(0x600e1000u,input);check(bank.writes==1&&bank.bytes!=before);for(unsigned i=0;i<bank.bytes.size();++i)check(bank.bytes[i]==(i>=0xe1000u&&i<0xe1010u?input[i-0xe1000u]:before[i]));
}'''
  source=owned/'model.cpp';source.write_text(cpp)
  mutants={
   'guest_backing_leak':('std::copy(in.begin(),in.end(),r.bytes.begin()+at);','outer_.write(p,in);std::copy(in.begin(),in.end(),r.bytes.begin()+at);'),
   'ignore_retained_alias':('for(const auto&r:retained)if(','for(const auto&r:retained)if(false&&')}
  for cc in ('g++','clang++'):
   for name,pair in [('baseline',None),*mutants.items()]:
    include=owned/(cc+'-'+name)/'include/halo2';include.mkdir(parents=True);text=original
    if pair:
     old,new=pair;assert text.count(old)==1;text=text.replace(old,new)
    (include/Path(header).name).write_text(text);exe=owned/(cc+'-'+name+'.exe')
    command=[cc,'-std=c++20','-O0','-I'+str(include.parent),'-I'+str(stage/'staged/include'),str(source),'-o',str(exe)];run=subprocess.run(command,capture_output=True,text=True);(out/(cc+'-'+name+'.log')).write_text(run.stdout+run.stderr);assert run.returncode==0,run.stderr[-3000:]
    result=subprocess.run([str(exe)],capture_output=True,text=True);expected=0 if pair is None else 1;passed=result.returncode==0 if expected==0 else result.returncode!=0;report['results'].append(dict(compiler=cc,model=name,passed=passed,exit_code=result.returncode,compile_command=command,diagnostic=result.stderr[-500:]));assert passed,(cc,name)
  assert sha(stage/'staged'/header)==pin
  report.update(passed=True,header_sha256=pin,source_model_sha256=sha(source),underlying_bank_bytes_checked=1048576,owned_private_bytes=65536,private_identity=hex(0x600ce000),input_bytes_fabricated=False,captured_bank_exclusions_added=False,private_or_retained_escape_refusals_tested=True,mutants=list(mutants),scope=__doc__)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owned.rglob('*')if p.is_file());(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
