#pragma once
// Boundary adapter for the cluster object query, entered at BD460
// (native_tu15_cluster_objects_systems.hpp). Its caller (the weapon
// targeting search 1AE090, combat action unit) is still translated, so the
// adapter owns that ABI and nothing else:
//   - [ESP+4] the partition flags, [ESP+8] the cluster count, [ESP+C] the
//     cluster list, [ESP+10] the capacity, [ESP+14] the output; RET 14;
//   - the frame: the flags are read first; SUB ESP,C, then EBX, EBP, ESI
//     saved, EBP zeroed (the found count lives in it), EDI saved; the locals
//     are [ESP+10..+18];
//   - the registers the walk leaves for the caller: EAX = the last pool or
//     flags load with AX = the count found (MOV AX,BP), ECX and EDX as the
//     original leaves them (the list position, cluster, entry handles, the
//     next entry's address, the mark index, the output); EDI and ESI are
//     restored before the active byte is cleared, EBX after it; ADD ESP,C
//     decides the flags;
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_CLUSTER_OBJECTS=1 restores the translated body.
// HALO2_VERIFY_CLUSTER_OBJECTS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_cluster_objects_systems.hpp"

namespace halo2::native_cluster_objects_detail {
using native_object_tick_detail::Trace;
using native_object_tick_detail::sx16;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_cluster_objects;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_CLUSTER_OBJECTS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_CLUSTER_OBJECTS");return p && *p;}();
    return value;
}
// Branch outcomes (verification coverage), read from the order of the
// original instructions.
enum Branch : unsigned {
    flags_zero, flags_given, no_clusters, clusters, collideable_wanted, collideable_skipped, collideable_empty,
    collideable_first, owner_none, owner_some, unmarked, marked, full, appended, next_none, next_some,
    noncollideable_wanted, noncollideable_skipped, noncollideable_empty, noncollideable_first, other_owner_none,
    other_owner_some, other_unmarked, other_marked, other_full, other_appended, other_next_none, other_next_some,
    more_clusters, last_cluster, branch_count
};
static_assert(branch_count<=64u);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Cluster objects verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

