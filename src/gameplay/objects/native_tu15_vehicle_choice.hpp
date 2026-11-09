#pragma once
#include "halo2/native_tu15_vehicle_choice_kernel.hpp"
#include "gameplay/objects/source_vehicle_choice_native_tu15_internal.hpp"
#include <cstdlib>

namespace halo2::native_vehicle_choice_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_VEHICLE_CHOICE");return p&&*p;}();return enabled;}
// Preserve the physical CPU's undefined shift flags in their original producer
// context. C++ memory providers must not accidentally seed native host flags.
inline std::uint32_t owner_flags(std::uint32_t owner) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t flags{},overflow{};
    asm volatile("andl $65535,%%ecx; shll $4,%%ecx; seto %%dl; lahf; movzbl %%ah,%%eax; movzbl %%dl,%%edx"
        :"+c"(owner),"=a"(flags),"=d"(overflow)::"cc");
    return flags|((overflow&1u)<<11u);
#else
    return 0;
#endif
}
inline std::uint32_t draw_flags(std::uint32_t seed,unsigned range) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t flags{},overflow{};
    if(range==5u)asm volatile("imull $0x19660d,%%ecx; addl $0x3c6ef35f,%%ecx; shrl $16,%%ecx; leal (%%ecx,%%ecx,4),%%ecx; shrl $16,%%ecx; incb %%cl; seto %%dl; lahf; movzbl %%ah,%%eax; movzbl %%dl,%%edx"
        :"+c"(seed),"=a"(flags),"=d"(overflow)::"cc");
    else if(range==2u)asm volatile("imull $0x19660d,%%ecx; addl $0x3c6ef35f,%%ecx; shrl $16,%%ecx; shll $1,%%ecx; shrl $16,%%ecx; incb %%cl; seto %%dl; lahf; movzbl %%ah,%%eax; movzbl %%dl,%%edx"
        :"+c"(seed),"=a"(flags),"=d"(overflow)::"cc");
    else asm volatile("imull $0x19660d,%%ecx; addl $0x3c6ef35f,%%ecx; shrl $16,%%ecx; shll $2,%%ecx; shrl $16,%%ecx; incb %%cl; seto %%dl; lahf; movzbl %%ah,%%eax; movzbl %%dl,%%edx"
        :"+c"(seed),"=a"(flags),"=d"(overflow)::"cc");
    return flags|((overflow&1u)<<11u);
#else
    return 0;
