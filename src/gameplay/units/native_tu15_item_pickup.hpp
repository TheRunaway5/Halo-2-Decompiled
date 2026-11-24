#pragma once
// Boundary adapter for the item pickup, 1538B0
// (native_tu15_item_pickup_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: the player's and the item's
// handles on the stack (RET 8); 0x28 bytes of frame with EBX and EDI (and,
// past a player without a unit, EBP and ESI) saved below them, read from the
// ESP each child left. 104D30 (five pushed, RET 14), 197E9D (two pushed, RET
// 8), CF080, 197E11 and 161320 (one pushed, RET 4), CF060, 13B360 and 153850
// are direct; C6920, BCDF0 (one pushed, RET 4), 1549D0 and B3140 (two pushed,
// RET 8), CFC90 (three pushed, RET C) and E8E50 go through the dispatch;
// every register each returns comes back. Each exit leaves through the
// original's epilogue. Each access publishes its original instruction.
// HALO2_REFERENCE_ITEM_PICKUP=1 restores the translated body.
// HALO2_VERIFY_ITEM_PICKUP=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_item_pickup_systems.hpp"

namespace halo2::native_item_pickup_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_item_pickup;
using Access=logic::item_pickup::Access;
using Exit=logic::item_pickup::Exit;
using Kept=logic::item_pickup::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ITEM_PICKUP");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ITEM_PICKUP");return v;}
enum Branch : unsigned {unitless, rests, parent_refused, owned, far, mode_refused, ammunition, no_count, grenade, grenade_refused,
    powerup, not_weapon, weapon_refused, handed, requested, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Item pickup"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline std::uint32_t low_word(std::uint32_t reg,std::uint32_t v){return (reg&0xffff0000u)|(v&0xffffu);}
inline __m128 lane0(std::uint32_t bits){return logic::sse::scalar(bits);}
inline constexpr std::uint32_t sites[]{
    0x1538b0u,0x1538b5u,0x1538bcu,0x1538cfu,0x1538dbu,0x1538e1u,0x1538efu,0x1538f3u,0x153903u,0x153907u,0x15390bu,0x153915u,
    0x153923u,0x153945u,0x153951u,0x153956u,0x15395bu,0x153960u,0x153965u,0x15396au,0x15396fu,0x153974u,0x1539a9u,0x1539aeu,
    0x1539b4u,0x1539d0u,0x1539f3u,0x1539ffu,0x153a03u,0x153a0bu,0x153a25u,0x153a2eu,0x153a35u,0x153a38u,0x153a45u,0x153a49u,
    0x153a51u,0x153a62u,0x153a65u,0x153a73u,0x153a77u,0x153a84u,0x153a88u,0x153a95u,0x153a99u,0x153aa9u,0x153abfu,0x153ac3u,
    0x153aceu,0x153ad2u,0x153ad8u,0x153adbu,0x153adfu,0x153af3u,0x153b06u,0x153b0eu,0x153b22u,0x153b2fu,0x153b45u,0x153b49u,
    0x153b4du,0x153b51u,0x153b55u,0x153b59u,0x153b5du,0x153b61u,0x153b69u,0x153b6du,0x153b75u,0x153b81u,0x153b85u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::notified_player)+1u);
