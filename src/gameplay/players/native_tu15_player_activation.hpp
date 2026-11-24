#pragma once
// Boundary adapter for the player activation, 14FDC0
// (native_tu15_player_activation_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: no arguments (RET); EBX,
// EBP, ESI and EDI saved past the game kind; 69AD0 (EAX the machine) and
// B7E50 (the slot and 0x200 pushed, RET 8) direct, the variant's method
// through the dispatch (ECX the record, the handle pushed), every register
// each returns coming back. Each access publishes its original instruction.
// HALO2_REFERENCE_PLAYER_ACTIVATION=1 restores the translated body.
// HALO2_VERIFY_PLAYER_ACTIVATION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/players/native_tu15_player_activation_systems.hpp"

namespace halo2::native_player_activation_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_player_activation;
using Access=logic::player_activation::Access;
using Kept=logic::player_activation::Kept;
using Outcome=logic::player_activation::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_PLAYER_ACTIVATION");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_PLAYER_ACTIVATION");return v;}
enum Branch : unsigned {skipped, walked, passed_over, activated, deactivated, joined, published, branch_count};
static_assert(unsigned(Outcome::walked)==Branch::walked);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Player activation"};return v;}
inline constexpr std::uint32_t sites[]{0x14fdc0u,0x14fdc5u,0x14fdccu,0x14fddfu,0x14fde6u,0x14fe01u,0x14fe15u,0x14fe18u,0x14fe20u,
    0x14fe2au,0x14fe3au,0x14fe4bu,0x14fe53u,0x14feb3u,0x14fe57u,0x14fe5du,0x14fe68u,0x14fe6bu,0x14fe6eu,0x14fe73u,0x14fe79u,
    0x14fe88u,0x14fe90u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::slot_load)+1u);
