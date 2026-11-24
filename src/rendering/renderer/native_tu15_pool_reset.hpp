#pragma once
// Boundary adapter for the pool reset, 140CF0
// (native_tu15_pool_reset_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EDX the pool; EBX, ESI and EDI saved. Each access
// publishes its original instruction.
// HALO2_REFERENCE_POOL_RESET=1 restores the translated body.
// HALO2_VERIFY_POOL_RESET=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/renderer/native_tu15_pool_reset_systems.hpp"

namespace halo2::native_pool_reset_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_pool_reset;
using Access=logic::pool_reset::Access;
using Outcome=logic::pool_reset::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_POOL_RESET");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_POOL_RESET");return v;}
enum Branch : unsigned {empty, linked, branch_count};
static_assert(unsigned(Outcome::linked)==Branch::linked);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Pool reset"};return v;}
inline constexpr std::uint32_t sites[]{0x140cf0u,0x140cf6u,0x140cfcu,0x140d0fu,0x140d16u,0x140d18u,0x140d1fu,0x140d27u,0x140d30u,0x140d33u,0x140d36u};
template<class State> struct Adapter : Port<State> {
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case size:gp[ecx]=fl.plus(v,logic::element_extra,4);break;                                     // ADD ECX,C
        case count:gp[ecx]=fl.product(gp[ecx],v);this->push(0x140cfau,gp[ebx]);this->push(0x140cfbu,gp[esi]);break;   // IMUL ECX,[EDX+24]; PUSH EBX; PUSH ESI
        case header:gp[esi]=fl.shift(v,2u,4);gp[ecx]=fl.plus(gp[ecx],gp[esi],4);break;                 // SHL ESI,2; ADD ECX,ESI
        case size_again:gp[ecx]=v;gp[eax]=gp[esi]+gp[edx]+logic::pool::storage;break;                   // LEA EAX,[ESI+EDX+3C]
        case count_again:gp[esi]=v;gp[ecx]=fl.plus(gp[ecx],logic::element_extra,4);this->test32(v);break;   // ADD ECX,C; TEST ESI,ESI
        case head:gp[edi]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case word_clear:gp[edi]+=step(4u);--gp[ecx];break;
        case byte_clear:gp[edi]+=step(1u);--gp[ecx];break;
        case head_store:gp[eax]=fl.plus(gp[eax],gp[ecx],4);gp[esi]=fl.increment(gp[esi],4,true);break;  // ADD EAX,ECX; DEC ESI
        default:break;
        }
    }
    std::uint32_t step(std::uint32_t width)const{return (this->s.other_flags&0x400u)?0u-width:width;}
    // MOV EBX,ECX; SHR ECX,2; PUSH EDI; XOR EAX,EAX; LEA EDI,[EDX+3C].
    void words_started(std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ebx]=gp[ecx];gp[ecx]=fl.right(gp[ecx],2u,4,false);this->push(0x140d09u,gp[edi]);
        gp[eax]=fl.logic(0u,4);gp[edi]=gp[edx]+logic::pool::storage;
    }
    // MOV ECX,EBX; AND ECX,3.
    void bytes_started(std::uint32_t){this->gp[ecx]=this->flags.logic(this->gp[ebx]&3u,4);}
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::pool_reset::reset(memory,*this,gp[edx])));
        gp[edi]=this->pop(0x140d3eu);gp[esi]=this->pop(0x140d3fu);gp[ebx]=this->pop(0x140d40u);
        this->ret(0x140d41u,0u);this->publish(0x140d41u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x140cf0u,0x140d41u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_pool_reset_detail
