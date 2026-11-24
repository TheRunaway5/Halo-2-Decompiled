#pragma once
// Boundary adapter for the player weapon sync, 168CA1
// (native_tu15_player_weapon_sync_systems.hpp). Its callers and the
// children keep the original ABI, which the adapter owns: no arguments
// (RET); an EBP frame of 12 bytes (addressed from the live EBP, as the
// original does) with EBX, ESI and EDI saved below it; 150990 (ECX the
// player's index) direct, 16B16A and 16AD45 (two arguments pushed, RET 8)
// and CE230 (EAX the unit, CX the weapon index) through the unit's dispatch,
// and 169C46 (the index pushed, RET 4) through the World's required owner,
// every register each returns coming back. Each access publishes its
// original instruction.
// HALO2_REFERENCE_PLAYER_WEAPON_SYNC=1 restores the translated body.
// HALO2_VERIFY_PLAYER_WEAPON_SYNC=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/players/native_tu15_player_weapon_sync_systems.hpp"

namespace halo2::native_player_weapon_sync_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_player_weapon_sync;
using Access=logic::player_weapon_sync::Access;
using Kept=logic::player_weapon_sync::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_PLAYER_WEAPON_SYNC");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_PLAYER_WEAPON_SYNC");return v;}
enum Branch : unsigned {player, attached, slots, changed, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Player weapon sync"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x168ca7u,0x168ca7u,0x168cabu,0x168cabu,0x168cb7u,0x168cc8u,0x168cceu,0x168cdcu,0x168ce2u,0x168ce5u,
    0x168cecu,0x168cefu,0x168cf4u,0x168cfau,0x168d02u,0x168d0du,0x168d10u,0x168d13u,0x168d19u,0x168d23u,0x168d27u,0x168d35u,0x168d38u,
    0x168d3du,0x168d45u,0x168d45u,0x168d52u,0x168d5au,0x168d5au,0x168d61u,0x168d61u,0x168d64u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::offset_compare)+1u);
