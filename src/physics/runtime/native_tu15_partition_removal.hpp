#pragma once
// Boundary adapter for the cluster partition removal, entered at 1D30F0
// (native_tu15_partition_removal_systems.hpp). Its callers and the child
// 16E8A0 keep the original ABI, which the adapter owns:
//   - EDI the partition; [ESP+4] the object, [ESP+8] the head pointer;
//     RET 8; EBP saved first, EBX/ESI once there are references; the
//     arguments are addressed from the live ESP, so a child that leaves the
//     stack unbalanced moves them as it does for the original;
//   - 16E8A0 (through the unit's route) gets EAX the handle and EDX the
//     pool, with ECX/ESI as the original leaves them; every register it
//     returns comes back to the removal;
//   - flags: the CMP that decided the last branch;
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_PARTITION_REMOVAL=1 restores the translated body.
// HALO2_VERIFY_PARTITION_REMOVAL=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_partition_removal_systems.hpp"

namespace halo2::native_partition_removal_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_partition_removal;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PARTITION_REMOVAL");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PARTITION_REMOVAL");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    no_references, references, unchained, chained, owned, not_owned, next_entry, chain_end, next_reference,
    last_reference, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Partition removal verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

inline constexpr Edge removal_edges[]{
    {0x1d30fau,0x1d3176u,no_references},{0x1d30fau,0x1d3100u,references},
    {0x1d3131u,0x1d3167u,unchained},{0x1d3131u,0x1d3136u,chained},
    {0x1d314cu,0x1d315bu,owned},{0x1d314cu,0x1d3154u,not_owned},
    {0x1d3154u,0x1d3140u,next_entry},{0x1d3154u,0x1d3167u,chain_end},
    {0x1d3167u,0x1d3102u,next_reference},{0x1d3167u,0x1d3170u,last_reference},
};
template<class State,class Route> struct Removal : Port<State> {
    using Access=logic::partition_removal::Access;
    using Local=logic::partition_removal::Local;
    using Kept=logic::partition_removal::Kept;
    Route &route;
    // E: where the return address sits; the walk's ESP is E-C.
    std::uint32_t frame{};bool saved{};
    Removal(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,removal_edges),route(r){}
    std::uint32_t local(Local l)const{return l==Local::object?frame+4u:frame+8u;}
    std::uint32_t partition()const{return this->gp[edi];}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case head_argument:return 0x1d30f0u;
        case head:return 0x1d30f4u;
        case object_argument:this->push(0x1d30f9u,gp[ebp]);return 0x1d30fau;
        case references_pool:
            if(!saved){saved=true;this->push(0x1d3100u,gp[ebx]);this->push(0x1d3101u,gp[esi]);}
            return 0x1d3102u;
        case references_data:return 0x1d3105u;
        case references_size:gp[ecx]=this->flags.logic(gp[eax]&0xffffu,4);return 0x1d3110u;   // MOV ECX,EAX; AND
        case reference_cluster:return 0x1d3116u;
        case reference_next:return 0x1d311au;
        case next_store:return 0x1d311du;
        case heads:return 0x1d3126u;
        // MOVSX EDX,SI; LEA ESI,[EAX+EDX*4].
        case link_check:gp[edx]=logic::widen16(gp[esi]);gp[esi]=gp[eax]+gp[edx]*4u;return 0x1d312eu;
        case entries_pool:return 0x1d3131u;
        case entries_size:return 0x1d3136u;
        case entries_data:return 0x1d3139u;
        case link_read:return 0x1d3140u;
        case entry_owner:return 0x1d314cu;
        case link_next_check:gp[esi]=gp[eax]+8u;return 0x1d3154u;                              // LEA ESI,[EAX+8]
        case entry_next:return 0x1d315bu;
        case link_handle:return 0x1d315eu;
        case unlink:return 0x1d3165u;
        case next_reload:return 0x1d3167u;
        case head_reload:return 0x1d3170u;
        case head_clear:return 0x1d3176u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case head_argument:case reference_next:case entries_size:gp[ecx]=v;break;
        case head:case next_reload:gp[eax]=v;this->compare32(v,logic::none);break;
        case object_argument:gp[ebp]=v;break;
        case references_pool:case entries_pool:gp[edx]=v;break;
        case references_data:gp[esi]=v;break;
        case references_size:gp[ecx]=this->flags.plus(this->flags.product(gp[ecx],v),gp[esi],4);break;   // IMUL; ADD ECX,ESI
        case reference_cluster:gp[esi]=(gp[esi]&0xffff0000u)|(v&0xffffu);break;                 // MOV SI,[ECX+4]
        case heads:case link_handle:gp[eax]=v;break;
        case link_check:case link_next_check:this->compare32(v,logic::none);break;
        case entries_data:case entry_next:gp[ebx]=v;break;
        // MOV EAX,[ESI]; AND EAX,FFFF; IMUL EAX,ECX; ADD EAX,EBX.
        case link_read:gp[eax]=this->flags.plus(this->flags.product(this->flags.logic(v&0xffffu,4),gp[ecx]),gp[ebx],4);break;
        case entry_owner:this->compare32(v,gp[ebp]);break;
        // MOV ECX,[ESP+14]; POP ESI; POP EBX.
        case head_reload:gp[ecx]=v;gp[esi]=this->pop(0x1d3174u);gp[ebx]=this->pop(0x1d3175u);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void returned(Kept &kept) {
        frame=this->gp[esp]+0xcu;
        kept={this->gp[ebp],this->gp[edi],this->gp[esi],this->gp[ebx]};
    }
    void release_reference(Kept &kept){this->call_routed(route,0x1d3121u,0x1d3126u,0x16e8a0u);returned(kept);}
    void release_entry(Kept &kept){this->call_routed(route,0x1d3160u,0x1d3165u,0x16e8a0u);returned(kept);}
    void run() {
        auto &gp=this->gp;
        frame=gp[esp];
        Memory<Removal> memory{*this};
        logic::partition_removal::remove(memory,*this);
        gp[ebp]=this->pop(0x1d317cu);
        this->ret(0x1d317du,8u);this->publish(0x1d317du);
    }
};

template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1d30f0u,0x1d317du,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Removal<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_partition_removal_detail
