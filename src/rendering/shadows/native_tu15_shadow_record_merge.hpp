#pragma once
// Boundary adapter for the shadow record merge, 4A7B0
// (native_tu15_shadow_record_merge_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the cache and the casters and their
// count on the stack (RET 8); 8 bytes of frame with EBX, EBP, ESI and EDI
// saved below them. The routine calls no child: what leaves it is EAX, ECX
// (the casters' count, or the count less three), EDX (the mask of casters
// taken), XMM0 (zero) and the flags of its last compare; the adapter keeps
// those as the original's instructions leave them, the stale records' REP
// MOVSD in the direction flag's direction. Each access publishes its
// original instruction.
// HALO2_REFERENCE_SHADOW_RECORD_MERGE=1 restores the translated body.
// HALO2_VERIFY_SHADOW_RECORD_MERGE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/shadows/native_tu15_shadow_record_merge_systems.hpp"

namespace halo2::native_shadow_record_merge_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_shadow_record_merge;
using Access=logic::shadow_record_merge::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SHADOW_RECORD_MERGE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SHADOW_RECORD_MERGE");return v;}
enum Branch : unsigned {refreshed, stale, four_at_a_time, full, appended, rest_appended, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Shadow record merge"};return v;}
inline std::uint32_t low(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
inline constexpr std::uint32_t sites[]{
    0x4a7b3u,0x4a7c1u,0x4a7d0u,0x4a7dau,0x4a7deu,0x4a7e4u,0x4a7ecu,0x4a7fbu,0x4a800u,0x4a802u,0x4a805u,0x4a808u,0x4a80bu,0x4a80eu,
    0x4a811u,0x4a814u,0x4a817u,0x4a81au,0x4a820u,0x4a82bu,0x4a82fu,0x4a83bu,0x4a841u,0x4a850u,0x4a85au,0x4a85du,0x4a873u,0x4a873u,
    0x4a875u,0x4a87du,0x4a886u,0x4a894u,0x4a8a0u,0x4a8aau,0x4a8aeu,0x4a8c0u,0x4a8e5u,0x4a8e7u,0x4a8e9u,0x4a8ecu,0x4a8efu,0x4a8f2u,
    0x4a8f5u,0x4a8f8u,0x4a8fbu,0x4a8feu,0x4a901u,0x4a907u,0x4a910u,0x4a914u,0x4a91au,0x4a920u,0x4a927u,0x4a92bu,0x4a934u,0x4a93bu,
    0x4a93eu,0x4a947u,0x4a94du,0x4a94du,0x4a955u,0x4a964u,0x4a97du,0x4a97fu,0x4a981u,0x4a984u,0x4a987u,0x4a98au,0x4a98du,0x4a990u,
    0x4a993u,0x4a996u,0x4a999u,0x4a99fu,0x4a9a8u,0x4a9acu,0x4a9b2u,0x4a9b8u,0x4a9bfu,0x4a9c3u,0x4a9ccu,0x4a9d3u,0x4a9d6u,0x4a9dfu,
    0x4a9e5u,0x4a9e5u,0x4a9edu,0x4a9fcu,0x4aa13u,0x4aa15u,0x4aa17u,0x4aa1au,0x4aa1du,0x4aa20u,0x4aa23u,0x4aa26u,0x4aa29u,0x4aa2cu,
    0x4aa2fu,0x4aa35u,0x4aa3eu,0x4aa42u,0x4aa48u,0x4aa4eu,0x4aa55u,0x4aa59u,0x4aa62u,0x4aa69u,0x4aa6cu,0x4aa75u,0x4aa7bu,0x4aa7bu,
    0x4aa83u,0x4aa92u,0x4aaabu,0x4aaadu,0x4aaafu,0x4aab2u,0x4aab5u,0x4aab8u,0x4aabbu,0x4aabeu,0x4aac1u,0x4aac4u,0x4aac7u,0x4aacdu,
    0x4aad6u,0x4aadau,0x4aae0u,0x4aae6u,0x4aaedu,0x4aaf1u,0x4aafau,0x4ab01u,0x4ab04u,0x4ab0du,0x4ab13u,0x4ab13u,0x4ab1bu,0x4ab1fu,
    0x4ab31u,0x4ab35u,0x4ab3fu,0x4ab49u,0x4ab54u,0x4ab60u,0x4ab7fu,0x4ab83u,0x4ab88u,0x4ab8au,0x4ab8du,0x4ab90u,0x4ab93u,0x4ab96u,
    0x4ab99u,0x4ab9cu,0x4ab9fu,0x4aba2u,0x4aba8u,0x4abb1u,0x4abb5u,0x4abbbu,0x4abc4u,0x4abc8u,0x4abd1u,0x4abd8u,0x4abdbu,0x4abe4u,
    0x4abeau,0x4abeau,0x4abf2u,0x4abf6u,0x4ac00u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::cursor_store)+1u);
template<class State> struct Adapter : Port<State> {
    std::uint32_t depth{};   // bytes pushed beyond the four saved registers (negative before them)
    std::uint32_t bit_mask{};// the bit of the caster last tested (EBX or EDI)
    bool resting{};          // past the four-at-a-time passes
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0u-4u:4u;}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        using enum Access;
        switch(a){case bit_init:case count_a_0:case count_b_0:case count_c_0:case count_d_0:case bit_1:case count_a_1:case count_b_1:case count_c_1:case count_d_1:case bit_2:case count_a_2:case count_b_2:case count_c_2:case count_d_2:case bit_3:case count_a_3:case count_b_3:case count_c_3:case count_d_3:case count_a_rest:case count_b_rest:case count_c_rest:case count_d_rest:case count_load:case index_load:case count_remove:case count_check:case count_next:case list_count_add:case pass_list_count:case list_count_tail:this->gp[ecx]=v;break;default:break;}
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {                                                  // REP MOVSD: ESI, EDI and ECX step
        auto &gp=this->gp;
        if(a==Access::stale_copy_store){const auto d=step();gp[esi]+=d;gp[edi]+=d;--gp[ecx];}
    }
    void framing(){auto &gp=this->gp;gp[esp]=this->flags.minus(gp[esp],8u,4);depth=0u-16u;}                  // SUB ESP,8
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / ESI / EDI
        static constexpr std::uint32_t pcs[]{0x4a7b9u,0x4a7bau,0x4a7bfu,0x4a7c0u};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t unmarked(){return this->gp[edx]=this->flags.logic(0u,4);}                                  // XOR EDX,EDX
    bool above(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)>std::int32_t(b);}   // CMP; JLE
    bool below(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)<std::int32_t(b);}   // CMP; JL / JGE
    bool positive(std::uint32_t v){this->test32(v);return std::int32_t(v)>0;}                                // TEST; JLE
    bool same(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return a==b;}                            // CMP
    // The refresh.
    std::uint32_t marked(std::uint32_t mask,std::uint32_t caster) {                                      // MOV ESI,1; SHL ESI,CL; OR EDX,ESI
        auto &fl=this->flags;
        this->reached(Branch::refreshed);
        return this->gp[edx]=fl.logic(mask|fl.shift(1u,caster&0xffu,4),4);
    }
    std::uint32_t next_caster(std::uint32_t j){return this->gp[ecx]=this->flags.increment(j,4,false);}        // INC ECX
    std::uint32_t next_record(std::uint32_t index){return this->gp[ecx]=this->flags.increment(index,4,false);}   // INC ECX
    // The stale records.
    std::uint32_t copying(std::uint32_t from,std::uint32_t to) {                                         // LEA ESI; MOV EDI,EBX; MOV ECX,9
        auto &gp=this->gp;
        this->reached(Branch::stale);
        gp[esi]=from;gp[edi]=to;gp[ecx]=logic::record_words;
        return from;
    }
    std::uint32_t dropped(std::uint32_t count){return this->gp[edi]=this->flags.increment(count,4,true);}     // DEC EDI
    // The appends.
    void weight_cleared(){this->s.xmm[0]=_mm_setzero_ps();}                                                   // XORPS XMM0,XMM0
    std::uint32_t weight()const{return low(this->s.xmm[0]);}                                                  // MOVSS [..],XMM0
    bool at_least_four(std::uint32_t count) {                                                            // CMP ECX,4; JL
        this->compare32(count,logic::unrolled);
        const bool four=std::int32_t(count)>=std::int32_t(logic::unrolled);
        if(four)this->reached(Branch::four_at_a_time);
        return four;
    }
    bool room(std::uint32_t total) {                                                                     // CMP reg,20; JGE
        this->compare32(total,logic::capacity);
        if(std::int32_t(total)>=std::int32_t(logic::capacity))this->reached(Branch::full);
        return std::int32_t(total)<std::int32_t(logic::capacity);
    }
    std::uint32_t unrolled_bit(std::uint32_t bit,std::uint32_t k) {                                      // (DEC ECX / INC ECX)
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=bit;
        if(k==1u)gp[ecx]=fl.increment(bit,4,true);
        else if(k==3u)gp[ecx]=fl.increment(bit,4,false);
        return gp[ecx];
    }
    bool marked_already(std::uint32_t mask,std::uint32_t bit) {                                         // MOV EBX/EDI,1; (MOV ECX,EBP); SHL reg,CL; TEST EDX,reg
        auto &fl=this->flags;
        this->gp[ecx]=bit;
        bit_mask=fl.shift(1u,bit&0xffu,4);
        return fl.logic(mask&bit_mask,4)!=0u;
    }
    std::uint32_t mark(std::uint32_t mask) {                                                             // OR EDX,EBX / EDI
        this->reached(resting?Branch::rest_appended:Branch::appended);
        return this->gp[edx]=this->flags.logic(mask|bit_mask,4);
    }
    std::uint32_t counted(std::uint32_t count){return this->flags.increment(count,4,false);}                 // INC dword
    std::uint32_t pass_limit(std::uint32_t count){return this->gp[ecx]=this->flags.plus(count,0u-(logic::unrolled-1u),4);}   // ADD ECX,-3
    std::uint32_t next_pass(std::uint32_t pass){return this->flags.plus(pass,logic::unrolled,4);}              // ADD EBP,4
    std::uint32_t next_pass_rest(std::uint32_t pass){resting=true;return this->flags.increment(pass,4,false);} // INC EBP
    void finish() {                                                                                      // POP EDI; POP ESI; POP EBP; POP EBX; ADD ESP,8; RET 8
        auto &gp=this->gp;
        gp[edi]=this->pop(0x4ac0au);gp[esi]=this->pop(0x4ac0bu);gp[ebp]=this->pop(0x4ac0cu);gp[ebx]=this->pop(0x4ac0du);
        gp[esp]=this->flags.plus(gp[esp],8u,4);
        this->ret(0x4ac11u,8u);this->publish(0x4ac11u);
    }
    void run() {
        Memory<Adapter> memory{*this};
        logic::shadow_record_merge::merge(memory,*this,this->gp[eax]);
        finish();
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x4a7b0u,0x4ac11u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_shadow_record_merge_detail
