#pragma once
// Boundary adapter for the rotation product, 1459B0
// (native_tu15_rotation_product_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the output, EDX and ECX the factors (RET);
// a 0x24-byte frame (the copy) below the saved EBX, ESI and EDI; ESI, EDI and
// ECX stepping with each REP MOVSD; XMM0 the last element's sum and XMM1 its
// last product. Each access publishes its original instruction.
// HALO2_REFERENCE_ROTATION_PRODUCT=1 restores the translated body.
// HALO2_VERIFY_ROTATION_PRODUCT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/renderer/native_tu15_rotation_product_systems.hpp"

namespace halo2::native_rotation_product_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_rotation_product;
using Access=logic::rotation_product::Access;
using Outcome=logic::rotation_product::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ROTATION_PRODUCT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ROTATION_PRODUCT");return v;}
enum Branch : unsigned {separate, left_aliased, right_aliased, both_aliased, branch_count};
static_assert(unsigned(Outcome::both_aliased)==Branch::both_aliased);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Rotation product"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
// The nine elements' instructions: per element, the three terms' first load and MULSS, and the store.
inline constexpr std::uint32_t first_sites[9][3]{{0x1459e2u,0x1459eau,0x1459f8u},{0x145a0au,0x145a14u,0x145a21u},{0x145a34u,0x145a3eu,0x145a4bu},
    {0x145a5eu,0x145a68u,0x145a75u},{0x145a88u,0x145a92u,0x145aa0u},{0x145ab3u,0x145abdu,0x145acbu},{0x145adeu,0x145ae7u,0x145af5u},
    {0x145b08u,0x145b12u,0x145b20u},{0x145b33u,0x145b3du,0x145b4bu}};
inline constexpr std::uint32_t second_sites[9][3]{{0x1459e6u,0x1459efu,0x1459fdu},{0x145a0fu,0x145a19u,0x145a26u},{0x145a39u,0x145a43u,0x145a50u},
    {0x145a63u,0x145a6du,0x145a7au},{0x145a8du,0x145a97u,0x145aa5u},{0x145ab8u,0x145ac2u,0x145ad0u},{0x145ae2u,0x145aecu,0x145afau},
    {0x145b0du,0x145b17u,0x145b25u},{0x145b38u,0x145b42u,0x145b50u}};
inline constexpr std::uint32_t store_sites[9]{0x145a06u,0x145a2fu,0x145a59u,0x145a83u,0x145aaeu,0x145ad9u,0x145b03u,0x145b2eu,0x145b59u};
template<class State> struct Adapter : Port<State> {
    std::uint32_t depth{};      // bytes below the return address
    unsigned element{},term_index{},compares{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t copy_to()const{return this->gp[esp]+depth-0x24u;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0u-4u:4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case copy_left_read:case copy_left_write:return 0x1459c7u;
        case copy_right_read:case copy_right_write:return 0x1459dcu;
        case first:return first_sites[element][term_index];
        case second:return second_sites[element][term_index];
        case product_store:return store_sites[element];
        }
        return 0u;
    }
    void loaded(Access,std::uint32_t,std::uint32_t){}
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        if(a==copy_left_write||a==copy_right_write){const auto d=step();gp[esi]+=d;gp[edi]+=d;--gp[ecx];}
    }
    void compared(std::uint32_t factor,std::uint32_t out) {                                            // CMP EDX,EAX; PUSH EBX,ESI,EDI; MOV EBX,ECX / CMP EBX,EAX
        auto &gp=this->gp;
        this->compare32(factor,out);
        if(compares++==0u){put(0x1459b5u,gp[ebx]);put(0x1459b6u,gp[esi]);put(0x1459b7u,gp[edi]);gp[ebx]=gp[ecx];}
    }
    void copying(unsigned which,std::uint32_t from,std::uint32_t to) {                                 // MOV ESI; MOV ECX,9; LEA EDI
        auto &gp=this->gp;
        gp[esi]=from;gp[ecx]=logic::words;gp[edi]=to;
        if(which==0u)gp[edx]=to;else gp[ebx]=to;                                                     // LEA EDX (EBX),[ESP+C] after
    }
    void at_term(unsigned e,unsigned k){element=e;term_index=k;}
    void term(unsigned,unsigned k,std::uint32_t product,std::uint32_t sum) {
        auto &x=this->s.xmm;
        if(k==0u)x[0]=lane0(product);else{x[1]=lane0(product);x[0]=lane0(sum);}
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x24u,4);depth=0x24u;                                                 // SUB ESP,24
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::rotation_product::multiply(memory,*this,gp[eax],gp[edx],gp[ecx])));
        gp[edi]=take(0x145b5eu);gp[esi]=take(0x145b5fu);gp[ebx]=take(0x145b60u);
        gp[esp]=fl.plus(gp[esp],0x24u,4);                                                              // ADD ESP,24
        this->ret(0x145b64u,0u);this->publish(0x145b64u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x1459b0u,0x145b64u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_rotation_product_detail
