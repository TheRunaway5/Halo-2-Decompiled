#pragma once
// Boundary adapter for the matrix reset, entered at 144CB0
// (native_tu15_matrix_reset_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: EDX the matrix and ESI the
// position; 144910 (through the unit's route) gets EAX the rotation, and
// every register it returns comes back; XMM0 one; EAX the matrix's
// position, ECX and EDX the last coordinates copied; RET. Each access
// publishes its original instruction.
// HALO2_REFERENCE_MATRIX_RESET=1 restores the translated body.
// HALO2_VERIFY_MATRIX_RESET=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_matrix_reset_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_matrix_reset_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_matrix_reset;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_MATRIX_RESET");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_MATRIX_RESET");return p && *p;}();
    return value;
}
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Matrix reset verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
enum Branch : unsigned {reset, branch_count};
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
template<class State,class Route> struct Reset : Port<State> {
    using Access=logic::matrix_reset::Access;
    using Kept=logic::matrix_reset::Kept;
    Route &route;
    Reset(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case position_clear_x:return 0x144cbeu;
        case position_clear_y:return 0x144cc2u;
        case position_clear_z:return 0x144cc7u;
        case one:return 0x144cccu;
        case scale:return 0x144cd4u;
        case source_x:return 0x144cd8u;
        case target_x:return 0x144cdau;
        case source_y:return 0x144cdcu;
        case target_y:return 0x144cdfu;
        case source_z:return 0x144ce2u;
        case target_z:return 0x144ce5u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case one:this->s.xmm[0]=lane0_only(v);break;
        case source_x:case source_z:gp[ecx]=v;break;
        case source_y:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // LEA EAX,[EDX+4]; CALL 144910; XORPS XMM0; LEA EAX,[EDX+28].
    void rotate(Kept &kept) {
        auto &gp=this->gp;
        gp[eax]=gp[edx]+logic::rotation;this->call_routed(route,0x144cb3u,0x144cb8u,0x144910u);
        kept={gp[edx],gp[esi]};this->s.xmm[0]=_mm_setzero_ps();gp[eax]=gp[edx]+logic::position;
        this->reached(Branch::reset);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Reset> memory{*this};
        logic::matrix_reset::reset(memory,*this,{gp[edx],gp[esi]});
        this->ret(0x144ce8u,0);this->publish(0x144ce8u);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x144cb0u,0x144ce8u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Reset<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_matrix_reset_detail
