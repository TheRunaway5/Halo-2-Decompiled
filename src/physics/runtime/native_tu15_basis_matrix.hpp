#pragma once
// Boundary adapter for the basis matrix, entered at 144BF0
// (native_tu15_basis_matrix_systems.hpp). The callers keep the original
// ABI, which the adapter owns: ECX the forward axis, EAX the up axis, ESI
// the position, EDX the matrix; RET; EBX/EBP/EDI saved around the copies;
// the exit leaves EAX the position slot, ECX the position's z, EDX its y,
// XMM0 zero, XMM1 the left y, XMM2 the left x, XMM3 uz*fy (lane 0), and the
// flags untouched; each access publishes its original instruction.
// HALO2_REFERENCE_BASIS_MATRIX=1 restores the translated body.
// HALO2_VERIFY_BASIS_MATRIX=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_basis_matrix_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_basis_matrix_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_basis_matrix;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_BASIS_MATRIX");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_BASIS_MATRIX");return p && *p;}();
    return value;
}
enum Branch : unsigned {composed, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Basis matrix verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr bool backend=logic::sse::available;
inline constexpr Edge basis_edges[]{{0x144c9du,0x144ca0u,composed}};
template<class State> struct Basis : Port<State> {
    using Access=logic::basis_matrix::Access;
    Basis(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,basis_edges){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case one:return 0x144bf0u;
        case scale_store:return 0x144bf8u;
        // PUSH EBX, EBP, EDI; MOV EBX,ECX.
        case forward_x:
            this->push(0x144bfcu,gp[ebx]);this->push(0x144bfdu,gp[ebp]);this->push(0x144bfeu,gp[edi]);gp[ebx]=gp[ecx];
            return 0x144c01u;
        case forward_x_store:return 0x144c06u;
        case forward_y:return 0x144c08u;
        case forward_y_store:return 0x144c0bu;
        case forward_z:return 0x144c0eu;
        case forward_z_store:return 0x144c11u;
        case fy:return 0x144c14u;
        case fx:return 0x144c19u;
        case uy:return 0x144c1du;
        case ux:return 0x144c22u;
        case fz:return 0x144c26u;
        case ux_again:return 0x144c2bu;
        case uz:return 0x144c2fu;
        case fy_again:return 0x144c34u;
        case uz_again:return 0x144c3du;
        case fx_again:return 0x144c42u;
        case fz_again:return 0x144c4au;
        case uy_again:return 0x144c4fu;
        case left_z_store:return 0x144c54u;
        case left_y_store:return 0x144c5cu;
        case left_x_store:return 0x144c65u;
        case up_x:return 0x144c6au;
        case up_x_store:return 0x144c6fu;
        case up_y:return 0x144c71u;
        case up_y_store:return 0x144c74u;
        case up_z:return 0x144c77u;
        case up_z_store:return 0x144c7au;
        case position_clear_x:gp[eax]=gp[edx]+logic::matrix::position;return 0x144c80u;         // LEA EAX,[EDX+28]
        case position_clear_y:return 0x144c84u;
        case position_clear_z:return 0x144c89u;
        case position_x:return 0x144c8eu;
        case position_x_store:return 0x144c90u;
        case position_y:return 0x144c92u;
        case position_y_store:return 0x144c96u;
        case position_z:return 0x144c99u;
        case position_z_store:return 0x144c9du;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case one:this->s.xmm[0]=lane0_only(v);break;
        case forward_x:gp[ebp]=v;gp[edi]=gp[edx]+logic::matrix::forward;break;                    // LEA EDI,[EDX+4]
        case forward_y:gp[ebp]=v;break;
        case forward_z:gp[ebx]=v;break;
        case up_x:gp[edi]=v;gp[ecx]=gp[edx]+logic::matrix::up;break;                              // LEA ECX,[EDX+1C]
        case up_y:gp[edi]=v;break;
        case up_z:gp[eax]=v;break;
        case position_x:gp[ecx]=v;break;
        case position_y:gp[edx]=v;gp[edi]=this->pop(0x144c95u);break;                            // POP EDI
        case position_z:gp[ecx]=v;gp[ebp]=this->pop(0x144c9cu);break;                            // POP EBP
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void cleared(){this->s.xmm[0]=_mm_setzero_ps();}
    void crossed(const logic::basis_matrix::Left &l) {
        auto &x=this->s.xmm;
        x[1]=lane0_only(l.y);x[2]=lane0_only(l.x);x[3]=lane0_only(l.uz_fy);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Basis> memory{*this};
        logic::basis_matrix::compose(memory,*this,gp[ecx],gp[eax],gp[esi],gp[edx]);
        gp[ebx]=this->pop(0x144ca0u);
        this->ret(0x144ca1u,0);this->publish(0x144ca1u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x144bf0u,0x144ca1u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Basis<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_basis_matrix_detail
