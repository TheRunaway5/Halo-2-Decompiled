#pragma once
// Boundary adapter for the kind entry, entered at 11E060
// (native_tu15_kind_entry_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: ESI the owner, EDX the kind; the entry in EAX;
// ECX the index (shifted and added for a selected entry) and EDX the count
// as the original leaves them; the flags of the last instruction that sets
// them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_KIND_ENTRY=1 restores the translated body.
// HALO2_VERIFY_KIND_ENTRY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_kind_entry_systems.hpp"

namespace halo2::native_kind_entry_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_kind_entry;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_KIND_ENTRY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_KIND_ENTRY");return p && *p;}();
    return value;
}
enum Branch : unsigned {other_kind, first_index, second_index, selected, first, empty, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Kind entry verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct KindEntry : Port<State> {
    using Access=logic::kind_entry::Access;
    KindEntry(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case kind_case:return 0x11e069u;
        case count:return 0x11e079u;
        case entries:return 0x11e083u;
        case first_entry:return 0x11e095u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        if(a==count){gp[edx]=v;this->flags.minus(gp[ecx],v,4);}                                // MOV EDX; CMP ECX,EDX
        else if(a==first_entry)gp[eax]=v;
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // XOR ECX,ECX (entry 0) or MOV ECX,1 (entry 1).
    void selected(std::uint32_t index) {
        if(index==0)this->gp[ecx]=this->flags.logic(0u,4);else this->gp[ecx]=index;
        this->reached(index==0?Branch::first_index:Branch::second_index);
    }
    [[noreturn]] void unknown_case(){throw std::runtime_error("Unbound TU1.5 contact dispatch target");}
    void run() {
        auto &gp=this->gp;
        // XOR EAX,EAX; XOR ECX,ECX; CMP EDX,3.
        gp[eax]=this->flags.logic(0u,4);gp[ecx]=this->flags.logic(0u,4);this->flags.minus(gp[edx],3u,4);
        if(gp[edx]>=logic::table_kinds)this->reached(Branch::other_kind);
        Memory<KindEntry> memory{*this};
        const auto result=logic::kind_entry::of(memory,*this,gp[esi],gp[edx]);
        using logic::kind_entry::Outcome;
        std::uint32_t exit=0x11e09bu;
        switch(result.outcome) {
        case Outcome::selected:
            // SHL ECX,4; ADD ECX,EAX (the list); MOV EAX,ECX.
            this->reached(Branch::selected);
            (void)this->flags.plus(this->flags.shift(gp[ecx],4u,4),result.entry-logic::entry_size*result.index,4);
            gp[ecx]=result.entry;gp[eax]=result.entry;exit=0x11e090u;break;
        case Outcome::first:this->reached(Branch::first);this->test32(gp[edx]);break;   // TEST EDX,EDX (EAX the first entry)
        case Outcome::empty:this->reached(Branch::empty);this->test32(gp[edx]);break;
        }
        this->ret(exit,0);this->publish(exit);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x11e060u,0x11e09bu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return KindEntry<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_kind_entry_detail
