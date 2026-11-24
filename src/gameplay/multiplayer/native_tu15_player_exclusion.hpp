#pragma once
// Boundary adapter for the player exclusion, 1617A0
// (native_tu15_player_exclusion_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: no arguments (RET); a
// 0x10-byte frame (the iterator) below the saved ESI, and EBX, EBP and EDI
// while the players are walked; 1A8E50 (ESI the iterator), 1601B0 and 160360
// (EAX the handle) direct, every register each returns coming back. Each
// access publishes its original instruction.
// HALO2_REFERENCE_PLAYER_EXCLUSION=1 restores the translated body.
// HALO2_VERIFY_PLAYER_EXCLUSION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_player_exclusion_systems.hpp"

namespace halo2::native_player_exclusion_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_player_exclusion;
using Access=logic::player_exclusion::Access;
using Kept=logic::player_exclusion::Kept;
using Outcome=logic::player_exclusion::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_PLAYER_EXCLUSION");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_PLAYER_EXCLUSION");return v;}
enum Branch : unsigned {skipped, no_players, walked, over_cap, waiting, set, cleared, slotted, marking, marked, branch_count};
static_assert(unsigned(Outcome::walked)==Branch::walked);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Player exclusion"};return v;}
inline constexpr std::uint32_t sites[]{0x1617a0u,0x1617a5u,0x1617b4u,0x1617c2u,0x1617c6u,0x1617cau,0x1617ddu,0x1617e4u,0x161802u,
    0x161808u,0x161828u,0x16182cu,0x161832u,0x161840u,0x161849u,0x16184fu,0x161866u,0x161869u,0x16187au,0x161888u,0x161888u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::record_flags_store)+1u);
