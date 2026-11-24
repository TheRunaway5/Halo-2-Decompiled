#pragma once
// Boundary adapter for the cluster partition insert, entered at 1D2F10
// (native_tu15_partition_insert_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns:
//   - EAX the cluster set; nine stack arguments, RET 24; SUB ESP,204 makes
//     the frame (the list at [ESP+10] and the remaining count at [ESP+C]
//     once EBX, EBP and EDI are saved; ESI is saved once the list is
//     walked); the arguments and locals are addressed from the live ESP, so
//     a child that leaves the stack unbalanced moves them as it does for the
//     original;
//   - 14D080 gets EBX the set, EDI the remaining local and (256, the list);
//     14D0E0 gets (ECX, center, radius, 256, the list), ECX the location
//     pointer with the cluster in CX; 16E6A0 gets EDX the pool (and ESI the
//     cluster, or the chain's head slot); every register a child returns
//     comes back;
//   - XMM0 the radius where the original loads it; the flags of the last
//     instruction that sets them (ADD ESP,204 at the exit);
//   - the direction flag clear (a set one is refused at admission);
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_PARTITION_INSERT=1 restores the translated body.
// HALO2_VERIFY_PARTITION_INSERT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_partition_insert_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_partition_insert_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_partition_insert;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PARTITION_INSERT");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PARTITION_INSERT");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    with_set, without_set, no_location, located, queried, single, clamped, none_gathered, gathered, reference_made,
    reference_failed, entry_made, entry_failed, no_payload, payload, words, bytes, next_cluster, last_cluster,
    branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Partition insert verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge insert_edges[]{
    {0x1d2f23u,0x1d2f2au,with_set},{0x1d2f23u,0x1d2f3fu,without_set},
    {0x1d2f46u,0x1d2fb8u,no_location},{0x1d2f46u,0x1d2f52u,located},
    {0x1d2f5bu,0x1d2f64u,queried},{0x1d2f5bu,0x1d2fa6u,single},
    {0x1d2fc8u,0x1d3085u,none_gathered},{0x1d2fc8u,0x1d2fd7u,gathered},
    {0x1d2fe6u,0x1d2ff0u,reference_made},{0x1d2fe6u,0x1d3015u,reference_failed},
    {0x1d3021u,0x1d3035u,entry_made},{0x1d3021u,0x1d3072u,entry_failed},
    {0x1d304fu,0x1d3072u,no_payload},{0x1d304fu,0x1d305au,payload},
    {0x1d305au,0x1d3069u,words},{0x1d3069u,0x1d3070u,bytes},{0x1d305au,0x1d3070u,bytes},
    {0x1d307au,0x1d2fe0u,next_cluster},{0x1d307au,0x1d3084u,last_cluster},
};
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr bool backend=logic::sse::available;
template<class State,class Gather,class Query,class Allocate> struct Insert : Port<State> {
    using Access=logic::partition_insert::Access;
    using Local=logic::partition_insert::Local;
    using Kept=logic::partition_insert::Kept;
    Gather &gather_child;Query &query_child;Allocate &allocate_child;
    // The frame's bytes below the return address: the return address sits at
    // ESP+depth.
    std::uint32_t depth{};
    Insert(State &s,std::uint32_t *c,Trace<State> *t,Gather &g,Query &q,Allocate &a)
        :Port<State>(s,c,t,insert_edges),gather_child(g),query_child(q),allocate_child(a){}
    std::uint32_t local(Local l)const {
        const auto entry=this->gp[esp]+depth;
        switch(l) {
        case Local::remaining:return entry-0x204u;
        case Local::list:return entry-0x200u;
        default:return entry+4u*(1u+unsigned(l));
        }
    }
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    std::uint32_t pool()const{return this->gp[edx];}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // SUB ESP,204; PUSH EBX; PUSH EBP.
        case partition_argument:
            gp[esp]=this->flags.minus(gp[esp],0x204u,4);depth=0x204u;
            save(0x1d2f16u,gp[ebx]);save(0x1d2f17u,gp[ebp]);return 0x1d2f18u;
        case set_total:return 0x1d2f39u;
        case location_argument:return 0x1d2f3fu;
        case location_word:return 0x1d2f46u;
        case radius_argument:return 0x1d2f52u;
        case radius_floor:return 0x1d2f5bu;
        case query_count:return 0x1d2f64u;
        case radius_again:return 0x1d2f6au;
        case query_count_store:return 0x1d2f72u;
        // LEA EDX,[ESP+10]; PUSH EDX.
        case center_argument:gp[edx]=local(Local::list);save(0x1d2f7cu,gp[edx]);return 0x1d2f7du;
        case querying_set:return 0x1d2f8cu;
        case querying_clear:return 0x1d2f98u;
        case single_cluster:gp[eax]=1u;return 0x1d2fa6u;
        case overflowed_argument:return 0x1d2fb8u;
        case overflowed_store:return 0x1d2fc8u;
        // MOVZX EAX,AX; LEA EBX,[ESP+10].
        case remaining_init:gp[eax]&=0xffffu;gp[ebx]=local(Local::list);return 0x1d2fd7u;
        case references_pool:return 0x1d2fe0u;
        case cluster_read:return 0x1d2fe3u;
        case references_data:return 0x1d2ff0u;
        case references_size:return 0x1d2ffbu;
        case reference_cluster:return 0x1d3004u;
        case head_argument:return 0x1d3007u;
        case list_head:return 0x1d300eu;
        case reference_next:return 0x1d3010u;
        case list_store:return 0x1d3013u;
        case heads:return 0x1d3015u;
        case entries_pool:return 0x1d3018u;
        // MOV ECX,EAX; AND ECX,FFFF; MOV EDI,ECX.
        case entries_size:gp[ecx]=this->flags.logic(gp[eax]&0xffffu,4);gp[edi]=gp[ecx];return 0x1d3035u;
        case entries_data:return 0x1d3039u;
        case object_argument:return 0x1d303cu;
        case entry_owner:return 0x1d3045u;
        case slot_head:return 0x1d3048u;
        case entry_next:return 0x1d304au;
        case slot_store:return 0x1d304du;
        case payload_argument:return 0x1d304fu;
        case size_argument:return 0x1d305au;
        case payload_word:case payload_word_store:return 0x1d3069u;
        case payload_byte:case payload_byte_store:return 0x1d3070u;
        case remaining_read:return 0x1d3072u;
        case remaining_store:return 0x1d307au;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // MOV EBX,EAX; TEST EBX,EBX; PUSH EDI.
        case partition_argument:
            gp[ebp]=v;gp[ebx]=gp[eax];this->test32(gp[ebx]);save(0x1d2f23u,gp[edi]);break;
        case set_total:case location_argument:case slot_head:case size_argument:gp[ecx]=v;
            // MOV EDX,ECX; SHR ECX,2; ADD EDI,C.
            if(a==size_argument) {
                gp[edx]=v;gp[ecx]=this->flags.right(v,2u,4,false);gp[edi]=this->flags.plus(gp[edi],0xcu,4);
            }
            break;
        // MOV CX,[ECX+4]; XOR EAX,EAX; CMP CX,-1.
        case location_word:
            gp[ecx]=(gp[ecx]&0xffff0000u)|(v&0xffffu);gp[eax]=this->flags.logic(0u,4);
            this->flags.minus(v&0xffffu,logic::no_cluster,2);break;
        case radius_argument:this->s.xmm[0]=lane0_only(v);break;
        case radius_floor:this->flags.scalar_compare(this->s.xmm[0],lane0_only(v),false);break;
        case query_count:case references_pool:case head_argument:case entries_pool:case object_argument:gp[edx]=v;
            // MOVSX EAX,SI; LEA ESI,[ECX+EAX*4].
            if(a==entries_pool){gp[eax]=logic::widen16(gp[esi]);gp[esi]=gp[ecx]+gp[eax]*4u;}
            // ADD EDI,ECX.
            if(a==object_argument)gp[edi]=this->flags.plus(gp[edi],gp[ecx],4);
            break;
        // INC EDX.
        case radius_again:gp[eax]=v;gp[edx]=this->flags.increment(gp[edx],4,false);break;
        // PUSH 100; PUSH EAX; PUSH EDX; PUSH ECX.
        case center_argument:
            gp[edx]=v;save(0x1d2f84u,logic::list_capacity);save(0x1d2f89u,gp[eax]);save(0x1d2f8au,gp[edx]);
            save(0x1d2f8bu,gp[ecx]);break;
        // CMP CX,AX; SETG CL; TEST AX,AX.
        case overflowed_argument: {
            gp[edx]=v;
            const bool greater=std::int16_t(gp[ecx])>std::int16_t(gp[eax]);
            this->flags.minus(gp[ecx]&0xffffu,gp[eax]&0xffffu,2);set_low_byte(gp[ecx],greater?1u:0u);
            this->flags.logic(gp[eax]&0xffffu,2);break;
        }
        case cluster_read:gp[esi]=(gp[esi]&0xffff0000u)|(v&0xffffu);break;
        // MOV ECX,EAX; AND ECX,FFFF.
        case references_data:gp[edi]=v;gp[ecx]=this->flags.logic(gp[eax]&0xffffu,4);break;
        // IMUL ECX,[EDX+24]; MOVSX EDX,SI; ADD ECX,EDI.
        case references_size:
            gp[ecx]=this->flags.product(gp[ecx],v);gp[edx]=logic::widen16(gp[esi]);
            gp[ecx]=this->flags.plus(gp[ecx],gp[edi],4);break;
        case list_head:gp[edi]=v;break;
        case heads:case entries_data:gp[ecx]=v;break;
        case entries_size:gp[edi]=this->flags.product(gp[edi],v);break;
        case payload_argument:gp[esi]=v;this->test32(v);break;
        // MOV EAX,[ESP+10]; ADD EBX,2; DEC EAX.
        case remaining_read:gp[eax]=v;gp[ebx]=this->flags.plus(gp[ebx],2u,4);gp[eax]=this->flags.increment(v,4,true);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case remaining_init:save(0x1d2fdbu,gp[esi]);break;                                   // PUSH ESI
        case payload_word_store:gp[esi]+=4u;gp[edi]+=4u;--gp[ecx];break;
        case payload_byte_store:++gp[esi];++gp[edi];--gp[ecx];break;
        default:break;
        }
    }
    // LEA EAX,[ESP+10]; PUSH EAX; PUSH 100; LEA EDI,[ESP+14]; CALL 14D080.
    std::uint32_t gather_set(Kept &kept) {
        auto &gp=this->gp;
        gp[eax]=local(Local::list);save(0x1d2f2au,gp[eax]);save(0x1d2f2bu,logic::list_capacity);
        gp[edi]=local(Local::remaining);
        this->call(0x1d2f34u,0x1d2f39u,0x14d080u,gather_child);depth-=8u;
        returned(kept);
        return gp[eax];
    }
    std::uint32_t query(Kept &kept) {
        this->call(0x1d2f93u,0x1d2f98u,0x14d0e0u,query_child);depth-=0x14u;
        returned(kept);
        return this->gp[eax];
    }
    // CMP AX,100; MOV ECX,EAX; (MOV EAX,100).
    void clamped(const logic::partition_insert::Gathered &g) {
        this->flags.minus(g.total&0xffffu,logic::list_capacity,2);
        this->gp[ecx]=g.total;this->gp[eax]=g.count;
        if(g.count!=g.total)this->reached(Branch::clamped);
    }
    void returned(Kept &kept)const{kept={this->gp[ebp],this->gp[ebx],this->gp[esi]};}
    std::uint32_t allocate(Kept &kept,std::uint32_t pc) {
        this->call(pc,pc+5u,0x16e6a0u,allocate_child);
        returned(kept);this->compare32(this->gp[eax],logic::none);
        return this->gp[eax];
    }
    std::uint32_t allocate_reference(Kept &kept){return allocate(kept,0x1d2fe6u);}
    std::uint32_t allocate_entry(Kept &kept){return allocate(kept,0x1d3021u);}
    // MOV ECX,EDX; AND ECX,3.
    void copied_words(){this->gp[ecx]=this->flags.logic(this->gp[edx]&3u,4);}
    void exit(std::uint32_t pc) {
        auto &gp=this->gp;
        gp[edi]=this->pop(pc);gp[ebp]=this->pop(pc+1u);gp[ebx]=this->pop(pc+2u);
        gp[esp]=this->flags.plus(gp[esp],0x204u,4);
        this->ret(pc+9u,0x24u);this->publish(pc+9u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Insert> memory{*this};
        if(logic::partition_insert::insert(memory,*this,gp[eax])==logic::partition_insert::Exit::inserted) {
            gp[esi]=this->pop(0x1d3084u);
        }
        exit(0x1d3085u);
    }
};
template<class State,class Reference,class Gather,class Query,class Allocate>
bool bridge(State &s,Reference reference,Gather gather,Query query,Allocate allocate) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.other_flags&0x400u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1d2f10u,0x1d308eu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Insert<State,Gather,Query,Allocate>(v,c,t,gather,query,allocate);});
    return true;
}
} // namespace halo2::native_partition_insert_detail
