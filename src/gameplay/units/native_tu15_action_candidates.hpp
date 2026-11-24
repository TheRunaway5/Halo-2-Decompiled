#pragma once
// Boundary adapter for the action candidates, 1AE7F0
// (native_tu15_action_candidates_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: seven arguments on the
// stack, EAX the count (RET 1C); 0xF0 bytes of frame with EBX, EBP, ESI and
// EDI saved below them, read from the ESP each child left; 1AEA30 (EAX, ECX,
// EDX and ESI; five pushed, RET 14) and 1AECE0 (EDI the scorer; one pushed,
// RET 4) direct, BAD60 (EBX the lookup record, CL 0; three pushed, RET C)
// through the unit's dispatch, every register each returns coming back.
// XMM0 holds what the original's scalar moves and product leave in it. Each
// access publishes its original instruction.
// HALO2_REFERENCE_ACTION_CANDIDATES=1 restores the translated body.
// HALO2_VERIFY_ACTION_CANDIDATES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_action_candidates_systems.hpp"

namespace halo2::native_action_candidates_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_action_candidates;
using Access=logic::action_candidates::Access;
using Kept=logic::action_candidates::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ACTION_CANDIDATES");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ACTION_CANDIDATES");return v;}
enum Branch : unsigned {scaled, unit_kept, entries, found_entry, entry_kept, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Action candidates"};return v;}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x1ae7f6u,0x1ae7fcu,0x1ae7ffu,0x1ae809u,0x1ae81au,0x1ae81eu,0x1ae827u,0x1ae830u,0x1ae834u,0x1ae83au,
    0x1ae846u,0x1ae851u,0x1ae85bu,0x1ae85fu,0x1ae86du,0x1ae872u,0x1ae878u,0x1ae87du,0x1ae883u,0x1ae88fu,0x1ae896u,0x1ae89du,0x1ae8adu,
    0x1ae8c9u,0x1ae8cdu,0x1ae8dfu,0x1ae8e6u,0x1ae8eau,0x1ae8f6u,0x1ae904u,0x1ae908u,0x1ae90fu,0x1ae913u,0x1ae920u,0x1ae924u,0x1ae932u,
    0x1ae936u,0x1ae939u,0x1ae93du,0x1ae95cu,0x1ae985u,0x1ae98cu,0x1ae995u,0x1ae99eu,0x1ae9b4u,0x1ae9b9u,0x1ae9bfu,0x1ae9c7u,0x1ae9d5u,
    0x1ae9d5u,0x1ae9dfu,0x1ae9e3u,0x1ae9eau,0x1ae9eeu,0x1ae9f2u,0x1ae9f8u,0x1aea02u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::total_load)+1u);
