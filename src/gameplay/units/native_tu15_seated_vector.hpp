#pragma once
// Boundary adapter for the seated vector, CDCF0
// (native_tu15_seated_vector_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the object, the vector
// pointer pushed (RET 4), the answer in AL; a 0x4C-byte frame (the axes and
// the matrix) reserved after the first two loads, below the saved EBP and ESI
// (EBX and EDI as the tests pass); BBFF0 (EAX the parent, ESI the forward,
// EDI the up axis), 30BD0 (EAX the side axis, its length in XMM0) and 144210
// (EAX up, EDX side, ECX the matrix) direct, every register each returns
// coming back. Each access publishes its original instruction.
// HALO2_REFERENCE_SEATED_VECTOR=1 restores the translated body.
// HALO2_VERIFY_SEATED_VECTOR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_seated_vector_systems.hpp"

namespace halo2::native_seated_vector_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_seated_vector;
using Access=logic::seated_vector::Access;
using Kept=logic::seated_vector::Kept;
using Outcome=logic::seated_vector::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SEATED_VECTOR");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SEATED_VECTOR");return v;}
enum Branch : unsigned {no_player, unseated, unturned, turned, fallback, branch_count};
static_assert(unsigned(Outcome::turned)==Branch::turned);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Seated vector"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline void set_high_byte(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff00ffu)|((value&0xffu)<<8);}
inline constexpr std::uint32_t sites[]{0xcdcf0u,0xcdcf6u,0xcdd04u,0xcdd09u,0xcdd0eu,0xcdd1fu,0xcdd31u,0xcdd3fu,0xcdd43u,0xcdd45u,
    0xcdd54u,0xcdd58u,0xcdd67u,0u,0u,0u,0u,0xcddf8u,0u,0u,0u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::vector_store)+1u);
// The side axis's instructions, by pass (the global axis, then the fallback).
inline constexpr std::uint32_t axis_pointers[2]{0xcdd86u,0xcde06u},up_reads[2][3]{{0xcdd8bu,0xcdd96u,0xcddc0u},{0xcde0bu,0xcde16u,0xcde40u}},
    axis_reads[2][6]{{0xcdd91u,0xcdd9cu,0xcddadu,0xcddb8u,0xcddd4u,0xcddddu},{0xcde11u,0xcde1cu,0xcde2du,0xcde38u,0xcde54u,0xcde5du}},
    side_stores[2][3]{{0xcddb2u,0xcddceu,0xcddedu},{0xcde32u,0xcde4eu,0xcde6du}},
    vector_reads[3]{0xcde89u,0xcde8eu,0xcde93u},column_reads[9]{0xcde98u,0xcde9eu,0xcdeb3u,0xcdebdu,0xcdec8u,0xcdedau,0xcdeedu,0xcdef7u,0xcdf01u},
    vector_stores[3]{0xcdec3u,0xcdee8u,0xcdf13u};
