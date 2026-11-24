#pragma once
// Boundary adapter for the animation select, entered at 1E5E40
// (native_tu15_animation_select_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: BL the forced byte, the
// graph, three keys and the flag on the stack (RET 14); EBP and, past the
// first two lookups, ESI and EDI saved; 1E5810 gets the table, the key and
// the stride pushed (and pops them), 126200 EAX the item and CL 1 or 0; every
// register each returns comes back; EAX, ECX and EDX as the original leaves
// them; the flags of the last instruction that sets them. Each access
// publishes its original instruction.
// HALO2_REFERENCE_ANIMATION_SELECT=1 restores the translated body.
// HALO2_VERIFY_ANIMATION_SELECT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_animation_select_systems.hpp"

namespace halo2::native_animation_select_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_animation_select;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ANIMATION_SELECT");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ANIMATION_SELECT");return p && *p;}();
    return value;
}
enum Branch : unsigned {skipped, unmatched, found, refreshed_on, refreshed_off, branch_count};
static_assert(unsigned(logic::animation_select::Outcome::found)==Branch::found);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Animation select verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
struct LookupSites {std::uint32_t stride,key,table,call;};
inline constexpr LookupSites lookups[]{{0x1e5e59u,0x1e5e5bu,0x1e5e5fu,0x1e5e60u},{0x1e5e71u,0x1e5e73u,0x1e5e77u,0x1e5e78u},
    {0x1e5e8au,0x1e5e8cu,0x1e5e90u,0x1e5e91u}};
struct ListSites {std::uint32_t count,list,index,items,in_use,call,count_again;};
inline constexpr ListSites list_sites[]{{0x1e5ea1u,0x1e5eb0u,0x1e5eb3u,0x1e5eb6u,0x1e5ec2u,0x1e5ecbu,0x1e5ed0u},
    {0x1e5ee0u,0x1e5ef0u,0x1e5ef3u,0x1e5ef6u,0x1e5f02u,0x1e5f0bu,0x1e5f10u}};
template<class State,class Lookup,class Refresh> struct AnimationSelect : Port<State> {
    using Access=logic::animation_select::Access;
    using Kept=logic::animation_select::Kept;
    Lookup &lookup_child;
    Refresh &refresh_child;
    std::uint32_t depth{4u},which{},saves{};
    AnimationSelect(State &s,std::uint32_t *c,Trace<State> *t,Lookup &l,Refresh &r)
        :Port<State>(s,c,t,{}),lookup_child(l),refresh_child(r){}
    std::uint32_t argument_slot(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        const auto &l=list_sites[which];
        switch(a) {
        case graph:return 0x1e5e43u;
        case flag:return 0x1e5e49u;
        case first_key:return 0x1e5e55u;
        case second_key:return 0x1e5e6du;
        case third_key:return 0x1e5e85u;
        case count:return l.count;
        case list:return l.list;
        case index:return l.index;
        case items:return l.items;
        case in_use:return l.in_use;
        case count_again:return l.count_again;
        case off_flag:return 0x1e5ed8u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case graph:gp[ebp]=v;break;
        case flag:case off_flag:set_low_byte(gp[eax],v);this->test8(v);break;                     // MOV AL,[ESP+..]; TEST AL,AL
        case first_key:gp[eax]=v;break;
        case second_key:gp[edx]=v;break;
        case third_key:gp[ecx]=v;break;
        case count:gp[eax]=v;gp[edi]=fl.logic(0u,4);this->test32(v);break;                          // MOV EAX; XOR EDI,EDI; TEST EAX,EAX
        case list:gp[edx]=v;break;
        case index:gp[eax]=v;break;
        case items:gp[ecx]=v;gp[eax]=v+4u*(gp[eax]*5u);break;                                       // MOV ECX; LEA EAX; LEA EAX
        case in_use:set_low_byte(gp[ecx],v);this->test8(v);break;                                  // MOV CL,[EAX+10]; TEST CL,CL
        case count_again:gp[eax]=v;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH stride; PUSH key; (LEA ECX,[EBP+34] or ADD EAX,4); PUSH table; CALL 1E5810; TEST EAX,EAX (or MOV ESI,EAX; TEST ESI,ESI).
    std::uint32_t lookup(Kept &kept,std::uint32_t k,std::uint32_t table,std::uint32_t key,std::uint32_t stride) {
        auto &gp=this->gp;auto &fl=this->flags;
        const auto &site=lookups[k];
        this->push(site.stride,stride);this->push(site.key,key);
        if(k==0)gp[ecx]=table;else gp[eax]=fl.plus(gp[eax],logic::link,4);
        this->push(site.table,table);
        this->call(site.call,site.call+5u,0x1e5810u,lookup_child);
        if(k==2)gp[esi]=gp[eax];
        this->test32(gp[eax]);
        kept={gp[ebp],gp[esi],gp[edi],gp[ebx]&0xffu};
        return gp[eax];
    }
    // PUSH ESI (before the third lookup); TEST BL,BL; PUSH EDI.
    void saved(std::uint32_t k) {
        auto &gp=this->gp;
        if(k==0){this->push(0x1e5e89u,gp[esi]);depth=8u;++saves;}
        else{this->test8(gp[ebx]);this->push(0x1e5e9eu,gp[edi]);depth=12u;++saves;}
    }
    void listing(std::uint32_t list_which){which=list_which;}
    // MOV CL,1 (or XOR CL,CL); CALL 126200.
    void refresh(Kept &kept,std::uint32_t item,std::uint32_t list_which) {
        auto &gp=this->gp;
        gp[eax]=item;
        if(list_which==0)set_low_byte(gp[ecx],1u);else set_low_byte(gp[ecx],this->flags.logic(0u,1));
        const auto &l=list_sites[list_which];
        this->call(l.call,l.call+5u,0x126200u,refresh_child);
        kept={gp[ebp],gp[esi],gp[edi],gp[ebx]&0xffu};
        this->reached(list_which==0?Branch::refreshed_on:Branch::refreshed_off);
    }
    // INC EDI; CMP EDI,EAX.
    void stepped(const Kept &kept,std::uint32_t count_value) {
        auto &gp=this->gp;
        this->flags.increment(kept.index-1u,4,false);gp[edi]=kept.index;this->flags.minus(kept.index,count_value,4);
    }
    void run() {
        auto &gp=this->gp;
        this->test8(gp[ebx]);this->push(0x1e5e42u,gp[ebp]);                                          // TEST BL,BL; PUSH EBP
        Memory<AnimationSelect> memory{*this};
        const auto outcome=logic::animation_select::select(memory,*this,gp[ebx]);
        this->reached(unsigned(outcome));
        if(saves>=2)gp[edi]=this->pop(0x1e5f18u);
        if(saves>=1)gp[esi]=this->pop(0x1e5f19u);
        gp[ebp]=this->pop(0x1e5f1au);this->ret(0x1e5f1bu,0x14u);this->publish(0x1e5f1bu);
    }
};
template<class State,class Reference,class Lookup,class Refresh> bool bridge(State &s,Reference reference,Lookup lookup,Refresh refresh) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1e5e40u,0x1e5f1bu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return AnimationSelect<State,Lookup,Refresh>(v,c,t,lookup,refresh);});
    return true;
}
} // namespace halo2::native_animation_select_detail