inline constexpr std::uint32_t entry_passes[]{0x1ae993u,0x1ae994u,0x1ae9a5u,0x1ae9a6u};
template<class State,class Fill,class Score,class Dispatch> struct Adapter : Port<State> {
    Fill &fill;Score &score;Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below the saved registers (negative before them)
    unsigned passes{};
    std::uint32_t exit{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Fill &f,Score &sc,Dispatch &d):Port<State>(s,c,t,{}),fill(f),score(sc),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    std::uint32_t top()const{return this->gp[esp];}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case headers_load:case tag_load:case definition_again:case context_load:case instances_again:case candidates_again:case count_loop:
        case second_loop:case context_loop:case count_next:case scales_load:gp[ecx]=v;break;
        case datums_load:case definition_load:case flags_load:case second_load:case scale_load:case entry_count:case offset_load:
        case first_loop:case index_loop:case offset_next:case definition_next:gp[edx]=v;break;
        case one_load:case scale_override:case weight_load:x[0]=lane0(v);break;                          // MOVSS XMM0
        case unit_argument:gp[ebp]=v;break;
        case object_load:case tag_again:case game_load:case candidates_load:case first_load:case definition_reload:case actions_load:
        case actions_definition:case definition_loop:case kind_load:case index_next:case total_load:gp[eax]=v;break;
        case instances_load:gp[esi]=v;break;
        case type_load:set_low_byte(gp[ecx],v);break;
        case count_load:gp[ebx]=v;break;
        case scorer_load:case entries_load:case scorer_loop:gp[edi]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing(){this->gp[esp]=this->flags.minus(this->gp[esp],logic::local::size,4);depth=0u-16u;}   // SUB ESP,F0
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==0u){this->push(0x1ae807u,gp[ebx]);this->push(0x1ae808u,gp[ebp]);depth=0u-8u;}               // PUSH EBX; PUSH EBP
        else if(n==1u){this->push(0x1ae826u,gp[esi]);depth=0u-4u;}                                         // PUSH ESI
        else{this->push(0x1ae845u,gp[edi]);depth=0u;}                                                       // PUSH EDI
    }
    std::uint32_t indexed(std::uint32_t handle) {                                                       // MOV EAX,EBP; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]
        const auto index=this->flags.logic(handle&logic::handle_mask,4);
        this->gp[eax]=3u*index;
        return index;
    }
    std::uint32_t masked(std::uint32_t tag){return this->gp[ecx]=this->flags.logic(tag&logic::handle_mask,4);}   // AND ECX,FFFF
    std::uint32_t shifted(std::uint32_t index){static_assert((1u<<4)==logic::tag_size);return this->gp[ecx]=this->flags.shift(index,4u,4);}   // SHL ECX,4
    std::uint32_t type_bit(std::uint32_t type){return this->gp[edx]=this->flags.shift(1u,type&0xffu,4);}   // MOV EDX,1; SHL EDX,CL
    bool typed_tested(std::uint32_t bit){this->flags.logic(bit&logic::typed_mask&0xffu,1);return bit&logic::typed_mask;}   // TEST DL,3
    std::uint32_t tag_offset(std::uint32_t tag) {                                                       // AND EAX,FFFF; SHL EAX,4
        auto &fl=this->flags;
        return this->gp[eax]=fl.shift(fl.logic(tag&logic::handle_mask,4),4u,4);
    }
    bool scaled_tested(std::uint32_t flags_value) {                                                      // SHR EDX,13; TEST DL,1
        auto &gp=this->gp;
        gp[edx]=this->flags.right(flags_value,logic::definition::scaled_bit,4,false);
        this->flags.logic(gp[edx]&1u,1);
        if(gp[edx]&1u)this->reached(Branch::scaled);
        return gp[edx]&1u;
    }
    bool room(std::uint32_t count){this->compare32(count,logic::most);return std::int32_t(count)<std::int32_t(logic::most);}   // CMP EBX,40
    std::uint32_t candidate(std::uint32_t count,std::uint32_t candidates,std::uint32_t second) {        // LEA ESI,[EBX+EBX*2]; PUSH EDX; SHL ESI,5; ADD ESI,EAX
        auto &gp=this->gp;auto &fl=this->flags;
        put(0x1ae8a7u,second);
        static_assert((3u<<5)==logic::candidate_size);
        return gp[esi]=fl.plus(fl.shift(3u*count,5u,4),candidates,4);
    }
    void unit_arguments(std::uint32_t first,std::uint32_t context) {                                    // PUSH EAX; PUSH 0; PUSH -1; PUSH ECX; XOR EAX,EAX; XOR ECX,ECX; MOV EDX,EBP
        auto &gp=this->gp;
        put(0x1ae8b4u,first);put(0x1ae8b5u,0u);put(0x1ae8b7u,logic::none);put(0x1ae8b9u,context);
        gp[eax]=this->flags.logic(0u,4);gp[ecx]=this->flags.logic(0u,4);gp[edx]=gp[ebp];
    }
    bool filled_unit(Kept &kept) {                                                                       // CALL 1AEA30; TEST AL,AL
        auto &gp=this->gp;
        this->call(0x1ae8c0u,0x1ae8c5u,0x1aea30u,fill);depth-=0x14u;
        refresh(kept);
        this->test8(gp[eax]);
        return gp[eax]&0xffu;
    }
    bool kept_scored(std::uint32_t scale,Kept &kept) {                                                   // PUSH EDX; CALL 1AECE0; TEST AL,AL
        auto &gp=this->gp;
        put(0x1ae8d4u,scale);
        this->call(0x1ae8d5u,0x1ae8dau,0x1aece0u,score);depth-=4u;
        refresh(kept);
        this->test8(gp[eax]);
        if(gp[eax]&0xffu)this->reached(Branch::unit_kept);
        return gp[eax]&0xffu;
    }
    std::uint32_t counted(std::uint32_t count){return this->gp[ebx]=this->flags.increment(count,4,false);}   // INC EBX
    void compared(std::uint32_t a,std::uint32_t b){this->compare32(a,b);}
    void entries_compared(std::uint32_t count) {                                                         // XOR ECX,ECX; CMP EDX,ECX
        this->gp[ecx]=this->flags.logic(0u,4);this->compare32(count,0u);
        if(std::int32_t(count)>0)this->reached(Branch::entries);
    }
    void candidate_index(std::uint32_t count){this->gp[esi]=3u*count;}                                  // LEA ESI,[EBX+EBX*2]
    std::uint32_t candidate_address(std::uint32_t candidates) {                                          // SHL ESI,5; ADD ESI,ECX
        auto &gp=this->gp;auto &fl=this->flags;
        return gp[esi]=fl.plus(fl.shift(gp[esi],5u,4),candidates,4);
    }
    std::uint32_t entry(std::uint32_t entries,std::uint32_t offset){return this->gp[edi]=this->flags.plus(entries,offset,4);}   // ADD EDI,EDX
    std::uint32_t looked_up(std::uint32_t kind,Kept &kept) {                                             // PUSH 2, EAX, EBP; XOR CL,CL; LEA EBX,[ESP+2C]; CALL BAD60; MOVSX EAX,AX; TEST EAX,EAX
        auto &gp=this->gp;
        put(0x1ae942u,logic::lookup_kind);put(0x1ae944u,kind);put(0x1ae945u,kept.unit);
        set_low_byte(gp[ecx],this->flags.logic(0u,1));gp[ebx]=local(logic::local::lookup);
        auto child=[&](State &v){dispatch(v,0xbad60u,0x1ae94cu);};
        this->call(0x1ae94cu,0x1ae951u,0xbad60u,child);depth-=12u;
        refresh(kept);
        gp[eax]=sx16(gp[eax]);this->test32(gp[eax]);
        return gp[eax];
    }
    std::uint32_t found(std::uint32_t result) {                                                          // XOR EDX,EDX; CMP EAX,2; SETNZ DL; LEA EAX,[ESP+90]; DEC EDX; AND EAX,EDX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.logic(0u,4);this->compare32(result,logic::found_kind);
        set_low_byte(gp[edx],fl.zf?0u:1u);
        gp[eax]=local(logic::local::found);gp[edx]=fl.increment(gp[edx],4,true);
        return gp[eax]=fl.logic(gp[eax]&gp[edx],4);
    }
    bool room_compared(std::uint32_t count) {                                                            // CMP ECX,40
        this->compare32(count,logic::most);
        if(std::int32_t(count)<std::int32_t(logic::most))this->reached(Branch::found_entry);
        return std::int32_t(count)<std::int32_t(logic::most);
    }
    void found_part(std::uint32_t found_address) {                                                       // TEST EAX,EAX; ADD EAX,38 / XOR EAX,EAX
        auto &gp=this->gp;
        this->test32(found_address);
        gp[eax]=found_address?this->flags.plus(found_address,logic::local::found_part,4):this->flags.logic(0u,4);
    }
    void pass(std::uint32_t v){put(entry_passes[passes++],v);}                                          // PUSH ECX/EDX (as loaded)
    void work_passed(){this->gp[ecx]=local(logic::local::work);put(0x1ae99du,this->gp[ecx]);}           // LEA ECX,[ESP+60]; PUSH ECX
    bool filled_entry(Kept &kept) {                                                                      // MOV ECX,EDI; MOV EDX,EBP; CALL 1AEA30; TEST AL,AL
        auto &gp=this->gp;
        passes=0u;
        gp[ecx]=kept.held;gp[edx]=kept.unit;
        this->call(0x1ae9abu,0x1ae9b0u,0x1aea30u,fill);depth-=0x14u;
        refresh(kept);
        this->test8(gp[eax]);
        return gp[eax]&0xffu;
    }
    void weight_room(){put(0x1ae9c6u,this->gp[ecx]);}                                                   // PUSH ECX
    std::uint32_t weighted(std::uint32_t product){this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0(product));return product;}   // MULSS XMM0,[ESP+1C]
    bool entry_scored(Kept &kept) {                                                                      // CALL 1AECE0; TEST AL,AL
        auto &gp=this->gp;
        this->call(0x1ae9ccu,0x1ae9d1u,0x1aece0u,score);depth-=4u;
        refresh(kept);
        this->test8(gp[eax]);
        if(gp[eax]&0xffu)this->reached(Branch::entry_kept);
        return gp[eax]&0xffu;
    }
    std::uint32_t incremented(std::uint32_t v){return this->flags.increment(v,4,false);}                // INC [ESP+118]
    std::uint32_t advanced(std::uint32_t candidate_address){return this->gp[esi]=this->flags.plus(candidate_address,logic::candidate_size,4);}   // ADD ESI,60
    std::uint32_t offset_advanced(std::uint32_t offset){return this->gp[edx]=this->flags.plus(offset,logic::entry::size,4);}   // ADD EDX,1C
    std::uint32_t index_advanced(std::uint32_t index,std::uint32_t count) {                              // INC EAX; CMP EAX,ECX
        auto &gp=this->gp;
        gp[eax]=this->flags.increment(index,4,false);this->compare32(gp[eax],count);
        return gp[eax];
    }
    void restoring(bool counted_in_memory) {
        auto &gp=this->gp;
        if(counted_in_memory) {                                                                          // POP EDI; POP ESI; POP EBP; POP EBX
            gp[edi]=this->pop(0x1aea09u);gp[esi]=this->pop(0x1aea0au);gp[ebp]=this->pop(0x1aea0bu);gp[ebx]=this->pop(0x1aea0cu);
            exit=0x1aea13u;
        } else {                                                                                         // POP EDI; POP ESI; POP EBP; MOV EAX,EBX; POP EBX
            gp[edi]=this->pop(0x1aea16u);gp[esi]=this->pop(0x1aea17u);gp[ebp]=this->pop(0x1aea18u);gp[eax]=gp[ebx];
            gp[ebx]=this->pop(0x1aea1bu);exit=0x1aea22u;
        }
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);                                         // ADD ESP,F0
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::action_candidates::gather(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]});
        this->ret(exit,0x1cu);this->publish(exit);
    }
};
template<class State,class Reference,class Fill,class Score,class Dispatch> bool bridge(State &s,Reference reference,Fill fill,Score score,Dispatch dispatch) {
    return run_owner(s,0x1ae7f0u,0x1aea22u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Fill,Score,Dispatch>(v,c,t,fill,score,dispatch);});
}
} // namespace halo2::native_action_candidates_detail
