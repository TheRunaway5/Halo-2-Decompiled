#pragma once
// Boundary adapter for the option flag, entered at 25BCC0
// (native_tu15_engine_queries_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: the kind the second argument (RET 8); EAX
// the options past the kind's bit over the answer (AL), ECX or EDX the game
// globals where the original reads them through it; the flags of the AND
// (or the last kind's CMP). Each access publishes its original instruction.
// HALO2_REFERENCE_OPTION_FLAG=1 restores the translated body.
// HALO2_VERIFY_OPTION_FLAG=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_engine_queries_systems.hpp"

namespace halo2::native_engine_queries_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace option_flag {
namespace logic=halo2::native_engine_queries::option_flag;
using Access=logic::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_OPTION_FLAG");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_OPTION_FLAG");return v;}
enum Branch : unsigned {other, first_kind, second_kind, third_kind, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Option flag"};return v;}
// Each kind's instructions: the globals' read (into a register), the options' read.
struct KindSites {std::uint32_t globals,options;unsigned into;std::uint32_t exit;};
inline constexpr KindSites sites[]{{0x25bccbu,0x25bcd0u,eax,0x25bcdbu},{0x25bce3u,0x25bce9u,ecx,0x25bcf4u},{0x25bcfcu,0x25bd02u,edx,0x25bd0du}};
template<class State> struct Adapter : Port<State> {
    unsigned k{3u};
    bool matched{};
    std::uint32_t kind_value{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t argument(std::uint32_t n)const{return this->gp[esp]+4u*n;}
    void tested(std::uint32_t kind) {
        this->compare32(this->gp[ecx],kind);                                                     // CMP ECX,kind
        k=kind==2u?0u:kind==3u?1u:2u;
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        switch(a){case Access::kind:return 0x25bcc0u;case Access::game_state:matched=true;return sites[k].globals;case Access::options:return sites[k].options;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        switch(a) {
        case Access::kind:gp[ecx]=v;set_low_byte(gp[eax],0u);break;                                // MOV ECX; XOR AL,AL
        case Access::game_state:gp[sites[k].into]=v;break;
        case Access::options:gp[eax]=v>>logic::kinds[k].bit;break;                                 // SHR EAX,bit
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        Memory<Adapter> memory{*this};
        const auto answer=logic::set(memory,*this);
        if(matched)set_low_byte(this->gp[eax],this->flags.logic(answer,1));                       // AND AL,1
        const auto exit=matched?sites[k].exit:0x25bd0du;
        this->reached(matched?Branch::first_kind+k:Branch::other);
        this->ret(exit,8u);this->publish(exit);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x25bcc0u,0x25bd0du,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace option_flag
} // namespace halo2::native_engine_queries_detail