template<class State,class Children,class Dispatch> struct Adapter : Port<State> {
    Children &children;      // 104D30, 197E9D, CF080, 197E11, 161320, CF060, 13B360 and 153850
    Dispatch &dispatch;      // C6920, BCDF0, 1549D0, CFC90, B3140 and E8E50
    std::uint32_t depth{};   // bytes pushed beyond the four saved registers (negative before them)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k,Dispatch &d):Port<State>(s,c,t,{}),children(k),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){auto &gp=this->gp;kept.player_address=gp[ebx];kept.subject=gp[ebp];kept.item=gp[esi];kept.counter=gp[edi];}
    void direct(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){children(v,target);};
        this->call(pc,pc+5u,target,child);depth-=popped;refresh(kept);
    }
    void routed(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto route=[&](State &v){dispatch(v,target,pc);};
        this->call(pc,pc+5u,target,route);depth-=popped;refresh(kept);
    }
    bool answered(){this->test8(this->gp[eax]);return low_byte(this->gp[eax])!=0u;}                       // TEST AL,AL
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case players_load:case item_argument:case parent_load:case game_load:case weapon_item:case count_load:case ammo_objects_data:
        case weapon_definition:case equipment_item:case equipment_definition:case definition_address:case grenade_definition:
        case powerup_unit:case powerup_item:case type_objects_data:case weapon_unit:case taken_player:case handing_unit:gp[eax]=v;break;
        case current_weapon:gp[eax]=sx8(v);break;                                                          // MOVSX EAX,byte
        case powerup_kind:gp[eax]=low_word(gp[eax],v);break;                                               // MOV AX,word
        case taken_load:set_low_byte(gp[eax],v);break;                                                     // MOV AL
        case players_data:case objects_data:case definitions_load:case offset_load:case weapon_address:case powerup_player:
        case parent_type:gp[ecx]=v;break;                                                                  // (MOVZX ECX,byte)
        case player_index:gp[ecx]=sx16(v);break;                                                           // MOVSX ECX,word
        case item_type:set_low_byte(gp[ecx],v);break;                                                      // MOV CL
        case objects_load:case parent_objects:case unit_handle:case ammo_objects:case type_objects:case handed_player:
        case notified_player:gp[edx]=v;break;
        case ammo_player_index:case grenade_player_index:gp[edx]=sx16(v);break;                            // MOVSX EDX,word
        case player_argument:gp[ebx]=v;break;
        case unit_address:gp[ebp]=v;break;
        case item_address:case mode_item:case weapon_load:case grenade_unit:gp[esi]=v;break;
        case unit_load:case grenade_item:case item_reload:case request_item_reload:gp[edi]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The frame, the player and the unit.
    void framing(){auto &gp=this->gp;gp[esp]=this->flags.minus(gp[esp],0x28u,4);depth=0u-16u;}               // SUB ESP,28
    void saving(unsigned n) {                                                                            // PUSH EBX / EDI / EBP / ESI
        static constexpr std::uint32_t pcs[]{0x1538bbu,0x1538ceu,0x1538eeu,0x153902u};
        static constexpr unsigned regs[]{ebx,edi,ebp,esi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t player_address(std::uint32_t handle,std::uint32_t players) {                          // AND EBX,FFFF; IMUL EBX,EBX,21C; ADD EBX,ECX
        auto &fl=this->flags;
        return this->gp[ebx]=fl.plus(fl.product(fl.logic(handle&logic::handle_mask,4),logic::player_size),players,4);
    }
    bool named(std::uint32_t v){this->compare32(v,logic::none);return v!=logic::none;}                        // CMP reg,-1
    std::uint32_t unit_index(std::uint32_t handle) {                                                     // MOV EAX,EDI; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]
        auto &gp=this->gp;
        const auto index=this->flags.logic(handle&logic::handle_mask,4);
        gp[eax]=3u*index;
        return index;
    }
    std::uint32_t item_offset(std::uint32_t handle) {                                                    // AND EAX,FFFF; LEA EAX,[EAX+EAX*2]; SHL EAX,2
        auto &fl=this->flags;
        return this->gp[eax]=fl.shift(3u*fl.logic(handle&logic::handle_mask,4),2u,4);
    }
    // The refusals.
    void unrested(){set_low_byte(this->gp[ecx],this->flags.logic(0u,1));}                                     // XOR CL,CL
    std::uint32_t parent_index(std::uint32_t parent) {                                                   // MOV ECX,EAX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2]
        const auto index=this->flags.logic(parent&logic::handle_mask,4);
        this->gp[ecx]=3u*index;
        return index;
    }
    bool resting(std::uint32_t type,std::uint32_t parent) {                                              // CMP ECX,B; CMP ECX,7; MOV CL,1 / XOR CL,CL; CMP EAX,-1; TEST CL,CL
        auto &gp=this->gp;
        this->compare32(type,logic::object_type::crate);
        bool rests=type==logic::object_type::crate;
        if(!rests){this->compare32(type,logic::object_type::machine);rests=type==logic::object_type::machine;}
        if(rests)set_low_byte(gp[ecx],1u);
        else set_low_byte(gp[ecx],this->flags.logic(0u,1));
        this->compare32(parent,logic::none);
        this->test8(gp[ecx]);
        this->reached(rests?Branch::rests:Branch::parent_refused);
        return rests;
    }
    bool different(std::uint32_t a,std::uint32_t b){this->compare32(a,b);if(a==b)this->reached(Branch::owned);return a!=b;}   // CMP [ESI+14C],EDI
    bool within(std::uint32_t dy,std::uint32_t dx2,std::uint32_t reach,std::uint32_t distance2,std::uint32_t reach2) {   // COMISS XMM0,XMM4; JBE
        auto &x=this->s.xmm;auto &fl=this->flags;
        x[0]=lane0(reach2);x[1]=lane0(dy);x[2]=lane0(dx2);x[3]=lane0(reach);x[4]=lane0(distance2);
        fl.scalar_compare(x[0],x[4],false);
        const bool near=!(fl.cf||fl.zf);
        if(!near)this->reached(Branch::far);
        return near;
    }
    bool checked(std::uint32_t mode){this->compare32(mode,logic::checked_mode);return mode==logic::checked_mode;}   // CMP dword,2
    bool mode_accepts(Kept &kept) {                                                                      // CALL C6920; TEST AL,AL
        routed(0x1539b8u,0xc6920u,0u,kept);
        const bool accepts=answered();
        if(!accepts)this->reached(Branch::mode_refused);
        return accepts;
    }
    // The ammunition.
    std::uint32_t first_slot(){return this->gp[edi]=this->flags.logic(0u,4);}                                 // XOR EDI,EDI
    std::uint32_t slot(std::uint32_t current,std::uint32_t counter) {                                   // XOR ECX,ECX; TEST EAX,EAX; SETL CL; MOVSX EDX,DI; DEC ECX; AND ECX,EAX; ADD ECX,EDX; AND ECX,80000003 (JNS; DEC; OR -4; INC)
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.logic(0u,4);
        fl.logic(current,4);
        set_low_byte(gp[ecx],fl.sf!=fl.of?1u:0u);
        gp[edx]=sx16(counter);
        gp[ecx]=fl.increment(gp[ecx],4,true);
        gp[ecx]=fl.logic(gp[ecx]&current,4);
        gp[ecx]=fl.plus(gp[ecx],gp[edx],4);
        gp[ecx]=fl.logic(gp[ecx]&0x80000003u,4);
        if(fl.sf){gp[ecx]=fl.increment(gp[ecx],4,true);gp[ecx]=fl.logic(gp[ecx]|0xfffffffcu,4);gp[ecx]=fl.increment(gp[ecx],4,false);}
        return gp[ecx];
    }
    void count_pushed(){auto &gp=this->gp;gp[eax]=local(logic::local::count);put(0x153a0au,gp[eax]);}         // LEA EAX,[ESP+10]; PUSH EAX
    bool ammunition_taken(Kept &kept,std::uint32_t index,std::uint32_t unit,std::uint32_t item) {       // PUSH ECX; PUSH EDX; PUSH EAX; PUSH ESI; CALL 104D30; TEST AL,AL
        put(0x153a0fu,index);put(0x153a10u,unit);put(0x153a11u,item);put(0x153a12u,kept.item);
        direct(0x153a13u,0x104d30u,0x14u,kept);
        return answered();
    }
    bool positive_count(std::uint32_t count) {                                                           // TEST AX,AX; JLE
        this->flags.logic(count&0xffffu,2);
        const bool positive=std::int16_t(count)>0;
        if(!positive)this->reached(Branch::no_count);
        return positive;
    }
    void count_reported(std::uint32_t count){put(0x153a34u,count);}                                         // PUSH EAX
    std::uint32_t weapon_index(std::uint32_t weapon) {                                                   // AND ESI,FFFF; LEA ECX,[ESI+ESI*2]
        auto &gp=this->gp;
        gp[esi]=this->flags.logic(weapon&logic::handle_mask,4);gp[ecx]=3u*gp[esi];
        return gp[esi];
    }
    void ammunition_reported(Kept &kept,std::uint32_t index,std::uint32_t,std::uint32_t) {             // PUSH EDX; CALL 197E9D
        put(0x153a4bu,index);direct(0x153a4cu,0x197e9du,8u,kept);this->reached(Branch::ammunition);
    }
    std::uint32_t next_slot(std::uint32_t counter){return this->gp[edi]=this->flags.increment(counter,4,false);}   // INC EDI
    bool slots_left(std::uint32_t counter) {                                                             // CMP DI,4; JL
        this->flags.minus(counter&0xffffu,logic::weapon_slots,2);
        return std::int16_t(counter)<std::int16_t(logic::weapon_slots);
    }
    // The equipment.
    std::uint32_t equipment(Kept &kept,std::uint32_t) {                                                  // PUSH 8; CALL BCDF0; MOV EBP,EAX
        auto &gp=this->gp;
        put(0x153a55u,logic::equipment_mask);routed(0x153a57u,0xbcdf0u,4u,kept);
        return kept.subject=gp[ebp]=gp[eax];
    }
    bool found(std::uint32_t v){this->test32(v);return v!=0u;}                                               // TEST EBP,EBP
    std::uint32_t definition_index(std::uint32_t tag) {                                                  // AND EAX,FFFF; SHL EAX,4
        auto &fl=this->flags;
        const auto index=fl.logic(tag&logic::handle_mask,4);
        this->gp[eax]=fl.shift(index,4u,4);
        return index;
    }
    bool grenade(std::uint32_t kind) {                                                                   // CMP AX,6
        this->flags.minus(kind&0xffffu,logic::equipment::grenade,2);
        return (kind&0xffffu)==logic::equipment::grenade;
    }
    bool grenade_taken(Kept &kept) {                                                                     // PUSH EDI; CALL CF080; TEST AL,AL
        put(0x153a8bu,kept.counter);direct(0x153a8cu,0xcf080u,4u,kept);
        const bool taken=answered();
        if(!taken)this->reached(Branch::grenade_refused);
        return taken;
    }
    void grenade_reported(Kept &kept,std::uint32_t index,std::uint32_t) {                               // PUSH EDX; CALL 197E11
        put(0x153a9cu,index);direct(0x153a9du,0x197e11u,4u,kept);this->reached(Branch::grenade);
    }
    bool powerup(std::uint32_t kind){this->flags.logic(kind&0xffffu,2);return (kind&0xffffu)!=logic::equipment::no_powerup;}   // TEST AX,AX
    std::uint32_t unit_powerup(Kept &kept,std::uint32_t){direct(0x153aacu,0xcf060u,0u,kept);return this->gp[eax];}   // CALL CF060
    bool deferred(Kept &kept,bool late){direct(late?0x153b1bu:0x153ab6u,0x13b360u,0u,kept);return answered();}   // CALL 13B360; TEST AL,AL
    void powerup_taken(Kept &kept,std::uint32_t item,std::uint32_t player) {                            // PUSH EAX; PUSH ECX; CALL 1549D0
        put(0x153ac7u,item);put(0x153ac8u,player);routed(0x153ac9u,0x1549d0u,8u,kept);this->reached(Branch::powerup);
    }
    // A weapon.
    bool weapon(std::uint32_t) {                                                                         // MOV EDX,1; SHL EDX,CL; TEST DL,4
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.shift(1u,gp[ecx]&0xffu,4);
        const bool is=fl.logic(gp[edx]&logic::weapon_type_bit,1)!=0u;
        if(!is)this->reached(Branch::not_weapon);
        return is;
    }
    bool weapon_offered(Kept &kept,std::uint32_t unit) {                                                 // LEA EDX,[ESP+14]; PUSH EDX; PUSH EDI; PUSH EAX; CALL CFC90; TEST AL,AL
        auto &gp=this->gp;
        gp[edx]=local(logic::local::taken);put(0x153afau,gp[edx]);put(0x153afbu,kept.counter);put(0x153afcu,unit);
        routed(0x153afdu,0xcfc90u,12u,kept);
        const bool offered=answered();
        if(!offered)this->reached(Branch::weapon_refused);
        return offered;
    }
    bool set(std::uint32_t v){this->test8(v);if(!(v&0xffu))this->reached(Branch::weapon_refused);return (v&0xffu)!=0u;}   // TEST AL,AL
    bool player_agrees(Kept &kept,std::uint32_t) {                                                       // CALL 153850; TEST AL,AL
        direct(0x153b12u,0x153850u,0u,kept);
        const bool agrees=answered();
        if(!agrees)this->reached(Branch::weapon_refused);
        return agrees;
    }
    void handed(Kept &kept) {                                                                            // PUSH 3; PUSH EDI; CALL B3140
        put(0x153b27u,logic::handed_mode);put(0x153b29u,kept.counter);routed(0x153b2au,0xb3140u,8u,kept);this->reached(Branch::handed);
    }
    std::uint32_t cleared(){return this->gp[ecx]=this->flags.logic(0u,4);}                                   // XOR ECX,ECX
    void request_address(){this->gp[edi]=local(logic::local::request);}                                      // LEA EDI,[ESP+18]
    void requested(Kept &kept){routed(0x153b7cu,0xe8e50u,0u,kept);this->reached(Branch::requested);}         // CALL E8E50
    void notify(Kept &kept,std::uint32_t player,bool after_handing) {                                    // PUSH EDX; CALL 161320
        const auto pc=after_handing?0x153b33u:0x153b89u;
        put(pc,player);direct(pc+1u,0x161320u,4u,kept);
    }
    void finish(Exit exit) {                                                                             // (POP ESI; POP EBP); POP EDI; POP EBX; ADD ESP,28; RET 8
        auto &gp=this->gp;
        const auto pc=exit==Exit::handed?0x153b39u:0x153b8fu;
        if(exit==Exit::unitless)this->reached(Branch::unitless);
        else{gp[esi]=this->pop(pc);gp[ebp]=this->pop(pc+1u);}
        gp[edi]=this->pop(pc+2u);gp[ebx]=this->pop(pc+3u);
        gp[esp]=this->flags.plus(gp[esp],0x28u,4);
        this->ret(pc+7u,8u);this->publish(pc+7u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        finish(logic::item_pickup::touch(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]}));
    }
};
template<class State,class Reference,class Children,class Dispatch> bool bridge(State &s,Reference reference,Children children,Dispatch dispatch) {
    return run_owner(s,0x1538b0u,0x153b96u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children,Dispatch>(v,c,t,children,dispatch);});
}
} // namespace halo2::native_item_pickup_detail
