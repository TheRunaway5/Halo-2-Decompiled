#pragma once
// Boundary adapter for the action target check, entered at 1AE500
// (native_tu15_action_target_check_systems.hpp). Its callers and the
// children keep the original ABI, which the adapter owns: the object, the
// target, the context and the result pointer at [ESP+4..+10], RET 10; EBP
// (the result pointer), EDI (the object), EBX and ESI (the link) saved as
// the original pushes them; 1AE5E0 gets the target, the context and the
// result pointer pushed (and pops them), and its EAX is the result; 1E7810
// gets EAX the object and ECX the context, and every register it returns
// comes back; AL 1 or 0 above the rest of EAX as the original leaves it;
// ECX and EDX as the original leaves them; the flags of the last
// instruction that sets them. Each access publishes its original
// instruction.
// HALO2_REFERENCE_ACTION_TARGET_CHECK=1 restores the translated body.
// HALO2_VERIFY_ACTION_TARGET_CHECK=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_action_target_check_systems.hpp"

namespace halo2::native_action_target_check_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_action_target_check;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ACTION_TARGET_CHECK");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ACTION_TARGET_CHECK");return p && *p;}();
    return value;
}
enum Branch : unsigned {self, other_type, object_excluded, tag_excluded, deferred, tested, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Action target check verify: %llu calls; %llu ordered accesses (%llu stores) "
            "identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Defer,class Route> struct TargetCheck : Port<State> {
    using Access=logic::action_target_check::Access;
    using Argument=logic::action_target_check::Argument;
    using Kept=logic::action_target_check::Kept;
    Defer &defer_child;Route &route;
    std::uint32_t depth{};
    TargetCheck(State &s,std::uint32_t *c,Trace<State> *t,Defer &d,Route &r):Port<State>(s,c,t,{}),defer_child(d),route(r){}
    std::uint32_t argument(Argument a)const{return this->gp[esp]+depth+4u+4u*std::uint32_t(a);}
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    std::uint32_t al()const{return this->gp[eax]&0xffu;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case target_read:return 0x1ae500u;
        case result_read:return 0x1ae505u;
        case object_read:return 0x1ae50au;
        case result_clear:return 0x1ae510u;
        case headers:return 0x1ae51au;
        case header_datums:return 0x1ae520u;
        case type:return 0x1ae532u;
        case object_address:return 0x1ae547u;
        case object_tag:return 0x1ae54au;
        case instances:return 0x1ae54cu;
        case tag_address:return 0x1ae55bu;
        case object_flags:return 0x1ae55fu;
        case link:return 0x1ae56cu;
        case tag_flags_read:return 0x1ae575u;
        case kind:return 0x1ae583u;
        case context_forward:return 0x1ae59du;
        case target_forward:return 0x1ae5a1u;
        case context_test:return 0x1ae5c4u;
        case result_store:return 0x1ae5d3u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case target_read:case object_address:case context_forward:gp[eax]=v;break;
        case result_read:gp[ebp]=v;break;
        case object_read:gp[edi]=v;break;
        case headers:gp[edx]=v;break;
        // MOV EDX,[EDX+44]; MOV ECX,EDI; AND ECX,FFFF; LEA ECX,[ECX+ECX*2]; (PUSH EBX); LEA EDX,[EDX+ECX*4].
        case header_datums:gp[ecx]=fl.logic(gp[edi]&0xffffu,4)*3u;gp[edx]=v+gp[ecx]*4u;break;
        // MOV CL; MOV EBX,1; SHL EBX,CL; MOV AL,1; TEST BL,3.
        case type:
            set_low_byte(gp[ecx],v);gp[ebx]=fl.shift(1u,v&0xffu,4);set_low_byte(gp[eax],1u);
            this->test8(gp[ebx]&logic::acting_types);break;
        case object_tag:case tag_address:case target_forward:case context_test:gp[ecx]=v;break;
        // MOV EDX,[4F0414]; AND ECX,FFFF; SHL ECX,4.
        case instances:gp[edx]=v;gp[ecx]=fl.shift(fl.logic(gp[ecx]&0xffffu,4),logic::tag_instance_shift,4);break;
        // MOV DL; SHR DL,2; TEST DL,1; PUSH ESI (saved_esi).
        case object_flags:set_low_byte(gp[edx],fl.right(v&0xffu,logic::excluded_bit,1,false));this->test8(gp[edx]&1u);break;
        case link:gp[esi]=(gp[esi]&0xffff0000u)|(v&0xffffu);break;                                 // MOV SI
        // MOV ECX,[ECX+BC]; SHR ECX,15; TEST CL,1.
        case tag_flags_read:gp[ecx]=fl.right(v,logic::tag_excluded_bit,4,false);this->test8(gp[ecx]&1u);break;
        // MOV CL; MOV EDX,1; SHL EDX,CL; MOV BL,1; TEST DL,2.
        case kind:
            set_low_byte(gp[ecx],v);gp[edx]=fl.shift(1u,v&0xffu,4);set_low_byte(gp[ebx],1u);
            this->test8(gp[edx]&logic::deferring_kinds);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saved_ebp(){save(0x1ae504u,this->gp[ebp]);}
    void saved_edi(){save(0x1ae509u,this->gp[edi]);}
    void saved_ebx(){save(0x1ae52eu,this->gp[ebx]);}
    void saved_esi(){save(0x1ae56bu,this->gp[esi]);}
    void compared(std::uint32_t acting,std::uint32_t target){this->flags.minus(acting,target,4);}  // CMP EDI,EAX
    void link_compared(std::uint32_t link){this->flags.minus(link&0xffffu,0xffffu,2);}            // CMP SI,-1
    // PUSH EBP; PUSH EAX; PUSH ECX; CALL 1AE5E0 (which pops them).
    void defer(std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        save(0x1ae5a5u,gp[ebp]);save(0x1ae5a6u,gp[eax]);save(0x1ae5a7u,gp[ecx]);
        this->call(0x1ae5a8u,0x1ae5adu,0x1ae5e0u,defer_child);depth-=12u;
    }
    // CALL 1E7810 (EAX the object, ECX the context).
    std::uint32_t refuses(Kept &kept,std::uint32_t,std::uint32_t) {
        this->call_routed(route,0x1ae5c8u,0x1ae5cdu,0x1e7810u);
        kept.result=this->gp[ebp];
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
    void run() {
        auto &gp=this->gp;
        Memory<TargetCheck> memory{*this};
        using logic::action_target_check::Outcome;
        const auto outcome=logic::action_target_check::check(memory,*this);
        static constexpr Branch branches[]{Branch::self,Branch::other_type,Branch::object_excluded,Branch::tag_excluded,
            Branch::deferred,Branch::tested};
        this->reached(branches[unsigned(outcome)]);
        const auto restore=[&](std::uint32_t pc,unsigned r){gp[r]=this->pop(pc);depth-=4u;};
        std::uint32_t exit=0;
        switch(outcome) {
        case Outcome::self:                                                                       // POP EDI; XOR AL,AL; POP EBP
            restore(0x1ae5b4u,edi);set_low_byte(gp[eax],this->flags.logic(0u,1));restore(0x1ae5b7u,ebp);exit=0x1ae5b8u;break;
        case Outcome::object_excluded:case Outcome::tag_excluded:                                 // POP ESI, EBX, EDI; XOR AL,AL; POP EBP
            restore(0x1ae5bbu,esi);restore(0x1ae5bcu,ebx);restore(0x1ae5bdu,edi);
            set_low_byte(gp[eax],this->flags.logic(0u,1));restore(0x1ae5c0u,ebp);exit=0x1ae5c1u;break;
        case Outcome::deferred:                                                                   // POP ESI, EBX, EDI, EBP
            restore(0x1ae5adu,esi);restore(0x1ae5aeu,ebx);restore(0x1ae5afu,edi);restore(0x1ae5b0u,ebp);exit=0x1ae5b1u;break;
        case Outcome::tested:                                                                     // MOV AL,BL; POP ESI
            set_low_byte(gp[eax],gp[ebx]);restore(0x1ae5d8u,esi);[[fallthrough]];
        case Outcome::other_type:                                                                 // POP EBX, EDI, EBP
            restore(0x1ae5d9u,ebx);restore(0x1ae5dau,edi);restore(0x1ae5dbu,ebp);exit=0x1ae5dcu;break;
        }
        this->ret(exit,16u);this->publish(exit);
    }
};
template<class State,class Reference,class Defer,class Route> bool bridge(State &s,Reference reference,Defer defer,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1ae500u,0x1ae5dcu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return TargetCheck<State,Defer,Route>(v,c,t,defer,route);});
    return true;
}
} // namespace halo2::native_action_target_check_detail
