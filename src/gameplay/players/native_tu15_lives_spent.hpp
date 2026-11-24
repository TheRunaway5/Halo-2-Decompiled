#pragma once
// Boundary adapter for the lives spent query, 160570
// (native_tu15_lives_spent_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: ECX the player (RET), the answer in AL, ESI saved.
// Each access publishes its original instruction.
// HALO2_REFERENCE_LIVES_SPENT=1 restores the translated body.
// HALO2_VERIFY_LIVES_SPENT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/players/native_tu15_lives_spent_systems.hpp"

namespace halo2::native_lives_spent_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_lives_spent;
using Access=logic::lives_spent::Access;
using Outcome=logic::lives_spent::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_LIVES_SPENT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_LIVES_SPENT");return v;}
enum Branch : unsigned {unlimited, alive, lives_left, spent, branch_count};
static_assert(unsigned(Outcome::spent)==Branch::spent);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Lives spent"};return v;}
inline constexpr std::uint32_t sites[]{0x160570u,0x160577u,0x160583u,0x160589u,0x160598u,0x1605a3u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::lives)+1u);
template<class State> struct Adapter : Port<State> {
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case game:case players_array:case unit:gp[edx]=v;break;
        case lives_limit:case players_data:gp[esi]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saving(){this->push(0x160576u,this->gp[esi]);}                                               // PUSH ESI
    void limit_tested(std::uint32_t limit){set_low_byte(this->gp[eax],this->flags.logic(0u,1));this->test32(limit);}   // XOR AL,AL; TEST ESI,ESI
    void indexing(std::uint32_t index) {                                                               // AND ECX,FFFF; IMUL ECX,21C
        auto &fl=this->flags;
        this->gp[ecx]=fl.product(fl.logic(index,4),logic::player_size);
    }
    void unit_tested(std::uint32_t unit_value,std::uint32_t scaled,std::uint32_t data) {               // ADD ECX,ESI; CMP EDX,-1
        this->gp[ecx]=this->flags.plus(scaled,data,4);this->compare32(unit_value,logic::none);
    }
    void lives_tested(std::uint32_t lives_value){this->flags.minus(lives_value&0xffffu,0u,2);}         // CMP word [ECX+1AC],0
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        const auto answer=logic::lives_spent::query(memory,*this,gp[ecx],outcome);
        this->reached(unsigned(outcome));
        set_low_byte(gp[eax],answer);                                                                    // MOV AL,1 (or the XOR's 0)
        gp[esi]=this->pop(0x1605afu);
        this->ret(0x1605b0u,0u);this->publish(0x1605b0u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x160570u,0x1605b0u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_lives_spent_detail