template<class State,class Axes,class Normalize,class Matrix> struct Adapter : Port<State> {
    Axes &axes_child;
    Normalize &normalize_child;
    Matrix &matrix_child;
    std::uint32_t depth{};      // bytes below the return address
    unsigned pass{},passes{},ups{},axes_read{},sides{},vectors{},columns{},stores{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Axes &a,Normalize &n,Matrix &m)
        :Port<State>(s,c,t,{}),axes_child(a),normalize_child(n),matrix_child(m){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    std::uint32_t argument()const{return this->gp[esp]+depth+4u;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebp]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case axis_pointer:pass=passes++;ups=axes_read=sides=0u;return axis_pointers[pass];
        case up_read:return up_reads[pass][ups++];
        case axis_read:return axis_reads[pass][axes_read++];
        case side_store:return side_stores[pass][sides++];
        case vector_read:return vector_reads[vectors++];
        case column_read:return column_reads[columns++];
        case vector_store:return vector_stores[stores++];
        default:return sites[unsigned(a)];
        }
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers:case object_address:case parent_tag:case definition:case seats:gp[ecx]=v;break;
        case datums:case parent_object:case instances:case seat_flags:gp[edx]=v;break;
        case vector_pointer:gp[ebp]=v;break;
        case player:gp[esi]=v;break;
        case seat:set_low_word(gp[esi],v);break;
        case parent:gp[ebx]=v;break;
        case axis_pointer:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing(std::uint32_t object_handle) {                                                        // AND EAX,FFFF; SUB ESP,4C; LEA EAX
        auto &gp=this->gp;auto &fl=this->flags;
        const auto index=fl.logic(object_handle&0xffffu,4);
        gp[esp]=fl.minus(gp[esp],0x4cu,4);depth=0x4cu;gp[eax]=3u*index;
    }
    void saving(unsigned n) {
        auto &gp=this->gp;
        static constexpr std::uint32_t at[4]{0xcdd08u,0xcdd0du,0xcdd30u,0xcdd76u};
        static constexpr unsigned saved[4]{ebp,esi,ebx,edi};
        put(at[n],gp[saved[n]]);
    }
    void player_tested(std::uint32_t player_value){set_low_byte(this->gp[eax],this->flags.logic(0u,1));this->compare32(player_value,logic::none);}   // XOR AL,AL; CMP ESI,-1
    void seat_tested(std::uint32_t seat_value){this->flags.minus(seat_value&0xffffu,0xffffu,2);}      // CMP SI,-1
    void parent_indexed(std::uint32_t parent_handle){this->gp[ecx]=3u*this->flags.logic(parent_handle&0xffffu,4);}   // MOV ECX,EBX; AND; LEA
    void tag_indexed(std::uint32_t tag){auto &fl=this->flags;this->gp[ecx]=fl.shift(fl.logic(tag&0xffffu,4),4u,4);}   // AND ECX,FFFF; SHL ECX,4
    void seat_scaled(std::uint32_t seat_value){this->gp[edx]=this->flags.product(logic::widen16(seat_value),logic::seat_size);}   // MOVSX EDX,SI; IMUL EDX,B0
    void flags_tested(std::uint32_t flags) {                                                           // SHR EDX,4; TEST DL,1
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.right(flags,logic::seat::unturned_bit,4,false);fl.logic(gp[edx]&1u,1);
    }
    void axes(std::uint32_t parent_handle,Kept &kept) {                                                // LEA EDI; LEA ESI; MOV EAX,EBX; CALL BBFF0
        auto &gp=this->gp;
        gp[edi]=local(logic::local::up);gp[esi]=local(logic::local::forward);gp[eax]=parent_handle;
        this->call(0xcdd81u,0xcdd86u,0xbbff0u,axes_child);
        refresh(kept);
    }
    void crossed(unsigned,std::uint32_t up_y,std::uint32_t y_term,std::uint32_t z,std::uint32_t x_term) {   // XMM0 to XMM3; LEA EAX
        auto &x=this->s.xmm;
        x[0]=lane0(up_y);x[1]=lane0(y_term);x[2]=lane0(z);x[3]=lane0(x_term);
        this->gp[eax]=local(logic::local::side);
    }
    std::uint32_t normalized(unsigned which,Kept &kept) {                                              // CALL 30BD0
        if(which)this->reached(Branch::fallback);
        const auto site=which?0xcde73u:0xcddf3u;
        this->call(site,site+5u,0x30bd0u,normalize_child);
        refresh(kept);
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(this->s.xmm[0])));
    }
    void length_tested(std::uint32_t length,std::uint32_t bound) {                                     // UCOMISS XMM0,[46A0E8]; LAHF; TEST AH,44; POP EDI
        auto &gp=this->gp;auto &fl=this->flags;
        fl.scalar_compare(lane0(length),lane0(bound),true);
        set_high_byte(gp[eax],fl.bits()|2u);fl.logic((gp[eax]>>8)&0x44u,1);
        gp[edi]=take(0xcde03u);
    }
    void matrix(Kept &kept) {                                                                          // LEA EAX; LEA EDX; LEA ECX; CALL 144210
        auto &gp=this->gp;
        gp[eax]=local(logic::local::up);gp[edx]=local(logic::local::side);gp[ecx]=local(logic::local::matrix);
        this->call(0xcde84u,0xcde89u,0x144210u,matrix_child);
        refresh(kept);
    }
    void turned(std::uint32_t x0,std::uint32_t x1,std::uint32_t x2,std::uint32_t x3,std::uint32_t x4) {
        auto &x=this->s.xmm;
        x[0]=lane0(x0);x[1]=lane0(x1);x[2]=lane0(x2);x[3]=lane0(x3);x[4]=lane0(x4);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        const auto answer=logic::seated_vector::turn(memory,*this,gp[eax],outcome);
        this->reached(unsigned(outcome));
        set_low_byte(gp[eax],answer);                                                                    // MOV AL,1 (or the XOR's 0)
        if(outcome==Outcome::unturned || outcome==Outcome::turned)gp[ebx]=take(0xcdf1au);
        gp[esi]=take(0xcdf1bu);gp[ebp]=take(0xcdf1cu);
        gp[esp]=fl.plus(gp[esp],0x4cu,4);                                                                // ADD ESP,4C
        this->ret(0xcdf20u,4u);this->publish(0xcdf20u);
    }
};
template<class State,class Reference,class Axes,class Normalize,class Matrix>
bool bridge(State &s,Reference reference,Axes axes,Normalize normalize,Matrix matrix) {
    return run_owner(s,0xcdcf0u,0xcdf20u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Axes,Normalize,Matrix>(v,c,t,axes,normalize,matrix);});
}
} // namespace halo2::native_seated_vector_detail
