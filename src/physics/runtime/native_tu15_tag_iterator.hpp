#pragma once
// Boundary adapter for the tag iterator, entered at 125790
// (native_tu15_tag_iterator_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: EDI the iterator; EAX the result;
// EBX (NONE while it runs) and ESI (the instance, once the instances are
// stepped) saved; 125810 (through the unit's route) gets ESI the instance
// and ECX its group, and every register it returns comes back; the flags
// of the last comparison; each access publishes its original instruction.
// HALO2_REFERENCE_TAG_ITERATOR=1 restores the translated body.
// HALO2_VERIFY_TAG_ITERATOR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_tag_iterator_systems.hpp"

namespace halo2::native_tag_iterator_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_tag_iterator;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_TAG_ITERATOR");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_TAG_ITERATOR");return p && *p;}();
    return value;
}
enum Branch : unsigned {exhausted, stepping, empty_instance, no_group, no_handle, any_group, group_test, group_match,
    group_miss, next_instance, end, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Tag iterator verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge iterator_edges[]{
    {0x12579cu,0x125803u,exhausted},{0x12579cu,0x1257a4u,stepping},{0x1257b8u,0x1257e3u,empty_instance},
    {0x1257bdu,0x1257e3u,no_group},{0x1257c3u,0x1257e3u,no_handle},{0x1257c8u,0x1257f5u,any_group},
    {0x1257c8u,0x1257cdu,group_test},{0x1257d5u,0x1257f5u,group_match},{0x1257d9u,0x1257f5u,group_match},
    {0x1257deu,0x1257f5u,group_match},{0x1257deu,0x1257e3u,group_miss},
    {0x1257ebu,0x1257a5u,next_instance},{0x1257ebu,0x1257f0u,end},
};
template<class State,class Route> struct Iterator : Port<State> {
    using Access=logic::tag_iterator::Access;
    using Kept=logic::tag_iterator::Kept;
    Route &route;
    Iterator(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,iterator_edges),route(r){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case tags_first:return 0x125790u;
        case index_first:return 0x125796u;
        case count_first:return 0x125799u;
        case index:return 0x1257a5u;
        case instances:return 0x1257a8u;
        case index_store:return 0x1257b8u;
        case group:return 0x1257bdu;
        case handle:return 0x1257c3u;
        case wanted_group:return 0x1257c8u;
        case wanted_again:return 0x1257d2u;
        case group_0:return 0x1257d5u;
        case group_1:return 0x1257d9u;
        case group_2:return 0x1257deu;
        case tags:return 0x1257e3u;
        case index_again:return 0x1257e8u;
        case count:return 0x1257ebu;
        case found_handle:return 0x1257f5u;
        case current_store:return 0x1257f8u;
        case found_again:return 0x1257fbu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case tags_first:gp[ecx]=v;break;
        case index_first:case index:case tags:case found_again:gp[eax]=v;break;
        // PUSH EBX; OR EBX,-1; CMP EAX,EDX (and PUSH ESI once stepping).
        case count_first:
            gp[edx]=v;this->push(0x12579cu,gp[ebx]);gp[ebx]=this->flags.logic(gp[ebx]|logic::none,4);
            this->compare32(gp[eax],v);
            if(std::int32_t(gp[eax])<std::int32_t(v))this->push(0x1257a4u,gp[esi]);
            break;
        // MOV ESI,EAX; SHL ESI,4; ADD ESI,ECX; LEA EAX,[EAX+1].
        case instances:
            gp[ecx]=v;gp[esi]=this->flags.plus(this->flags.shift(gp[eax],logic::instance_shift,4),v,4);gp[eax]+=1u;break;
        case group:gp[ecx]=v;this->compare32(v,gp[ebx]);break;
        case handle:case wanted_group:this->compare32(v,gp[ebx]);break;
        case wanted_again:case found_handle:gp[ecx]=v;break;
        case group_0:case group_1:case group_2:this->compare32(gp[ecx],v);break;
        case index_again:gp[edx]=v;break;
        case count:this->compare32(gp[edx],v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    std::uint32_t group_groups(Kept &kept) {
        this->call_routed(route,0x1257cdu,0x1257d2u,0x125810u);
        kept={this->gp[edi],this->gp[esi],this->gp[ebx]};
        return this->gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        Memory<Iterator> memory{*this};
        using logic::tag_iterator::Exit;
        const auto result=logic::tag_iterator::next(memory,*this,gp[edi]);
        if(result.exit==Exit::exhausted) {
            gp[eax]=gp[ebx];gp[ebx]=this->pop(0x125803u);this->ret(0x125804u,0);this->publish(0x125804u);
        } else if(result.exit==Exit::ended) {
            gp[esi]=this->pop(0x1257f0u);gp[eax]=gp[ebx];gp[ebx]=this->pop(0x1257f3u);
            this->ret(0x1257f4u,0);this->publish(0x1257f4u);
        } else {
            gp[esi]=this->pop(0x1257feu);gp[ebx]=this->pop(0x1257ffu);this->ret(0x125800u,0);this->publish(0x125800u);
        }
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x125790u,0x125804u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Iterator<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_tag_iterator_detail