#endif
}
template<class State>struct Adapter {
    State&s;NativeTu15VehicleChoiceLayout p;std::uint32_t fixed{},args{};
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;}
    void push(std::uint32_t pc,std::uint32_t value) {
        at(pc);const auto address=s.gp[4]-4u;SourcePhysicsContactRecordStore e{pc,address,{},std::vector<std::byte>(4)};
        if(s.observe){e.before.resize(4);s.memory.read(address,e.before);}
        for(unsigned n=0;n<4;++n)e.written[n]=std::byte(value>>(8u*n));
        s.memory.write(address,e.written);s.gp[4]=address;if(s.observe)s.observe(e);
    }
    std::uint32_t pop(std::uint32_t pc){at(pc);const auto value=native_node_resource_value(s.memory,s.gp[4]);s.gp[4]+=4u;return value;}
    std::uint32_t choice()const{return native_node_resource_value(s.memory,s.gp[4]+12u,1);}
    std::uint32_t output()const{return native_node_resource_value(s.memory,s.gp[4]+12u);}
    std::uint32_t symbol_sink()const{return native_node_resource_value(s.memory,s.gp[4]+20u);}
    void random_argument(){s.gp[3]=native_node_resource_value(s.memory,s.gp[4]+24u);}
    void publish_symbol(const NativeTu15VehicleChoice&d) {
        const auto value=d.symbol?source_vehicle_compiled_symbol_tu15(s.memory,fixed+11u,d.symbol):0u;
        physics_world_runtime_detail::w(s.memory,s.observe,fixed+11u,d.symbol_sink,4,value);
    }
    void publish_seed(const NativeTu15VehicleChoice&d){physics_world_runtime_detail::w(s.memory,s.observe,p.draw+19u,d.seed_owner,4,d.next_seed);}
    std::uint32_t result()const{return s.gp[0];}
    void reselect(const NativeTu15VehicleChoice&d) {
        push(args,s.gp[3]);s.gp[1]=s.gp[6];push(args+3u,p.random_range==5u?s.gp[2]:s.gp[0]);
        push(args+4u,s.gp[7]);push(args+5u,args+10u);
        run(s,d.family);
    }
    static void run(State&s,NativeTu15VehicleChoiceFamily family) {
        using namespace physics_world_runtime_detail;
        vehicle_choice_native_calls.fetch_add(1,std::memory_order_relaxed);
        if(family==NativeTu15VehicleChoiceFamily::flying)vehicle_choice_native_flying_calls.fetch_add(1,std::memory_order_relaxed);
        Adapter child{s,native_tu15_vehicle_choice_layout(family)};
        native_tu15_vehicle_choice(s.memory,family,s.gp[1],child,[&](auto phase,const auto&d){child.progress(phase,d);});
    }
    void progress(NativeTu15VehicleChoicePhase phase,const NativeTu15VehicleChoice&d) {
        using P=NativeTu15VehicleChoicePhase;auto&g=s.gp;const bool early=p.early_choice;
        const unsigned cr=early?1u:2u,rr=early?2u:1u;
        switch(phase) {
        case P::save_object:push(p.entry,g[6]);g[6]=d.object;break;
        case P::context:at(p.entry+3u);break;
        case P::owner:g[1]=d.context;at(p.entry+9u);break;
        case P::rows:g[2]=d.owner;at(p.entry+15u);break;
        case P::definition:g[1]=d.rows;g[2]=(d.owner&0xffffu)*16u;s.flags.load(owner_flags(d.owner));at(p.entry+30u);break;
        case P::runtime:
            g[2]=d.definition;if(early)g[1]=d.choice;at(p.entry+(early?39u:34u));break;
        case P::choice:
            if(early)g[2]=d.definition;else g[1]=d.runtime;at(p.entry+(early?34u:37u));break;
        case P::save_output:g[cr]=d.choice;g[rr]=d.runtime;push(p.entry+0x2au,g[7]);break;
        case P::output:at(p.entry+0x2bu);break;
        case P::selected:g[7]=d.output;g[cr]=d.selected;s.flags.minus(d.selected,p.count-1u,4);g[0]=d.output;break;
        case P::table:at(p.dispatch);break;
        case P::references:fixed=p.fixed[d.selected];at(fixed);break;
        case P::symbol_sink:g[d.selected==0u?0u:2u]=d.references;at(fixed+3u);break;
        case P::reference:g[1]=d.symbol_sink;at(fixed+7u);break;
        case P::restore_output:g[0]=d.value;g[7]=pop(fixed+10u);break;
        case P::publish_symbol:at(fixed+11u);break;
        case P::restore_object:g[6]=pop(fixed+17u);break;
        case P::fixed_return:pop(fixed+18u);g[4]+=12u;break;
        case P::no_choice:g[0]=s.flags.logic(0xffffffffu,4);break;
        case P::finish:g[7]=pop(p.finish);g[6]=pop(p.finish+1u);pop(p.finish+2u);g[4]+=12u;break;
        case P::save_random:push(p.random,g[3]);break;
        case P::random_argument:at(p.random+1u);break;
        case P::seed_owner:physics_world_runtime_detail::vehicle_choice_native_random_draws.fetch_add(1,std::memory_order_relaxed);at(p.draw);break;
        case P::seed:g[1]=d.seed_owner;at(p.draw+6u);break;
        case P::update_seed:g[0]=s.flags.plus(d.seed*0x19660du,0x3c6ef35fu,4);at(p.draw+19u);break;
        case P::reselect:
            g[0]=d.next_seed>>16u;if(p.random_range==5u)g[2]=d.random_choice;else g[0]=d.random_choice;
            s.flags.load(draw_flags(d.seed,p.random_range));args=p.draw+(early?31u:32u);break;
        case P::retry:s.flags.minus(g[0],0xffffffffu,4);break;
        case P::random_return:g[3]=pop(p.ret);g[7]=pop(p.ret+1u);g[6]=pop(p.ret+2u);pop(p.ret+3u);g[4]+=12u;break;
        }
    }
};
template<class State>bool bridge(State&s,NativeTu15VehicleChoiceFamily family) {
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.observe&&!s.services.instruction)return false;
    if(s.services.instruction) {
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace)||(s.observe&&!c->observation_only_stores))return false;
        for(const auto pc:c->service_sites)if(pc>=0x15cb60u&&pc<=0x15cedau)return false;
    }
    Adapter<State>::run(s,family);return true;
#endif
}
} // namespace halo2::native_vehicle_choice_detail
