#!/usr/bin/env python3
"""Exact staged reorientation Host handoff, FP/observer and transport controls."""
import argparse
import json
import os
import shutil
import subprocess
from pathlib import Path
from prepare_native_look_reorientation_world_tu15 import ROOT,FIELDS,sha,save

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--stage',required=True,type=Path)
    parser.add_argument('--source-root',required=True,type=Path);parser.add_argument('--output',required=True,type=Path)
    a=parser.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        project=scratch/'project';project.mkdir();shutil.copytree(a.source_root/'include',project/'include')
        for p in(stage/'include').rglob('*.hpp'):shutil.copyfile(p,project/'include'/p.relative_to(stage/'include'))
        handoff=(a.stage/'host-handoff.cpp').read_text();assert 'run_actual'not in handoff and '.gp'not in handoff
        live=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();start=live.index('    const auto&look_reorientation=');end=live.index('    const auto&look_initialization=',start);getter=live[start:end]
        acceptance=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();start=acceptance.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_REORIENTATION"))');end=acceptance.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION"))',start);gate=acceptance[start:end]
        serialized=''.join(f'        {{"native_look_reorientation_{name}",native_frames.native_look_reorientation_{name}}},\n'for name in FIELDS);assert acceptance.count(serialized)==2
        assignments=''.join('m.native_look_reorientation.'+value+'='+str(17+n)+';'for n,value in enumerate(FIELDS.values()))
        getter_checks=''.join('check(r.native_look_reorientation_'+field+'=='+str(17+n)+',"Getter source mapping");'for n,field in enumerate(FIELDS))
        json_checks=''.join('check(j.at("native_look_reorientation_'+field+'") == '+str(17+n)+',"JSON source mapping");'for n,field in enumerate(FIELDS))
        negatives=['calls=0','attempts=2','refusals=1','kernel_calls=0','kernel_calls=2','kernel_refusals=1']
        cpp=r'''#include "halo2/native_tu15_look_reorientation_binding.hpp"
#include "halo2/native_tu15_look_reorientation_input_observer.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <map>
#include <optional>
#include <cstdlib>
#include <bit>
#include <xmmintrin.h>
using namespace halo2;
void check(bool b,const char*why){if(!b)throw std::runtime_error(why);}
struct Stop{};
struct Memory:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::byte> bytes;unsigned writes{},entries{};
 void put(std::uint32_t p,std::uint32_t n){for(unsigned k=0;k<4;++k)bytes[p+k]=std::byte(n>>(k*8u));}
 std::uint32_t word(std::uint32_t p)const{std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(bytes.at(p+k))<<(8u*k);return n;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{for(unsigned k=0;k<b.size();++k)b[k]=bytes.at(p+k);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{for(unsigned k=0;k<b.size();++k){check(bytes.contains(p+k),"Model DATA extent");bytes.at(p+k)=b[k];}++writes;}
};
struct Bytes {std::span<const std::byte> bytes;std::uint32_t u32(unsigned p)const{std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(bytes[p+k])<<(k*8u);return n;}};
struct Control{std::uint16_t x87_control_word{0x27f};std::uint32_t mxcsr{0x1f80};};
struct Frame{std::optional<Control> floating_point{Control{}};std::uint16_t x87_status{};unsigned x87_depth{};};
void source_execution_untracked(Memory&m,std::uint32_t owner){check(owner==0x18a2a0u,"Actual source owner");++m.entries;}
struct Model{
 Memory bank;NativeTu15LookReorientationBindingActivity native_look_reorientation;
 SourcePhysicsContactRecordObserver physics_store_observer;bool physics_store_observer_read_only{true};
 std::uint32_t last_instruction{};unsigned frames{},executions{},depth{};
 struct Execution{Model&owner;explicit Execution(Model&o):owner(o){++owner.executions;}~Execution(){--owner.executions;}};
 Frame frame(std::span<const std::uint32_t>args){check(args.empty(),"Actual zero-formal fresh Host frame");++frames;Frame a;a.x87_depth=depth;return a;}
 void reorient(std::span<const std::byte,12>completed_forward){constexpr std::uint32_t forward=0x6005e000;const std::uint32_t player=0x12340001,unit=0xabcd0001;
'''+handoff+r'''}
 std::array<std::byte,12> forward()const{std::array<std::byte,12> b{};bank.read(0x6005e000,b);return b;}
 Model(){bank.put(0x4f9b54,0x1000);for(unsigned n=0;n<0x94;++n)bank.bytes[0x1014+n]=std::byte{0xa5};bank.put(0x44923c,0x40c90fdb);
 bank.put(0x6005e000,0x3f800000);bank.put(0x6005e004,0xbf800000);bank.put(0x6005e008,0x3f000000);}
};
NativeTu15PhysicsFrameExecutionStats get(const Model*impl_){NativeTu15PhysicsFrameExecutionStats result;
'''+getter+r'''return result;}
bool gate(NativeTu15PhysicsFrameExecutionStats native_frames){bool passed=true;nlohmann::json report;setenv("HALO2_REQUIRE_NATIVE_LOOK_REORIENTATION","1",1);
'''+gate+r'''return passed;}
nlohmann::json serialize(NativeTu15PhysicsFrameExecutionStats native_frames){return {
'''+serialized+r'''};}
std::uint16_t cw(){std::uint16_t n;asm volatile("fnstcw %0":"=m"(n));return n;}
std::uint16_t sw(){std::uint16_t n;asm volatile("fnstsw %0":"=a"(n));return n;}
int main(){Model m;
'''+assignments+'auto r=get(&m);'+getter_checks+'auto j=serialize(r);'+json_checks+r'''
 NativeTu15PhysicsFrameExecutionStats p;p.native_look_reorientation_attempts=1;p.native_look_reorientation_calls=1;p.native_look_reorientation_kernel_calls=1;check(gate(p),"Positive gate");auto b=p;
'''+''.join('b=p;b.native_look_reorientation_'+control+';check(!gate(b),"Negative gate control");'for control in negatives)+r'''
 const auto host_cw=cw(),host_sw=sw();const auto host_mx=_mm_getcsr();
 Model plain;const auto forward=plain.forward();plain.reorient(forward);check(gate(get(&plain))&&plain.native_look_reorientation.reorientation.yaw_wraps==1&&plain.bank.writes==3&&plain.frames==1&&plain.executions==0,"Native exact Host completion");
 Model observed;unsigned input_calls{},stores{};NativeTu15LookReorientationInputObserverBinding input(observed.bank,[&](const auto&memory,const auto&v){
 check(&memory==&observed.bank&&observed.executions==0&&v.control_index==0&&v.forward==0x6005e000&&v.forward_words==std::array<std::uint32_t,3>{0x3f800000,0xbf800000,0x3f000000}&&v.floating_point.control_word==0x27f&&v.floating_point.status==0&&v.floating_point.mxcsr==0x1f80&&v.preserved_x87_depth==0&&v.caller_player==0x12340001&&v.caller_unit==0xabcd0001,"Preserved actual typed BEFORE fields");++input_calls;});
 observed.physics_store_observer=[&](const auto&e){check(observed.executions==1&&observed.last_instruction==e.instruction,"SourcePC/Execution observer phase");check(e.instruction==0x18a2bau||e.instruction==0x18a2ddu||e.instruction==0x18a2f3u,"Actual store owner");++stores;};observed.reorient(forward);
 check(input_calls==1&&stores==3&&observed.bank.bytes==plain.bank.bytes,"Readonly observation changed DATA");
 Model failed;failed.physics_store_observer=[](const auto&e){if(e.instruction==0x18a2bau)throw Stop{};};bool stopped{};try{failed.reorient(forward);}catch(const Stop&){stopped=true;}
 check(stopped&&failed.executions==0&&failed.native_look_reorientation.attempts==1&&failed.native_look_reorientation.refusals==1&&failed.native_look_reorientation.reorientation.calls==1&&failed.native_look_reorientation.reorientation.refusals==1&&failed.bank.writes==1,"Completed-store refusal publication");
 for(unsigned mode:{0u,1u}){Model refused;const auto before=refused.bank.bytes;if(mode==0){refused.physics_store_observer=[](const auto&){};refused.physics_store_observer_read_only=false;}else refused.depth=1;bool error{};try{refused.reorient(forward);}catch(const std::runtime_error&){error=true;}
 check(error&&refused.executions==0&&refused.bank.bytes==before&&refused.native_look_reorientation.attempts==1&&refused.native_look_reorientation.refusals==1&&refused.native_look_reorientation.reorientation.calls==0,"Guard refused before native DATA stores");}
 Model outer,inner;bool nested{};outer.physics_store_observer=[&](const auto&e){check(outer.last_instruction==e.instruction,"Outer storePC");if(!nested){nested=true;inner.reorient(inner.forward());}};outer.reorient(forward);check(outer.bank.bytes==plain.bank.bytes&&inner.bank.bytes==plain.bank.bytes&&outer.executions==0&&inner.executions==0,"Independent nested readonly owner changed DATA/lifetime");
 check(host_cw==cw()&&host_sw==sw()&&host_mx==_mm_getcsr(),"Native Host operation leaked hardware FP");
}
'''
        (project/'model.cpp').write_text(cpp);results=[]
        for compiler in ('g++','clang++'):
            binary=project/compiler.replace('+','x');cmd=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(project/'include'),str(project/'model.cpp'),str(stage/'src/physics/native_tu15_look_reorientation.cpp'),'-o',str(binary)]
            r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-5000:]
            r=subprocess.run([str(binary)],capture_output=True,text=True);assert r.returncode==0,(r.stdout,r.stderr);results.append(dict(compiler=compiler,passed=True))
        report.update(passed=True,compiler_results=results,counter_fields=6,serializers=2,negative_gate_controls=len(negatives),host_modes=6,
            model_sha256=sha(project/'model.cpp'),source_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},
            scope='Exact selected Host handoff + actual readonly BEFORE/store-PC scope/current FP/native resource/exception models and honest counter/getter/twoJSON gates. Original core/captured DATA proofs separate; not native gameplay or whole-frame acceptance.')
    except BaseException as e:report['error']=str(e)or repr(e);raise
    finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)

if __name__=='__main__':main()
