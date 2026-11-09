#pragma once
#include "halo2/native_tu15_animation_resource_read_kernel.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include "physics/runtime/native_tu15_halfword_bit_query.hpp"
#include <cstdlib>

namespace halo2::native_animation_resource_read_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_ANIMATION_RESOURCE_READ");return p&&*p;}();return enabled;}
template<class State>std::uint32_t shifted_mode(State&s,std::uint32_t value){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);unsigned char overflow;
    asm volatile("addb $127,%%al;sahf;shll $2,%1;seto %2;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+r"(value),"=qm"(overflow)::"cc");
    s.flags.load((flags&0xd5u)|(std::uint32_t(overflow)<<11u));return value;
#else
    return s.flags.shift(value,2u,4);
#endif
}
template<class State,class Call>bool bridge(State&s,Call call){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#endif
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
        if(s.observe&&!c->observation_only_stores)return false;
        for(const auto site:c->service_sites)if(site>=0x1260c0u&&site<=0x12617eu)return false;
    }
    using P=NativeTu15AnimationReadPhase;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto progress=[&](P phase,const NativeTu15AnimationRead&d){
        auto&g=s.gp;
        switch(phase){
        case P::cache:at(0x1260c0u);break;
        case P::resource:at(0x1260d0u);s.push(0x1260d0u,g[7]);g[7]=d.tracking;at(0x1260d3u);break;
        case P::pending:g[2]=d.resource;at(0x1260d6u);break;
        case P::pages:at(0x1260dau);break;
        case P::handle:g[5]=d.pages;at(0x1260ddu);break;
        case P::rows:g[0]=d.handle;at(0x1260e0u);break;
        case P::shift:g[5]=d.rows;at(0x1260e3u);break;
        case P::offset:g[1]=d.shift;g[3]=d.tracking+4u;g[0]=(d.handle&65535u)*3u;s.flags.logic(d.handle&65535u,4);at(0x1260f1u);break;
        case P::base:g[0]=native_halfword_bit_query_detail::shifted_mask(s,d.offset,d.shift);at(0x1260f7u);break;
        case P::bytes:g[0]=d.destination;s.flags.plus(d.offset<<(d.shift&31u),d.base,4);at(0x1260fdu);break;
        case P::ready:
            g[1]=d.rounded_bytes;s.flags.logic(d.bytes&511u,4);
            if(d.bytes&511u){s.flags.logic(d.bytes|511u,4);s.flags.increment(d.bytes|511u,4,false);}at(0x12610fu);break;
        case P::priority:g[6]=shifted_mode(s,d.mode*3u);at(0x126118u);break;
        case P::file:at(0x126122u);break;
        case P::epoch:g[5]=d.file;at(0x126125u);break;
        case P::job:at(0x12613eu);break;
        case P::refresh:at(0x12614eu);break;
        case P::time:g[1]=d.cache;at(0x126154u);break;
        case P::age:g[2]=d.time;at(0x126157u);break;
        case P::resource_again:g[3]=d.age;at(0x12615du);break;
        case P::handle_again:g[0]=d.resource;at(0x126160u);break;
        case P::pages_again:g[0]=d.handle;at(0x126163u);break;
        case P::rows_again:g[1]=d.pages;at(0x126166u);break;
        case P::finished:
            g[1]=d.rows;at(0x126169u);g[7]=s.pop();g[2]=s.flags.minus(d.time,d.age,4);
            at(0x12616cu);g[6]=s.pop();g[0]=(d.handle&65535u)*3u;s.flags.logic(d.handle&65535u,4);
            at(0x126175u);g[5]=s.pop();at(0x126176u);physics_step_detail::w(s.memory,s.observe,0x126176u,d.rows+(d.handle&65535u)*24u+20u,4,g[2]);
            at(0x12617au);g[3]=s.pop();g[4]=s.flags.plus(g[4],8u,4);at(0x12617eu);s.pop();g[4]+=8u;break;
        }
    };
    struct Owners{
        State&s;Call&call;decltype(at)&position;
        std::uint32_t read_word(std::uint32_t address,std::uint32_t pc){position(pc);return native_node_resource_value(s.memory,address);}
        void store(std::uint32_t address,std::uint32_t value,unsigned width,std::uint32_t site){physics_step_detail::w(s.memory,s.observe,site,address,width,value);}
        void invoke(std::uint32_t entry,std::uint32_t site,std::uint32_t continuation){position(site);s.push(site,continuation);call(s,entry,site,false);}
        std::uint32_t mode(const NativeTu15AnimationRead&d){
            auto&g=s.gp;g[1]=d.cache;g[4]=s.flags.minus(g[4],8u,4);
            position(0x1260c9u);s.push(0x1260c9u,g[3]);position(0x1260cau);s.push(0x1260cau,g[5]);position(0x1260cbu);s.push(0x1260cbu,g[6]);
            g[6]=read_word(g[4]+28u,0x1260ccu);return g[6];
        }
        void priority(const NativeTu15AnimationRead&d){s.gp[5]=d.priority;position(0x12611eu);store(s.gp[4]+16u,d.priority,4,0x12611eu);}
        void epoch(const NativeTu15AnimationRead&d){s.gp[2]=d.epoch;position(0x126127u);store(s.gp[4]+20u,d.epoch,4,0x126127u);}
        std::uint32_t saved_priority(const NativeTu15AnimationRead&){s.gp[2]=read_word(s.gp[4]+16u,0x12612bu);return s.gp[2];}
        NativeTu15AnimationReadResume resume()const{return{s.gp[7],s.gp[6],s.gp[0]};}
        NativeTu15AnimationReadResume read(const NativeTu15AnimationRead&d){
            for(const auto [pc,value]:std::array<std::pair<std::uint32_t,std::uint32_t>,6>{{{0x12612fu,d.priority},{0x126130u,6u},{0x126132u,s.gp[3]},{0x126133u,0u},{0x126135u,s.gp[0]},{0x126136u,s.gp[1]}}}){position(pc);s.push(pc,value);}
            s.gp[1]=s.gp[5];invoke(0x1a3b70u,0x126139u,0x12613eu);return resume();
        }
        std::uint32_t wait_mode(const NativeTu15AnimationRead&){s.gp[0]=read_word(s.gp[4]+32u,0x126141u);s.flags.logic(s.gp[0],4);return s.gp[0];}
        NativeTu15AnimationReadResume wait(const NativeTu15AnimationRead&){invoke(0x1262b0u,0x126149u,0x12614eu);return resume();}
    }owners{s,call,at};
    native_tu15_read_animation_resource(s.memory,s.gp[0],owners,progress);return true;
}
} // namespace halo2::native_animation_resource_read_detail
