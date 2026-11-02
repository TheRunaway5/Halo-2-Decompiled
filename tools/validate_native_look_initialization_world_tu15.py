#!/usr/bin/env python3
"""Exact staged constructor Host block, six counters, both JSON and gate model."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
from prepare_native_look_initialization_world_tu15 import ROOT,FIELDS,source_file,sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  project=scratch/'project';project.mkdir();shutil.copytree(a.source_root/'include',project/'include')
  for q in(stage/'include').rglob('*.hpp'):shutil.copyfile(q,project/'include'/q.relative_to(stage/'include'))
  live=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();start=live.index('        if(!initialized) {',live.index('    void prepare_player_look('));end=live.index('        // Native zoom selection',start);host=live[start:end];assert 'run_actual'not in host and 'a.gp'not in host
  inner=host[len('        if(!initialized) {\n'):-len('        }\n')];caller_count=live.count(inner);assert caller_count in(1,3)
  start=live.index('    const auto&look_initialization=');end=live.index('    result.world_integrations=',start);getter=live[start:end]
  acceptance=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();start=acceptance.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION"))');end=acceptance.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_BODY_UPDATE"))',start);gate=acceptance[start:end]
  for field in FIELDS:assert acceptance.count('{"native_look_initialization_'+field+'",native_frames.native_look_initialization_'+field+'},')==2
  assignments=''.join('a.native_look_initialization.'+value+'='+str(17+n)+';'for n,value in enumerate(FIELDS.values()));checks=''.join('check(r.native_look_initialization_'+name+'=='+str(17+n)+',"getter");'for n,name in enumerate(FIELDS));rows=''.join('{"native_look_initialization_'+field+'",r.native_look_initialization_'+field+'},'for field in FIELDS)
  negatives=['b.native_look_initialization_calls=0;','b.native_look_initialization_attempts=2;','b.native_look_initialization_refusals=1;','b.native_look_initialization_unit_directions=0;','b.native_look_initialization_cleared_records=0;']
  cpp='''#include "halo2/native_tu15_look_initialization_binding.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <map>
#include <optional>
#include <cstdlib>
using namespace halo2;
void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Memory:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::byte> bytes;std::uint64_t writes{};
 void put(std::uint32_t p,std::uint32_t n,unsigned width=4){for(unsigned k=0;k<width;++k)bytes[p+k]=std::byte(n>>(k*8u));}
 void read(std::uint32_t p,std::span<std::byte> b)const override{for(unsigned k=0;k<b.size();++k)b[k]=bytes.at(p+k);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{for(unsigned k=0;k<b.size();++k){check(bytes.contains(p+k),"Model DATA extent");bytes.at(p+k)=b[k];}++writes;}
};
struct Control {std::uint16_t x87_control_word{0x27f};std::uint32_t mxcsr{0x1f80};};
struct Frame {std::optional<Control>floating_point{Control{}};std::uint16_t x87_status{};unsigned x87_depth{};};
struct Model;
void source_execution_untracked(Memory&,std::uint32_t owner){check(owner==0x18a0c0u,"Actual constructor owner");}
struct Model {
 Memory bank;NativeTu15LookInitializationBindingActivity native_look_initialization;
 SourcePhysicsContactRecordObserver physics_store_observer;bool physics_store_observer_read_only{true};
 std::uint32_t last_instruction{};unsigned executions{},frames{},observed_stores{};
 struct Execution {Model&owner;explicit Execution(Model&o):owner(o){++owner.executions;}~Execution(){--owner.executions;}};
 Frame frame(std::span<const std::uint32_t>args){check(args.empty(),"Zero-formal constructor");++frames;return {};}
 void initialize(bool initialized,std::uint32_t unit){
'''+host+'''}
'''+(' void camera_constructor(std::uint32_t unit){\n'+inner+'}\n void reorient_constructor(std::uint32_t unit){\n'+inner+'}\n'if caller_count==3 else '')+'''
 Model(){constexpr std::uint32_t input=0x1000,record=input+0x14,objects=0x2000,headers=0x2100,unit=0x3000;
 for(unsigned n=0;n<0x94;++n)bank.bytes[record+n]=std::byte{0xa5};bank.put(input,0,1);bank.put(0x4f9b54,input);bank.put(0x4ecbd0,objects);bank.put(objects+0x44,headers);bank.put(headers+12+8,unit);
 bank.put(unit+0x150,0x3f800000);bank.put(unit+0x154,0xbf000000);bank.put(unit+0x158,0x3f000000);bank.put(unit+0x214,0x12345678);bank.put(unit+0x23d,0x80,1);bank.put(unit+0x241,0xff,1);
 bank.put(0x46a58c,0x3f800000);bank.put(0x46a588,0xbf800000);bank.put(0x44923c,0x40c90fdb);
 physics_store_observer=[this](const auto&e){check(last_instruction==e.instruction,"Observer SourcePC publication");++observed_stores;};}
};
NativeTu15PhysicsFrameExecutionStats get(const Model*impl_){NativeTu15PhysicsFrameExecutionStats result;
'''+getter+'''return result;}
bool gate(NativeTu15PhysicsFrameExecutionStats native_frames){bool passed=true;nlohmann::json report;setenv("HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION","1",1);
'''+gate+'''return passed;}
int main(){Model a;
'''+assignments+'''auto r=get(&a);'''+checks+'''nlohmann::json serialized={'''+rows+'''};check(serialized.size()==6,"Both serializers");
 NativeTu15PhysicsFrameExecutionStats p;p.native_look_initialization_attempts=1;p.native_look_initialization_calls=1;p.native_look_initialization_unit_directions=1;p.native_look_initialization_cleared_records=1;check(gate(p),"Positive gate");auto b=p;
'''+''.join('b=p;'+n+'check(!gate(b),"Negative gate");'for n in negatives)+'''
 Model positive;positive.initialize(false,0xabcd0001);
'''+(' positive.camera_constructor(0xabcd0001);positive.reorient_constructor(0xabcd0001);\n'if caller_count==3 else '')+f'''check(positive.frames=={caller_count}&&positive.executions==0&&positive.observed_stores>{60*caller_count},"Exact Host scope/observer");check(gate(get(&positive)),"Positive Host model");\n'''+'''
 Model skipped;const auto before=skipped.bank.bytes;skipped.initialize(true,0xabcd0001);check(skipped.frames==0&&skipped.bank.bytes==before&&skipped.native_look_initialization.attempts==0,"Initialized Host skip");
 Model refused;refused.physics_store_observer_read_only=false;bool failure{};const auto untouched=refused.bank.bytes;try{refused.initialize(false,0xabcd0001);}catch(const std::runtime_error&){failure=true;}check(failure&&refused.executions==0&&refused.bank.bytes==untouched&&refused.native_look_initialization.attempts==1&&refused.native_look_initialization.completed==0&&refused.native_look_initialization.refusals==1,"Native observer refusal");
 return 0;}
''';(project/'model.cpp').write_text(cpp);results=[]
  source=stage/'src/physics/native_tu15_look_initialization.cpp'
  for compiler in('g++','clang++'):
   binary=project/compiler.replace('+','x');cmd=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(project/'include'),str(project/'model.cpp'),str(source),'-o',str(binary)];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:];r=subprocess.run([str(binary)],capture_output=True,text=True);assert r.returncode==0,(r.stdout,r.stderr);results.append(dict(compiler=compiler,passed=True))
  report.update(passed=True,compiler_results=results,getter_counters=6,serializers=2,negative_gate_cases=len(negatives),exact_host_constructor_callers=caller_count,host_models=['exact production fresh-frame zero-formal/source-entry/native service/observer scope at each identical caller','initialized skip','nonreadonly observer counted refusal before native stores'],same_native_service_builder=True,generated_model_sha256=sha(project/'model.cpp'),validator_sha256=sha(Path(__file__)),source_hashes={str(q.relative_to(stage)):sha(q)for q in stage.rglob('*')if q.is_file()},scope='Exact source Host block and counter/serializer/gate model; full original kernel and authentic-bank proofs are separate; no live acceptance claim')
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(q.stat().st_blocks*512 for q in scratch.rglob('*')if q.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
