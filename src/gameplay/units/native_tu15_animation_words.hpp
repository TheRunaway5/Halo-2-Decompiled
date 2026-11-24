#pragma once
// Boundary adapter for the animation words, entered at 111D10
// (native_tu15_animation_words_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the object handle, ESI and EDX the
// outputs; EBX saved (the graph); the result in AL (XOR AL,AL, MOV AL,1)
// above the rest of EAX as the original leaves it; ECX as the original
// leaves it (the block, or its second word); the flags of the last
// instruction that sets them; RET. Each access publishes its original
// instruction.
// HALO2_REFERENCE_ANIMATION_WORDS=1 restores the translated body.
// HALO2_VERIFY_ANIMATION_WORDS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_animation_words_systems.hpp"

namespace halo2::native_animation_words_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_animation_words;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ANIMATION_WORDS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ANIMATION_WORDS");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_graph, no_handle, no_index, read_words, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Animation words verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct AnimationWords : Port<State> {
    using Access=logic::animation_words::Access;
    AnimationWords(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0x111d10u;
        case header_datums:return 0x111d16u;
        case object_address:return 0x111d21u;
        case block:return 0x111d25u;
        case graph:this->push(0x111d2eu,this->gp[ebx]);return 0x111d2fu;                          // PUSH EBX
        case handle:return 0x111d39u;
        case index:return 0x111d3eu;
        case first:return 0x111d45u;
        case first_store:return 0x111d48u;
        case second:return 0x111d4au;
        case second_store:return 0x111d4du;
        case first_clear:return 0x111d53u;
        case second_clear:return 0x111d59u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:case header_datums:gp[ecx]=v;if(a==header_datums)gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;
        case object_address:gp[eax]=v;break;
        case block:gp[ecx]=fl.plus(logic::widen16(v),gp[eax],4);break;                             // MOVSX ECX; ADD ECX,EAX
        // MOV EBX,[ECX+68]; XOR AL,AL; CMP EBX,-1.
        case graph:gp[ebx]=v;set_low_byte(gp[eax],fl.logic(0u,1));fl.minus(v,logic::none,4);
            if(v==logic::none)this->reached(Branch::no_graph);break;
        case handle:fl.minus(v,logic::none,4);if(v==logic::none)this->reached(Branch::no_handle);break;   // CMP [ECX],-1
        case index:fl.minus(v&0xffffu,0xffffu,2);if((v&0xffffu)==0xffffu)this->reached(Branch::no_index);break;
        case first:gp[eax]=v;break;
        case second:gp[ecx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        Memory<AnimationWords> memory{*this};
        if(logic::animation_words::read(memory,gp[eax],gp[esi],gp[edx])) {
            this->reached(Branch::read_words);
            set_low_byte(gp[eax],1u);gp[ebx]=this->pop(0x111d51u);this->ret(0x111d52u,0);this->publish(0x111d52u);
        } else {
            gp[ebx]=this->pop(0x111d5fu);this->ret(0x111d60u,0);this->publish(0x111d60u);
        }
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x111d10u,0x111d60u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return AnimationWords<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_animation_words_detail
