#pragma once
// Boundary adapter for the animation wildcard search, entered at 2A0100
// (native_tu15_animation_wildcard_systems.hpp). Its callers and the child
// keep the original ABI, which the adapter owns: ESI the search, EBX the
// third output, the first and second outputs on the stack (RET 8); EBP and
// EDI saved; 1E5810 gets the table, the key and the stride pushed (and pops
// them), every register it returns coming back; EAX the entry found, ECX
// and EDX as the original leaves them; the flags of the closing CMP (the
// stage against 8, or the entry against the none). The unit's copy decides
// the second key's symbol (the physics live support copy asks the MCC
// class symbol scope, the AI copies pass it as it is): the bridge's
// `symbol`. Each access publishes its original instruction.
// HALO2_REFERENCE_ANIMATION_WILDCARD=1 restores the translated body.
// HALO2_VERIFY_ANIMATION_WILDCARD=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_animation_wildcard_systems.hpp"

namespace halo2::native_animation_wildcard_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_animation_wildcard;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ANIMATION_WILDCARD");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ANIMATION_WILDCARD");return p && *p;}();
    return value;
}
enum Branch : unsigned {exhausted, found, first_missing, second_missing, third_missing, first_repeat, second_repeat, third_repeat,
    second_kept, branch_count};
static_assert(unsigned(logic::wildcard_search::Outcome::found)==Branch::found);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Animation wildcard verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
// A level's choice, with its own key ([0]) or the any key ([1]): the key's
// read (into a register, or a CMP), the output's argument and its store.
struct ChoiceSites {std::uint32_t key,slot,store;unsigned key_into,slot_into;};
inline constexpr unsigned none_register=8u;
inline constexpr ChoiceSites choices[3][2]{
    {{0x2a013au,0x2a013du,0x2a0141u,ecx,edx},{0x2a0125u,0x2a012eu,0x2a0132u,none_register,eax}},
    {{0x2a0186u,0x2a0189u,0x2a018du,ecx,edx},{0x2a0171u,0x2a017au,0x2a017eu,none_register,eax}},
    {{0x2a01c3u,0u,0x2a01c6u,eax,none_register},{0x2a01b2u,0u,0x2a01bbu,none_register,none_register}}};
// A level's lookup: the output's argument, its key, the table's base, the
// pushes, the table's ADD and the CALL.
struct LookupSites {std::uint32_t slot,key,table,stride,key_push,add,table_push,call;};
inline constexpr LookupSites lookups[3]{
    {0x2a0143u,0x2a0147u,0x2a0149u,0x2a014bu,0x2a014du,0x2a014eu,0x2a0151u,0x2a0152u},
    {0x2a018fu,0x2a0193u,0x2a0195u,0x2a0198u,0x2a019au,0x2a019bu,0x2a019eu,0x2a019fu},
    {0u,0x2a01c8u,0x2a01cau,0x2a01cdu,0x2a01cfu,0x2a01d0u,0x2a01d3u,0x2a01d4u}};
struct AdvanceSites {std::uint32_t read,store,second_clear,first_clear;};
inline constexpr AdvanceSites advances[3]{{0x2a0209u,0x2a0212u,0x2a0215u,0u},{0x2a01f4u,0x2a01ffu,0u,0x2a0204u},
    {0x2a01dbu,0x2a01e3u,0x2a01eau,0x2a01efu}};
