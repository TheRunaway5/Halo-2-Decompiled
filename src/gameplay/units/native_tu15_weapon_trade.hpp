#pragma once
// Boundary adapter for the weapon trade check, E79C0
// (native_tu15_weapon_trade_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the player, the other unit
// on the stack, AL the answer (RET 4); 0x28 bytes of frame with ESI, EDI,
// EBX and EBP saved below them as the paths save them, read from the ESP
// each child left; CD440 (EAX the unit, ESI the output), CDCC0 (ECX the
// unit, EAX the output) and 11B560 (EAX the unit, ESI the output) direct,
// 30BD0 (EAX the offset; XMM0 its length) and CFC90 (the unit, the weapon
// and the hands record pushed, RET C) through the unit's dispatch, every
// register each returns coming back (XMM5 to XMM7 too). XMM0, XMM1 and XMM5
// to XMM7 hold what the original's scalar moves and arithmetic leave in
// them. Each access publishes its original instruction.
// HALO2_REFERENCE_WEAPON_TRADE=1 restores the translated body.
// HALO2_VERIFY_WEAPON_TRADE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_weapon_trade_systems.hpp"

namespace halo2::native_weapon_trade_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_weapon_trade;
using Access=logic::weapon_trade::Access;
using Kept=logic::weapon_trade::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_WEAPON_TRADE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_WEAPON_TRADE");return v;}
enum Branch : unsigned {busy, armed, in_range, facing, opposed, allowed, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Weapon trade"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t low(__m128 v){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
inline constexpr std::uint32_t sites[]{0xe79c3u,0xe79c7u,0xe79d4u,0xe79ddu,0xe79e2u,0xe79e9u,0xe79f4u,0xe79fau,0xe7a0cu,0xe7a0fu,0xe7a13u,
    0xe7a28u,0xe7a2fu,0xe7a32u,0xe7a3du,0xe7a41u,0xe7a56u,0xe7a6fu,0xe7a7bu,0xe7a88u,0xe7a8eu,0xe7a94u,0xe7a9au,0xe7aa0u,0xe7aa6u,
    0xe7aacu,0xe7ab2u,0xe7abcu,0xe7ac7u,0xe7ad4u,0xe7ae5u,0xe7af1u,0xe7af7u,0xe7afdu,0xe7b03u,0xe7b09u,0xe7b1eu,0xe7b28u,0xe7b31u,
    0xe7b3eu,0xe7b44u,0xe7b56u,0xe7b6cu,0xe7b80u,0xe7b88u,0xe7b9bu,0xe7ba3u,0xe7baau};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::answer_load)+1u);
