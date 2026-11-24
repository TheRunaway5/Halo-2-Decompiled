#pragma once
// Boundary adapter for the animation record, entered at 1E5DF0
// (native_tu15_animation_record_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EDI the record, the
// definition and the reference on the stack (RET 8); EBP and, past the
// reference's test, EBX and ESI saved; 1E3150 gets EDX the definition and
// 1E5A70 ECX the definition, each the reference pushed (and pops it), and
// every register each returns comes back; EAX (the value, or the reference's
// high word), ECX (the entry's word widened) and EDX (its kind widened) as
// the original leaves them; the flags of the last instruction that sets
// them. Each access publishes its original instruction.
// HALO2_REFERENCE_ANIMATION_RECORD=1 restores the translated body.
// HALO2_VERIFY_ANIMATION_RECORD=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_animation_record_systems.hpp"

namespace halo2::native_animation_record_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_animation_record;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ANIMATION_RECORD");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ANIMATION_RECORD");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_reference, filled, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Animation record verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Entry,class Value> struct AnimationRecord : Port<State> {
    using Access=logic::animation_record::Access;
    using Kept=logic::animation_record::Kept;
    Entry &entry_child;
    Value &value_child;
    std::uint32_t depth{};   // bytes pushed so far
    AnimationRecord(State &s,std::uint32_t *c,Trace<State> *t,Entry &e,Value &v):Port<State>(s,c,t,{}),entry_child(e),value_child(v){}
    // The arguments past the return address, read at the current depth.
    std::uint32_t definition_slot()const{return this->gp[esp]+4u+depth;}
    std::uint32_t reference_slot()const{return this->gp[esp]+8u+depth;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case high:return 0x1e5df0u;
        case definition:return 0x1e5dfcu;
        case reference:this->push(0x1e5e02u,this->gp[ebx]);depth=8u;return 0x1e5e03u;               // PUSH EBX
        case group:return 0x1e5e1au;
        case word:return 0x1e5e1eu;
        case kind:return 0x1e5e22u;
        case data_store:return 0x1e5e29u;
        case group_store:this->gp[esi]=this->pop(0x1e5e2cu);return 0x1e5e2du;                       // POP ESI
        case value_store:return 0x1e5e30u;
        case kind_store:return 0x1e5e32u;
        case word_store:return 0x1e5e35u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV EAX,[ESP+8]; SHR EAX,10; CMP AX,FFFF; PUSH EBP.
        case high:gp[eax]=fl.right(v,16u,4,false);fl.minus(gp[eax]&0xffffu,0xffffu,2);this->push(0x1e5dfbu,gp[ebp]);depth=4u;break;
        case definition:gp[ebp]=v;break;                                                            // MOV EBP,[ESP+8]
        case reference:gp[ebx]=v;break;                                                             // MOV EBX,[ESP+10]
        case group:gp[ebx]=v&0xffu;break;                                                           // MOVZX EBX,byte
        case word:gp[ecx]=logic::widen16(v);break;                                                  // MOVSX ECX,word
        // MOVSX EDX,byte; ADD ESI,3C.
        case kind:gp[edx]=logic::widen8(v);gp[esi]=fl.plus(gp[esi],logic::entry::data,4);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH EBX (the reference), before the child.
    void pass(Kept &kept,std::uint32_t pc){this->push(pc,kept.reference);}
    // PUSH ESI; PUSH EBX; MOV EDX,EBP; CALL 1E3150.
    std::uint32_t entry(Kept &kept) {
        auto &gp=this->gp;
        this->push(0x1e5e07u,gp[esi]);depth=12u;pass(kept,0x1e5e08u);gp[edx]=kept.definition;     // PUSH ESI
        this->call(0x1e5e0bu,0x1e5e10u,0x1e3150u,entry_child);
        kept={gp[ebp],gp[ebx],gp[eax],gp[edi]};
        return gp[eax];
    }
    // PUSH EBX; MOV ECX,EBP; MOV ESI,EAX; CALL 1E5A70.
    std::uint32_t value(Kept &kept) {
        auto &gp=this->gp;
        pass(kept,0x1e5e10u);gp[ecx]=kept.definition;gp[esi]=kept.entry;
        this->call(0x1e5e15u,0x1e5e1au,0x1e5a70u,value_child);
        kept={gp[ebp],gp[ebx],gp[esi],gp[edi]};
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        Memory<AnimationRecord> memory{*this};
        if(logic::animation_record::fill(memory,*this,gp[edi])) {
            this->reached(Branch::filled);gp[ebx]=this->pop(0x1e5e39u);                             // POP EBX
        } else this->reached(Branch::no_reference);
        gp[ebp]=this->pop(0x1e5e3au);this->ret(0x1e5e3bu,8u);this->publish(0x1e5e3bu);
    }
};
template<class State,class Reference,class Entry,class Value> bool bridge(State &s,Reference reference,Entry entry,Value value) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1e5df0u,0x1e5e3bu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return AnimationRecord<State,Entry,Value>(v,c,t,entry,value);});
    return true;
}
} // namespace halo2::native_animation_record_detail
