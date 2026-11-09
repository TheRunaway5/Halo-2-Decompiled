#pragma once
#include "halo2/native_tu15_word_copy_kernel.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_object_node_admission.hpp"

namespace halo2::native_object_prediction_copy_detail {
inline bool reference_mode(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_OBJECT_PREDICTION_COPY");return p&&*p;}();return value;}
struct Activity {std::uint64_t entries{};};
inline Activity&activity(){thread_local Activity value;return value;}
// Actual ADCD0, reached by registered object-entity descriptor virtual+88.
// The actual-owner dispatcher supplies the authenticated runtime entry.
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Before=Noop>struct Access {
    State &s;Before before{};
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;before(s,pc);}
    std::uint32_t read_word(std::uint32_t address){at(0xadce2u);return physics_step_detail::r(s.memory,address,4u);}
    void write_word(std::uint32_t address,std::uint32_t value){at(0xadce2u);physics_step_detail::w(s.memory,s.observe,0xadce2u,address,4u,value);}
    void advanced(const NativeTu15WordCopy&data){s.gp[6]=data.source;s.gp[7]=data.destination;s.gp[1]=data.remaining;}
};
template<class State,class Before=Noop>void run_private(State &s,Before before={}){
    Access<State,Before> memory{s,before};auto &g=s.gp;
    memory.at(0xadcd0u);s.push(0xadcd0u,g[6]);
    memory.at(0xadcd1u);g[6]=physics_step_detail::r(s.memory,g[4]+12u,4u);
    memory.at(0xadcd5u);s.push(0xadcd5u,g[7]);
    memory.at(0xadcd6u);g[7]=physics_step_detail::r(s.memory,g[4]+20u,4u);
    g[6]=s.flags.plus(g[6],0x68u,4u);g[1]=10u;
    NativeTu15WordCopy copy{g[6],g[7],g[1],0u};native_tu15_copy_words(memory,copy,(s.other_flags&1024u)!=0u);
    memory.at(0xadce4u);g[7]=s.pop();s.set(0,1u,1u,0u);
    memory.at(0xadce7u);g[6]=s.pop();memory.at(0xadce8u);s.pop();g[4]+=12u;
}
template<class State>bool bridge(State&s){
    if(reference_mode()||!native_object_node_admission_detail::admitted(s,{{0xadcd0u,0xadce8u}}))return false;
    ++activity().entries;run_private(s);return true;
}
} // namespace halo2::native_object_prediction_copy_detail
