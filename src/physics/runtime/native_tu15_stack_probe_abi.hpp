#pragma once
#include "halo2/native_tu15_stack_probe_kernel.hpp"
#include "physics/runtime/native_tu15_page_window_abi.hpp"

namespace halo2::native_stack_probe_detail {
struct Activity {std::uint64_t calls{};};
inline Activity &activity(){thread_local Activity value;return value;}
template<class State,class Before=native_page_window_detail::Noop>struct Access {
    using P=NativeTu15StackProbePhase;State &s;Before before{};std::uint32_t canonical_pc_offset{};
    void at(std::uint32_t pc){pc-=canonical_pc_offset;if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;before(s,pc);}
    void prepare(P phase,const NativeTu15StackProbe&){
        auto &g=s.gp;
        if(phase==P::begin){s.flags.logic(g[0],4);if(g[0])s.flags.minus(g[0],4096u,4);}
        else if(phase==P::large_begin){at(0x32a679u);s.push(0x32a679u-canonical_pc_offset,g[1]);g[1]=g[4]+8u;}
        else if(phase==P::page_compare)s.flags.minus(g[0],4096u,4);
    }
    std::uint32_t read(P phase,const NativeTu15StackProbe&,std::uint32_t address){
        auto &g=s.gp;
        switch(phase){
        case P::small_probe:g[0]=s.flags.minus(0,g[0],4);g[0]=s.flags.plus(g[0],g[4],4);g[0]=s.flags.plus(g[0],4u,4);at(0x32a672u);break;
        case P::small_return_word:std::swap(g[4],g[0]);at(0x32a675u);break;
        case P::page_probe:g[1]=s.flags.minus(g[1],4096u,4);g[0]=s.flags.minus(g[0],4096u,4);at(0x32a689u);break;
        case P::tail_probe:g[1]=s.flags.minus(g[1],g[0],4);g[0]=g[4];at(0x32a696u);break;
        case P::saved_counter:g[4]=g[1];at(0x32a69au);break;
        case P::return_word:at(0x32a69cu);break;
        default:throw std::runtime_error("Invalid native stack probe read boundary");
        }
        const auto value=physics_step_detail::r(s.memory,address,4);
        switch(phase){
        case P::small_probe:case P::page_probe:case P::tail_probe:s.flags.logic(value&g[0],4);break;
        case P::small_return_word:case P::return_word:g[0]=value;break;
        case P::saved_counter:g[1]=value;break;
        default:break;
        }
        return value;
    }
    void write(P phase,const NativeTu15StackProbe&,std::uint32_t,std::uint32_t value){const auto pc=phase==P::small_return_store?0x32a677u:0x32a69fu;at(pc);s.push(pc-canonical_pc_offset,value);}
    void complete(P phase,const NativeTu15StackProbe&){at(phase==P::large_return?0x32a6a0u:0x32a678u);s.pop();}
};
template<class State,class Before=native_page_window_detail::Noop>bool run(State &s,Before before={},std::uint32_t canonical_pc_offset=0){
    if(native_page_window_detail::reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().calls;NativeTu15StackProbe data;data.bytes=s.gp[0];data.entry_sp=s.gp[4];Access<State,Before> access{s,before,canonical_pc_offset};native_tu15_probe_stack(access,data);return true;
}
} // namespace halo2::native_stack_probe_detail
