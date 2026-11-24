#pragma once
// Boundary adapter for the unit free test, entered at CAAA0
// (native_tu15_unit_free_systems.hpp). Its callers and the children keep the
// original ABI, which the adapter owns: EAX the unit handle; the entry PUSH
// ECX makes the result byte's slot (byte 3), so ECX returns its bits (POP
// ECX); EBX, ESI, EDI and (in the slot walk) EBP saved; E9530 gets EAX the
// handle and ECX the bit, 105950 EAX the slot's handle and EBX NONE, each
// returning AL, and every register they return comes back; the result in AL
// above the rest of EAX as the original leaves it; EDX as the original
// leaves it; the flags of the last instruction that sets them; RET. Each
// access publishes its original instruction.
// HALO2_REFERENCE_UNIT_FREE=1 restores the translated body.
// HALO2_VERIFY_UNIT_FREE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_unit_free_systems.hpp"

namespace halo2::native_unit_free_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_unit_free;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_UNIT_FREE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_UNIT_FREE");return p && *p;}();
    return value;
}
enum Branch : unsigned {parented, blocked, flagged, empty_slot, slot_active, free_unit, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Unit free verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
// E9530's call sites, in order (the bit each was written for).
struct FlagSite {std::uint32_t bit,set,call,next;};
inline constexpr FlagSite flag_sites[]{
    {0x16u,0xcaaf7u,0xcaafeu,0xcab03u},{8u,0xcab07u,0xcab0eu,0xcab13u},{0x12u,0xcab17u,0xcab1eu,0xcab23u},
    {0u,0xcab27u,0xcab2bu,0xcab30u},{1u,0xcab34u,0xcab3bu,0xcab40u},{0xau,0xcab44u,0xcab4bu,0xcab50u},
    {0xbu,0xcab54u,0xcab5bu,0xcab60u},
};
template<class State,class Flag,class Active> struct UnitFree : Port<State> {
    using Access=logic::unit_free::Access;
    using Kept=logic::unit_free::Kept;
    Flag &flag_child;Active &active_child;
    std::uint32_t depth{};unsigned block{},site{};
    UnitFree(State &s,std::uint32_t *c,Trace<State> *t,Flag &f,Active &a):Port<State>(s,c,t,{}),flag_child(f),active_child(a){}
    std::uint32_t result_slot()const{return this->gp[esp]+depth+3u;}
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    void restore(std::uint32_t pc,unsigned r){this->gp[r]=this->pop(pc);depth-=4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:this->push(0xcaaa0u,this->gp[ecx]);save(0xcaaa1u,this->gp[ebx]);return 0xcaaa2u;   // PUSH ECX; PUSH EBX
        case header_datums:return 0xcaab4u;
        case unit_address:return 0xcaabau;
        case parent:return 0xcaabeu;
        case result_init:return 0xcaac2u;
        case block_offset:return block?0xcaae4u:0xcaacdu;
        case block_word:return block++?0xcaaebu:0xcaad4u;
        case datums_again:return 0xcab6bu;
        case unit_again:return 0xcab6eu;
        case slot_index:return 0xcab80u;
        case slot:return 0xcab8du;
        case result_read:return 0xcabacu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV EBX,[4ECBD0]; PUSH ESI; PUSH EDI; MOV EDI,EAX; AND EAX,FFFF; LEA ESI,[EAX+EAX*2].
        case headers:
            gp[ebx]=v;save(0xcaaa8u,gp[esi]);save(0xcaaa9u,gp[edi]);gp[edi]=gp[eax];
            gp[eax]=fl.logic(gp[eax]&0xffffu,4);gp[esi]=gp[eax]*3u;break;
        case header_datums:gp[eax]=v;gp[esi]=fl.shift(gp[esi],2u,4);break;                          // SHL ESI,2
        case unit_address:case datums_again:gp[eax]=v;break;
        case parent:fl.minus(v,logic::none,4);if(v!=logic::none)this->reached(Branch::parented);break;   // CMP [EAX+14],-1
        case block_offset:gp[ecx]=logic::widen16(v);break;
        // MOV EDX,[ECX+EAX+4]; SHR EDX,1A/1B; TEST DL,1.
        case block_word: {
            const auto bit=logic::blocking_bits[block-1u];
            gp[edx]=fl.right(v,bit,4,false);this->test8(gp[edx]&1u);
            if(gp[edx]&1u)this->reached(Branch::blocked);break;
        }
        case unit_again:gp[esi]=v;break;
        // MOVSX AX,byte [EDI]; CMP AX,FFFF.
        case slot_index: {
            const auto value=std::uint32_t(std::int32_t(std::int8_t(v)))&0xffffu;
            gp[eax]=(gp[eax]&0xffff0000u)|value;fl.minus(value,0xffffu,2);
            if(value==0xffffu)this->reached(Branch::empty_slot);break;
        }
        // MOVSX ECX,AX; MOV EAX,[ESI+ECX*4+218]; CMP EAX,-1.
        case slot:gp[ecx]=logic::widen16(gp[eax]&0xffffu);gp[eax]=v;fl.minus(v,logic::none,4);break;
        case result_read:set_low_byte(gp[eax],v);break;                                              // MOV AL,[ESP+13]
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOV ECX,bit (XOR ECX,ECX for 0); MOV EAX,EDI; CALL E9530; TEST AL,AL.
    std::uint32_t flag(Kept &kept,std::uint32_t bit) {
        auto &gp=this->gp;
        const auto &call=flag_sites[site++];
        gp[ecx]=bit?bit:this->flags.logic(0u,4);gp[eax]=gp[edi];
        this->call(call.call,call.next,0xe9530u,flag_child);
        this->test8(gp[eax]);
        kept.handle=gp[edi];kept.headers=gp[ebx];kept.header=gp[esi];
        if(gp[eax]&0xffu)this->reached(Branch::flagged);
        return gp[eax]&0xffu;
    }
    // PUSH EBP; XOR EBP,EBP; LEA EDI,[ESI+212].
    void slot_walk(const Kept &kept) {
        auto &gp=this->gp;
        save(0xcab72u,gp[ebp]);gp[ebp]=this->flags.logic(0u,4);gp[edi]=kept.header+logic::unit::slot_indices;
    }
    // OR EBX,-1; CALL 105950; TEST AL,AL.
    std::uint32_t active(Kept &kept,std::uint32_t) {
        auto &gp=this->gp;
        gp[ebx]=this->flags.logic(gp[ebx]|logic::none,4);
        this->call(0xcab9cu,0xcaba1u,0x105950u,active_child);
        this->test8(gp[eax]);
        kept.index=gp[ebp];kept.slot=gp[edi];kept.header=gp[esi];
        if(gp[eax]&0xffu)this->reached(Branch::slot_active);
        return gp[eax]&0xffu;
    }
    // INC EBP; INC EDI; CMP EBP,2 (the logic stepped kept).
    void stepped(const Kept &) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ebp]=fl.increment(gp[ebp],4,false);gp[edi]=fl.increment(gp[edi],4,false);fl.minus(gp[ebp],logic::slot_count,4);
    }
    void run() {
        auto &gp=this->gp;
        Memory<UnitFree> memory{*this};
        using logic::unit_free::Outcome;
        const auto outcome=logic::unit_free::free(memory,*this,gp[eax]);
        std::uint32_t exit=0;
        switch(outcome) {
        case Outcome::parented:case Outcome::blocked:case Outcome::flagged:              // POP EDI; POP ESI; XOR AL,AL; POP EBX; POP ECX
            restore(0xcab64u,edi);restore(0xcab65u,esi);set_low_byte(gp[eax],this->flags.logic(0u,1));
            restore(0xcab68u,ebx);gp[ecx]=this->pop(0xcab69u);exit=0xcab6au;break;
        case Outcome::slot_active:                                                         // POP EBP, EDI, ESI; XOR AL,AL; POP EBX; POP ECX
            restore(0xcabb6u,ebp);restore(0xcabb7u,edi);restore(0xcabb8u,esi);set_low_byte(gp[eax],this->flags.logic(0u,1));
            restore(0xcabbbu,ebx);gp[ecx]=this->pop(0xcabbcu);exit=0xcabbdu;break;
        case Outcome::free:                                                                 // POP EBP, EDI, ESI, EBX, ECX
            this->reached(Branch::free_unit);
            restore(0xcabb0u,ebp);restore(0xcabb1u,edi);restore(0xcabb2u,esi);restore(0xcabb3u,ebx);
            gp[ecx]=this->pop(0xcabb4u);exit=0xcabb5u;break;
        }
        this->ret(exit,0);this->publish(exit);
    }
};
template<class State,class Reference,class Flag,class Active> bool bridge(State &s,Reference reference,Flag flag,Active active) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xcaaa0u,0xcabbdu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return UnitFree<State,Flag,Active>(v,c,t,flag,active);});
    return true;
}
} // namespace halo2::native_unit_free_detail
