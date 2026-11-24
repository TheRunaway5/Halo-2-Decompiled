#pragma once
// Boundary adapter for the HUD weapon flags, 13DC44
// (native_tu15_hud_weapon_flags_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX the unit, EBX the record
// (RET); an EBP frame of 12 bytes (addressed from the live EBP, as the
// original does) with EDI and ESI saved below it; BCDF0 (-1 pushed, RET 4)
// and 1E7810 through the unit's dispatch and 10B3D0 (EDI the slot) direct,
// every register each returns coming back. Each access publishes its
// original instruction.
// HALO2_REFERENCE_HUD_WEAPON_FLAGS=1 restores the translated body.
// HALO2_VERIFY_HUD_WEAPON_FLAGS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/weapons/native_tu15_hud_weapon_flags_systems.hpp"

namespace halo2::native_hud_weapon_flags_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_hud_weapon_flags;
using Access=logic::hud_weapon_flags::Access;
using Kept=logic::hud_weapon_flags::Kept;
using Outcome=logic::hud_weapon_flags::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_HUD_WEAPON_FLAGS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_HUD_WEAPON_FLAGS");return v;}
enum Branch : unsigned {unarmed, armed, enemy, counted, full, branch_count};
static_assert(unsigned(Outcome::armed)==Branch::armed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"HUD weapon flags"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x13dc4au,0x13dc50u,0x13dc5eu,0x13dc62u,0x13dc62u,0x13dc66u,0x13dc6eu,0x13dc71u,0x13dc8eu,0x13dc90u,
    0x13dc9bu,0x13dc9fu,0x13dca5u,0x13dcb1u,0x13dcb4u,0x13dcbbu,0x13dccbu,0x13dccbu,0x13dccfu,0x13dcd2u,0x13dcd8u,0x13dcddu,0x13dce5u,
    0x13dceau,0x13dcf3u,0x13dcfeu,0x13dd02u,0x13dd0au,0x13dd0du,0x13dd13u,0x13dd13u,0x13dd17u,0x13dd1du,0x13dd1du,0x13dd23u,0x13dd26u,
    0x13dd2cu,0x13dd35u,0x13dd3bu,0x13dd4fu,0x13dd54u,0x13dd5du,0x13dd5du};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::full_store)+1u);
