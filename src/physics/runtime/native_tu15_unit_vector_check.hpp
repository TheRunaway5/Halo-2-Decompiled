#pragma once
// Boundary adapter for the unit vector check, entered at B1720
// (native_tu15_unit_vector_check_systems.hpp). The callers and the children
// keep the original ABI, which the adapter owns: ESI the vector, EDI the
// other; EAX the result; SUB ESP,8 for the two copies of the length error;
// the x87 comparison pushes two values and pops them (C1 cleared; a full
// stack is the translated route's fault, so admission needs two free
// registers); AA200 gets EAX the other vector and the FCOMIP flags; AA270
// gets 0 pushed, XMM0 the dot product and XMM1 its x term; every register a
// child returns comes back; the exit ADD ESP,8 decides the flags; each
// access publishes its original instruction.
// HALO2_REFERENCE_UNIT_VECTOR_CHECK=1 restores the translated body.
// HALO2_VERIFY_UNIT_VECTOR_CHECK=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_unit_vector_check_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_unit_vector_check_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_unit_vector_check;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_UNIT_VECTOR_CHECK");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_UNIT_VECTOR_CHECK");return p && *p;}();
    return value;
}
enum Branch : unsigned {not_finite, finite, outside, within, invalid_other, valid_other, disagrees, agrees_, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Unit vector check verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr bool backend=logic::sse::available && logic::x87::available;
inline constexpr Edge check_edges[]{
    {0xb1765u,0xb17c7u,not_finite},{0xb1765u,0xb176du,finite},
    {0xb1773u,0xb17c7u,outside},{0xb1773u,0xb1781u,within},
    {0xb1781u,0xb17c7u,invalid_other},{0xb1781u,0xb178au,valid_other},
    {0xb17b0u,0xb17c7u,disagrees},{0xb17b0u,0xb17c1u,agrees_},
};
template<class State,class Valid,class Agree> struct Check : Port<State> {
    using Access=logic::unit_vector_check::Access;
    using Local=logic::unit_vector_check::Local;
    Valid &valid_child;Agree &agree_child;
    std::uint32_t frame{};bool passed{};
    Check(State &s,std::uint32_t *c,Trace<State> *t,Valid &v,Agree &a):Port<State>(s,c,t,check_edges),valid_child(v),agree_child(a){}
    std::uint32_t local(Local l)const{return l==Local::error?frame-8u:frame-4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case vy:return 0xb1723u;
        case vz:return 0xb1728u;
        case vx:return 0xb172du;
        case one:return 0xb174bu;
        case error_store:return 0xb1753u;
        case error_bits:return 0xb1758u;
        case error_copy_store:return 0xb1765u;
        case error_for_x87:return 0xb176du;
        case tolerance:return 0xb1773u;
        case other_z:return 0xb178au;
        case vz_again:return 0xb178fu;
        case other_y:return 0xb1794u;
        case vy_again:return 0xb1799u;
        case vx_again:return 0xb17a2u;
        case other_x:return 0xb17a6u;
        }
        return 0;
    }
    // MOV EAX,[ESP]; AND EAX,7F800000; CMP EAX,7F800000.
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        if(a==Access::error_bits) {
            this->gp[eax]=this->flags.logic(v&logic::exponent_mask,4);this->compare32(this->gp[eax],logic::exponent_mask);
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void measured(const logic::unit_vector_check::Length &l) {
        auto &x=this->s.xmm;
        x[0]=lane0_only(l.error);x[1]=lane0_only(l.z);x[2]=lane0_only(l.zz);x[3]=lane0_only(l.yy);
    }
    // The logical x87 stack balances (C1 cleared by the loads); FCOMIP's flags,
    // from the exact single values compared as ordered integers (no SSE
    // compare: it would raise MXCSR flags and treat denormals as zero).
    static bool not_a_number(std::uint32_t bits){return (bits&0x7fffffffu)>0x7f800000u;}
    static std::int64_t ordered(std::uint32_t bits) {
        const auto magnitude=std::int64_t(bits&0x7fffffffu);
        return bits&0x80000000u?-magnitude:magnitude;
    }
    void compared(std::uint32_t d,std::uint32_t limit) {
        this->s.fp.status=std::uint16_t(this->s.fp.status&~0x200u);
        const auto magnitude=d&0x7fffffffu;
        const bool unordered=not_a_number(magnitude)||not_a_number(limit);
        this->flags.cf=unordered||ordered(limit)<ordered(magnitude);
        this->flags.zf=unordered||ordered(limit)==ordered(magnitude);
        this->flags.pf=unordered;this->flags.af=this->flags.sf=this->flags.of=false;
    }
    // MOV EAX,EDI; CALL AA200; TEST AL,AL.
    std::uint32_t other_valid(logic::unit_vector_check::Kept &kept) {
        auto &gp=this->gp;
        gp[eax]=gp[edi];
        this->call(0xb1781u,0xb1786u,0xaa200u,valid_child);this->test8(gp[eax]);
        kept={gp[esi],gp[edi]};
        return gp[eax]&0xffu;
    }
    // PUSH 0; XMM0 the dot product, XMM1 the x term.
    void dotted(const logic::unit_vector_check::Dot &d) {
        auto &x=this->s.xmm;
        this->push(0xb17aau,0);
        x[0]=lane0_only(d.dot);x[1]=lane0_only(d.x_term);
    }
    // CALL AA270; TEST AL,AL.
    std::uint32_t agrees() {
        auto &gp=this->gp;
        this->call(0xb17b0u,0xb17b5u,0xaa270u,agree_child);this->test8(gp[eax]);
        return gp[eax]&0xffu;
    }
    void run() {
        auto &gp=this->gp;
        frame=gp[esp];
        gp[esp]=this->flags.minus(gp[esp],8u,4);                                                   // SUB ESP,8
        Memory<Check> memory{*this};
        if(logic::unit_vector_check::check(memory,*this,gp[esi],gp[edi])) {
            gp[eax]=1u;gp[esp]=this->flags.plus(gp[esp],8u,4);this->ret(0xb17c1u,0);this->publish(0xb17c1u);
        } else {
            gp[eax]=this->flags.logic(0,4);gp[esp]=this->flags.plus(gp[esp],8u,4);this->ret(0xb17c7u,0);this->publish(0xb17c7u);
        }
    }
};
template<class State,class Reference,class Valid,class Agree>
bool bridge(State &s,Reference reference,Valid valid,Agree agree) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.fp.count>6u)return false;
    if((_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xb1720u,0xb17c7u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Check<State,Valid,Agree>(v,c,t,valid,agree);});
    return true;
}
} // namespace halo2::native_unit_vector_check_detail
