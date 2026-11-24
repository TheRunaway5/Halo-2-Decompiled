#pragma once
// Boundary adapter for the partition unlink, entered at C5250
// (native_tu15_partition_unlink_systems.hpp). Its callers and 1D30F0 keep
// the original ABI, which the adapter owns: EAX the record handle; ESI and
// (around 1D30F0) EDI saved; 1D30F0 gets EDI the partition and the handle
// and the record's references pushed (and pops them), and every register
// it returns comes back; ECX (the flags byte) and EDX as the original
// leaves them; the flags of the last instruction that sets them; RET. Each
// access publishes its original instruction.
// HALO2_REFERENCE_PARTITION_UNLINK=1 restores the translated body.
// HALO2_VERIFY_PARTITION_UNLINK=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_partition_unlink_systems.hpp"

namespace halo2::native_partition_unlink_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_partition_unlink;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PARTITION_UNLINK");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PARTITION_UNLINK");return p && *p;}();
    return value;
}
enum Branch : unsigned {unmarked, unlinked, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Partition unlink verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Remove> struct PartitionUnlink : Port<State> {
    using Access=logic::partition_unlink::Access;
    using Kept=logic::partition_unlink::Kept;
    Remove &remove_child;
    PartitionUnlink(State &s,std::uint32_t *c,Trace<State> *t,Remove &r):Port<State>(s,c,t,{}),remove_child(r){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case table:return 0xc5250u;
        case datums:return 0xc5256u;
        case flags_read:return 0xc5268u;
        case flags_clear:return 0xc5283u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case table:gp[ecx]=v;break;
        // MOV EDX,[ECX+44]; PUSH ESI; MOV ESI,EAX; AND ESI,FFFF; IMUL ESI,ESI,110.
        case datums:gp[edx]=v;this->push(0xc5259u,gp[esi]);gp[esi]=fl.product(fl.logic(gp[eax]&0xffffu,4),logic::record_size);break;
        // MOV CL,[ESI+EDX+2]; ADD ESI,EDX; TEST CL,2.
        case flags_read:set_low_byte(gp[ecx],v);gp[esi]=fl.plus(gp[esi],gp[edx],4);this->test8(v&logic::partitioned);break;
        default:break;
        }
    }
    // AND byte [ESI+2],F7: the flags of the result.
    void stored(Access a,std::uint32_t,std::uint32_t v){if(a==Access::flags_clear)(void)this->flags.logic(v&0xffu,1);}
    // PUSH EDI; LEA EDX,[ESI+10]; PUSH EDX; PUSH EAX; MOV EDI,4ECBE0; CALL 1D30F0.
    void remove(Kept &kept,std::uint32_t handle,std::uint32_t references) {
        auto &gp=this->gp;
        this->push(0xc5273u,gp[edi]);gp[edx]=references;this->push(0xc5277u,references);this->push(0xc5278u,handle);
        gp[edi]=logic::globals::partition;
        this->call(0xc527eu,0xc5283u,0x1d30f0u,remove_child);
        kept.record=gp[esi];
    }
    void run() {
        auto &gp=this->gp;
        Memory<PartitionUnlink> memory{*this};
        if(logic::partition_unlink::unlink(memory,*this,gp[eax])) {
            this->reached(Branch::unlinked);gp[edi]=this->pop(0xc5287u);                            // POP EDI
        } else this->reached(Branch::unmarked);
        gp[esi]=this->pop(0xc5288u);this->ret(0xc5289u,0);this->publish(0xc5289u);
    }
};
template<class State,class Reference,class Remove> bool bridge(State &s,Reference reference,Remove remove) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xc5250u,0xc5289u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return PartitionUnlink<State,Remove>(v,c,t,remove);});
    return true;
}
} // namespace halo2::native_partition_unlink_detail
