#pragma once
// Boundary adapter for the cluster notice, entered at 1CD620
// (native_tu15_cluster_notify_systems.hpp). Its callers and 1DC270 keep the
// original ABI, which the adapter owns: EAX the object handle, the two byte
// arguments on the stack (RET 8); ESI saved; 1DC270 gets the cluster handle
// pushed (and pops it), and every register it returns comes back; EAX (the
// cluster's address), ECX and EDX (the flags shifted, the tested bytes) as
// the original leaves them; the flags of the last instruction that sets
// them. Each access publishes its original instruction.
// HALO2_REFERENCE_CLUSTER_NOTIFY=1 restores the translated body.
// HALO2_VERIFY_CLUSTER_NOTIFY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_cluster_notify_systems.hpp"

namespace halo2::native_cluster_notify_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_cluster_notify;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_CLUSTER_NOTIFY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_CLUSTER_NOTIFY");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_cluster, first_held, sealed, second_held, notified, second_notified, branch_count};
static_assert(unsigned(logic::cluster_notify::Outcome::second_notified)==Branch::second_notified);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Cluster notify verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Notify> struct ClusterNotify : Port<State> {
    using Access=logic::cluster_notify::Access;
    Notify &notify_child;
    ClusterNotify(State &s,std::uint32_t *c,Trace<State> *t,Notify &n):Port<State>(s,c,t,{}),notify_child(n){}
    // [ESP+8] and [ESP+C] after PUSH ESI.
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+8u+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0x1cd620u;
        case header_datums:return 0x1cd626u;
        case object_address:return 0x1cd631u;
        case cluster:this->push(0x1cd635u,this->gp[esi]);return 0x1cd636u;                         // PUSH ESI
        case clusters:return 0x1cd641u;
        case cluster_datums:return 0x1cd647u;
        case flags:return 0x1cd657u;
        case first_argument:return 0x1cd666u;
        case second_argument:return 0x1cd680u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;                 // AND EAX,FFFF; LEA
        case object_address:gp[eax]=v;break;
        case cluster:gp[esi]=v;fl.minus(v,logic::none,4);break;                                  // CMP ESI,-1
        case clusters:gp[ecx]=v;break;
        // MOV EAX,ESI; AND EAX,FFFF; LEA EAX,[EAX+EAX*4]; SHL EAX,5.
        case cluster_datums:gp[edx]=v;gp[eax]=fl.shift(fl.logic(gp[esi]&0xffffu,4)*5u,5u,4);break;
        // MOV ECX,[EAX+EDX+4]; ADD EAX,EDX; MOV EDX,ECX; SHR EDX,1; TEST DL,1.
        case flags:gp[ecx]=v;gp[eax]=fl.plus(gp[eax],gp[edx],4);gp[edx]=fl.right(v,1u,4,false);this->test8(gp[edx]&1u);break;
        case first_argument:set_low_byte(gp[edx],v);this->test8(v);break;                        // TEST DL,DL
        // MOV CL,[ESP+C]; TEST CL,CL; then TEST DL,DL when set.
        case second_argument:set_low_byte(gp[ecx],v);this->test8(v);if(v&0xffu)this->test8(gp[edx]);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOV EDX,ECX; SHR EDX,8; AND DL,1; and unless sealed SHR ECX,7; TEST CL,1.
    void examined(std::uint32_t flags_value,bool is_sealed) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.right(flags_value,8u,4,false);set_low_byte(gp[edx],fl.logic(gp[edx]&1u,1));
        if(is_sealed)return;
        gp[ecx]=fl.right(gp[ecx],7u,4,false);this->test8(gp[ecx]&1u);
    }
    // PUSH ESI; CALL 1DC270.
    void notify(std::uint32_t cluster_handle) {
        this->push(0x1cd68cu,cluster_handle);this->call(0x1cd68du,0x1cd692u,0x1dc270u,notify_child);
    }
    void run() {
        auto &gp=this->gp;
        Memory<ClusterNotify> memory{*this};
        this->reached(unsigned(logic::cluster_notify::notify(memory,*this,gp[eax])));             // Branch follows Outcome
        gp[esi]=this->pop(0x1cd692u);this->ret(0x1cd693u,8u);this->publish(0x1cd693u);
    }
};
template<class State,class Reference,class Notify> bool bridge(State &s,Reference reference,Notify notify) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1cd620u,0x1cd693u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ClusterNotify<State,Notify>(v,c,t,notify);});
    return true;
}
} // namespace halo2::native_cluster_notify_detail
