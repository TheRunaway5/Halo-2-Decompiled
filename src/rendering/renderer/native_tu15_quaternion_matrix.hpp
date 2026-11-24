#pragma once
// Boundary adapter for the quaternion matrix, 144910
// (native_tu15_quaternion_matrix_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: ECX the quaternion, EAX the matrix (RET); a
// 12-byte frame reserved after the norm; XMM0 to XMM7 as the original leaves
// them (every value a scalar with zero upper lanes, as MOVSS loads them).
// Each access publishes its original instruction.
// HALO2_REFERENCE_QUATERNION_MATRIX=1 restores the translated body.
// HALO2_VERIFY_QUATERNION_MATRIX=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/renderer/native_tu15_quaternion_matrix_systems.hpp"

namespace halo2::native_quaternion_matrix_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_quaternion_matrix;
using Access=logic::quaternion_matrix::Access;
using Registers=logic::quaternion_matrix::Registers;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_QUATERNION_MATRIX");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_QUATERNION_MATRIX");return v;}
enum Branch : unsigned {scaled, degenerate, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Quaternion matrix"};return v;}
inline constexpr std::uint32_t components[16]{0x144910u,0x144915u,0x14491au,0x14491fu,0x144965u,0x144969u,0x14496eu,0x144983u,0x14498bu,
    0x144993u,0x14499eu,0x1449a6u,0x1449b3u,0x1449c2u,0x1449c7u,0x1449d0u},
    ones[2]{0x144973u,0x1449fdu},local_stores[3]{0x144998u,0x1449aeu,0x1449d5u},local_loads[4]{0x1449dfu,0x1449e8u,0x144a0fu,0x144a15u},
    elements[9]{0x1449eeu,0x144a05u,0x144a0au,0x144a21u,0x144a40u,0x144a45u,0x144a4au,0x144a4fu,0x144a54u};
template<class State> struct Adapter : Port<State> {
    unsigned counts[7]{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+0xcu+std::uint32_t(offset);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        const auto k=counts[unsigned(a)]++;
        switch(a) {
        case component:return components[k];
        case least:return 0x14494bu;
        case two:return 0x144954u;
        case one:return ones[k];
        case local_store:return local_stores[k];
        case local_load:return local_loads[k];
        default:return elements[k];
        }
    }
    void loaded(Access,std::uint32_t,std::uint32_t){}
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing(){this->gp[esp]=this->flags.minus(this->gp[esp],0xcu,4);}                          // SUB ESP,C
    void compared(std::uint32_t norm,std::uint32_t least_value) {                                      // COMISS XMM1,[46A0EC]
        const auto lane=[](std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(v)));};
        this->flags.scalar_compare(lane(norm),lane(least_value),false);
        this->reached(logic::sse::not_above(norm,least_value)?Branch::degenerate:Branch::scaled);
    }
    void finished(const Registers &r) {
        for(unsigned k=0;k<8u;++k)this->s.xmm[k]=_mm_castsi128_ps(_mm_cvtsi32_si128(int(r.xmm[k])));
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::quaternion_matrix::build(memory,*this,gp[ecx],gp[eax]);
        gp[esp]=this->flags.plus(gp[esp],0xcu,4);                                                       // ADD ESP,C
        this->ret(0x144a5cu,0u);this->publish(0x144a5cu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x144910u,0x144a5cu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_quaternion_matrix_detail
