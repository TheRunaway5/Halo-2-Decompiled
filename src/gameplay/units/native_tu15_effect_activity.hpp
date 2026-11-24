#pragma once
// Boundary adapter for the effect activity, entered at 17C5C0
// (native_tu15_effect_activity_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EBX the effect handle; ESI
// and EDI saved (the effect, its tag); 3EB10 gets EDX the location
// (MOVSX), 11E7B0 and 11E780 EAX the position, 17A4A0 EAX the handle, each
// returning AL, and every register they return comes back; the result in
// EAX (XOR EAX,EAX; TEST ESI,ESI; SETZ AL: ESI cleared means retired);
// ECX and EDX as the original leaves them; the flags of TEST ESI,ESI; RET.
// Each access publishes its original instruction.
// HALO2_REFERENCE_EFFECT_ACTIVITY=1 restores the translated body.
// HALO2_VERIFY_EFFECT_ACTIVITY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_effect_activity_systems.hpp"

namespace halo2::native_effect_activity_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_effect_activity;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_EFFECT_ACTIVITY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_EFFECT_ACTIVITY");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    no_location, located_owned, held, flagged_position, position, unchanged, deactivated, kept, activated, retired,
    branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Effect activity verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Route> struct Activity : Port<State> {
    using Access=logic::effect_activity::Access;
    using Test=logic::effect_activity::Test;
    using Kept=logic::effect_activity::Kept;
    Route &route;
    Activity(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t al()const{return this->gp[eax]&0xffu;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case effects:return 0x17c5c0u;
        case effects_data:return 0x17c5c5u;
        case tag_instances:return 0x17c5c8u;
        case effect_tag:return 0x17c5dfu;
        case tag_address:return 0x17c5ecu;
        case location:return 0x17c5f0u;
        case owner:return 0x17c608u;
        case game:return 0x17c60eu;
        case hold:return 0x17c617u;
        case tag_flags:return 0x17c61eu;
        case flags_read:return 0x17c646u;
        case flags_set:return 0x17c657u;
        case flags_word:this->gp[eax]=this->flags.logic(0u,4);return 0x17c65du;                // XOR EAX,EAX
        case flags_clear:return 0x17c688u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case effects:gp[eax]=v;break;
        case effects_data:gp[ecx]=v;break;
        // MOV EDX,[4F0414]; PUSH ESI; MOV ESI,EBX; AND ESI,FFFF; IMUL ESI,ESI,190; ADD ESI,ECX.
        case tag_instances:
            gp[edx]=v;this->push(0x17c5ceu,gp[esi]);
            gp[esi]=fl.plus(fl.product(fl.logic(gp[ebx]&0xffffu,4),logic::effect_size),gp[ecx],4);break;
        // MOV ECX,[ESI+4]; AND ECX,FFFF; SHL ECX,4; PUSH EDI.
        case effect_tag:gp[ecx]=fl.shift(fl.logic(v&0xffffu,4),logic::tag_instance_shift,4);this->push(0x17c5ebu,gp[edi]);break;
        case tag_address:gp[edi]=v;break;
        // MOV CX,[ESI+20]; CMP CX,-1; MOV AL,1.
        case location:
            gp[ecx]=(gp[ecx]&0xffff0000u)|(v&0xffffu);fl.minus(v&0xffffu,0xffffu,2);set_low_byte(gp[eax],1u);
            if((v&0xffffu)==logic::no_location)this->reached(Branch::no_location);break;
        case owner:fl.minus(v,logic::none,4);if(v!=logic::none)this->reached(Branch::located_owned);break;   // CMP [ESI+48],-1
        case game:gp[eax]=v;this->test32(v);break;                                                   // MOV EAX; TEST EAX,EAX
        case hold:set_low_byte(gp[ecx],v);this->test8(v);if(v&0xffu)this->reached(Branch::held);break;   // MOV CL; TEST CL,CL
        // MOV EAX,[EDI]; SHR EAX,2; TEST AL,1; LEA EAX,[ESI+1C].
        case tag_flags:
            gp[eax]=fl.right(v,logic::tag_position_bit,4,false);this->test8(gp[eax]&1u);
            this->reached((gp[eax]&1u)?Branch::flagged_position:Branch::position);
            gp[eax]=gp[esi]+logic::effect::position;break;
        // MOV CL,[ESI+2]; SHR CL,3; AND CL,1.
        case flags_read:set_low_byte(gp[ecx],fl.logic(fl.right(v&0xffu,logic::effect::active_bit,1,false)&1u,1));break;
        // MOV AX,[ESI+2]; MOV DL,AL; SHR DL,1; TEST DL,1; (SHR EAX,9; TEST AL,1).
        case flags_word: {
            gp[eax]=(gp[eax]&0xffff0000u)|(v&0xffffu);
            set_low_byte(gp[edx],fl.right(v&0xffu,logic::effect::kept_bit,1,false));this->test8(gp[edx]&1u);
            if(!(gp[edx]&1u)){gp[eax]=fl.right(gp[eax],logic::effect::persistent_bit,4,false);this->test8(gp[eax]&1u);}
            break;
        }
        default:break;
        }
    }
    // OR/AND byte [ESI+2]: the flags of the result.
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        if(a==Access::flags_set||a==Access::flags_clear)(void)this->flags.logic(v&0xffu,1);
    }
    std::uint32_t test(Kept &kept,Test t,std::uint32_t argument) {
        auto &gp=this->gp;
        switch(t) {
        case Test::location:
            gp[edx]=std::uint32_t(std::int32_t(std::int16_t(argument)));                           // MOVSX EDX,CX
            this->call_routed(route,0x17c5ffu,0x17c604u,0x3eb10u);this->test8(al());break;      // TEST AL,AL
        case Test::flagged_position:this->call_routed(route,0x17c62au,0x17c62fu,0x11e7b0u);break;
        case Test::position:this->call_routed(route,0x17c637u,0x17c63cu,0x11e780u);break;
        case Test::retire:
            gp[eax]=argument;this->call_routed(route,0x17c673u,0x17c678u,0x17a4a0u);this->test8(al());break;   // MOV EAX,EBX
        }
        kept={gp[ebx],gp[esi],gp[edi]};
        return al();
    }
    // NEG AL; SBB AL,AL; INC AL.
    std::uint32_t negated(std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        set_low_byte(gp[eax],fl.minus(0u,al(),1));
        set_low_byte(gp[eax],fl.borrow(al(),al(),1));
        set_low_byte(gp[eax],fl.increment(al(),1,false));
        return al();
    }
    void owned(){set_low_byte(this->gp[eax],this->flags.logic(0u,1));}                            // XOR AL,AL
    void held(){set_low_byte(this->gp[eax],this->flags.logic(0u,1));}                             // XOR AL,AL
    // CMP AL,CL; (TEST AL,AL when they differ).
    void compared(bool active,bool flag) {
        this->flags.minus(al(),this->gp[ecx]&0xffu,1);
        if(active!=flag)this->test8(al());
    }
    void retired(Kept &kept){this->gp[esi]=this->flags.logic(0u,4);kept.effect=0u;}              // XOR ESI,ESI
    void run() {
        auto &gp=this->gp;
        Memory<Activity> memory{*this};
        const auto result=logic::effect_activity::update(memory,*this,gp[ebx]);
        using logic::effect_activity::Result;
        static constexpr Branch results[]{Branch::unchanged,Branch::deactivated,Branch::activated,Branch::kept,Branch::retired};
        this->reached(results[unsigned(result)]);
        // XOR EAX,EAX; TEST ESI,ESI; POP EDI; SETZ AL; POP ESI.
        const bool second=result==Result::deactivated;
        gp[eax]=this->flags.logic(0u,4);this->test32(gp[esi]);const auto zero=gp[esi]==0;
        gp[edi]=this->pop(second?0x17c690u:0x17c682u);gp[eax]=zero?1u:0u;gp[esi]=this->pop(second?0x17c694u:0x17c686u);
        const auto exit=second?0x17c695u:0x17c687u;
        this->ret(exit,0);this->publish(exit);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x17c5c0u,0x17c695u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Activity<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_effect_activity_detail
