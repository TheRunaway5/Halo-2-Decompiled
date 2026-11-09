#pragma once
#include "halo2/native_tu15_page_window_kernel.hpp"
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"

namespace halo2::native_page_window_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_PAGE_ALLOCATOR");return p&&*p;}();return enabled;}
struct Activity {std::uint64_t calls{};};
inline Activity &activity(){thread_local Activity value;return value;}
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Before=Noop>struct Access {
    using P=NativeTu15PageWindowPhase;State &s;Before before{};
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;before(s,pc);}
    std::uint32_t read(P phase,const NativeTu15PageWindow&,std::uint32_t address){
        auto &g=s.gp;
        switch(phase){
        case P::owner_tick:at(0x13fe00u);break;
        case P::candidate_tick:at(0x13fe03u);s.push(0x13fe03u,g[3]);at(0x13fe04u);break;
        case P::best_tick:
            g[1]=s.flags.minus(g[2],g[3],4);s.set(0,s.flags.logic(0,1),1,0);s.flags.minus(g[1],300u,4);if(g[1]>300u)g[1]=300u;
            at(0x13fe1au);break;
        case P::candidate_count:at(0x13fe42u);break;
        case P::best_count:at(0x13fe45u);break;
        default:throw std::runtime_error("Invalid native page-window read boundary");
        }
        const auto value=physics_step_detail::r(s.memory,address,4);
        switch(phase){
        case P::owner_tick:case P::candidate_count:g[2]=value;break;
        case P::candidate_tick:g[3]=value;break;
        case P::best_tick:
            g[2]=s.flags.minus(g[2],value,4);s.flags.minus(g[2],300u,4);if(g[2]>300u)g[2]=300u;
            g[3]=g[1]+150u;s.flags.minus(g[3],g[2],4);at(0x13fe32u);g[3]=s.pop();break;
        case P::best_count:s.flags.minus(g[2],value,4);break;
        default:break;
        }
        return value;
    }
    void prepare(P phase,const NativeTu15PageWindow&){
        if(phase==P::age_range){s.gp[2]=s.flags.plus(s.gp[2],150u,4);s.flags.minus(s.gp[1],s.gp[2],4);}
    }
    void complete(P phase,const NativeTu15PageWindow &data){
        if(phase==P::early_return){s.set(0,s.flags.logic(0,1),1,0);at(0x13fe37u);s.pop();}
        else{if(data.prefer)s.set(0,1u,1,0);at(0x13fe4cu);s.pop();}
    }
};
template<class State,class Before=Noop>bool run(State &s,Before before={}){
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().calls;NativeTu15PageWindow data;data.owner=s.gp[1];data.candidate=s.gp[6];data.best=s.gp[7];Access<State,Before> access{s,before};
    native_tu15_prefer_page_window(access,data);return true;
}
} // namespace halo2::native_page_window_detail
