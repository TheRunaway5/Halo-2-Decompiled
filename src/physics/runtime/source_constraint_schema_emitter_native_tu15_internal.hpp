#pragma once
#include "halo2/native_constraint_schema_emitter_consumer_tu15.hpp"
#include "halo2/native_tu15_constraint_schema_emitter.hpp"
#include <stdexcept>
namespace halo2::physics_world_runtime_detail {
template<class State>[[noreturn]]void schema_emitter_native_refuse(State&s,NativeConstraintSchemaEmitterRefusalTu15 why,const char*message){native_constraint_schema_emitter_refused_tu15(s.memory,why);throw std::runtime_error(message);}
template<class State>void schema_emitter_native_observation(State&s,const NativeConstraintSchemaEmitterConsumerPolicyTu15&policy){
    if((policy.first_trace&&*policy.first_trace)||(policy.second_trace&&*policy.second_trace)||(s.observe&&!policy.observation_only_stores))schema_emitter_native_refuse(s,NativeConstraintSchemaEmitterRefusalTu15::Observation,"Native live schema emitter refuses full instruction or mutable store observation");
}
inline std::uint32_t schema_emitter_input_word(SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t result{};for(unsigned n=0;n<4;++n)result|=std::to_integer<unsigned>(b[n])<<(n*8u);return result;}
template<class State>bool try_native_constraint_schema_emitter_tu15(State&s){
    const auto*policy=native_constraint_schema_emitter_policy_tu15(s.memory);
    if(!policy){if(native_constraint_schema_emitter_bank_scoped_tu15(s.memory))schema_emitter_native_refuse(s,NativeConstraintSchemaEmitterRefusalTu15::Executor,"Native live schema emitter refuses another executor's retained bank");return false;}
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)schema_emitter_native_refuse(s,NativeConstraintSchemaEmitterRefusalTu15::Image,"Native live schema emitter requires TU1.5");
    schema_emitter_native_observation(s,*policy);const auto sp=s.gp[4];
    if(sp<4u||sp-4u<policy->caller_stack_begin||std::uint64_t(sp)+20u>policy->caller_stack_end)schema_emitter_native_refuse(s,NativeConstraintSchemaEmitterRefusalTu15::Stack,"Native live schema emitter requires ordinary retained caller storage");
    const auto caller=policy->current_actual_phase();unsigned kind;
    if(caller==0x31f7c3u)kind=0;else if(caller==0x31f8a0u)kind=1;else if(caller==0x31f95eu)kind=2;else schema_emitter_native_refuse(s,NativeConstraintSchemaEmitterRefusalTu15::Caller,"Native live schema emitter requires an authenticated actual internal caller");
    native_constraint_schema_emitter_call_tu15(s.memory,kind);
    const auto phase=[&](std::uint32_t site){schema_emitter_native_observation(s,*policy);policy->publish_phase(site);};
    phase(0x32a390u);const auto bytes=schema_emitter_input_word(s.memory,sp+8u);phase(0x32a394u);const NativeTu15ConstraintSchemaPacket packet{schema_emitter_input_word(s.memory,sp+0x10u)};
    SourcePhysicsContactRecordObserver observe;
    if(s.observe)observe=[&](const SourcePhysicsContactRecordStore&event){auto mapped=event;bool actual=false;if constexpr(requires{s.selected_observation_pcs;})actual=s.selected_observation_pcs;
        if(!actual){if(mapped.instruction==0x32a3a8u)mapped.instruction=0x326838u;else if(mapped.instruction==0x32a3aeu)mapped.instruction=0x32683eu;else if(mapped.instruction==0x32a3b6u)mapped.instruction=0x326846u;else throw std::runtime_error("Unbound native schema emitter store observation");}s.observe(mapped);};
    const auto result=native_tu15_emit_custom_constraint_schema(s.memory,bytes,packet,[&]{return NativeTu15PhysicsVirtualOwner{schema_emitter_input_word(s.memory,sp+4u)};},observe,nullptr,phase);
    s.gp[0]=result;phase(0x32a3bau);const auto target=s.pop();
    if(target!=caller+5u)schema_emitter_native_refuse(s,NativeConstraintSchemaEmitterRefusalTu15::Caller,"Native live schema emitter returned to an unqualified actual continuation");
    native_constraint_schema_emitter_completed_tu15(s.memory);return true;
}
} // namespace halo2::physics_world_runtime_detail