template<class State,class Lookup,class Symbol> struct AnimationWildcard : Port<State> {
    using Access=logic::wildcard_search::Access;
    using Kept=logic::wildcard_search::Kept;
    using Level=logic::Level;
    Lookup &lookup_child;
    Symbol &symbol;
    Level level{};
    bool any{};
    AnimationWildcard(State &s,std::uint32_t *c,Trace<State> *t,Lookup &l,Symbol &y)
        :Port<State>(s,c,t,{}),lookup_child(l),symbol(y){}
    const ChoiceSites &choice()const{return choices[unsigned(level)][any?1:0];}
    const LookupSites &lookup_sites()const{return lookups[unsigned(level)];}
    const AdvanceSites &advance_sites()const{return advances[unsigned(level)];}
    // [ESP+C] and [ESP+10] past the saves.
    std::uint32_t slot(Level which)const{return this->gp[esp]+(which==Level::first?0xcu:0x10u);}
    void choosing(Level which,bool any_key){level=which;any=any_key;}
    void advancing(Level which) {
        level=which;
        if(which!=Level::third)this->reached(which==Level::first?Branch::first_missing:Branch::second_missing);
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case stage:return 0x2a0110u;
        case first_cached:return 0x2a011cu;
        case first_store:return 0x2a0157u;
        case second_reset:return 0x2a015au;
        case first_found:return 0x2a015du;
        case second_cached:return 0x2a0166u;
        case second_bits:return 0x2a016bu;
        case third_bits:return 0x2a01acu;
        case second_store:return 0x2a01a4u;
        case second_found:return 0x2a01a7u;
        case key:return choice().key;
        case out_slot:return choice().slot;
        case out_store:return choice().store;
        case lookup_slot:return lookup_sites().slot;
        case lookup_key:return lookup_sites().key;
        case lookup_table:return lookup_sites().table;
        case advance_read:return advance_sites().read;
        case advance_store:return advance_sites().store;
        case second_clear:return advance_sites().second_clear;
        case first_clear:return advance_sites().first_clear;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case stage:gp[eax]=v;this->compare32(v,logic::stages);break;                              // MOV EAX; CMP EAX,8
        case second_cached:if(v!=gp[edi])this->reached(Branch::second_kept);break;
        case key:
            if(choice().key_into!=none_register)gp[choice().key_into]=v;
            if(any && v==logic::any_key)this->reached(Branch::first_repeat+unsigned(level));
            break;
        case out_slot:gp[choice().slot_into]=v;break;
        case lookup_slot:gp[eax]=v;break;
        case lookup_key:gp[ecx]=v;break;
        case lookup_table:gp[edx]=v;break;
        case advance_read:if(level==Level::third)gp[edx]=v;else gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        // ADD EAX,4 / 2; AND EAX (or INC EDX; MOV EAX,EDX).
        if(a!=Access::advance_store)return;
        if(level==Level::third)this->gp[edx]=v;
        this->gp[eax]=v;
    }
    std::uint32_t class_symbol(std::uint32_t key_value){return symbol(this->s,key_value);}
    // PUSH stride; PUSH key; ADD EDX,offset; PUSH EDX; CALL 1E5810 (MOV EBP,EAX after the third).
    std::uint32_t lookup(Kept &kept,Level which,std::uint32_t table,std::uint32_t key_value,std::uint32_t stride) {
        auto &gp=this->gp;
        level=which;
        const auto &site=lookup_sites();
        this->push(site.stride,stride);this->push(site.key_push,key_value);
        gp[edx]=this->flags.plus(gp[edx],table-gp[edx],4);this->push(site.table_push,gp[edx]);
        this->call(site.call,site.call+5u,0x1e5810u,lookup_child);
        if(which==Level::third) {
            if(gp[eax]==gp[edi])this->reached(Branch::third_missing);
            gp[ebp]=gp[eax];
        }
        kept={gp[esi],gp[ebx],gp[edi],gp[ebp]};
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        using Outcome=logic::wildcard_search::Outcome;
        this->push(0x2a0100u,gp[ebp]);this->push(0x2a0101u,gp[edi]);
        gp[edi]=fl.logic(0u,4);gp[ebp]=fl.logic(0u,4);                                            // XOR EDI,EDI; XOR EBP,EBP
        Kept kept{gp[esi],gp[ebx],0u,0u};
        Memory<AnimationWildcard> memory{*this};
        const auto outcome=logic::wildcard_search::next(memory,*this,kept);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::found)this->compare32(gp[ebp],gp[edi]);                             // CMP EBP,EDI
        gp[edi]=this->pop(0x2a0220u);gp[eax]=gp[ebp];gp[ebp]=this->pop(0x2a0223u);
        this->ret(0x2a0224u,8u);this->publish(0x2a0224u);
    }
};
template<class State,class Reference,class Lookup,class Symbol>
bool bridge(State &s,Reference reference,Lookup lookup,Symbol symbol) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x2a0100u,0x2a0224u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return AnimationWildcard<State,Lookup,Symbol>(v,c,t,lookup,symbol);});
    return true;
}
} // namespace halo2::native_animation_wildcard_detail