template<class State,class Next,class OverCap,class Waiting> struct Adapter : Port<State> {
    Next &next_child;
    OverCap &over_cap_child;
    Waiting &waiting_child;
    std::uint32_t depth{};      // bytes below the return address
    bool walking{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Next &n,OverCap &o,Waiting &w)
        :Port<State>(s,c,t,{}),next_child(n),over_cap_child(o),waiting_child(w){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx]&0xffu,this->gp[ebp],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t address,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case game_address:case variant_index:case network_mode:case network_state:gp[eax]=v;break;
        case game_kind:set_low_byte(gp[ecx],v);break;
        case players_array:case variant_entry:case slot_load:gp[ecx]=v;break;
        case session_address:gp[ebp]=v;break;
        case handle_load:gp[edi]=v;break;
        case record_load:gp[esi]=v;gp[eax]=fl.logic(0u,4);break;                                       // MOV ESI,[ESP+10]; XOR EAX,EAX
        case flags_load:gp[eax]=v&0xffffu;break;
        case network_address:case slot_table:gp[edx]=v;break;
        case slot_flags:fl.logic(v&logic::slot_replicated,1);break;                                    // TEST byte [EDX+ECX*8+44],4
        case record_flags_load:gp[ecx]=9u*gp[ecx];gp[eax]=address-logic::record_flags;break;           // LEA ECX,[ECX+ECX*8]; LEA EAX,[EAX+ECX*4+14]
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t){if(a==Access::record_flags_store)this->reached(Branch::marked);}
    void kind_tested(std::uint32_t kind) {                                                             // SUB ESP,10; CMP CL,4
        auto &fl=this->flags;
        this->gp[esp]=fl.minus(this->gp[esp],0x10u,4);depth=0x10u;fl.minus(kind&0xffu,logic::game::skipped_kind,1);
    }
    void iterating() {                                                                                 // OR EAX,-1; PUSH ESI; LEA ESI,[ESP+4]
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(0xffffffffu,4);put(0x1617bdu,gp[esi]);gp[esi]=local(logic::local::record);
    }
    std::uint32_t next(Kept &kept) {
        auto &gp=this->gp;
        if(walking)gp[esi]=local(logic::local::record);                                                // LEA ESI,[ESP+10]
        const std::uint32_t pc=walking?0x161893u:0x1617ceu;
        this->call(pc,pc+5u,0x1a8e50u,next_child);
        refresh(kept);
        this->test8(gp[eax]&0xffu);                                                                    // TEST AL,AL
        return gp[eax];
    }
    void saving(){put(0x1617dbu,this->gp[ebx]);put(0x1617dcu,this->gp[ebp]);walking=true;}
    void saved(){put(0x1617e3u,this->gp[edi]);}
    void restoring(){auto &gp=this->gp;gp[edi]=take(0x1618a0u);gp[ebp]=take(0x1618a1u);gp[ebx]=take(0x1618a2u);}
    void testing(const Kept &kept){this->gp[eax]=kept.handle;set_low_byte(this->gp[ebx],this->flags.logic(kept.excluded,1));}   // MOV EAX,EDI; XOR BL,BL
    std::uint32_t over_cap(Kept &kept) {
        this->call(0x1617ecu,0x1617f1u,0x1601b0u,over_cap_child);
        refresh(kept);
        this->test8(this->gp[eax]&0xffu);
        if(this->gp[eax]&0xffu)this->reached(Branch::over_cap);
        return this->gp[eax];
    }
    std::uint32_t waiting(Kept &kept) {
        this->gp[eax]=kept.handle;                                                                     // MOV EAX,EDI
        this->call(0x1617f7u,0x1617fcu,0x160360u,waiting_child);
        refresh(kept);
        this->test8(this->gp[eax]&0xffu);
        if(this->gp[eax]&0xffu)this->reached(Branch::waiting);
        return this->gp[eax];
    }
    void excluded(const Kept &kept){set_low_byte(this->gp[ebx],kept.excluded);}                        // MOV BL,1
    void compared(std::uint32_t flags,std::uint32_t was,const Kept &kept) {                            // MOV EDX,EAX; SHR EDX,E; AND DL,1; CMP BL,DL
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.right(flags,logic::player::waiting_shift,4,false);set_low_byte(gp[edx],fl.logic(was,1));
        fl.minus(kept.excluded&0xffu,was,1);
    }
    void flagged(const Kept &kept,std::uint32_t now) {                                                 // TEST BL,BL; OR EAX,4000 or AND EAX,BFFF
        this->test8(kept.excluded&0xffu);
        this->gp[eax]=this->flags.logic(now,4);
        this->reached((kept.excluded&0xffu)?Branch::set:Branch::cleared);
    }
    void variant_tested(std::uint32_t record){this->test32(record);}
    void slot_tested(std::uint32_t slot){this->compare32(slot,logic::none);if(slot!=logic::none)this->reached(Branch::slotted);}
    void mode_tested(std::uint32_t mode) {                                                             // CMP EAX,4 / 5; CMP EAX,3; CMP EAX,5
        this->compare32(mode,4u);
        if(mode!=4u){this->compare32(mode,5u);if(mode!=5u)return;}
        this->compare32(mode,3u);this->compare32(mode,5u);
        if(mode==logic::network::marking_mode)this->reached(Branch::marking);
    }
    void slot_indexed(std::uint32_t state_value,std::uint32_t slot) {                                  // ADD EAX,2098; AND ECX,3FF
        auto &fl=this->flags;
        this->gp[eax]=fl.plus(state_value,logic::state::records,4);this->gp[ecx]=fl.logic(slot&logic::slot_mask,4);
    }
    void marked(std::uint32_t value){this->flags.logic(value,4);}                                      // OR dword [EAX+C],400
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::player_exclusion::update(memory,*this);
        this->reached(unsigned(outcome));
        if(outcome!=Outcome::skipped)gp[esi]=take(0x1618a3u);
        gp[esp]=fl.plus(gp[esp],0x10u,4);                                                              // ADD ESP,10
        this->ret(0x1618a7u,0u);this->publish(0x1618a7u);
    }
};
template<class State,class Reference,class Next,class OverCap,class Waiting>
bool bridge(State &s,Reference reference,Next next,OverCap over_cap,Waiting waiting) {
    return run_owner(s,0x1617a0u,0x1618a7u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Next,OverCap,Waiting>(v,c,t,next,over_cap,waiting);});
}
} // namespace halo2::native_player_exclusion_detail
