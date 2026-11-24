#pragma once
// Boundary adapter for the permutation choice, 1700E0
// (native_tu15_permutation_choice_systems.hpp). Its callers and the
// children keep the original ABI, which the adapter owns: five arguments on
// the stack (RET 14); 0x3AC bytes of frame with EBX, EBP, ESI and EDI saved
// below them, read from the ESP each child left. 170930 (EDI the list, EDX
// the count; six pushed, RET 18) and 170B40 (EBX the list, EDX the count)
// are direct; every register each returns comes back, XMM4 (the weight)
// included. The unrolled listing keeps the original's six candidate field
// pointers in EAX, ECX, ESI, EDI, EDX and EBX. The routine leaves through
// the original's epilogue. Each access publishes its original instruction.
// HALO2_REFERENCE_PERMUTATION_CHOICE=1 restores the translated body.
// HALO2_VERIFY_PERMUTATION_CHOICE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_permutation_choice_systems.hpp"

namespace halo2::native_permutation_choice_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_permutation_choice;
using Access=logic::permutation_choice::Access;
using Kept=logic::permutation_choice::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_PERMUTATION_CHOICE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_PERMUTATION_CHOICE");return v;}
enum Branch : unsigned {regionless, cleared, gathered, ungathered, unrolled, rest, flagged, first_listed, held, chosen, unchosen,
    branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Permutation choice"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t low(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
inline constexpr std::uint32_t sites[]{
    0x1700e6u,0x1700edu,0x1700fdu,0x170109u,0x17010bu,0x17010fu,0x170119u,0x170122u,0x17012cu,0x17012fu,0x170132u,0x170136u,
    0x170139u,0x17013cu,0x170147u,0x17014au,0x170154u,0x17015fu,0x170163u,0x170169u,0x17016du,0x170171u,0x170174u,0x170182u,
    0x170186u,0x17018cu,0x170192u,0x1701a5u,0x1701a9u,0x1701abu,0x1701b0u,0x1701b4u,0x1701c6u,0x1701ddu,0x1701e9u,0x1701f1u,
    0x1701fbu,0x170204u,0x17020cu,0x17022bu,0x170231u,0x170235u,0x17023bu,0x17023fu,0x170241u,0x170245u,0x17024bu,0x170251u,
    0x170257u,0x17025du,0x170263u,0x170268u,0x170268u,0x17026cu,0x170285u,0x170289u,0x17028du,0x170293u,0x170298u,0x17029au,
    0x17029eu,0x1702a4u,0x1702aau,0x1702b0u,0x1702b6u,0x1702bcu,0x1702c1u,0x1702c1u,0x1702c5u,0x1702deu,0x1702e2u,0x1702e6u,
    0x1702ecu,0x1702f3u,0x1702f5u,0x1702f9u,0x1702ffu,0x170305u,0x17030bu,0x170311u,0x170317u,0x17031cu,0x17031cu,0x170320u,
    0x170339u,0x17033du,0x170341u,0x170347u,0x17034eu,0x170350u,0x170354u,0x17035au,0x170360u,0x170366u,0x17036cu,0x170372u,
    0x170377u,0x170377u,0x17037bu,0x170394u,0x170398u,0x170398u,0x17039du,0x17039du,0x1703a2u,0x1703a6u,0x1703b0u,0x1703b4u,
    0x1703b8u,0x1703beu,0x1703ceu,0x1703d5u,0x1703dau,0x1703ddu,0x1703e3u,0x1703e6u,0x1703e9u,0x1703f0u,0x1703f3u,0x17040fu,
    0x170413u,0x170417u,0x17041bu,0x17041fu,0x170423u,0x170427u,0x170432u,0x17043du,0x170456u,0x17045du,0x170468u,0x17046au,
    0x17046du,0x170470u,0x170474u,0x170477u,0x17047au,0x17047du,0x170480u,0x170484u,0x170488u,0x17048cu,0x170493u,0x170497u,
    0x17049bu,0x1704a4u,0x1704a8u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::region_offset_advance)+1u);
