#pragma once
// Boundary adapter for the body matrix, 1D8B90
// (native_tu15_body_matrix_systems.hpp). Its callers and 2E4650 keep the
// original ABI, which the adapter owns: EAX the body index, ECX the model,
// ESI the matrix (RET); EBP saved and ESP aligned down to 16 below 0x40
// bytes of matrix; 2E4650 (ECX the stack matrix, the offset pushed, RET 4)
// direct, every register it returns coming back, the stack read from the ESP
// it left. XMM0 holds what each move loads. Each access publishes its
// original instruction.
// HALO2_REFERENCE_BODY_MATRIX=1 restores the translated body.
// HALO2_VERIFY_BODY_MATRIX=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/renderer/native_tu15_body_matrix_systems.hpp"

namespace halo2::native_body_matrix_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_body_matrix;
using Access=logic::body_matrix::Access;
using Kept=logic::body_matrix::Kept;
using Outcome=logic::body_matrix::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_BODY_MATRIX");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_BODY_MATRIX");return v;}
enum Branch : unsigned {plain, offset, branch_count};
static_assert(unsigned(Outcome::offset)==Branch::offset);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Body matrix"};return v;}
inline constexpr std::uint32_t row_loads[]{0x1d8ba9u,0x1d8bbcu,0x1d8bc5u,0x1d8bd4u},row_stores[]{0x1d8bb8u,0x1d8bc0u,0x1d8bcfu,0x1d8bd8u};
inline constexpr std::uint32_t element_loads[]{0x1d8bf8u,0x1d8c02u,0x1d8c0du,0x1d8c18u,0x1d8c23u,0x1d8c2eu,0x1d8c39u,0x1d8c44u,0x1d8c4fu,
    0x1d8c5au,0x1d8c65u,0x1d8c70u};
inline constexpr std::uint32_t element_stores[]{0x1d8bfdu,0x1d8c08u,0x1d8c13u,0x1d8c1eu,0x1d8c29u,0x1d8c34u,0x1d8c3fu,0x1d8c4au,0x1d8c55u,
    0x1d8c60u,0x1d8c6bu,0x1d8c76u};
template<class State,class Compose> struct Adapter : Port<State> {
    Compose &compose;
    unsigned rows_loaded{},rows_stored{},elements_loaded{},elements_stored{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Compose &child):Port<State>(s,c,t,{}),compose(child){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+offset;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case bodies:return 0x1d8b99u;
        case body_entry:return 0x1d8ba2u;
        case motion:return 0x1d8ba6u;
        case row_load:return row_loads[rows_loaded++];
        case flags:return 0x1d8bb0u;
        case row_store:return row_stores[rows_stored++];
        case one:return 0x1d8becu;
        case scale_store:return 0x1d8bf4u;
        case element_load:return element_loads[elements_loaded++];
        case element_store:return element_stores[elements_stored++];
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case bodies:case flags:gp[edx]=v;break;
        case body_entry:case motion:gp[eax]=v;break;
        case one:case element_load:this->s.xmm[0]=_mm_castsi128_ps(_mm_cvtsi32_si128(int(v)));break;   // MOVSS XMM0
        default:break;
        }
    }
    void loaded_vector(Access,std::uint32_t,__m128 v){this->s.xmm[0]=v;}                               // MOVAPS XMM0
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // PUSH EBP; MOV EBP,ESP; AND ESP,-16; SUB ESP,40
        auto &gp=this->gp;auto &fl=this->flags;
        this->push(0x1d8b90u,gp[ebp]);gp[ebp]=gp[esp];
        gp[esp]=fl.minus(fl.logic(gp[esp]&0xfffffff0u,4),logic::rows*logic::row_size,4);
    }
    std::uint32_t indexed(std::uint32_t index) {                                                        // LEA EAX,[EAX+EAX*2]; SHL EAX,5
        static_assert((3u<<5)==logic::body::size);
        return this->gp[eax]=this->flags.shift(3u*index,5u,4);
    }
    std::uint32_t rows(std::uint32_t motion_address) {                                                  // ADD EAX,80
        return this->gp[eax]=this->flags.plus(motion_address,logic::body::transform,4);
    }
    std::uint32_t offset_tested(std::uint32_t flags_value) {                                            // SHR EDX,6; TEST DL,1
        auto &gp=this->gp;
        gp[edx]=this->flags.right(flags_value,logic::model::offset_bit,4,false);
        this->flags.logic(gp[edx]&1u,1);
        return gp[edx];
    }
    void composed(std::uint32_t offset_address,Kept &kept) {                                           // ADD ECX,30; PUSH ECX; LEA ECX,[ESP+4]; CALL 2E4650
        auto &gp=this->gp;
        gp[ecx]=this->flags.plus(gp[ecx],logic::model::offset,4);
        this->push(0x1d8be2u,offset_address);gp[ecx]=gp[esp]+4u;
        this->call(0x1d8be7u,0x1d8becu,0x2e4650u,compose);
        kept.matrix=gp[esi];
        this->reached(Branch::offset);
    }
    void restoring() {                                                                                   // MOV ESP,EBP; POP EBP
        auto &gp=this->gp;
        gp[esp]=gp[ebp];gp[ebp]=this->pop(0x1d8c7du);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::body_matrix::convert(memory,*this,gp[eax],gp[ecx],Kept{gp[esi]});
        this->reached(unsigned(outcome));
        this->ret(0x1d8c7eu,0u);this->publish(0x1d8c7eu);
    }
};
template<class State,class Reference,class Compose> bool bridge(State &s,Reference reference,Compose compose) {
    return run_owner(s,0x1d8b90u,0x1d8c7eu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Compose>(v,c,t,compose);});
}
} // namespace halo2::native_body_matrix_detail