template<class State,class Active,class Route,class Publish> struct Adapter : Port<State> {
    Active &active_child;
    Route &route;
    Publish &publish_child;
    bool saved{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Active &a,Route &r,Publish &p)
        :Port<State>(s,c,t,{}),active_child(a),route(r),publish_child(p){}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case game_address:case session_address:case session_again:case variant_entry_again:case session_slots:case slot_load:gp[eax]=v;break;
        case players_array:gp[ebx]=v;break;
        case first_unused:case datum_size:gp[esi]=v;break;
        case bitmap:case variant_vtable:gp[edx]=v;break;
        case datum_data:gp[edi]=v;gp[esi]=fl.plus(fl.product(gp[esi],gp[eax]),v,4);break;              // IMUL ESI,EAX; ADD ESI,EDI
        case flags_byte:fl.logic(v&logic::player::passed_over,1);break;                                // TEST byte [ESI+2],2
        case flags_word:gp[ecx]=fl.logic(0u,4)|(v&0xffffu);break;                                      // XOR ECX,ECX; MOV CX,[ESI+2]
        case variant_index:case variant_entry:case variant_index_again:gp[ecx]=v;break;
        default:break;
        }
        if(a==session_slots)gp[edx]=logic::widen16(gp[edi]&0xffffu);                                  // MOVSX EDX,DI
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        if(a==Access::set_store)this->reached(Branch::activated);
        if(a==Access::clear_store)this->reached(Branch::deactivated);
    }
    void kind_tested(std::uint32_t kind){this->flags.minus(kind&0xffu,logic::game::skipped_kind,1);}   // CMP byte [EAX+C],4
    void saving(){this->push(0x14fdcbu,this->gp[ebx]);saved=true;}                                      // PUSH EBX
    void walking(const Kept &kept) {                                                                   // PUSH EBP; PUSH ESI; OR EBP,-1; PUSH EDI
        auto &gp=this->gp;
        this->push(0x14fdd2u,gp[ebp]);this->push(0x14fdd3u,gp[esi]);gp[ebp]=this->flags.logic(kept.index,4);this->push(0x14fdd7u,gp[edi]);
    }
    void restoring() {
        auto &gp=this->gp;
        gp[edi]=this->pop(0x14fe0bu);gp[esi]=this->pop(0x14fe0cu);gp[ebp]=this->pop(0x14fe0du);gp[ebx]=this->pop(0x14fe0eu);
    }
    void starting(std::uint32_t index){this->gp[eax]=index;this->test32(index);}                       // LEA EAX,[EBP+1]; TEST EAX,EAX
    void limit_compared(std::uint32_t index,std::uint32_t limit){this->compare32(index,limit);}        // CMP EAX,ESI
    void bit_tested(std::uint32_t index,std::uint32_t word) {                                          // MOV ECX,EAX; AND ECX,1F; MOV EDI,1; SHL EDI,CL; MOV ECX,EAX; SAR ECX,5; TEST
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.logic(index&31u,4);gp[edi]=fl.shift(1u,gp[ecx]&0xffu,4);gp[ecx]=fl.right(index,5u,4,true);fl.logic(word&gp[edi],4);
    }
    void advanced(std::uint32_t index,std::uint32_t limit) {                                           // INC EAX; CMP EAX,ESI
        this->gp[eax]=this->flags.increment(index-1u,4,false);this->compare32(index,limit);
    }
    void found_tested(std::uint32_t index){this->compare32(index,logic::none);}                        // CMP EAX,-1
    void player_found(const Kept &kept,std::uint32_t salt_value) {                                     // MOVSX EDI,word [ESI]; SHL EDI,10; OR EDI,EAX; MOV EBP,EAX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esi]=kept.player;fl.shift(salt_value,16u,4);gp[edi]=fl.logic(kept.handle,4);gp[ebp]=kept.index;
    }
    void skip_tested(std::uint32_t byte){if(byte&logic::player::passed_over)this->reached(Branch::passed_over);}
    std::uint32_t active(std::uint32_t machine,Kept &kept) {                                           // LEA EAX,[ESI+14]; CALL 69AD0
        this->gp[eax]=machine;
        this->call(0x14fe33u,0x14fe38u,0x69ad0u,active_child);
        refresh(kept);
        return this->gp[eax];
    }
    void compared(std::uint32_t flags,std::uint32_t answer) {                                          // MOV DL,CL; AND DL,1; CMP DL,AL
        auto &gp=this->gp;auto &fl=this->flags;
        set_low_byte(gp[edx],fl.logic(flags&logic::player::active,1));fl.minus(flags&logic::player::active,answer,1);
    }
    void activity_tested(std::uint32_t answer){this->test8(answer);}                                   // TEST AL,AL
    void flagged(std::uint32_t value){this->gp[ecx]=this->flags.logic(value,4);}                       // OR ECX,1 or AND ECX,FFFE
    void variant_tested(std::uint32_t record){this->test32(record);}
    void handing(const Kept &kept){this->push(0x14fe6au,kept.handle);}                                 // PUSH EDI
    void joined(std::uint32_t,std::uint32_t method,Kept &kept) {                                       // CALL [EDX+18]
        this->call_routed(route,0x14fe6bu,0x14fe6eu,method);
        refresh(kept);
        this->reached(Branch::joined);
    }
    void slot_tested(std::uint32_t slot){this->compare32(slot,logic::none);}
    void dirty(std::uint32_t slot,Kept &kept) {                                                        // PUSH 200; PUSH EAX; CALL B7E50
        this->push(0x14fe9du,logic::dirty_mask);this->push(0x14fea2u,slot);
        this->call(0x14fea3u,0x14fea8u,0xb7e50u,publish_child);
        refresh(kept);
        this->reached(Branch::published);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::player_activation::update(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]})));
        this->ret(0x14fe0fu,0u);this->publish(0x14fe0fu);
    }
};
template<class State,class Reference,class Active,class Route,class Publish>
bool bridge(State &s,Reference reference,Active active,Route route,Publish publish) {
    return run_owner(s,0x14fdc0u,0x14fe0fu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Active,Route,Publish>(v,c,t,active,route,publish);});
}
} // namespace halo2::native_player_activation_detail