template<class State,class Position,class Facing,class OtherFacing,class Dispatch> struct Adapter : Port<State> {
    Position &position;Facing &facing_child;OtherFacing &other_facing_child;Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below the saved registers (negative before them)
    unsigned slot_register{eax};bool first_position{true},first_hands{true};
    std::uint32_t exit{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Position &p,Facing &f,OtherFacing &o,Dispatch &d)
        :Port<State>(s,c,t,{}),position(p),facing_child(f),other_facing_child(o),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        kept={gp[ebx],gp[ebp],gp[edi],low(x[5]),low(x[6]),low(x[7])};
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case other_argument:case other_again:case unit_facing:gp[ecx]=v;break;
        case headers_load:case datums_third:case unit_hands:gp[edx]=v;break;
        case datums_load:case other_load:case players_load:gp[esi]=v;break;
        case player_datums:gp[edi]=v;break;
        case datums_again:case unit_handle:case unit_load:case unit_again:case other_argument_again:case other_facing:case other_hands:
            gp[eax]=v;break;
        case other_index:set_low_word(gp[eax],sx8(v));slot_register=eax;break;                        // MOVSX AX,byte
        case unit_index:set_low_word(gp[ecx],sx8(v));slot_register=ecx;break;                          // MOVSX CX,byte
        case other_weapon:gp[ebx]=v;break;
        case unit_weapon:gp[ebp]=v;break;
        case other_x:case other_y:case other_z:case vector_z:case vector_z_again:x[0]=lane0(v);break;  // MOVSS XMM0
        case longest_load:case vector_y:case vector_y_again:case vector_x_again:x[1]=lane0(v);break;   // MOVSS XMM1
        case offset_z_load:x[5]=lane0(v);break;
        case offset_y_load:x[6]=lane0(v);break;
        case offset_x_load:x[7]=lane0(v);break;
        case other_held:case unit_held:case answer_load:set_low_byte(gp[eax],v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing(){this->gp[esp]=this->flags.minus(this->gp[esp],logic::local::size,4);depth=0u-16u;}   // SUB ESP,28
    std::uint32_t masked(std::uint32_t handle){return this->gp[ecx]=this->flags.logic(handle&logic::handle_mask,4);}   // AND ECX,FFFF
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==0u)this->push(0xe79d3u,gp[esi]);else this->push(0xe79e1u,gp[edi]);                         // PUSH ESI / PUSH EDI
        depth+=4u;
    }
    std::uint32_t offset_of(std::uint32_t index){return this->gp[ecx]=this->flags.shift(3u*index,2u,4);}   // LEA ECX,[ECX+ECX*2]; SHL ECX,2
    void compared(std::uint32_t a,std::uint32_t b){this->compare32(a,b);}
    std::uint32_t player_address(std::uint32_t player_datums) {                                         // AND EAX,FFFF; IMUL EAX,EAX,21C; ADD EAX,EDI; MOV EDI,EAX
        auto &gp=this->gp;auto &fl=this->flags;
        this->reached(Branch::busy);
        gp[eax]=fl.plus(fl.product(fl.logic(gp[eax]&logic::handle_mask,4),logic::player::size),player_datums,4);
        return gp[edi]=gp[eax];
    }
    std::uint32_t saving_weapon(unsigned n) {                                                           // PUSH EBX; OR EBX,-1 / PUSH EBP; OR EBP,-1
        auto &gp=this->gp;
        const unsigned reg=n==2u?ebx:ebp;
        this->push(n==2u?0xe7a1bu:0xe7a49u,gp[reg]);depth+=4u;
        return gp[reg]=this->flags.logic(gp[reg]|logic::none,4);
    }
    bool slot_compared(std::uint32_t slot_value){this->flags.minus(sx8(slot_value)&0xffffu,0xffffu,2);return sx8(slot_value)!=logic::none;}   // CMP AX/CX,FFFF
    std::uint32_t slot(std::uint32_t slot_value){return this->gp[slot_register]=sx8(slot_value);}      // MOVSX EAX,AX / ECX,CX
    std::uint32_t indexed(std::uint32_t handle) {                                                       // AND EAX,FFFF; LEA ECX,[EAX+EAX*2]
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(handle&logic::handle_mask,4);gp[ecx]=3u*gp[eax];
        return gp[eax];
    }
    void positioned(std::uint32_t,std::uint32_t out,Kept &kept) {                                       // LEA ESI,[ESP+out]; CALL CD440
        this->gp[esi]=local(out);
        if(first_position)this->reached(Branch::armed);
        const std::uint32_t pc=first_position?0xe7a76u:0xe7a83u;first_position=false;
        this->call(pc,pc+5u,0xcd440u,position);
        refresh(kept);
    }
    void differenced(std::uint32_t difference,std::uint32_t c) {                                        // SUBSS XMM0,...; (LEA EAX,[ESP+14])
        this->s.xmm[0]=lane0(difference);
        if(c==logic::z)this->gp[eax]=local(logic::local::offset);
    }
    std::uint32_t normalized(Kept &kept) {                                                               // CALL 30BD0
        auto child=[&](State &v){dispatch(v,0x30bd0u,0xe7ac2u);};
        this->call(0xe7ac2u,0xe7ac7u,0x30bd0u,child);
        refresh(kept);
        return low(this->s.xmm[0]);
    }
    std::uint32_t shortest_compared(std::uint32_t shortest){this->flags.scalar_compare(this->s.xmm[0],lane0(shortest),false);return shortest;}   // COMISS XMM0,[46A0E8]
    void longest_compared(std::uint32_t) {                                                               // COMISS XMM1,XMM0
        this->flags.scalar_compare(this->s.xmm[1],this->s.xmm[0],false);
        if(!(this->flags.cf||this->flags.zf))this->reached(Branch::in_range);
    }
    void faced(std::uint32_t,Kept &kept) {                                                               // LEA EAX,[ESP+2C]; CALL CDCC0
        this->gp[eax]=local(logic::local::vector);
        this->call(0xe7aecu,0xe7af1u,0xcdcc0u,facing_child);
        refresh(kept);
    }
    void dotted(std::uint32_t along,std::uint32_t x_term) {                                             // the MULSS/ADDSS registers
        this->s.xmm[0]=lane0(along);this->s.xmm[1]=lane0(x_term);
    }
    std::uint32_t facing_compared(std::uint32_t facing_value) {                                         // COMISS XMM0,[456910]
        this->flags.scalar_compare(this->s.xmm[0],lane0(facing_value),false);
        if(!(this->flags.cf||this->flags.zf))this->reached(Branch::facing);
        return facing_value;
    }
    void other_faced(std::uint32_t,Kept &kept) {                                                         // LEA ESI,[ESP+2C]; CALL 11B560
        this->gp[esi]=local(logic::local::vector);
        this->call(0xe7b39u,0xe7b3eu,0x11b560u,other_facing_child);
        refresh(kept);
    }
    void other_dotted(std::uint32_t against,std::uint32_t) {                                            // ...; XORPS XMM1; COMISS XMM1,XMM0
        auto &x=this->s.xmm;
        x[0]=lane0(against);x[1]=_mm_setzero_ps();
        this->flags.scalar_compare(x[1],x[0],false);
        if(!(this->flags.cf||this->flags.zf))this->reached(Branch::opposed);
    }
    bool held(std::uint32_t handle,std::uint32_t weapon,Kept &kept) {                                   // LEA reg,[ESP+3C]; PUSH reg, weapon, handle; CALL CFC90; TEST AL,AL
        auto &gp=this->gp;
        const std::uint32_t base=first_hands?0xe7b74u:0xe7b8fu;
        const unsigned record_register=first_hands?edx:ecx;
        gp[record_register]=local(logic::local::other);
        put(base,gp[record_register]);put(base+1u,weapon);put(base+2u,handle);
        auto child=[&](State &v){dispatch(v,0xcfc90u,base+3u);};
        this->call(base+3u,base+8u,0xcfc90u,child);depth-=12u;
        first_hands=false;
        refresh(kept);
        this->test8(gp[eax]);
        return gp[eax]&0xffu;
    }
    bool byte_tested(std::uint32_t v){this->test8(v);return v&0xffu;}                                 // TEST AL,AL
    void restoring(bool saved_weapons) {                                                                 // POP EBP; POP EBX
        auto &gp=this->gp;
        if(saved_weapons){gp[ebp]=this->pop(0xe7ba8u);gp[ebx]=this->pop(0xe7ba9u);depth-=8u;}
    }
    void finished() {                                                                                    // POP EDI; POP ESI; ADD ESP,28
        auto &gp=this->gp;
        if(gp[eax]&0xffu)this->reached(Branch::allowed);
        gp[edi]=this->pop(0xe7baeu);gp[esi]=this->pop(0xe7bafu);
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);
    }
    void run() {
        auto &gp=this->gp;auto &x=this->s.xmm;
        Memory<Adapter> memory{*this};
        logic::weapon_trade::allowed(memory,*this,Kept{gp[ebx],gp[ebp],gp[edi],low(x[5]),low(x[6]),low(x[7])});
        this->ret(0xe7bb3u,4u);this->publish(0xe7bb3u);
    }
};
template<class State,class Reference,class Position,class Facing,class OtherFacing,class Dispatch>
bool bridge(State &s,Reference reference,Position position,Facing facing,OtherFacing other_facing,Dispatch dispatch) {
    return run_owner(s,0xe79c0u,0xe7bb3u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Position,Facing,OtherFacing,Dispatch>(v,c,t,position,facing,other_facing,dispatch);});
}
} // namespace halo2::native_weapon_trade_detail