template<class State,class Player,class Update,class Dispatch> struct Adapter : Port<State> {
    Player &player_child;Update &update_child;Dispatch &dispatch;
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Player &p,Update &u,Dispatch &d):Port<State>(s,c,t,{}),player_child(p),update_child(u),dispatch(d){}
    std::uint32_t local(std::uint32_t n)const{return this->gp[ebp]-n;}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case index_load:case records_load:case players_load:case slot_load:gp[ecx]=v;break;
        case offset_load:case headers_load:case object_datums:gp[edx]=v;break;
        case state_load:gp[ecx]=sx8(v);break;
        case unit_load:case slot_unit:gp[eax]=v;break;
        case unit_address:gp[ecx]=v;break;
        case weapon_index:set_low_word(gp[ecx],sx8(v));break;                                            // MOVSX CX,byte
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // PUSH EBP; MOV EBP,ESP; SUB ESP,C
        auto &gp=this->gp;
        this->push(0x168ca1u,gp[ebp]);gp[ebp]=gp[esp];gp[esp]=this->flags.minus(gp[esp],logic::local::slot,4);
    }
    std::uint32_t cleared(std::uint32_t v){return this->flags.logic(v&0u,4);}                          // AND [EBP-n],0
    void saving(Kept &kept) {                                                                            // PUSH EBX; PUSH ESI; PUSH EDI; MOV EDI,FFFF
        auto &gp=this->gp;
        this->push(0x168cafu,gp[ebx]);this->push(0x168cb0u,gp[esi]);this->push(0x168cb1u,gp[edi]);
        kept.mask=gp[edi]=0xffffu;
    }
    std::uint32_t player(std::uint32_t,Kept &kept) {                                                    // CALL 150990
        this->call(0x168cbau,0x168cbfu,0x150990u,player_child);
        refresh(kept);
        return this->gp[eax];
    }
    void compared(std::uint32_t a,std::uint32_t b){this->compare32(a,b);}
    std::uint32_t indexed(std::uint32_t handle,std::uint32_t mask) {                                    // AND EAX,EDI; IMUL EAX,EAX,21C
        auto &fl=this->flags;
        this->reached(Branch::player);
        return this->gp[eax]=fl.product(fl.logic(handle&mask,4),logic::player::size);
    }
    std::uint32_t record(std::uint32_t records,std::uint32_t offset){return this->gp[esi]=offset+records;}   // LEA ESI,[EDX+ECX]
    std::uint32_t player_address(std::uint32_t offset,std::uint32_t datums){return this->gp[eax]=this->flags.plus(offset,datums,4);}  // ADD EAX,[ECX+44]
    std::uint32_t signed_state(std::uint32_t state)const{return sx8(state);}
    void state_pushed(std::uint32_t state){this->push(0x168cf9u,state);}                               // PUSH ECX
    void attached(std::uint32_t index,Kept &kept) {                                                      // PUSH [EBP-4]; CALL 16B16A
        this->push(0x168cfau,index);
        auto child=[&](State &v){dispatch(v,0x16b16au,0x168cfdu);};
        this->call(0x168cfdu,0x168d02u,0x16b16au,child);
        refresh(kept);
        this->reached(Branch::attached);
    }
    std::uint32_t first_slot(Kept &kept) {                                                               // LEA EAX,[ESI+10]; XOR EBX,EBX
        auto &gp=this->gp;
        gp[eax]=kept.record+logic::record::slots;kept.slot=gp[ebx]=this->flags.logic(0u,4);
        this->reached(Branch::slots);
        return gp[eax];
    }
    std::uint32_t unit_index(std::uint32_t handle,std::uint32_t mask) {                                 // MOV ECX,EAX; AND ECX,EDI; LEA ECX,[ECX+ECX*2]
        const auto index=this->flags.logic(handle&mask,4);
        this->gp[ecx]=3u*index;
        return index;
    }
    std::uint32_t weapon(std::uint32_t,Kept &kept) {                                                     // CALL CE230
        auto child=[&](State &v){dispatch(v,0xce230u,0x168d30u);};
        this->call(0x168d30u,0x168d35u,0xce230u,child);
        refresh(kept);
        return this->gp[eax];
    }
    void slot_pushed(std::uint32_t slot){this->push(0x168d3cu,slot);}                                   // PUSH EBX
    void changed(std::uint32_t index,Kept &kept) {                                                       // PUSH [EBP-4]; CALL 16AD45
        this->push(0x168d3du,index);
        auto child=[&](State &v){dispatch(v,0x16ad45u,0x168d40u);};
        this->call(0x168d40u,0x168d45u,0x16ad45u,child);
        refresh(kept);
        this->reached(Branch::changed);
    }
    std::uint32_t advanced(std::uint32_t v,std::uint32_t by){return this->flags.plus(v,by,4);}          // ADD [EBP-n],by
    std::uint32_t incremented(std::uint32_t v){return this->flags.increment(v,4,false);}                // INC [EBP-4]
    std::uint32_t next_slot(std::uint32_t slot) {                                                        // INC EBX; CMP EBX,2
        auto &gp=this->gp;
        gp[ebx]=this->flags.increment(slot,4,false);this->compare32(gp[ebx],logic::slots);
        return gp[ebx];
    }
    void updated(std::uint32_t index,Kept &kept) {                                                       // PUSH [EBP-4]; CALL 169C46
        this->push(0x168d52u,index);
        this->call(0x168d55u,0x168d5au,0x169c46u,update_child);
        refresh(kept);
    }
    void restoring() {                                                                                   // POP EDI; POP ESI; POP EBX; MOV ESP,EBP; POP EBP
        auto &gp=this->gp;
        gp[edi]=this->pop(0x168d71u);gp[esi]=this->pop(0x168d72u);gp[ebx]=this->pop(0x168d73u);
        gp[esp]=gp[ebp];gp[ebp]=this->pop(0x168d74u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::player_weapon_sync::sync(memory,*this,Kept{gp[ebx],gp[esi],gp[edi]});
        this->ret(0x168d75u,0u);this->publish(0x168d75u);
    }
};
template<class State,class Reference,class Player,class Update,class Dispatch>
bool bridge(State &s,Reference reference,Player player,Update update,Dispatch dispatch) {
    return run_owner(s,0x168ca1u,0x168d75u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Player,Update,Dispatch>(v,c,t,player,update,dispatch);});
}
} // namespace halo2::native_player_weapon_sync_detail
