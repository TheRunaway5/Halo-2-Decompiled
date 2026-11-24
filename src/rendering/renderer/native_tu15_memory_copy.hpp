#pragma once
// Boundary adapter for the memory copy, 2E2E30
// (native_tu15_memory_copy_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: the destination, source and byte count on the
// stack (the caller pops them); ESI and EDI saved; ECX, ESI and EDI step
// with the string instructions and EAX keeps the count. Each access
// publishes its original instruction.
// HALO2_REFERENCE_MEMORY_COPY=1 restores the translated body.
// HALO2_VERIFY_MEMORY_COPY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/renderer/native_tu15_memory_copy_systems.hpp"

namespace halo2::native_memory_copy_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_memory_copy;
using Access=logic::memory_copy::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_MEMORY_COPY");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_MEMORY_COPY");return v;}
enum Branch : unsigned {copied, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Memory copy"};return v;}
inline constexpr std::uint32_t sites[]{0x2e2e30u,0x2e2e35u,0x2e2e3cu,0x2e2e43u,0x2e2e43u,0x2e2e4au,0x2e2e4au};
template<class State> struct Adapter : Port<State> {
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case bytes_count:gp[ecx]=v;this->push(0x2e2e34u,gp[esi]);depth=4u;break;                      // PUSH ESI
        case source:gp[esi]=v;gp[eax]=gp[ecx];this->push(0x2e2e3bu,gp[edi]);depth=8u;break;            // MOV EAX,ECX; PUSH EDI
        case destination:gp[edi]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        if(a==Access::word_write){const auto d=step(4u);gp[esi]+=d;gp[edi]+=d;--gp[ecx];}
        if(a==Access::byte_write){const auto d=step(1u);gp[esi]+=d;gp[edi]+=d;--gp[ecx];}
    }
    std::uint32_t step(std::uint32_t width)const{return (this->s.other_flags&0x400u)?0u-width:width;}
    void words_started(){this->gp[ecx]=this->flags.right(this->gp[ecx],2u,4,false);}                 // SHR ECX,2
    void bytes_started(){this->gp[ecx]=this->flags.logic(this->gp[eax]&3u,4);}                         // MOV ECX,EAX; AND ECX,3
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::memory_copy::copy(memory,*this)));
        gp[edi]=this->pop(0x2e2e4cu);gp[esi]=this->pop(0x2e2e4du);
        this->ret(0x2e2e4eu,0u);this->publish(0x2e2e4eu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x2e2e30u,0x2e2e4eu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_memory_copy_detail
