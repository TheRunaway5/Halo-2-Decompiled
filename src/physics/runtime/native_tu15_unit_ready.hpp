#pragma once
// Boundary adapter for the unit readiness, entered at E65A0
// (native_tu15_unit_ready_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the handle; EAX the result; SUB ESP,8 holds
// the scaled ticks and their rounding; ECX and EDX as the original leaves
// them (EDX 1); XMM0 the scaled ticks (CVTSI2SS and MULSS keep its upper
// lanes); the rounding runs on the logical x87 as FLD m32 and FISTP m32 do
// (one free register is required at admission); the flags of the last
// instruction that sets them (ADD ESP,8 at the exit); RET. Each access
// publishes its original instruction.
// HALO2_REFERENCE_UNIT_READY=1 restores the translated body.
// HALO2_VERIFY_UNIT_READY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_unit_ready_systems.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include <emmintrin.h>

namespace halo2::native_unit_ready_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_unit_ready;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_UNIT_READY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_UNIT_READY");return p && *p;}();
    return value;
}
enum Branch : unsigned {flagged, not_ready, counting, short_count, other_state, parented, ready, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Unit ready verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge ready_edges[]{
    {0xe65b5u,0xe661eu,flagged},{0xe65c7u,0xe661eu,not_ready},{0xe65c7u,0xe65cfu,counting},
    {0xe65f8u,0xe661eu,short_count},{0xe65feu,0xe661eu,other_state},{0xe660du,0xe661eu,parented},
    {0xe660du,0xe6618u,ready},
};
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline __m128 with_lane0(__m128 x,std::uint32_t bits){return _mm_move_ss(x,lane0_only(bits));}
inline constexpr bool backend=logic::sse::available && logic::scalar::available;
template<class State> struct Ready : Port<State> {
    using Access=logic::unit_ready::Access;
    using Local=logic::unit_ready::Local;
    Ready(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,ready_edges){}
    std::uint32_t local(Local l)const{return this->gp[esp]+(l==Local::scaled?0u:4u);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:return 0xe65a0u;
        case object_datums:return 0xe65a6u;
        // AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case object_address:gp[eax]=this->flags.logic(gp[eax]&0xffffu,4)*3u;return 0xe65b1u;
        case flags:return 0xe65b5u;
        case ready_flags:return 0xe65c7u;
        case game_time:return 0xe65cfu;
        case ticks:return 0xe65d5u;
        case settle_seconds:return 0xe65ddu;
        case scaled_store:return 0xe65e5u;
        case scaled_x87:return 0xe65eau;
        case rounded_store:return 0xe65edu;
        case count:return 0xe65f1u;
        case rounded_read:return 0xe65f8u;
        case state:return 0xe65feu;
        case parent:return 0xe660du;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:case game_time:gp[ecx]=v;break;
        case object_datums:gp[edx]=v;break;
        case object_address:gp[eax]=v;break;
        // MOV CL; MOV EDX,1; SUB ESP,8; TEST DL,CL.
        case flags:
            set_low_byte(gp[ecx],v);gp[edx]=1u;gp[esp]=this->flags.minus(gp[esp],8u,4);this->test8(gp[ecx]&gp[edx]);break;
        case ready_flags:this->test8(v&gp[edx]);break;                                             // TEST byte,DL
        // MOVSX ECX,word; CVTSI2SS XMM0,ECX.
        case ticks:gp[ecx]=logic::widen16(v);break;
        case count:gp[ecx]=std::uint32_t(std::int32_t(std::int8_t(v)));break;
        case rounded_read:this->compare32(gp[ecx],v);break;                                       // CMP ECX,[ESP+4]
        // MOV CL; CMP CL,DL; (CMP CL,3).
        case state:
            set_low_byte(gp[ecx],v);this->flags.minus(v&0xffu,gp[edx]&0xffu,1);
            if((v&0xffu)!=1u)this->flags.minus(v&0xffu,3u,1);
            break;
        case parent:this->compare32(v,logic::none);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void scaled(std::uint32_t value){this->s.xmm[0]=with_lane0(this->s.xmm[0],value);}
    // FLD [ESP]; FISTP [ESP+4].
    std::uint32_t rounded(std::uint32_t value) {
        auto &fp=this->s.fp;
        fp.push(physics_step_detail::ext(physics_step_detail::f(value)));
        const auto result=physics_contact_runtime_detail::integer_spill(fp.st(0),fp.status);
        fp.pop();
        return result;
    }
    void run() {
        auto &gp=this->gp;
        Memory<Ready> memory{*this};
        const bool result=logic::unit_ready::ready(memory,*this,gp[eax]);
        gp[eax]=result?gp[edx]:this->flags.logic(0u,4);                                          // MOV EAX,EDX or XOR
        gp[esp]=this->flags.plus(gp[esp],8u,4);
        const auto pc=result?0xe6618u:0xe661eu;
        this->ret(pc,0);this->publish(pc);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.fp.count>7u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xe65a0u,0xe661eu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Ready<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_unit_ready_detail
