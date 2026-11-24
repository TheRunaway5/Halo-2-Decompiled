#pragma once
// Boundary adapter for the animation entry value, entered at 1E5A70
// (native_tu15_animation_entry_systems.hpp). Its callers and 1262F0 keep the
// original ABI, which the adapter owns: ECX the definition, the reference
// on the stack (RET 4); ESI saved (the entry meanwhile); 1262F0 gets EAX the
// row and the tag pushed (and pops it), and every register it returns comes
// back; the value in EAX; ECX (the owner, then the tag) and EDX as the
// original leaves them; the flags of the last instruction that sets them.
// Each access publishes its original instruction.
// HALO2_REFERENCE_ANIMATION_ENTRY=1 restores the translated body.
// HALO2_VERIFY_ANIMATION_ENTRY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_animation_entry_systems.hpp"

namespace halo2::native_animation_entry_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_animation_entry;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ANIMATION_ENTRY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ANIMATION_ENTRY");return p && *p;}();
    return value;
}
enum Branch : unsigned {own, picked, plain, computed, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Animation entry verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Lookup> struct AnimationEntry : Port<State> {
    using Access=logic::animation_entry::Access;
    using Kept=logic::animation_entry::Kept;
    Lookup &lookup_child;
    bool pushed{};
    AnimationEntry(State &s,std::uint32_t *c,Trace<State> *t,Lookup &l):Port<State>(s,c,t,{}),lookup_child(l){}
    // [ESP+4], then [ESP+8] after PUSH ESI.
    std::uint32_t reference_slot()const{return this->gp[esp]+(pushed?8u:4u);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case reference:return 0x1e5a70u;
        case count:return 0x1e5a7bu;
        case list:return 0x1e5a87u;
        case list_tag:return 0x1e5a8fu;
        case tags:return 0x1e5a97u;
        case definition:return 0x1e5aa5u;
        case reference_again:return 0x1e5aabu;
        case entries:return 0x1e5ab9u;
        case flags:return 0x1e5ac3u;
        case row:return 0x1e5ac9u;
        case rows:return 0x1e5acfu;
        case tag:return 0x1e5ad5u;
        case offset:return 0x1e5ae0u;
        case plain:return 0x1e5ae7u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV EAX,[ESP+4]; CMP AX,-1; PUSH ESI.
        case reference:gp[eax]=v;fl.minus(v&0xffffu,0xffffu,2);this->push(0x1e5a78u,gp[esi]);pushed=true;
            if((v&0xffffu)==0xffffu)this->reached(Branch::own);else this->reached(Branch::picked);break;
        // MOV ESI,[ECX+4C]; MOVSX EAX,AX; XOR EDX,EDX; CMP EAX,ESI.
        case count:gp[esi]=v;gp[eax]=logic::widen16(gp[eax]);gp[edx]=fl.logic(0u,4);fl.minus(gp[eax],v,4);break;
        // MOV ESI,[ECX+50]; SHL EAX,5; ADD EAX,ESI.
        case list:gp[esi]=v;gp[eax]=fl.plus(fl.shift(gp[eax],5u,4),v,4);break;
        case list_tag:gp[eax]=v;fl.minus(v,logic::none,4);break;                                    // MOV EAX,[EAX+4]; CMP EAX,-1
        case tags:gp[ecx]=v;gp[eax]=fl.shift(fl.logic(gp[eax]&0xffffu,4),4u,4);break;              // AND EAX,FFFF; SHL EAX,4
        case definition:gp[edx]=v;break;
        // MOV EAX,[ESP+8]; SAR EAX,10; XOR ESI,ESI; CMP EAX,-1.
        case reference_again:gp[eax]=fl.right(v,16u,4,true);gp[esi]=fl.logic(0u,4);fl.minus(gp[eax],logic::none,4);break;
        // MOV EDX,[ECX+30]; IMUL EAX,EAX,6C; ADD EAX,EDX; MOV ESI,EAX.
        case entries:gp[edx]=v;gp[eax]=fl.plus(fl.product(gp[eax],logic::entry::size),v,4);gp[esi]=gp[eax];break;
        case flags:this->test8(v&logic::computed_bit);break;                                        // TEST byte [ESI+16],10
        case row:gp[eax]=v;gp[edx]=v*5u;break;                                                      // MOV EAX,[ESI+24]; LEA EDX
        case rows:gp[eax]=v;break;
        case tag:gp[ecx]=v;gp[eax]+=4u*gp[edx];break;                                               // MOV ECX,[ESI]; LEA EAX
        case offset:gp[eax]=fl.plus(gp[eax],v,4);break;                                             // ADD EAX,[ESI+28]
        case plain:gp[eax]=v;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void resolved(std::uint32_t chosen){this->gp[ecx]=chosen;}                                       // MOV ECX,EDX
    // PUSH ECX; CALL 1262F0.
    std::uint32_t lookup(Kept &kept,std::uint32_t row_address,std::uint32_t tag_handle) {
        auto &gp=this->gp;
        gp[eax]=row_address;this->push(0x1e5adau,tag_handle);this->call(0x1e5adbu,0x1e5ae0u,0x1262f0u,lookup_child);
        kept.entry=gp[esi];
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        Memory<AnimationEntry> memory{*this};
        const auto result=logic::animation_entry::value(memory,*this,gp[ecx]);
        gp[eax]=result.value;
        this->reached(result.computed?Branch::computed:Branch::plain);
        const auto pop_pc=result.computed?0x1e5ae3u:0x1e5aeau;
        gp[esi]=this->pop(pop_pc);this->ret(pop_pc+1u,4u);this->publish(pop_pc+1u);
    }
};
template<class State,class Reference,class Lookup> bool bridge(State &s,Reference reference,Lookup lookup) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1e5a70u,0x1e5aebu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return AnimationEntry<State,Lookup>(v,c,t,lookup);});
    return true;
}
} // namespace halo2::native_animation_entry_detail
