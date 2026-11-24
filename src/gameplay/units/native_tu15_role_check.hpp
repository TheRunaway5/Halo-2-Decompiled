#pragma once
// Boundary adapter for the role check, entered at 17E100
// (native_tu15_role_check_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EDX the owner, ECX the record, ESI the side; the
// result in AL (SETNE; XOR AL,AL; MOV AL,1) over AX the role word read last;
// EDX the local players' record; the flags of the last instruction that
// sets them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_ROLE_CHECK=1 restores the translated body.
// HALO2_VERIFY_ROLE_CHECK=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_role_check_systems.hpp"

namespace halo2::native_role_check_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_role_check;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ROLE_CHECK");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ROLE_CHECK");return p && *p;}();
    return value;
}
enum Branch : unsigned {several, matching, refused_two, accepted, accepted_one, refused_one, branch_count};
static_assert(unsigned(logic::role_check::Outcome::refused_one)==Branch::refused_one);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Role check verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct RoleCheck : Port<State> {
    using Access=logic::role_check::Access;
    RoleCheck(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case owner_flags:return 0x17e100u;case role:return 0x17e103u;case players:return 0x17e119u;
            case local_count:return 0x17e127u;case second_role:return 0x17e132u;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case owner_flags:set_low_byte(gp[edx],v);break;                                             // MOV DL,[EDX+2]
        // MOV AX,[ECX+10]; SHR DL,5; TEST DL,1.
        case role:gp[eax]=(gp[eax]&0xffff0000u)|(v&0xffffu);
            set_low_byte(gp[edx],fl.right(gp[edx]&0xffu,logic::first_role_bit,1,false));this->test8(gp[edx]&1u);break;
        case players:gp[edx]=v;break;                                                               // MOV EDX,[4F54F0]
        case second_role:gp[eax]=(gp[eax]&0xffff0000u)|(v&0xffffu);break;                          // MOV AX,[ECX+12]
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void compared(std::uint32_t value,std::uint32_t wanted,unsigned bytes){this->flags.minus(value,wanted,bytes);}
    void side_tested(std::uint32_t side){this->test32(side);}                                       // TEST ESI,ESI
    // SETNE AL; TEST AL,AL; SETNE AL.
    void noted(std::uint32_t differs){set_low_byte(this->gp[eax],differs);this->test8(differs);}
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::role_check::Outcome;
        Memory<RoleCheck> memory{*this};
        const auto result=logic::role_check::check(memory,*this,gp[edx],gp[ecx],gp[esi]);
        this->reached(unsigned(result.outcome));
        if(result.outcome==Outcome::several){this->ret(0x17e150u,0);this->publish(0x17e150u);return;}   // AL from the SETNE
        if(result.value){set_low_byte(gp[eax],1u);this->ret(0x17e14du,0);this->publish(0x17e14du);return;}  // MOV AL,1
        if(result.outcome==Outcome::matching)this->test8(gp[eax]);                                   // TEST AL,AL
        set_low_byte(gp[eax],this->flags.logic(0u,1));this->ret(0x17e150u,0);this->publish(0x17e150u);    // XOR AL,AL
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x17e100u,0x17e150u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return RoleCheck<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_role_check_detail
