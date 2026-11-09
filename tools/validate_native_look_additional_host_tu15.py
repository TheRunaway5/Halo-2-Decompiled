#!/usr/bin/env python3
"""Exact two-site constructor blocks, restore/FP fields and counter transport."""
from pathlib import Path
import hashlib
import json
import os
import subprocess

ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-look-additional-host-stage-20261102'
OUT=ROOT/'analysis/physics/native-look-additional-host-model-20261102-second'


def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    OUT.mkdir(exist_ok=False)
    owned=Path(os.environ['HALO2_TEST_SCRATCH'])
    report=dict(passed=False,owned_scratch=str(owned),retained_large_artifacts=[])
    try:
        pins=json.loads((STAGE/'source-pins.json').read_text())
        for name,pin in pins['source_sha256'].items():assert sha(STAGE/'staged'/name)==pin,name
        remote=(STAGE/'remote-block.cpp.txt').read_text()
        collection=(STAGE/'input-block.cpp.txt').read_text()
        live=(STAGE/'staged/src/physics/physics_live_world_tu15.cpp').read_text()
        assert live.count(remote)==live.count(collection)==1
        assert all('run_actual'not in b and '.gp'not in b for b in(remote,collection))
        begin=live.index('            struct Restore {',live.index('    void prepare_player_look_births()'))
        tail='            bank.read(input,restore.bytes);put(bank,input,0xffffffffu);'
        end=live.index(tail,begin)+len(tail)
        remote_restore=live[begin:end]
        begin=live.index('    struct Context {',live.index('PhysicsLiveWorldTu15::collect_native_input('))
        tail='    context.save(input,0x94);'
        end=live.index(tail,begin)+len(tail)
        collection_restore=live[begin:end]
        begin=live.index('    const auto&look_initialization=')
        end=live.index('    result.world_integrations=',begin)
        getter=live[begin:end]
        host=(STAGE/'staged/tests/physics_multiplayer_acceptance.cpp').read_text()
        begin=host.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_REMOTE_BIRTH"))')
        end=host.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION"))',begin)
        gates=host[begin:end]
        fields=list(pins['fields'])
        rows=[[],[]]
        for name in fields:
            hits=[line.strip().rstrip(',') for line in host.splitlines() if line.strip().startswith('{"'+name+'",')]
            assert len(hits)==2,name
            for k,line in enumerate(hits):rows[k].append(line)
        model=r'''#include "halo2/native_tu15_look_initialization_binding.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <map>
#include <memory>
#include <optional>
#include <iostream>
#include <cstdlib>
using namespace halo2;
void check(bool ok,const char*s){if(!ok)throw std::runtime_error(s);}
struct Memory:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::byte>bytes;unsigned writes{};
 void put(std::uint32_t p,std::uint32_t v,unsigned n=4){for(unsigned k=0;k<n;++k)bytes[p+k]=std::byte(v>>(8*k));}
 std::uint32_t word(std::uint32_t p)const{std::uint32_t v{};for(unsigned k=0;k<4;++k)v|=std::uint32_t(bytes.at(p+k))<<(8*k);return v;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{for(unsigned k=0;k<b.size();++k)b[k]=bytes.at(p+k);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{for(unsigned k=0;k<b.size();++k){check(bytes.contains(p+k),"Outside declared DATA");bytes.at(p+k)=b[k];}++writes;}
};
struct Fp{std::uint16_t x87_control_word{0x27f};std::uint32_t mxcsr{0x1f80};};
struct Frame{std::optional<Fp>floating_point{Fp{}};std::uint16_t x87_status{0x20};unsigned x87_depth{};};
using SourceRuntimeMemory=Memory;
void put(Memory&m,std::uint32_t p,std::uint32_t v){m.put(p,v);}
void source_execution_untracked(Memory&,std::uint32_t owner){check(owner==0x18a0c0u,"Constructor identity");}
struct State{
 Memory bank;NativeTu15LookInitializationBindingActivity native_look_initialization;
 NativeTu15LookInitializationBindingActivity native_remote_look_birth_initialization,native_input_collection_look_initialization;
 SourcePhysicsContactRecordObserver physics_store_observer;bool physics_store_observer_read_only{true};
 std::uint32_t last_instruction{};unsigned executions{},frames{},legacy_calls{},observed{};
 std::array<std::byte,0x94>published{};
 struct Execution{State&s;explicit Execution(State&v):s(v){++s.executions;}~Execution(){--s.executions;}};
 Frame frame(std::span<const std::uint32_t>a){check(a.empty(),"No constructor stack arguments");++frames;return{};}
 void remote(std::uint32_t unit){
 const auto input=0x1014u;
'''+remote_restore+'\n'+remote+r''' bank.read(input,published);}
 State(){
  for(unsigned i=0;i<4;++i)for(unsigned k=0;k<0x94;++k)bank.bytes[0x1014+i*0x94+k]=std::byte{0xa5};
  bank.put(0x1000,0,1);bank.put(0x4f9b54,0x1000);bank.put(0x4ecbd0,0x6000);bank.put(0x6044,0x7000);bank.put(0x7000+12+8,0x8000);
  bank.put(0x8150,0x3f800000);bank.put(0x8154,0xbf000000);bank.put(0x8158,0x3f000000);bank.put(0x8214,0x12345678);bank.put(0x823d,0x80,1);bank.put(0x8241,0xff,1);
  bank.put(0x46a58c,0x3f800000);bank.put(0x46a588,0xbf800000);bank.put(0x44923c,0x40c90fdb);
 }
};
struct Host{using Impl=State;State*impl_;void collect(unsigned view,std::uint32_t unit){auto&bank=impl_->bank;
 const auto input=0x1014u+view*0x94u;
'''+collection_restore+'\n'+collection+r''' bank.read(input,impl_->published);}};
NativeTu15PhysicsFrameExecutionStats get(State*impl_){NativeTu15PhysicsFrameExecutionStats result;
'''+getter+r'''return result;}
bool gate(const NativeTu15PhysicsFrameExecutionStats&native_frames){bool passed=true;nlohmann::json report;
'''+gates+r'''return passed;}
void run(unsigned role,unsigned index,unsigned mode){State state;Host host{&state};const auto address=0x1014u+index*0x94u;
 std::array<std::byte,0x94>saved{};state.bank.read(address,saved);unsigned seen{};
 if(mode!=0)state.physics_store_observer=[&](const auto&e){check(e.instruction==state.last_instruction,"SourcePC tee");++seen;if(mode==3&&e.instruction==0x18a1a4u)throw std::runtime_error("Observed completed angle");};
 if(mode==2)state.physics_store_observer_read_only=false;
 bool failed{};try{if(role==0)state.remote(0xabcd0001u);else host.collect(index,0xabcd0001u);}catch(const std::runtime_error&){failed=true;}
 auto&activity=role?state.native_input_collection_look_initialization:state.native_remote_look_birth_initialization;
 check(activity.attempts==1&&activity.completed==unsigned(mode<2)&&activity.refusals==unsigned(mode>=2),"Per-site native balance");
 check(failed==(mode>=2)&&state.frames==1&&!state.executions&&!state.legacy_calls,"Native entry/scope/old-owner");
 check((role?state.native_remote_look_birth_initialization:state.native_input_collection_look_initialization).attempts==0,"Other site counted");
 if(mode<2){std::uint32_t published_unit{};for(unsigned k=0;k<4;++k)published_unit|=std::uint32_t(state.published[k])<<(8*k);check(published_unit==0xabcd0001u&&activity.initialization.unit_directions==1,"Actual index/unit publication");check(seen>60||mode==0,"Observer selected");}
 if(mode==2){std::array<std::byte,0x94>current{};state.bank.read(address,current);check(current==saved&&seen==0&&activity.initialization.calls==0,"Refusal touched kernel DATA");}
 if(mode==3)check(seen>60&&state.bank.writes>60,"Throw was not after committed nonREP angle");
 // Exact surrounding production restore fragments execute on both normal
 // and exceptional returns; no test-side repair supplies restored bytes.
 std::array<std::byte,0x94>restored{};state.bank.read(address,restored);check(restored==saved,"Caller restore");
 auto values=get(&state);const auto observed_calls=role?values.native_look_initialization_input_collection_calls:values.native_look_initialization_remote_birth_calls;
 check(observed_calls==unsigned(mode<2)&&values.native_look_initialization_attempts==1&&values.native_look_initialization_calls==unsigned(mode<2),"Getter aggregation");
}
int main(){for(unsigned mode=0;mode<4;++mode){run(0,0,mode);for(unsigned view=0;view<4;++view)run(1,view,mode);}
 State state;state.native_look_initialization.attempts=10;state.native_look_initialization.completed=8;state.native_look_initialization.refusals=2;
 state.native_remote_look_birth_initialization.attempts=17;state.native_remote_look_birth_initialization.completed=16;state.native_remote_look_birth_initialization.refusals=1;
 state.native_input_collection_look_initialization.attempts=23;state.native_input_collection_look_initialization.completed=21;state.native_input_collection_look_initialization.refusals=2;
 auto native_frames=get(&state);check(native_frames.native_look_initialization_attempts==50&&native_frames.native_look_initialization_calls==45&&native_frames.native_look_initialization_refusals==5,"Existing aggregate lost sites");
 nlohmann::json first={'''+','.join(rows[0])+r'''},second={'''+','.join(rows[1])+r'''};check(first==second&&first.size()==6,"Exact twoJSON fields");
 check(first["native_look_initialization_remote_birth_attempts"]==17&&first["native_look_initialization_remote_birth_calls"]==16&&first["native_look_initialization_remote_birth_refusals"]==1,"Remote per-site getter/JSON values");
 check(first["native_look_initialization_input_collection_attempts"]==23&&first["native_look_initialization_input_collection_calls"]==21&&first["native_look_initialization_input_collection_refusals"]==2,"Input per-site getter/JSON values");
 for(unsigned role=0;role<2;++role){unsetenv("HALO2_REQUIRE_NATIVE_LOOK_REMOTE_BIRTH");unsetenv("HALO2_REQUIRE_NATIVE_LOOK_INPUT_COLLECTION");setenv(role?"HALO2_REQUIRE_NATIVE_LOOK_INPUT_COLLECTION":"HALO2_REQUIRE_NATIVE_LOOK_REMOTE_BIRTH","1",1);NativeTu15PhysicsFrameExecutionStats good;
 if(role){good.native_look_initialization_input_collection_attempts=1;good.native_look_initialization_input_collection_calls=1;}else{good.native_look_initialization_remote_birth_attempts=1;good.native_look_initialization_remote_birth_calls=1;}check(gate(good),"Per-site positive gate");
 for(unsigned control=0;control<3;++control){auto bad=good;if(role){if(control==0)bad.native_look_initialization_input_collection_calls=0;if(control==1)++bad.native_look_initialization_input_collection_attempts;if(control==2)++bad.native_look_initialization_input_collection_refusals;}else{if(control==0)bad.native_look_initialization_remote_birth_calls=0;if(control==1)++bad.native_look_initialization_remote_birth_attempts;if(control==2)++bad.native_look_initialization_remote_birth_refusals;}check(!gate(bad),"Per-site bad balance admitted");}}
 std::cout<<"20 exact two-site/index/observer controls,6counter/twoJSON/6negative gates passed\n";}
'''
        p=owned/'model.cpp';p.write_text(model)
        for cc in('g++','clang++'):
            binary=owned/cc
            command=[cc,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                '-I'+str(STAGE/'staged/include'),'-I'+str(ROOT/'include'),str(p),
                str(STAGE/'staged/src/physics/native_tu15_look_initialization.cpp'),'-o',str(binary)]
            built=subprocess.run(command,capture_output=True,text=True);(OUT/(cc+'-build.log')).write_text(built.stdout+built.stderr)
            assert built.returncode==0,built.stderr[-6000:]
            run=subprocess.run([str(binary)],capture_output=True,text=True);(OUT/(cc+'-run.log')).write_text(run.stdout+run.stderr)
            assert run.returncode==0,run.stderr
        report.update(passed=True,source_sha256=pins['source_sha256'],generated_model_sha256=sha(p),
            exact_selected_host_blocks=2,host_controls_per_compiler=20,per_site_scalar_counters=6,
            exact_json_serializers=2,negative_gates_per_compiler=6,old_selected_owner_calls=0,
            kernel_arithmetic_unchanged=True,physical_core_and_actual_bank_evidence_inherited=True,
            source_observer_throw_scope='After committed nonREP angle only; Host scope/restore test, no new arbitrary observer fault equivalence.',
            scope='Exact additional canonical Host blocks and counters/gates; current foreign caller predicates preserved. No generated scheduler, native device collection, fullTUs or live claim.')
    except BaseException as error:
        report['error']=str(error)or repr(error)
        raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owned.rglob('*')if p.is_file())
        (OUT/'report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()
