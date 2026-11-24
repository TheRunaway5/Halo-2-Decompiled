#pragma once
// Boundary adapter for the biped action test, 1540D0
// (native_tu15_biped_action_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the biped, the player and the
// context on the stack (RET 8); an 8-byte frame (the candidate interaction)
// below the saved EBX, EBP, ESI and EDI; 1E7810 gets CX the player's team and
// SI the biped's; E8110 EDI the player's unit and the biped and the target
// pushed (RET 8); E79C0 EAX the player and the biped pushed (RET 4); 153E10
// EBX the candidate, EDI the context and the player pushed (RET 4); every
// register each returns coming back. Each access publishes its original
// instruction.
// HALO2_REFERENCE_BIPED_ACTION=1 restores the translated body.
// HALO2_VERIFY_BIPED_ACTION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_biped_action_systems.hpp"

namespace halo2::native_biped_action_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_biped_action;
using Access=logic::biped_action::Access;
using Kept=logic::biped_action::Kept;
using Outcome=logic::biped_action::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_BIPED_ACTION");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_BIPED_ACTION");return v;}
enum Branch : unsigned {no_biped_unit, ignored, melee, refused, interaction, branch_count};
static_assert(unsigned(Outcome::interaction)==Branch::interaction);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Biped action"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x1540d5u,0x1540ebu,0x1540f0u,0x1540f3u,0x1540f9u,0x154106u,0x154109u,0x154115u,0x154125u,0x154132u,
    0x15413au,0x15414au,0x15414eu,0x154151u,0x15415cu,0x154174u,0x15417au,0x15417eu,0x154182u,0x15418bu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::type_store)+1u);
template<class State,class Melee,class Accepts,class Offer,class Route> struct Adapter : Port<State> {
    Melee &melee_child;
    Accepts &accepts_child;
    Offer &offer_child;
    Route &route;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Melee &m,Accepts &a,Offer &o,Route &r)
        :Port<State>(s,c,t,{}),melee_child(m),accepts_child(a),offer_child(o),route(r){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t candidate(std::uint32_t offset)const{return this->gp[esp]+depth-8u+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case player_argument:                                                                          // PUSH ESI; PUSH EDI; MOV EDI,EBP; AND EDI,FFFF; MOV EBX,EAX; IMUL EDI,EDI,21C
            gp[ebp]=v;put(0x1540d9u,gp[esi]);put(0x1540dau,gp[edi]);
            gp[edi]=fl.logic(gp[ebp]&logic::handle_mask,4);gp[ebx]=gp[eax];gp[edi]=fl.product(gp[edi],logic::player_size);break;
        case players:case unit:gp[eax]=v;break;
        case player_data:gp[edi]=fl.plus(gp[edi],v,4);break;                                          // ADD EDI,[EAX+44]
        case objects:gp[ecx]=v;break;
        case object_data:gp[ecx]=v;gp[edx]=3u*fl.logic(gp[ebx]&logic::handle_mask,4);gp[eax]=gp[ebx]&logic::handle_mask;break;   // MOV EAX,EBX; AND EAX,FFFF; LEA EDX,[EAX+EAX*2]
        case biped_address:gp[esi]=v;gp[eax]=3u*fl.logic(gp[eax]&logic::handle_mask,4);break;         // AND EAX,FFFF; LEA EAX,[EAX+EAX*2]
        case unit_type:                                                                                // MOV EDX,1; SHL EDX,CL; TEST DL,1
            set_low_byte(gp[ecx],v);gp[edx]=fl.shift(1u,v&0xffu,4);this->test8(gp[edx]&1u);break;
        case biped_flags:set_low_byte(gp[eax],fl.right(v&0xffu,2u,1,false));this->test8(gp[eax]&1u);break;   // SHR AL,2; TEST AL,1
        case player_team:set_low_word(gp[ecx],std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v)))));break;   // MOVSX CX,byte
        case biped_team:set_low_word(gp[esi],v);break;
        case context_argument:gp[esi]=v;break;
        case melee_target:gp[ecx]=v;break;
        case unit_again:gp[edi]=v;break;
        case interaction_argument:gp[edi]=v;gp[edx]=fl.logic(0u,4);break;                             // XOR EDX,EDX
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void refresh(Kept &kept){kept={this->gp[edi],this->gp[ebx],this->gp[ebp],this->gp[esi]};}
    std::uint32_t hostile(std::uint32_t,std::uint32_t,Kept &kept) {
        this->call_routed(route,0x154141u,0x154146u,0x1e7810u);this->test8(this->gp[eax]&0xffu);refresh(kept);
        return this->gp[eax];
    }
    // PUSH ECX; PUSH EBX; CALL E8110; POP EDI.
    std::uint32_t melee(std::uint32_t,std::uint32_t,std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        put(0x154154u,gp[ecx]);put(0x154155u,gp[ebx]);
        this->call(0x154156u,0x15415bu,0xe8110u,melee_child);depth-=8u;
        refresh(kept);gp[edi]=take(0x15415bu);
        return gp[eax];
    }
    // PUSH EBX; MOV EAX,EBP; CALL E79C0; TEST AL,AL.
    std::uint32_t accepts(std::uint32_t,std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        put(0x154168u,gp[ebx]);gp[eax]=gp[ebp];
        this->call(0x15416bu,0x154170u,0xe79c0u,accepts_child);depth-=4u;
        this->test8(gp[eax]&0xffu);refresh(kept);
        return gp[eax];
    }
    // PUSH EBP; LEA EBX,[ESP+14].
    void offering(const Kept &){put(0x154186u,this->gp[ebp]);this->gp[ebx]=candidate(0u);}
    void offer(std::uint32_t,std::uint32_t,std::uint32_t,Kept &kept) {
        this->call(0x154192u,0x154197u,0x153e10u,offer_child);depth-=4u;
        refresh(kept);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],8u,4);depth=8u;                                                      // SUB ESP,8
        put(0x1540d3u,gp[ebx]);put(0x1540d4u,gp[ebp]);
        Memory<Adapter> memory{*this};
        const auto outcome=logic::biped_action::test(memory,*this,gp[eax]);
        this->reached(unsigned(outcome));
        std::uint32_t exit;
        if(outcome==Outcome::melee)exit=0x15415fu;
        else{gp[edi]=take(0x154197u);exit=0x154198u;}
        gp[esi]=take(exit);gp[ebp]=take(exit+1u);gp[ebx]=take(exit+2u);
        gp[esp]=fl.plus(gp[esp],8u,4);                                                                // ADD ESP,8
        this->ret(exit+6u,8u);this->publish(exit+6u);
    }
};
template<class State,class Reference,class Melee,class Accepts,class Offer,class Route>
bool bridge(State &s,Reference reference,Melee melee,Accepts accepts,Offer offer,Route route) {
    return run_owner(s,0x1540d0u,0x15419eu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Melee,Accepts,Offer,Route>(v,c,t,melee,accepts,offer,route);});
}
} // namespace halo2::native_biped_action_detail
