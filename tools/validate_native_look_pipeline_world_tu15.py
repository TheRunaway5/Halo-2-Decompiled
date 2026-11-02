#!/usr/bin/env python3
"""Exact staged Host native-look FP/DATA/observer admission and counter/gate proof."""
import argparse,json,os,shutil,struct,subprocess,sys
from pathlib import Path
from prepare_native_look_pipeline_world_tu15 import ROOT,FIELDS,sha,save
import validate_native_camera_position_tu15 as parent
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();stage=a.stage.resolve()/'staged';a.output.mkdir(parents=True,exist_ok=False);owned=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(owned),retained_large_artifacts=[])
 try:
  project=owned/'project';project.mkdir();shutil.copytree(a.source_root/'include',project/'include')
  for q in(stage/'include').rglob('*.hpp'):shutil.copyfile(q,project/'include'/q.relative_to(stage/'include'))
  live=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();at=live.index('            source_execution_untracked(bank,0x18a300u);');begin=live.rfind('        {\n',0,at);end=live.index('        bank.write(retained,',at);host=live[begin:end];assert host.count('native_tu15_update_bound_look_controller')==1 and'run_actual'not in host
  start=live.index('    const auto&look_pipeline=');end=live.index('    const auto&look_initialization=',start);getter=live[start:end]
  acceptance=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();start=acceptance.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_CONTROLLER"))');end=acceptance.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION"))',start);gate=acceptance[start:end]
  for name in FIELDS:assert acceptance.count('{"native_look_update_'+name+'",native_frames.native_look_update_'+name+'},')==2
  assignments=''.join('a.native_look_pipeline.'+value+'='+str(29+n)+';'for n,value in enumerate(FIELDS.values()));checks=''.join('check(r.native_look_update_'+name+'=='+str(29+n)+',"getter value");'for n,name in enumerate(FIELDS));rows=''.join('{"native_look_update_'+name+'",r.native_look_update_'+name+'},'for name in FIELDS)
  negatives=['b.native_look_update_calls=0;','b.native_look_update_attempts=2;','b.native_look_update_refusals=1;','b.native_look_update_context_calls=0;','b.native_look_update_context_refusals=1;','b.native_look_update_camera_calls=0;','b.native_look_update_camera_completions=0;','b.native_look_update_angle_calls=1;','b.native_look_update_angle_refusals=1;','b.native_look_update_query_refusals=1;','b.native_look_update_marker_observer_refusals=1;']
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);case=next(c for c in parent.cases(image)if c['name'].startswith('parent_camera_(0.8, 0.0, -0.6)_1.2_0.2_1'))
  banks={b['address']:bytearray.fromhex(b['before'])for b in case['banks']if b['address']!=parent.DATA}
  def put(at,n,width=4):
   for first,raw in banks.items():
    if first<=at and at+width<=first+len(raw):raw[at-first:at-first+width]=(n&((1<<(width*8))-1)).to_bytes(width,'little');return
   banks[at]=bytearray((n&((1<<(width*8))-1)).to_bytes(width,'little'))
  controls=parent.base.BASE+0x4500;globals=parent.base.BASE+0x4600;put(0x4f9b54,controls);put(0x4ecc1c,globals);put(globals+0xf4,parent.base.DEFINITION);put(parent.base.UNIT+0x1fc,0xffff,2)
  for k in range(0x94):put(controls+0x14+k,0,1)
  put(controls+0x14,0);banks[0x6005d000]=bytearray([0xa5])*0x240
  fixture=owned/'fixture.json';fixture_data=dict(banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],control_record=controls+0x14)
  for key,case_name in(('camera_marker','marker_parent_True_0_1_123_False_False'),('look_marker','marker_parent_True_0_1_0_False_False')):
   mc=next(c for c in parent.cases(image)if c['name']==case_name);mb={b['address']:bytearray.fromhex(b['before'])for b in mc['banks']if b['address']!=parent.DATA}
   for p,b in banks.items():
    if p!=parent.base.BASE:mb[p]=bytearray(b)
   raw=mb[parent.base.BASE]
   for at,n in((controls+0x14,0),(globals+0xf4,parent.base.DEFINITION)):
    raw[at-parent.base.BASE:at-parent.base.BASE+4]=struct.pack('<I',n)
   raw[controls-parent.base.BASE+0x14:controls-parent.base.BASE+0x14+0x94]=bytes(0x94)
   if key=='look_marker':
    for off,value in((0x88,1.),(0x8c,2.)):raw[0x2800+off:0x2800+off+4]=struct.pack('<I',parent.base.bits(value))
   fixture_data[key]=dict(banks=[dict(address=p,before=b.hex())for p,b in sorted(mb.items())],control_record=controls+0x14)
  save(fixture,fixture_data)
  cpp='''#include "halo2/native_tu15_look_pipeline_binding.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <map>
#include <optional>
#include <fstream>
#include <xmmintrin.h>
#include <cstdlib>
using namespace halo2;using J=nlohmann::json;
void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Memory:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::vector<std::byte>>banks;unsigned writes{};
 auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing model read");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Model extent");return i;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
 void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);++writes;}
 void put(std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(8*k));write(p,b);}
};
struct Control {std::uint16_t x87_control_word{0x27f};std::uint32_t mxcsr{0x1f80};};
struct Frame {std::array<std::uint32_t,8>gp{};std::optional<Control>floating_point{Control{}};std::uint16_t x87_status{};unsigned x87_depth{};};
void source_execution_untracked(Memory&,std::uint32_t owner){check(owner==0x18a300u,"Actual Host look owner");}
struct Model {
 Memory bank;NativeTu15LookPipelineActivity native_look_pipeline;
 SourcePhysicsContactRecordObserver physics_store_observer;bool physics_store_observer_read_only{true};std::uint32_t last_instruction{},record{};unsigned executions{},observed_stores{},frames{};Frame last;
 struct Execution {Model&owner;explicit Execution(Model&o):owner(o){++owner.executions;}~Execution(){--owner.executions;}};
 Frame frame(std::span<const std::uint32_t>args){check(args.size()==2,"Actual look formals");++frames;return {};}
 void invoke(unsigned depth=0){const std::array<std::uint32_t,2>args{0x3dcccccdu,0xbd4ccccdu};auto a=frame(args);a.gp[0]=0;a.x87_depth=depth;struct Save{Model&owner;Frame&value;~Save(){owner.last=value;}}save{*this,a};
'''+host+'''
 last=a;}
 explicit Model(const J&fixture){for(const auto&b:fixture.at("banks")){std::vector<std::byte>v;auto s=b.at("before").get<std::string>();for(std::size_t n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));bank.banks.emplace(b.at("address"),std::move(v));}record=fixture.at("control_record");physics_store_observer=[this](const auto&e){check(last_instruction==e.instruction,"SourcePC publication");++observed_stores;};}
};
NativeTu15PhysicsFrameExecutionStats get(const Model*impl_){NativeTu15PhysicsFrameExecutionStats result;
'''+getter+'''return result;}
bool gate(NativeTu15PhysicsFrameExecutionStats native_frames){bool passed=true;J report;setenv("HALO2_REQUIRE_NATIVE_LOOK_CONTROLLER","1",1);
'''+gate+'''return passed;}
int main(int argc,char**argv){try{check(argc==2,"Supply declared input only");std::ifstream f(argv[1]);const auto fixture=J::parse(f);Model a(fixture);
'''+assignments+'''auto r=get(&a);'''+checks+'''J serialized={'''+rows+'''};check(serialized.size()==15,"15 fields in both serializers");
 NativeTu15PhysicsFrameExecutionStats p;p.native_look_update_attempts=p.native_look_update_calls=p.native_look_update_context_calls=p.native_look_update_camera_calls=p.native_look_update_camera_completions=1;check(gate(p),"Positive gate");auto b=p;
'''+''.join('b=p;'+n+'check(!gate(b),"Negative gate");'for n in negatives)+'''
 Model inactive(fixture);inactive.bank.put(inactive.record,0xffffffffu);inactive.invoke();check(inactive.native_look_pipeline.completed==1&&inactive.native_look_pipeline.camera.calls==0&&inactive.frames==1&&inactive.executions==0&&inactive.observed_stores>0,"Exact Host inactive-kernel scope");check(!gate(get(&inactive)),"Inactive camera was called positive");check(inactive.last.floating_point->x87_control_word==0x27f&&inactive.last.x87_depth==0,"Actual FP fields not published");
 Model refused(fixture);refused.physics_store_observer_read_only=false;const auto untouched=refused.bank.banks;bool failed{};try{refused.invoke();}catch(...){failed=true;}check(failed&&refused.bank.banks==untouched&&refused.native_look_pipeline.attempts==1&&refused.native_look_pipeline.refusals==1&&refused.native_look_pipeline.controller.calls==0&&refused.executions==0,"Observer admission before stores");
 Model deep(fixture);const auto before=deep.bank.banks;failed=false;try{deep.invoke(1);}catch(...){failed=true;}check(failed&&deep.bank.banks==before&&deep.native_look_pipeline.refusals==1&&deep.executions==0,"Unprovided x87 domain invented");
 Model query(fixture);failed=false;try{query.invoke();}catch(const std::runtime_error&e){failed=std::string(e.what()).find("16C620")!=std::string::npos;}check(failed&&query.executions==0&&query.native_look_pipeline.attempts==1&&query.native_look_pipeline.refusals==1&&query.native_look_pipeline.query_refusals==1&&query.native_look_pipeline.camera.segment_calls==1&&query.native_look_pipeline.controller.refusals==1,"Computed native geometric-query requirement was not counted/refused");check(!gate(get(&query)),"Unqualified query admitted");
 for(const auto kind:{"camera_marker","look_marker"}){Model marker(fixture.at(kind));failed=false;try{marker.invoke();}catch(const std::runtime_error&e){failed=std::string(e.what()).find("BAD60 semantic store-observer")!=std::string::npos;}check(failed&&marker.executions==0&&marker.native_look_pipeline.refusals==1&&marker.native_look_pipeline.marker_observer_refusals==1,"Unobserved BAD60 stores were silently admitted");const auto packet=std::string(kind)=="camera_marker"?0x6005d134u:0x6005d044u;std::array<std::byte,112>data{};marker.bank.read(packet,data);check(std::all_of(data.begin(),data.end(),[](auto b){return b==std::byte{0xa5};}),"BAD60 wrote before observation-domain refusal");if(std::string(kind)=="camera_marker")check(marker.native_look_pipeline.camera.markers==1&&marker.native_look_pipeline.marker_calls==0,"Camera marker attribution differs");else check(marker.native_look_pipeline.camera.completed==1&&marker.native_look_pipeline.marker_calls==1,"Look marker attribution differs");check(!gate(get(&marker)),"Marker observation gap admitted");}
 Model nested(fixture),outer(fixture);nested.bank.put(nested.record,0xffffffffu);outer.bank.put(outer.record,0xffffffffu);bool entered{};outer.physics_store_observer=[&](const auto&e){check(outer.last_instruction==e.instruction,"Nested SourcePC tee");++outer.observed_stores;if(!entered){entered=true;nested.invoke();}};outer.invoke();check(entered&&outer.executions==0&&nested.executions==0&&outer.native_look_pipeline.completed==1&&nested.native_look_pipeline.completed==1,"Nested independent observer/context lifetime differs");
 Model observer_error(fixture);observer_error.bank.put(observer_error.record,0xffffffffu);observer_error.physics_store_observer=[](const auto&){throw std::runtime_error("observer failure");};failed=false;try{observer_error.invoke();}catch(...){failed=true;}check(failed&&observer_error.executions==0&&observer_error.native_look_pipeline.refusals==1&&observer_error.last.floating_point->x87_control_word==0x27f,"Observer failure FP/Execution publication differs");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\\n';return 1;}}
''';cpp=cpp.replace('#include <fstream>','#include <fstream>\n#include <iostream>');(project/'model.cpp').write_text(cpp)
  import validate_native_look_pipeline_bound_actual_tu15 as actual
  results=[]
  for cc in('g++','clang++'):
   units=[]
   for n in actual.UNITS:units.append(stage/n if(stage/n).exists()else a.source_root/n)
   target=project/cc;cmd=[cc,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(project/'include'),str(project/'model.cpp'),*[str(q)for q in units],'-o',str(target)];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(cc+'-build.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:];r=subprocess.run([str(target),str(fixture)],capture_output=True,text=True);(a.output/(cc+'-run.log')).write_text(r.stdout+r.stderr);results.append(dict(compiler=cc,exit_code=r.returncode));assert r.returncode==0,r.stderr
  report.update(passed=True,compiler_results=results,counter_fields=15,serializers=2,negative_gate_cases=len(negatives),host_models=['exact fresh-frame fields/typed args/SourcePC tee/Execution scope','inactive kernel genuine return never mislabeled camera positive','nonreadonly observation refuses before native stores','unprovided live x87 refuses before native stores','positive native-computed geometry query is counted/refused without a CPU child','both camera and Look BAD60 paths refuse before any unobservable packet store','nested independent readonly observer/FP contexts restore','observer exception publishes actual FP fields and closes Execution'],source_hashes={str(q.relative_to(stage)):sha(q)for q in stage.rglob('*')if q.is_file()},model_sha256=sha(project/'model.cpp'),scope='Same production native-only helper under EXACT staged Host block. Declared ordinary input/authoritative mathDATA only; actual onfoot/seated whole-original proofs separate, no live qualification')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owned.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
