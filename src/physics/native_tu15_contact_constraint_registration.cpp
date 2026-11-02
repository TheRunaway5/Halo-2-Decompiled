#include "halo2/native_tu15_contact_constraint_registration.hpp"
#include "halo2/native_tu15_pair_creation_queues.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p,unsigned n=4)const {
        std::array<std::byte,4>b{};m_.read(p,std::span(b).first(n));std::uint32_t v{};
        for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(b[k])<<(8u*k);return v;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t v,unsigned n=4){
        std::array<std::byte,4>b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(v>>(8u*k));
        SourcePhysicsContactRecordStore e{pc,p,{},{}};
        if(o_){e.before.resize(n);m_.read(p,e.before);}
        m_.write(p,std::span(b).first(n));if(o_){e.written.assign(b.begin(),b.begin()+n);o_(e);}
    }
};
template<class F>void require(const F&f,const char*message){if(!f)throw std::runtime_error(message);}
template<class F>void attempt(NativeTu15ContactConstraintRegistrationStats*stats,F&&f){
    if(stats)++stats->attempts;
    try{f();if(stats)++stats->completed;}catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace

void native_tu15_register_island_contact_constraint(SourcePhysicsContactMemory&memory,
    std::uint32_t island,std::uint32_t constraint,std::uint32_t info,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ContactConstraintRegistrationServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ContactConstraintRegistrationStats*stats){
    attempt(stats,[&]{
        Memory m(memory,observe);
        const auto capacity=m.word(island+0x60u)&0x7fffffffu,count=m.word(island+0x5cu);
        if(count==capacity){NativeTu15ArrayGrowthMemory storage(memory,{island+0x58u},observe);
            native_tu15_grow_array(storage,4u,fp,services.allocation);if(stats)++stats->array_growths;}
        const auto current_count=m.word(island+0x5cu),data=m.word(island+0x58u);
        m.word(0x320e6eu,data+current_count*4u,constraint);
        const auto next_count=m.word(island+0x5cu)+1u;
        m.word(0x320e79u,island+0x5cu,next_count);
        const auto constraint_table=m.word(constraint),info_owner=m.word(constraint_table+0x28u);
        if(stats)++stats->info_queries;
        require(services.get_info,"Native TU1.5 contact registration requires constraint info owner");
        services.get_info(constraint,{info_owner},info,fp);
        const auto island_table=m.word(island),metadata_owner=m.word(island_table+8u);
        if(stats)++stats->metadata_updates;
        require(services.add_info,"Native TU1.5 contact registration requires island metadata owner");
        services.add_info(island,{metadata_owner},constraint,info,fp);
        m.word(0x320e91u,constraint+6u,m.word(constraint+6u,2)+1u,2);
        m.word(0x320e96u,constraint+0x14u,island);
    });
}

std::uint32_t native_tu15_register_world_contact_constraint(SourcePhysicsContactMemory&memory,
    std::uint32_t world,std::uint32_t constraint,std::uint32_t info,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ContactConstraintRegistrationServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ContactConstraintRegistrationStats*stats){
    attempt(stats,[&]{
        Memory m(memory,observe);
        const auto second_body=m.word(constraint+0x10u),second_island=m.word(second_body+0x44u);
        const auto first_body=m.word(constraint+0xcu),first_island=m.word(first_body+0x44u);
        const auto default_island=m.word(world+0x2cu);
        const auto append=[&](std::uint32_t island){
            native_tu15_register_island_contact_constraint(memory,island,constraint,info,fp,services,observe,stats);
            if(!m.word(island+0x29u,1)){
                if(stats)++stats->activation_requests;
                require(services.activate_island,"Native TU1.5 contact registration requires native island activation");
                services.activate_island(world,island,fp);
            }
        };
        if(first_island==default_island)append(second_island);
        else {
            if(second_island!=default_island&&first_island!=second_island){
                if(stats)++stats->merge_requests;
                native_tu15_queue_body_pair_merge(memory,world,first_body,second_body,fp,services.allocation,observe);
            }
            append(first_island);
        }
        native_tu15_notify_world_constraint_addition(memory,world,constraint,fp,services.listeners,observe,stats?&stats->notifications:nullptr);
    });
    return constraint;
}
} // namespace halo2