template<class State,class Children> struct Adapter : Port<State> {
    Children &children;      // 170930 and 170B40
    std::uint32_t depth{};   // bytes pushed beyond the four saved registers (negative before them)
    bool any_region{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k):Port<State>(s,c,t,{}),children(k){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void direct(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){children(v,target);};
        this->call(pc,pc+5u,target,child);depth-=popped;
        kept.zero=this->gp[ebx];kept.region=this->gp[ebp];kept.weight=low(this->s.xmm[4]);
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case tag_argument:case region_count:case gathered_count:case count_after:case output_cursor_load:case region_index_next:
            gp[eax]=v;break;
        case chosen_permutation:set_low_byte(gp[eax],v);break;                                             // MOV AL
        case definitions_load:case variant_argument:case region_index_load:case variant_candidates:case permutation_count:
        case mask_region:case region_count_next:gp[ecx]=v;break;
        case variant_permutation:gp[ecx]=sx8(v);break;                                                     // MOVSX ECX,byte
        case chosen_first:set_low_byte(gp[ecx],v);break;                                                   // MOV CL
        case region_count_again:case variants_load:case permutations_rest:case chosen_region:case output_cursor_next:
        case definition_next:gp[edx]=v;break;
        case chosen_marker:case chosen_second:set_low_byte(gp[edx],v);break;                               // MOV DL
        case output_argument:case regions_load:case permutations_load:case permutation_cursor_load:case index_0:case index_1:
        case index_2:case index_3:case weight_cursor_0:case weight_cursor_1:case weight_cursor_2:case weight_cursor_3:
        case weight_cursor_again_0:case weight_cursor_again_1:case weight_cursor_again_2:case weight_cursor_again_3:
        case permutation_cursor_0:case permutation_cursor_1:case permutation_cursor_2:case unrolled_limit_load:case region_reload:
            gp[ebp]=v;break;
        case definition_load:case definition_reload:case tag_test:case entry_load:case entry_definition:case entry_variants:
        case permutation_index_after:case chosen_argument_again:case region_offset_next:gp[esi]=v;break;
        case chosen_argument:case region_offset_load:case permutation_total:gp[edi]=v;break;
        case weight_load:this->s.xmm[4]=lane0(v);break;                                                    // MOVSS XMM4
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The frame and the definition.
    void framing(){auto &gp=this->gp;gp[esp]=this->flags.minus(gp[esp],0x3acu,4);depth=0u-16u;}             // SUB ESP,3AC
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / ESI / EDI
        static constexpr std::uint32_t pcs[]{0x1700f8u,0x1700fcu,0x170108u,0x170118u};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t tag_index(std::uint32_t tag){return this->gp[eax]=this->flags.logic(tag&logic::handle_mask,4);}   // AND EAX,FFFF
    std::uint32_t tag_offset(std::uint32_t index){return this->gp[eax]=this->flags.shift(index,4u,4);}          // SHL EAX,4
    std::uint32_t entry_address(std::uint32_t address){return this->gp[eax]=address;}                         // LEA EAX,[EAX+ECX+8]
    std::uint32_t zeroed(){return this->gp[ebx]=this->flags.logic(0u,4);}                                     // XOR EBX,EBX
    std::uint32_t first_cleared(){return this->gp[ecx]=this->flags.logic(0u,4);}                              // XOR ECX,ECX
    bool above(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)>std::int32_t(b);}   // CMP; JLE
    bool below(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)<std::int32_t(b);}   // CMP; JL / JGE
    bool different(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return a!=b;}                       // CMP EAX,EBX
    bool named(std::uint32_t v){this->compare32(v,logic::none);return v!=logic::none;}                        // CMP reg,-1
    // The cleared records.
    std::uint32_t record_cursor(std::uint32_t records){this->reached(Branch::cleared);return this->gp[eax]=records+2u;}   // LEA EAX,[EBP+2]
    std::uint32_t none_value(){return this->gp[edx]=this->flags.logic(logic::none,4);}                        // OR EDX,-1
    std::uint32_t next_cleared(std::uint32_t index){return this->gp[ecx]=this->flags.increment(index,4,false);}   // INC ECX
    std::uint32_t next_record(std::uint32_t cursor){return this->gp[eax]=this->flags.plus(cursor,logic::record::size,4);}   // ADD EAX,8
    // The regions.
    std::uint32_t first_region(){any_region=true;return this->gp[edi]=this->flags.logic(0u,4);}                // XOR EDI,EDI
    std::uint32_t output_cursor(std::uint32_t records){return this->gp[ebp]=this->flags.increment(records,4,false);}   // INC EBP
    std::uint32_t region_address(std::uint32_t regions,std::uint32_t offset){return this->gp[ebp]=this->flags.plus(regions,offset,4);}   // ADD EBP,EDI
    std::uint32_t count_cleared(){return this->gp[eax]=this->flags.logic(0u,4);}                              // XOR EAX,EAX
    std::uint32_t variant_offset(std::uint32_t variant){return this->gp[ecx]=this->flags.product(variant,logic::variant::size);}   // IMUL ECX,ECX,38
    std::uint32_t variant_address(std::uint32_t variants,std::uint32_t offset){return this->gp[edx]=this->flags.plus(variants,offset,4);}   // ADD EDX,ECX
    bool region_listed(std::uint32_t offset){this->compare32(offset,0u-logic::region::size);return offset!=0u-logic::region::size;}   // CMP EDI,-10
    std::uint32_t variant_again(std::uint32_t variants,std::uint32_t offset){return this->gp[esi]=this->flags.plus(variants,offset,4);}   // ADD ESI,ECX
    void gather_pushed(Kept &kept,std::uint32_t listed) {                                                // PUSH 20; PUSH 1; PUSH EBX; LEA EAX,[ECX+ECX*4]
        put(0x1701beu,logic::listed_capacity);put(0x1701c0u,logic::gather_mode);put(0x1701c2u,kept.zero);
        this->gp[eax]=5u*listed;
    }
    void gather(Kept &kept,std::uint32_t candidates) {                                                   // PUSH EBX; LEA EDX; PUSH -1; PUSH EDX; LEA EDI,[ESP+54]; LEA EDX,[ESP+2C]; CALL 170930
        auto &gp=this->gp;
        put(0x1701c9u,kept.zero);gp[edx]=candidates;put(0x1701cdu,logic::none);put(0x1701cfu,gp[edx]);
        gp[edi]=local(logic::local::list);gp[edx]=local(logic::local::count);
        direct(0x1701d8u,0x170930u,0x18u,kept);
    }
    bool gathered(std::uint32_t count,std::uint32_t zero) {                                              // CMP EAX,EBX; JNE
        this->compare32(count,zero);
        this->reached(count!=zero?Branch::gathered:Branch::ungathered);
        return count!=zero;
    }
    // The permutations.
    std::uint32_t first_permutation(){return this->gp[esi]=this->flags.logic(0u,4);}                          // XOR ESI,ESI
    bool unrolled(std::uint32_t total) {                                                                 // CMP ECX,4; JL
        this->compare32(total,logic::unrolled);
        const bool four=std::int32_t(total)>=std::int32_t(logic::unrolled);
        if(four)this->reached(Branch::unrolled);
        return four;
    }
    std::uint32_t unrolled_limit(std::uint32_t count,std::uint32_t total) {                             // IMUL EAX,EAX,1C; ADD ECX,-3
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.product(count,logic::candidate::size);
        return gp[ecx]=fl.plus(total,0u-(logic::unrolled-1u),4);
    }
    std::uint32_t weight_cursor(){auto &gp=this->gp;return gp[ecx]=local(logic::local::list+logic::candidate::weight)+gp[eax];}   // LEA ECX,[ESP+EAX+54]
    std::uint32_t flags_cursor(std::uint32_t permutations) {                                             // ADD EBP,0C; LEA EBX,EDX,EDI,ESI,ECX,EAX
        auto &gp=this->gp;
        gp[ebp]=this->flags.plus(permutations,logic::permutation::size+logic::permutation::flags,4);
        const auto base=local(logic::local::list)+gp[eax];
        gp[ebx]=base+logic::candidate::spare;gp[edx]=base+logic::candidate::link;gp[edi]=base+logic::candidate::second;
        gp[esi]=base+logic::candidate::first;gp[ecx]=base+logic::candidate::marker;gp[eax]=base+logic::candidate::permutation;
        return gp[ebp];
    }
    bool flagged(std::uint32_t v){const bool set=this->flags.logic(v&logic::permutation::flagged,1)!=0u;if(set)this->reached(Branch::flagged);return set;}   // TEST byte,1
    std::uint32_t listed_index(std::uint32_t index,std::uint32_t k) {                                    // (INC EBP / ADD EBP,k)
        auto &gp=this->gp;
        if(k==1u)gp[ebp]=this->flags.increment(index,4,false);
        else if(k>1u)gp[ebp]=this->flags.plus(index,k,4);
        return gp[ebp];
    }
    std::uint32_t incremented(std::uint32_t v){return this->flags.increment(v,4,false);}                       // INC dword
    std::uint32_t added(std::uint32_t v,std::uint32_t n){return this->flags.plus(v,n,4);}                      // ADD dword,n
    std::uint32_t listed(std::uint32_t) {                                                                // ADD EAX,ECX,ESI,EDI,EDX,EBX,EBP,1C
        auto &gp=this->gp;auto &fl=this->flags;
        for(const auto r:{eax,ecx,esi,edi,edx,ebx,ebp})gp[r]=fl.plus(gp[r],logic::candidate::size,4);
        return gp[ebp];
    }
    std::uint32_t rest_cursor(std::uint32_t count) {                                                     // MOV EDX,EAX; IMUL EDX,EDX,1C; LEA ECX,[ESP+EDX+40]
        auto &gp=this->gp;
        this->reached(Branch::rest);
        gp[edx]=this->flags.product(count,logic::candidate::size);
        gp[ecx]=local(logic::local::list+logic::candidate::marker)+gp[edx];
        return gp[ecx]-logic::candidate::marker;
    }
    std::uint32_t rest_flags(std::uint32_t address){return this->gp[edx]=address;}                            // LEA EDX,[EDX+ESI*8+4]
    std::uint32_t rest_listed(std::uint32_t count) {                                                     // INC EAX; ADD ECX,1C
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.increment(count,4,false);gp[ecx]=fl.plus(gp[ecx],logic::candidate::size,4);
        return gp[eax];
    }
    std::uint32_t next_permutation(std::uint32_t index) {                                                // INC ESI; ADD EDX,8
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esi]=fl.increment(index,4,false);gp[edx]=fl.plus(gp[edx],logic::permutation::size,4);
        return gp[esi];
    }
    std::uint32_t none_count(){this->reached(Branch::first_listed);return this->gp[eax]=this->flags.logic(logic::none,4);}   // OR EAX,-1
    std::uint32_t one_listed(){return this->gp[eax]=1u;}                                                      // MOV EAX,1
    // The choice.
    bool held(std::uint32_t mask,std::uint32_t) {                                                        // MOV EDX,1; SHL EDX,CL; TEST [ESP+3D0],EDX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.shift(1u,gp[ecx]&0xffu,4);
        const bool is=fl.logic(mask&gp[edx],4)!=0u;
        if(is)this->reached(Branch::held);
        return is;
    }
    std::uint32_t chosen(Kept &kept,std::uint32_t count) {                                               // LEA EBX,[ESP+3C]; MOV EDX,EAX; CALL 170B40
        auto &gp=this->gp;
        gp[ebx]=local(logic::local::list);gp[edx]=count;
        direct(0x17044cu,0x170b40u,0u,kept);
        this->reached(gp[eax]==logic::none?Branch::unchosen:Branch::chosen);
        return gp[eax];
    }
    std::uint32_t choice_offset(std::uint32_t choice){return this->gp[eax]=this->flags.product(choice,logic::candidate::size);}   // IMUL EAX,EAX,1C
    std::uint32_t choice_address(std::uint32_t offset){return this->gp[ecx]=local(logic::local::list)+offset;}   // LEA ECX,[ESP+EAX+3C]
    std::uint32_t next_output(std::uint32_t cursor){return this->gp[edx]=this->flags.plus(cursor,logic::record::size,4);}   // ADD EDX,8
    std::uint32_t next_region(std::uint32_t index){return this->gp[eax]=this->flags.increment(index,4,false);}   // INC EAX
    std::uint32_t next_offset(std::uint32_t offset){return this->gp[esi]=this->flags.plus(offset,logic::region::size,4);}   // ADD ESI,10
    void finish() {                                                                                      // POP EDI; POP ESI; POP EBP; POP EBX; ADD ESP,3AC; RET 14
        auto &gp=this->gp;
        if(!any_region)this->reached(Branch::regionless);
        gp[edi]=this->pop(0x1704b2u);gp[esi]=this->pop(0x1704b3u);gp[ebp]=this->pop(0x1704b4u);gp[ebx]=this->pop(0x1704b5u);
        gp[esp]=this->flags.plus(gp[esp],0x3acu,4);
        this->ret(0x1704bcu,0x14u);this->publish(0x1704bcu);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::permutation_choice::choose(memory,*this,Kept{gp[ebx],gp[ebp],low(this->s.xmm[4])});
        finish();
    }
};
template<class State,class Reference,class Children> bool bridge(State &s,Reference reference,Children children) {
    return run_owner(s,0x1700e0u,0x1704bcu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children>(v,c,t,children);});
}
} // namespace halo2::native_permutation_choice_detail