// Coverage edges: an access at `to` right after the access at `from`.
inline constexpr Edge query_edges[]{
    {0xbd46eu,0xbd471u,flags_zero},{0xbd46eu,0xbd479u,flags_given},
    {0xbd48du,0xbd67du,no_clusters},{0xbd48du,0xbd497u,clusters},
    {0xbd4bdu,0xbd4cau,collideable_wanted},{0xbd4bdu,0xbd588u,collideable_skipped},
    {0xbd4d0u,0xbd588u,collideable_empty},{0xbd4d0u,0xbd4d8u,collideable_first},
    {0xbd504u,0xbd588u,owner_none},{0xbd504u,0xbd511u,owner_some},{0xbd57bu,0xbd588u,owner_none},{0xbd57bu,0xbd511u,owner_some},
    {0xbd51fu,0xbd528u,unmarked},{0xbd51fu,0xbd54fu,marked},{0xbd51fu,0xbd588u,marked},
    {0xbd52du,0xbd67du,full},{0xbd52du,0xbd53au,appended},
    {0xbd544u,0xbd588u,next_none},{0xbd51fu,0xbd588u,next_none},{0xbd544u,0xbd54fu,next_some},{0xbd51fu,0xbd54fu,next_some},
    {0xbd588u,0xbd593u,noncollideable_wanted},{0xbd588u,0xbd669u,noncollideable_skipped},
    {0xbd59eu,0xbd669u,noncollideable_empty},{0xbd59eu,0xbd5a6u,noncollideable_first},
    {0xbd5d8u,0xbd669u,other_owner_none},{0xbd5d8u,0xbd5f0u,other_owner_some},
    {0xbd65cu,0xbd669u,other_owner_none},{0xbd65cu,0xbd5f0u,other_owner_some},
    {0xbd5feu,0xbd607u,other_unmarked},{0xbd5feu,0xbd62au,other_marked},{0xbd5feu,0xbd669u,other_marked},
    {0xbd60cu,0xbd67du,other_full},{0xbd60cu,0xbd615u,other_appended},
    {0xbd61fu,0xbd669u,other_next_none},{0xbd5feu,0xbd669u,other_next_none},
    {0xbd61fu,0xbd62au,other_next_some},{0xbd5feu,0xbd62au,other_next_some},
    {0xbd673u,0xbd4a9u,more_clusters},{0xbd673u,0xbd67du,last_cluster},
};
template<class State> struct Query : Port<State> {
    using Access=logic::cluster_objects::Access;
    using Local=logic::cluster_objects::Local;
    std::uint32_t entry_sp{};bool saved{};
    Query(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,query_edges){}
    // SUB ESP,C; PUSH EBX, EBP, ESI; XOR EBP,EBP; PUSH EDI.
    void save_registers() {
        if(saved)return;
        saved=true;auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],0xcu,4);
        this->push(0xbd467u,gp[ebx]);this->push(0xbd468u,gp[ebp]);this->push(0xbd469u,gp[esi]);
        gp[ebp]=0;this->push(0xbd46eu,gp[edi]);
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case flags_argument:return 0xbd460u;
        case flags_all:save_registers();return 0xbd471u;
        case stamp_read:save_registers();return 0xbd479u;
        case stamp_store:return 0xbd479u;
        case count_check:gp[ecx]=0;return 0xbd481u;                       // XOR ECX,ECX
        case active_set:return 0xbd486u;
        case position_clear:return 0xbd48du;
        case flags_read:return 0xbd497u;
        case collideable_pool:return 0xbd49bu;
        case collideable_flag_store:return 0xbd4a3u;
        case collideable_flag_read:return 0xbd4a9u;
        case cluster_list:return 0xbd4b2u;
        case cluster_index:return 0xbd4b9u;
        case cluster_store:return 0xbd4bdu;
        case collideable_heads:gp[edx]=gp[ecx];return 0xbd4cau;             // MOVSX EDX,CX
        case collideable_head:return 0xbd4d0u;
        case first_size:return 0xbd4d8u;
        case first_data:return 0xbd4dbu;
        case first_next:return 0xbd4ebu;
        case first_owner:return 0xbd504u;
        case stamp_now:return 0xbd511u;
        case mark:gp[edx]=gp[esi]&0xffffu;return 0xbd51fu;
        case capacity:return 0xbd528u;
        case mark_store:return 0xbd52du;
        case output_argument:return 0xbd53au;
        case output_store:gp[edx]=sx16(gp[ebp]);return 0xbd541u;
        case pool_after_append:return 0xbd544u;
        case next_size:return 0xbd54fu;
        case next_data:return 0xbd552u;
        case next_next:return 0xbd562u;
        case next_owner:return 0xbd57bu;
        case noncollideable_wanted:return 0xbd588u;
        case cluster_reload:return 0xbd593u;
        case noncollideable_heads:return 0xbd598u;
        case noncollideable_head:return 0xbd59eu;
        case other_first_pool:return 0xbd5a6u;
        case other_first_size:return 0xbd5acu;
        case other_first_data:return 0xbd5afu;
        case other_first_next:return 0xbd5bfu;
        case other_first_owner:return 0xbd5d8u;
        case other_stamp_now:return 0xbd5f0u;
        case other_mark:gp[edx]=gp[esi]&0xffffu;return 0xbd5feu;
        case other_capacity:return 0xbd607u;
        case other_mark_store:return 0xbd60cu;
        case other_output_argument:return 0xbd615u;
        case other_output_store:gp[eax]=sx16(gp[ebp]);return 0xbd61cu;
        case other_pool_after_append:return 0xbd61fu;
        case other_next_pool:return 0xbd62au;
        case other_next_size:return 0xbd630u;
        case other_next_data:return 0xbd633u;
        case other_next_next:return 0xbd643u;
        case other_next_owner:return 0xbd65cu;
        case position_read:return 0xbd669u;
        case count_bound:return 0xbd66eu;
        case position_store:return 0xbd673u;
        // POP EDI, POP ESI, MOV AX,BP, POP EBP, then the active byte.
        case active_clear:
            gp[edi]=this->pop(0xbd67du);gp[esi]=this->pop(0xbd67eu);
            gp[eax]=(gp[eax]&0xffff0000u)|(gp[ebp]&0xffffu);
            gp[ebp]=this->pop(0xbd682u);
            return 0xbd683u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case flags_argument:case collideable_pool:case output_argument:gp[eax]=v;break;
        case pool_after_append:case other_pool_after_append:gp[eax]=v;++gp[ebp];break;
        case flags_read:case collideable_flag_read:case noncollideable_heads:case other_first_pool:case other_next_pool:
        case other_output_argument:gp[edx]=v;break;
        case cluster_index:case cluster_reload:gp[ecx]=sx16(v);break;
        case collideable_heads:case collideable_head:case noncollideable_head:case position_read:gp[ecx]=v;break;
        case first_size:case next_size:case other_first_size:case other_next_size:gp[esi]=v;break;
        case first_data:case next_data:case other_first_data:case other_next_data:gp[edi]=v;break;
        // The next entry's handle; its address goes to EDX (the prefetch).
        case first_next:case next_next:case other_first_next:case other_next_next:
            gp[ecx]=v;if(v!=logic::none)gp[edx]=(v&0xffffu)*gp[esi]+gp[edi];break;
        case first_owner:case next_owner:case other_first_owner:case other_next_owner:gp[esi]=v;break;
        case stamp_now:case other_stamp_now:gp[edi]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        if(a==collideable_flag_store)gp[edx]=v;
        else if(a==position_store)gp[ecx]=v;
    }
    // Frame. E is the stack pointer at entry; after SUB ESP,C and the four
    // saves the locals are [ESP+10..+18] (E-C..E-4), the arguments [E+4..+14].
    std::uint32_t local(Local l)const {
        switch(l) {
        case Local::position:return entry_sp-0xcu;
        case Local::collideable_wanted:return entry_sp-0x8u;
        case Local::cluster:return entry_sp-0x4u;
        case Local::flags:return entry_sp+0x4u;
        case Local::cluster_count:return entry_sp+0x8u;
        case Local::clusters:return entry_sp+0xcu;
        case Local::capacity:return entry_sp+0x10u;
        case Local::output:return entry_sp+0x14u;
        }
        return 0;
    }
    void run() {
        auto &gp=this->gp;
        entry_sp=gp[esp];
        Memory<Query> memory{*this};
        (void)logic::cluster_objects::query(memory,*this);
        gp[ebx]=this->pop(0xbd68au);
        gp[esp]=this->flags.plus(gp[esp],0xcu,4);                                   // ADD ESP,C
        this->ret(0xbd68eu,0x14u);this->publish(0xbd68eu);
    }
};

template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xbd460u,0xbd68eu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Query<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_cluster_objects_detail
