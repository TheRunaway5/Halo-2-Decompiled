#pragma once
// Boundary adapter for the deferred action target check, 1AE5E0
// (native_tu15_action_target_defer_systems.hpp). Its callers and the
// children keep the original ABI, which the adapter owns: EDI the object,
// the target, the context and the result's address on the stack (RET C);
// EBX, EBP and ESI saved; 1AE500 gets the parent, the target, the context
// and the result pushed (RET 10); 1AE6F0 ECX the object and the context and
// the result pushed (RET 8); every register each returns coming back. Each
// access publishes its original instruction.
// HALO2_REFERENCE_ACTION_TARGET_DEFER=1 restores the translated body.
// HALO2_VERIFY_ACTION_TARGET_DEFER=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_action_target_defer_systems.hpp"

namespace halo2::native_action_target_defer_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_action_target_defer;
using Access=logic::action_target_defer::Access;
using Kept=logic::action_target_defer::Kept;
using Outcome=logic::action_target_defer::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ACTION_TARGET_DEFER");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ACTION_TARGET_DEFER");return v;}
enum Branch : unsigned {passing_tag, parent_decides, passed, child_passed, failed, branch_count};
static_assert(unsigned(Outcome::failed)==Branch::failed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Action target defer"};return v;}
inline constexpr std::uint32_t sites[]{0x1ae5e0u,0x1ae5e6u,0x1ae5eau,0x1ae5f9u,0x1ae5feu,0x1ae602u,0x1ae604u,0x1ae612u,0x1ae616u,
    0x1ae61au,0x1ae62bu,0x1ae633u,0x1ae63du,0x1ae64au,0x1ae65au,0x1ae65eu,0x1ae671u,0x1ae682u,0x1ae690u,0x1ae696u,0x1ae6a3u,
    0x1ae6a7u,0x1ae6b1u,0x1ae6cfu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::sibling)+1u);
template<class State,class Parent,class Rider> struct Adapter : Port<State> {
    Parent &parent_child;
    Rider &rider_child;
    std::uint32_t depth{};      // bytes below the return address
    bool asked{};               // the object asked 1AE6F0 already
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Parent &p,Rider &r):Port<State>(s,c,t,{}),parent_child(p),rider_child(r){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        if(a==Access::tag_address)gp[eax]=fl.shift(fl.logic(gp[eax]&0xffffu,4),4u,4);                // AND EAX,FFFF; SHL EAX,4
        return sites[unsigned(a)];
    }
    // AND EAX,FFFF; LEA EAX,[EAX+EAX*2] of a handle.
    void indexed(std::uint32_t handle){this->gp[eax]=3u*this->flags.logic(handle&0xffffu,4);}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:case headers_again:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;put(0x1ae5e9u,gp[ebx]);break;                                     // PUSH EBX
        case context_argument:gp[ebx]=v;indexed(gp[edi]);put(0x1ae5f8u,gp[ebp]);break;                  // MOV EAX,EDI; ...; PUSH EBP
        case result_argument:gp[ebp]=v;put(0x1ae5fdu,gp[esi]);break;                                    // PUSH ESI
        case object_address:case child_address:gp[esi]=v;break;
        case object_tag:case tag_address:case context_forward:gp[eax]=v;break;
        case instances:gp[edx]=v;break;
        case tag_flags_read:gp[eax]=fl.right(v,logic::tag_passing_bit,4,false);this->test8(gp[eax]&1u);break;
        case parent:case first_child:case sibling:gp[edx]=v;fl.minus(v,logic::none,4);break;
        case object_flags:case child_flags:gp[eax]=fl.right(v,logic::riding_bit,4,false);this->test8(gp[eax]&1u);break;
        case parent_datums:case child_datums:gp[ecx]=v;indexed(gp[edx]);break;
        case parent_type:set_low_byte(gp[ecx],v);gp[ebx]=fl.shift(1u,v&0xffu,4);this->test8(gp[ebx]&logic::acting_types);break;
        case child_kind:set_low_byte(gp[ecx],v);gp[eax]=fl.shift(1u,v&0xffu,4);this->test8(gp[eax]&logic::acting_types);break;
        case target_forward:gp[ecx]=v;break;
        case context_reload:gp[ebx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH EBP; PUSH EAX; PUSH ECX; PUSH EDX; CALL 1AE500.
    void parent_decides(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        put(0x1ae662u,gp[ebp]);put(0x1ae663u,gp[eax]);put(0x1ae664u,gp[ecx]);put(0x1ae665u,gp[edx]);
        this->call(0x1ae666u,0x1ae66bu,0x1ae500u,parent_child);depth-=16u;
    }
    // PUSH EBP; PUSH EBX; MOV ECX,EDI (the object) or EDX (a child); CALL 1AE6F0; TEST AL,AL.
    std::uint32_t rider_passes(std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        const std::uint32_t at=asked?0x1ae6c2u:0x1ae675u;
        put(at,gp[ebp]);put(at+1u,gp[ebx]);gp[ecx]=asked?gp[edx]:gp[edi];
        this->call(at+4u,at+9u,0x1ae6f0u,rider_child);depth-=8u;asked=true;
        this->test8(gp[eax]&0xffu);
        kept={gp[esi],gp[ebp],gp[ebx]};
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::action_target_defer::check(memory,*this,gp[edi]);
        this->reached(unsigned(outcome));
        std::uint32_t exit;
        switch(outcome) {
        case Outcome::parent_decides:exit=0x1ae66bu;break;
        case Outcome::failed:exit=0x1ae6d7u;break;
        default:exit=0x1ae6dfu;break;
        }
        gp[esi]=take(exit);gp[ebp]=take(exit+1u);
        if(outcome==Outcome::parent_decides){gp[ebx]=take(exit+2u);this->ret(exit+3u,0xcu);this->publish(exit+3u);return;}
        if(outcome==Outcome::failed)set_low_byte(gp[eax],this->flags.logic(0u,1));                     // XOR AL,AL
        else set_low_byte(gp[eax],1u);                                                                 // MOV AL,1
        gp[ebx]=take(exit+4u);this->ret(exit+5u,0xcu);this->publish(exit+5u);
    }
};
template<class State,class Reference,class Parent,class Rider> bool bridge(State &s,Reference reference,Parent parent,Rider rider) {
    return run_owner(s,0x1ae5e0u,0x1ae6e4u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Parent,Rider>(v,c,t,parent,rider);});
}
} // namespace halo2::native_action_target_defer_detail
