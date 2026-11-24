#pragma once
// Boundary adapter for the ready slot, entered at 111f00
// (native_tu15_object_leaves_systems.hpp). Its callers keep the
// original ABI, which the adapter owns: EAX the object handle; ESI saved;
// the result in AL (XOR AL,AL; MOV AL,1) above the object's address; ECX
// the block, EDX NONE; the flags of the last instruction that sets them;
// RET.
// HALO2_REFERENCE_OBJECT_LEAVES=1 restores the translated body.
// HALO2_VERIFY_OBJECT_LEAVES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_object_leaves_systems.hpp"

namespace halo2::native_ready_slot_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_leaves;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_LEAVES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_LEAVES");return p && *p;}();
    return value;
}
enum Branch : unsigned {first_ready, second_ready, none_ready, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object leaves (ready slot) verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Leaf : Port<State> {
    Leaf(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    using Access=logic::ready_slot::Access;
    std::uint32_t k{};
    void slot(std::uint32_t index){k=index;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        static constexpr std::uint32_t handles[]{0x111f1fu,0x111f49u},words[]{0x111f2eu,0x111f51u},
            armed_sites[]{0x111f37u,0x111f5au},blocked_sites[]{0x111f40u,0x111f63u};
        switch(a) {
        case headers:return 0x111f00u;
        case header_datums:return 0x111f06u;
        case object_address:return 0x111f11u;
        case block:return 0x111f15u;
        case handle:return handles[k];
        case word:return words[k];
        case armed:return armed_sites[k];
        case blocked:return blocked_sites[k];
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;                 // AND EAX,FFFF; LEA
        case object_address:gp[eax]=v;break;
        // MOVSX ECX,word [EAX+33E]; ADD ECX,EAX; PUSH ESI.
        case block:gp[ecx]=fl.plus(logic::widen16(v),gp[eax],4);this->push(0x111f1eu,gp[esi]);break;
        // The first: MOV ESI,[ECX+9C]; OR EDX,-1; XOR AL,AL; CMP ESI,EDX. The second: CMP [ECX+BC],EDX.
        case handle:if(k==0){gp[esi]=v;gp[edx]=fl.logic(gp[edx]|logic::none,4);set_low_byte(gp[eax],fl.logic(0u,1));}
            fl.minus(v,gp[edx],4);break;
        case word:fl.minus(v&0xffffu,gp[edx]&0xffffu,2);break;                                      // CMP word [ECX+..],DX
        case armed:this->test8(v&logic::ready_slot::armed_bit);break;
        case blocked:this->test8(v&logic::ready_slot::blocking_bits);break;
        }
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::ready_slot::Outcome;
        Memory<Leaf> memory{*this};
        const auto outcome=logic::ready_slot::any(memory,*this,gp[eax]);
        this->reached(unsigned(outcome));
        if(outcome!=Outcome::none_ready)set_low_byte(gp[eax],1u);                                    // MOV AL,1
        gp[esi]=this->pop(0x111f6eu);this->ret(0x111f6fu,0);this->publish(0x111f6fu);
    }
    template<class A> void stored(A,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x111f00u,0x111f6fu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Leaf<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_ready_slot_detail