template<class State,class Accepts,class Dispatch> struct Adapter : Port<State> {
    Accepts &accepts_child;Dispatch &dispatch;
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Accepts &a,Dispatch &d):Port<State>(s,c,t,{}),accepts_child(a),dispatch(d){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[ebp]+std::uint32_t(offset);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers:case datums:case instances:case magazines:case magazine_definition:gp[ecx]=v;break;
        case object_address:case weapon_handle:case weapon_tag:case definition:case unit_load:case definition_load:case magazine:gp[eax]=v;break;
        case weapon_type:set_low_byte(gp[ecx],v);break;
        case weapon_team:set_low_word(gp[esi],v);break;
        case unit_team:set_low_word(gp[ecx],v);break;
        case slot_load:case slot_again:gp[esi]=v;break;
        case charge:case slot_charge:this->s.xmm[0]=lane0(v);break;
        case magazine_instances:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // PUSH EBP; MOV EBP,ESP; SUB ESP,C
        auto &gp=this->gp;
        this->push(0x13dc44u,gp[ebp]);gp[ebp]=gp[esp];gp[esp]=this->flags.minus(gp[esp],0xcu,4);
    }
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==0u){this->push(0x13dc53u,gp[edi]);gp[edi]=0xffffu;}                                     // PUSH EDI; MOV EDI,FFFF
        else{this->push(0x13dc7cu,gp[esi]);this->push(0x13dc7du,logic::none);}                         // PUSH ESI; PUSH -1
    }
    void indexing(std::uint32_t unit_handle,std::uint32_t mask){this->gp[eax]=3u*this->flags.logic(unit_handle&mask,4);}   // AND EAX,EDI; LEA
    std::uint32_t anded(std::uint32_t v,std::uint32_t mask){return this->flags.logic(v&mask,4);}       // AND dword [EBX+4C],0
    std::uint32_t slot(std::uint32_t unit_address){return this->gp[eax]=this->flags.plus(unit_address,logic::unit::slot,4);}   // ADD EAX,1C8
    void compared(std::uint32_t a,std::uint32_t b){this->compare32(a,b);}
    std::uint32_t weapon(std::uint32_t,Kept &kept) {                                                    // CALL BCDF0; MOV ESI,EAX
        auto child=[&](State &v){dispatch(v,0xbcdf0u,0x13dc7fu);};
        this->call(0x13dc7fu,0x13dc84u,0xbcdf0u,child);
        refresh(kept);
        return this->gp[esi]=this->gp[eax];
    }
    void tested(std::uint32_t v){this->test32(v);}                                                     // TEST ESI,ESI
    std::uint32_t tag_index(std::uint32_t tag,std::uint32_t mask,unsigned n) {                          // AND EAX(ECX),EDI; SHL 4
        auto &fl=this->flags;
        return this->gp[n?ecx:eax]=fl.shift(fl.logic(tag&mask,4),4u,4);
    }
    std::uint32_t typed(std::uint32_t type) {                                                           // XOR EAX,EAX; INC EAX; SHL EAX,CL; TEST AL,3
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.shift(fl.increment(fl.logic(0u,4),4,false),type&0xffu,4);fl.logic(gp[eax]&logic::typed_mask&0xffu,1);
        return gp[eax];
    }
    std::uint32_t allied(std::uint32_t unit_now,std::uint32_t,std::uint32_t,Kept &kept) {              // CALL 1E7810; TEST AL,AL
        auto &gp=this->gp;
        gp[eax]=unit_now;
        auto child=[&](State &v){dispatch(v,0x1e7810u,0x13dcc2u);};
        this->call(0x13dcc2u,0x13dcc7u,0x1e7810u,child);
        refresh(kept);
        this->test8(gp[eax]);
        if(!(gp[eax]&0xffu))this->reached(Branch::enemy);
        return gp[eax];
    }
    std::uint32_t ored(std::uint32_t v,std::uint32_t bit){return this->flags.logic(v|bit,4);}           // OR dword [EBX+4C],bit
    void charged(std::uint32_t charge_value,std::uint32_t one_value){this->flags.scalar_compare(lane0(charge_value),lane0(one_value),false);}   // COMISS
    std::uint32_t scaled(std::uint32_t magazine_value){return this->gp[eax]=this->flags.product(magazine_value,logic::entry_size);}   // IMUL EAX,1C
    std::uint32_t entry(std::uint32_t scaled_value,std::uint32_t entries_value) {                      // ADD EAX,[ECX+6C]
        this->reached(Branch::counted);
        return this->gp[eax]=this->flags.plus(scaled_value,entries_value,4);
    }
    void tested8(std::uint32_t v){this->test8(v);}
    void byte_compared(std::uint32_t v){this->flags.minus(v&0xffu,0u,1);}                              // CMP byte,0
    std::uint32_t accepts(Kept &kept) {                                                                 // MOV EDI,ESI; CALL 10B3D0; TEST AL,AL
        auto &gp=this->gp;
        gp[edi]=kept.source;
        this->call(0x13dd46u,0x13dd4bu,0x10b3d0u,accepts_child);
        refresh(kept);
        this->test8(gp[eax]);
        if(gp[eax]&0xffu)this->reached(Branch::full);
        return gp[eax];
    }
    void restoring(unsigned n) {                                                                         // (POP ESI); POP EDI; MOV ESP,EBP; POP EBP
        auto &gp=this->gp;
        if(n==1u)gp[esi]=this->pop(0x13dd61u);
        gp[edi]=this->pop(0x13dd62u);gp[esp]=gp[ebp];gp[ebp]=this->pop(0x13dd63u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::hud_weapon_flags::compute(memory,*this,gp[eax],Kept{gp[ebx],gp[esi],gp[edi]});
        this->reached(unsigned(outcome));
        this->ret(0x13dd64u,0u);this->publish(0x13dd64u);
    }
};
template<class State,class Reference,class Accepts,class Dispatch> bool bridge(State &s,Reference reference,Accepts accepts,Dispatch dispatch) {
    return run_owner(s,0x13dc44u,0x13dd64u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Accepts,Dispatch>(v,c,t,accepts,dispatch);});
}
} // namespace halo2::native_hud_weapon_flags_detail
