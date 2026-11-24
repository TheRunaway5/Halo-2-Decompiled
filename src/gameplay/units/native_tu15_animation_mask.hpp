#pragma once
// Boundary adapter for the animation mask query, entered at 113C70
// (native_tu15_animation_mask_systems.hpp). Its callers and 1E3150 keep the
// original ABI, which the adapter owns: EAX the object handle, the bit on
// the stack (RET 4); EBX (BL the answer), EBP (the bit) and ESI (the second
// block) saved; 1E3150 gets EDX the definition and the argument pushed (and
// pops it), and every register it returns comes back; the result in AL
// (MOV AL,BL; XOR AL,AL) above the rest of EAX; ECX (the first block, then
// the bit) and EDX (the mask) as the original leaves them; the flags of the
// last instruction that sets them. Each access publishes its original
// instruction.
// HALO2_REFERENCE_ANIMATION_MASK=1 restores the translated body.
// HALO2_VERIFY_ANIMATION_MASK=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_animation_mask_systems.hpp"

namespace halo2::native_animation_mask_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_animation_mask;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ANIMATION_MASK");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ANIMATION_MASK");return p && *p;}();
    return value;
}
enum Branch : unsigned {first_set, second_set, neither, branch_count};
static_assert(unsigned(logic::animation_mask::Outcome::neither)==Branch::neither);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Animation mask verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Entry> struct AnimationMask : Port<State> {
    using Access=logic::animation_mask::Access;
    using Kept=logic::animation_mask::Kept;
    Entry &entry_child;
    AnimationMask(State &s,std::uint32_t *c,Trace<State> *t,Entry &e):Port<State>(s,c,t,{}),entry_child(e){}
    // [ESP+C] after PUSH EBX; PUSH EBP.
    std::uint32_t bit_slot()const{return this->gp[esp]+0xcu;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0x113c70u;
        case header_datums:return 0x113c76u;
        case object_address:return 0x113c82u;
        case first_offset:return 0x113c86u;
        case bit:return 0x113c8eu;
        case second_offset:this->push(0x113c92u,this->gp[esi]);return 0x113c93u;                    // PUSH ESI
        case first_high:return 0x113c9eu;
        case first_tag:return 0x113ca7u;
        case first_tags:return 0x113ca9u;
        case first_definition:return 0x113cb7u;
        case first_argument:return 0x113cbbu;
        case first_mask:return 0x113cc8u;
        case second_high:return 0x113cdbu;
        case second_tag:return 0x113ce5u;
        case second_tags:return 0x113cebu;
        case second_argument:return 0x113cf1u;
        case second_definition:return 0x113d00u;
        case second_mask:return 0x113d0eu;
        }
        return 0;
    }
    // MOVZX EDX,word [EAX+18]; MOV EAX,1; MOV ECX,EBP; SHL EAX,CL; TEST EAX,EDX.
    void masked(std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=v&0xffffu;gp[ecx]=gp[ebp];gp[eax]=fl.shift(1u,gp[ecx]&0xffu,4);this->test32(gp[eax]&gp[edx]);
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        // MOV EDX,[ECX+44]; AND EAX,FFFF; PUSH EBX; LEA EAX,[EAX+EAX*2].
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4);this->push(0x113c7eu,gp[ebx]);gp[eax]*=3u;break;
        case object_address:gp[eax]=v;break;
        case first_offset:gp[ecx]=logic::widen16(v);this->push(0x113c8du,gp[ebp]);break;            // MOVSX ECX; PUSH EBP
        case bit:gp[ebp]=v;break;
        // MOVSX ESI; ADD ECX,EAX; ADD ESI,EAX.
        case second_offset:gp[esi]=logic::widen16(v);gp[ecx]=fl.plus(gp[ecx],gp[eax],4);gp[esi]=fl.plus(gp[esi],gp[eax],4);break;
        case first_high:fl.minus(v&0xffffu,0xffffu,2);set_low_byte(gp[ebx],1u);break;               // CMP word [ECX+6],-1; MOV BL,1
        case first_tag:gp[eax]=v;break;
        case first_tags:gp[edx]=v;gp[eax]=fl.shift(fl.logic(gp[eax]&0xffffu,4),4u,4);break;        // AND EAX,FFFF; SHL EAX,4
        case first_definition:gp[edx]=v;break;
        case first_argument:gp[eax]=v;break;
        case first_mask:masked(v);break;
        case second_high:fl.minus(v&0xffffu,0xffffu,2);break;                                       // CMP word [ESI+A2],-1
        case second_tag:gp[ecx]=v;break;
        case second_tags:gp[edx]=v;break;
        case second_argument:gp[eax]=v;gp[ecx]=fl.shift(fl.logic(gp[ecx]&0xffffu,4),4u,4);break;   // MOV EAX; AND ECX; SHL ECX,4
        case second_definition:gp[edx]=v;break;
        case second_mask:masked(v);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void marked(const Kept &kept){set_low_byte(this->gp[ebx],this->flags.logic(kept.answer,1));}   // XOR BL,BL
    // PUSH EAX (the argument); CALL 1E3150; TEST EAX,EAX.
    std::uint32_t entry(Kept &kept,std::uint32_t definition,std::uint32_t argument,std::uint32_t k) {
        auto &gp=this->gp;
        static constexpr std::uint32_t pushes[]{0x113cbeu,0x113d04u};
        gp[edx]=definition;gp[eax]=argument;this->push(pushes[k],argument);
        this->call(pushes[k]+1u,pushes[k]+6u,0x1e3150u,entry_child);this->test32(gp[eax]);
        kept={gp[ebp],gp[esi],gp[ebx]&0xffu};
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::animation_mask::Outcome;
        Memory<AnimationMask> memory{*this};
        const auto result=logic::animation_mask::clear(memory,*this,gp[eax]);
        this->reached(unsigned(result.outcome));
        if(result.outcome==Outcome::second_set) {
            gp[esi]=this->pop(0x113d1fu);gp[ebp]=this->pop(0x113d20u);set_low_byte(gp[eax],this->flags.logic(0u,1));
            gp[ebx]=this->pop(0x113d23u);this->ret(0x113d24u,4u);this->publish(0x113d24u);return;
        }
        gp[esi]=this->pop(0x113d27u);gp[ebp]=this->pop(0x113d28u);set_low_byte(gp[eax],result.value);   // MOV AL,BL
        gp[ebx]=this->pop(0x113d2bu);this->ret(0x113d2cu,4u);this->publish(0x113d2cu);
    }
};
template<class State,class Reference,class Entry> bool bridge(State &s,Reference reference,Entry entry) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x113c70u,0x113d2cu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return AnimationMask<State,Entry>(v,c,t,entry);});
    return true;
}
} // namespace halo2::native_animation_mask_detail
