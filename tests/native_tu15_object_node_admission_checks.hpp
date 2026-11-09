#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <stdexcept>

namespace halo2::object_node_test_detail {
template<class Bridge,class Reference>
unsigned check_live_admission_fallbacks(Bridge bridge,Reference reference){
    using namespace physics_world_runtime_detail;
    struct Stop {};
    struct Memory:SourcePhysicsContactMemory {
        mutable unsigned reads{};unsigned writes{};
        void read(std::uint32_t,std::span<std::byte>)const override{++reads;throw Stop{};}
        void write(std::uint32_t,std::span<const std::byte>)override{++writes;throw Stop{};}
    };
    struct Result {SourcePhysicsWorldRuntimeAbi abi;unsigned callbacks{},observations{},reads{},writes{};std::uint32_t pc=0x12345678u;};
    const auto equal=[](const auto&a,const auto&b){
        if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_depth!=b.x87_depth||a.x87_status!=b.x87_status||
           a.floating_point->mxcsr!=b.floating_point->mxcsr||a.floating_point->x87_control_word!=b.floating_point->x87_control_word)return false;
        for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;
        return true;
    };
    unsigned passed{};
    for(unsigned policy=0;policy<13;++policy){
        const auto run=[&](bool candidate){
            Memory memory;Result result;bool stop_callback{},first{},second{};
            SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;
            services.instruction=[&](auto pc){result.pc=pc;++result.callbacks;if(stop_callback)throw Stop{};};
            constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
            constexpr std::array extra{0x38846eu,0x21f590u,0x33b595u,0x123456u};
            auto&cursor=services.instruction_cursor;
            cursor={&result.pc,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
            WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
            SourcePhysicsWorldRuntimeAbi incoming{};incoming.gp={1u,2u,3u,4u,0x62001000u,6u,7u,8u};incoming.eflags=0x2d7u;
            incoming.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};incoming.x87_depth=2;
            incoming.x87[0]=1.25L;incoming.x87[1]=-2.5L;state.load(incoming);
            // Authentic AFTER-construction changes invalidate cached policy.
            stop_callback=true;
            if(policy==0)services.instruction={};
            if(policy==1)services.instruction=[&,different_type=1u](auto pc){result.pc=pc;++result.callbacks;(void)different_type;throw Stop{};};
            if(policy==2)cursor.executor={};
            if(policy==3)cursor.callback_type=nullptr;
            if(policy==4)cursor.callback_type=&typeid(int);
            if(policy==5)cursor.position=nullptr;
            if(policy==6)cursor.literal_service_sites=false;
            auto detached=cursor;if(policy==7)state.instruction_cursor=&detached;
            if(policy==8)cursor.service_sites={};
            if(policy==9)cursor.service_sites=extra;
            if(policy==10)first=true;
            if(policy==11)second=true;
            if(policy==12){observe=[&](const auto&){++result.observations;};cursor.observation_only_stores=false;}
            bool stopped{};
            try{
                if(candidate){
                    const auto before=state.save();
                    if(bridge(state))throw std::runtime_error("Unsafe live object-node policy admitted");
                    if(!equal(before,state.save())||result.pc!=0x12345678u||result.callbacks||result.observations||memory.reads||memory.writes)
                        throw std::runtime_error("Live object-node refusal touched retained state");
                }
                // Execute the genuine complete retained body. Clearing the
                // callback reaches its first real memory fault; every other
                // policy delivers the actual first callback before any IO.
                reference(state);
            }catch(const Stop&){stopped=true;}
            if(!stopped)throw std::runtime_error("Live object-node fallback missed actual retained boundary");
            result.abi=state.save();result.reads=memory.reads;result.writes=memory.writes;return result;
        };
        const auto expected=run(false),actual=run(true);
        if(!equal(expected.abi,actual.abi)||expected.pc!=actual.pc||expected.callbacks!=actual.callbacks||
           expected.observations!=actual.observations||expected.reads!=actual.reads||expected.writes!=actual.writes||
           (policy==0?(actual.callbacks!=0||actual.pc!=0x12345678u||actual.reads+actual.writes!=1):
                       (actual.callbacks!=1||actual.reads||actual.writes)))
            throw std::runtime_error("Live object-node genuine fallback differs, policy="+std::to_string(policy));
        ++passed;
    }
    return passed;
}

template<class Memory,class Bridge>
unsigned check_admission_refusals(Bridge bridge,std::uint32_t begin,std::uint32_t end,bool reference_only=false){
    using namespace physics_world_runtime_detail;
    struct SelectedState:WorldState {using WorldState::WorldState;bool selected_observation_pcs=true;};
    unsigned passed{};
    for(unsigned index=0;index<(reference_only?1u:13u);++index){const auto refusal=reference_only?13u:index;
        Memory memory;unsigned callbacks{},observations{};std::uint32_t position=0x12345678u;bool first{},second{};
        SourcePhysicsContactRecordObserver observer=[&](const auto&){++observations;};SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](std::uint32_t){++callbacks;};auto&cursor=services.instruction_cursor;
        std::array sites{0x38846eu,0x21f590u,0x33b595u},changed_sites{begin,end,0x33b595u};
        cursor.position=&position;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;cursor.observation_only_stores=true;cursor.first_trace=&first;cursor.second_trace=&second;
        if(refusal==1)cursor.executor={};
        if(refusal==2)cursor.position=nullptr;
        if(refusal==3)cursor.literal_service_sites=false;
        if(refusal==4)first=true;
        if(refusal==5)second=true;
        if(refusal==6)cursor.observation_only_stores=false;
        if(refusal==7)services.instruction={};
        if(refusal==8)cursor.callback_type=nullptr;
        if(refusal==9)services.instruction=[](std::uint32_t){};
        SelectedState state{memory,observer,services,refusal==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp={1u,2u,3u,4u,5u,6u,7u,8u};abi.eflags=0x2d7u;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};state.load(abi);
        if(refusal==10)state.selected_observation_pcs=false;
        if(refusal==11)state.active_mmx=SourcePhysicsMmxAlias{};
        if(refusal==12)cursor.service_sites=changed_sites;
        const auto before=state.save();const auto banks=memory.banks;
        const bool accepted=bridge(state);const auto after=state.save();
        const bool mmx_same=bool(before.active_mmx)==bool(after.active_mmx)&&(!before.active_mmx||
            (before.active_mmx->known_mask==after.active_mmx->known_mask&&before.active_mmx->physical==after.active_mmx->physical));
        if(accepted||before.gp!=after.gp||before.eflags!=after.eflags||before.xmm!=after.xmm||before.x87_status!=after.x87_status||!mmx_same||
            before.floating_point->mxcsr!=after.floating_point->mxcsr||before.floating_point->x87_control_word!=after.floating_point->x87_control_word||
            banks!=memory.banks||!memory.reads.empty()||callbacks||observations||position!=0x12345678u)
            throw std::runtime_error("Object node admission refusal mutated state, case="+std::to_string(refusal));
        ++passed;
    }
    return passed;
}
} // namespace halo2::object_node_test_detail
