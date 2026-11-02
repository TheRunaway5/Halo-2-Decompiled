#!/usr/bin/env python3
"""Exact staged Hostblock, real BEFORE DATA, readonly stores/FP/error/resource ownership."""
import argparse,hashlib,json,os,shutil,subprocess,sys,tarfile
from pathlib import Path
import validate_native_look_query_pipeline_bound_actual_tu15 as native
ROOT=native.ROOT
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--base',type=Path,required=True);p.add_argument('--archive',type=Path,required=True);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);owned=Path(os.environ['HALO2_TEST_SCRATCH']);stage=a.stage/'staged';report=dict(passed=False,owned_scratch=str(owned),retained_large_artifacts=[])
 try:
  assert sha(a.archive)==a.archive_sha256;capture=json.loads(a.archive.with_name('capture-native-before.json').read_text());wanted={r['before_file']:r for r in capture['regions']};bankdir=owned/'banks';bankdir.mkdir();rows=[];stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=Path(member.name).name
    if name not in wanted:continue
    r=wanted[name];raw=tar.extractfile(member).read();assert len(raw)==r['size'];q=bankdir/name;q.write_bytes(raw);rows.append(dict(address=r['address'],path=str(q)))
  assert stream.wait()==0 and len(rows)==len(wanted);fixture=owned/'input.json';fixture.write_text(json.dumps(rows));live=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();at=live.index('            source_execution_untracked(bank,0x18a300u);');start=live.rfind('        {\n',0,at);end=live.index('        bank.write(retained,',at);block=live[start:end];assert'0x600a0000u'in block and'run_actual'not in block
  cpp=CPP.replace('@HOST@',block);src=owned/'host.cpp';src.write_text(cpp);results=[]
  for cc in('g++','clang++'):
   binary=owned/('host-'+cc);cmd=[cc,'-std=c++20','-O1','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(stage/'include'),'-I'+str(a.base/'include'),'-I'+str(ROOT/'include'),str(src),*[str((stage/u)if(stage/u).exists()else(a.base/u)if(a.base/u).exists()else(ROOT/u))for u in native.UNITS],'-o',str(binary)];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(cc+'-build.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:];r=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True);(a.output/(cc+'-run.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr;results.append(dict(compiler=cc,passed=True,command=cmd,result=json.loads(r.stdout)))
  report.update(passed=True,compiler_results=results,source_hashes={str(p):sha(p)for p in(stage/'src/physics/physics_live_world_tu15.cpp',stage/'include/halo2/native_tu15_look_pipeline_binding.hpp',stage/'include/halo2/native_tu15_camera_query_binding.hpp',Path(__file__))},host_function_sha256=hashlib.sha256(block.encode()).hexdigest(),archive_sha256=a.archive_sha256,captured_before_bytes=sum(r['size']for r in capture['regions']),scope=__doc__,tests=['ExactHostfragment actualindex/deltas/freshFP/Scratch600A0000','ActualcurrentData nativeLook/controller/camera/querypositive','SourcePCtee/read-onlystoreobservation andExecutionRAII','Readonlyobserver query-store throw counts/refusals/FPpublish/hostFPrestore','Mutableobserver/depth/alternateCW refuse beforeanybankwrite','Existing6005E000forward/60060000point sentinels survive newquery region','NoOriginalCPU/GP/postimage/results supplied'])
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owned.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 return 0
CPP=r'''#include "halo2/native_tu15_look_pipeline_binding.hpp"
#include <nlohmann/json.hpp>
#include <map>
#include <fstream>
#include <optional>
#include <iostream>
#include <xmmintrin.h>
using namespace halo2;using J=nlohmann::json;
void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Memory:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::vector<std::byte>>banks;unsigned writes{};
 auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Hostmodel missingDATA");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Hostmodel extent");return i;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
 void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);++writes;}
};
std::uint16_t cw(){std::uint16_t n;asm volatile("fnstcw %0":"=m"(n));return n;}std::uint16_t sw(){std::uint16_t n;asm volatile("fnstsw %0":"=a"(n));return n;}
unsigned untracked{};void source_execution_untracked(Memory&,unsigned owner){check(owner==0x18a300u,"Hostowner");++untracked;}
struct Control{std::uint16_t x87_control_word{0x27f};std::uint32_t mxcsr{0x1f80};};struct Frame{std::array<std::uint32_t,8>gp{};std::optional<Control>floating_point{Control{}};std::uint16_t x87_status{};unsigned x87_depth{};};
struct Model {
 Memory bank;NativeTu15LookPipelineActivity native_look_pipeline;SourcePhysicsContactRecordObserver physics_store_observer;bool physics_store_observer_read_only{true};unsigned last_instruction{},executions{},frames{},observed{};Frame last;
 struct Execution{Model&owner;explicit Execution(Model&o):owner(o){++owner.executions;}~Execution(){--owner.executions;}};
 Frame frame(std::span<const std::uint32_t>a){check(a.size()==2,"TypedHostargs");++frames;return{};}
 void invoke(unsigned mode){const std::array<std::uint32_t,2>args{0,0};auto a=frame(args);a.gp[0]=0;if(mode==2)physics_store_observer_read_only=false;if(mode==3)a.x87_depth=1;if(mode==4)a.floating_point->x87_control_word=0x67f;struct Save{Model&owner;Frame&a;~Save(){owner.last=a;}}save{*this,a};
@HOST@
 }
};
int main(int argc,char**argv)try{check(argc==2,"fixture");std::ifstream input(argv[1]);J rows=J::parse(input);Memory before;for(const auto&r:rows){std::ifstream f(r.at("path").get<std::string>(),std::ios::binary);std::vector<char>b((std::istreambuf_iterator<char>(f)),{});std::vector<std::byte>v(b.size());std::transform(b.begin(),b.end(),v.begin(),[](char x){return std::byte(std::uint8_t(x));});before.banks.emplace(r.at("address"),std::move(v));}
 for(unsigned mode=0;mode<5;++mode){Model m;m.bank=before;std::array<std::byte,12>old_forward{},old_point{};m.bank.read(0x6005e000u,old_forward);m.bank.read(0x60060000u,old_point);m.physics_store_observer=[&](const auto&e){check(m.last_instruction==e.instruction,"SourcePCtee");++m.observed;std::vector<std::byte>actual(e.written.size());m.bank.read(e.address,actual);check(actual==e.written,"Readonlystore observed wrongwrittenDATA");if(mode==1&&e.instruction==0x16c672u)throw std::runtime_error("ownedobserver-stop");};const auto host_cw=cw(),host_sw=sw();const auto host_mx=_mm_getcsr();bool failed{};try{m.invoke(mode);}catch(const std::runtime_error&){failed=true;}check(cw()==host_cw&&sw()==host_sw&&_mm_getcsr()==host_mx,"HostFPleak");check(m.executions==0&&m.frames==1&&m.native_look_pipeline.attempts==1,"Hostlifetime");
  if(mode==0){check(!failed&&m.native_look_pipeline.completed==1&&m.native_look_pipeline.scene_queries.completed>0&&m.native_look_pipeline.scene_queries.refusals==0&&m.observed>0,"PositiveHost nativequery");}
  else{check(failed&&m.native_look_pipeline.completed==0&&m.native_look_pipeline.refusals==1,"Hostrefusal");if(mode==1)check(m.native_look_pipeline.scene_queries.refusals==1&&m.native_look_pipeline.query_refusals==1&&m.observed>0,"Observerexception counts");else check(m.bank.writes==0&&m.native_look_pipeline.controller.calls==0&&m.last.floating_point->x87_control_word==(mode==4?0x67f:0x27f)&&m.last.x87_status==0,"Admission changedDATA/FP");}
  check(m.native_look_pipeline.unsupported_fp_domains==(mode==4),"FPadmissioncounter");std::array<std::byte,12>now{};m.bank.read(0x6005e000u,now);check(now==old_forward,"Reorientforward overwritten");m.bank.read(0x60060000u,now);check(now==old_point,"Setpositionpoint overwritten");
 }
 check(untracked==5,"ActualHostuntrackedowner");std::cout<<J{{"passed",true},{"modes",5},{"positive_native_query",true},{"readonly_exception_and_FP_publish",true},{"old_forward_and_point_disjoint",true}}.dump()<<'\n';return0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return1;}
'''.replace('return0;','return 0;').replace('return1;','return 1;')
if __name__=='__main__':raise SystemExit(main())
