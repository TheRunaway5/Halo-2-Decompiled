#pragma once
// Boundary adapter for the root point, entered at BBF20
// (native_tu15_root_point_systems.hpp). Its callers and BBE00 keep the
// original ABI, which the adapter owns: EAX the object handle, ECX the
// point, returned in EAX; EBX, ESI and EDI saved; BBE00 gets every register
// as the original leaves it (ESI the point), and every register it returns
// comes back; ECX and EDX as the original leaves them; XMM0 and XMM1 the
// point's z and y when copied; the flags of the last instruction that sets
// them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_ROOT_POINT=1 restores the translated body.
// HALO2_VERIFY_ROOT_POINT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "gameplay/objects/native_tu15_root_point_systems.hpp"

namespace halo2::native_root_point_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_root_point;
namespace sse=native_aim_clusters::sse;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ROOT_POINT");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ROOT_POINT");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_cluster, no_entry, copied, walked_parents, branch_count};
static_assert(unsigned(logic::root_point::Outcome::copied)==Branch::copied);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Root point verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Fallback> struct RootPoint : Port<State> {
    using Access=logic::root_point::Access;
    using Kept=logic::root_point::Kept;
    Fallback &fallback_child;
    std::uint32_t steps{};
    RootPoint(State &s,std::uint32_t *c,Trace<State> *t,Fallback &f):Port<State>(s,c,t,{}),fallback_child(f){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:this->push(0xbbf20u,gp[ebx]);return 0xbbf21u;                                  // PUSH EBX
        case header_datums:return 0xbbf35u;
        case walk_object:return 0xbbf4bu;
        case parent:return 0xbbf4fu;
        // AND EDX,FFFF; LEA EDX,[EDX+EDX*2] before MOV ECX,[ECX+EDX*4+8].
        case root_datums:return 0xbbf57u;
        case root_object:gp[edx]=fl.logic(gp[edx]&0xffffu,4)*3u;return 0xbbf63u;
        case cluster_handle:return 0xbbf67u;
        case clusters:return 0xbbf72u;
        case cluster_datums:return 0xbbf78u;
        // AND ECX,FFFF; LEA ECX,[ECX+ECX*4]; SHL ECX,5 before MOV DL,[ECX+EBX+18].
        case entry_index:gp[ecx]=fl.shift(fl.logic(gp[ecx]&0xffffu,4)*5u,5u,4);return 0xbbf87u;
        case count:return 0xbbf91u;
        case entries:return 0xbbfacu;
        // LEA EAX,[EDX+EDX*2]; SHL EAX,5 before MOV EDX,[EAX+ECX+40].
        case entry_record:gp[eax]=fl.shift(gp[edx]*3u,5u,4);return 0xbbfb5u;
        case point_record:return 0xbbfb9u;
        case point_z:return 0xbbfbcu;
        case point_y:return 0xbbfc1u;
        case point_x:gp[eax]=fl.plus(gp[eax],logic::point,4);return 0xbbfc9u;                       // ADD EAX,70
        case point_x_store:return 0xbbfcbu;
        case point_y_store:gp[edi]=this->pop(0xbbfcdu);return 0xbbfceu;                              // POP EDI
        case point_z_store:return 0xbbfd3u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ebx]=v;break;
        case header_datums:gp[edi]=v;break;
        case walk_object:gp[ecx]=v;break;
        case parent:gp[ecx]=v;fl.minus(v,logic::none,4);break;                                      // MOV ECX,[ECX+14]; CMP ECX,-1
        case root_datums:gp[ecx]=v;break;
        case root_object:gp[ecx]=v;break;
        case cluster_handle:gp[ecx]=v;fl.minus(v,logic::none,4);break;                              // CMP ECX,-1
        case clusters:gp[edx]=v;break;
        case cluster_datums:gp[ebx]=v;break;
        // MOV DL,[ECX+EBX+18]; ADD ECX,EBX; TEST DL,DL.
        case entry_index:set_low_byte(gp[edx],v);gp[ecx]=fl.plus(gp[ecx],gp[ebx],4);this->test8(v);break;
        // MOV EBX,[ECX+74]; MOVSX EDI,DL; CMP EDI,EBX.
        case count:gp[ebx]=v;gp[edi]=logic::widen8(gp[edx]);fl.minus(gp[edi],v,4);break;
        case entries:gp[ecx]=v;break;
        case entry_record:gp[edx]=v;break;
        case point_record:gp[eax]=v;break;
        case point_z:this->s.xmm[0]=sse::scalar(v);break;
        case point_y:this->s.xmm[1]=sse::scalar(v);break;
        case point_x:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH ESI; OR EDX,-1; CMP EAX,-1; MOV ESI,ECX; PUSH EDI; MOV ECX,EAX.
    void saved() {
        auto &gp=this->gp;auto &fl=this->flags;
        this->push(0xbbf27u,gp[esi]);gp[edx]=fl.logic(gp[edx]|logic::none,4);fl.minus(gp[eax],logic::none,4);
        gp[esi]=gp[ecx];this->push(0xbbf30u,gp[edi]);gp[ecx]=gp[eax];
    }
    // MOV EDX,ECX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2] (each step of the walk).
    void walked(std::uint32_t handle) {
        auto &gp=this->gp;
        gp[edx]=handle;gp[ecx]=this->flags.logic(handle&0xffffu,4)*3u;
        if(++steps>1)this->reached(Branch::walked_parents);
    }
    // MOVSX DX,DL; MOVSX EDX,DX; CMP EDX,-1.
    void indexed(std::uint32_t index){this->gp[edx]=index;this->flags.minus(index,logic::none,4);}
    // OR EDX,-1; MOVSX EDX,DX; CMP EDX,-1.
    void unindexed(){auto &gp=this->gp;gp[edx]=this->flags.logic(gp[edx]|logic::none,4);this->flags.minus(gp[edx],logic::none,4);}
    // CALL BBE00.
    void fallback(Kept &kept) {
        auto &gp=this->gp;
        this->call(0xbbfddu,0xbbfe2u,0xbbe00u,fallback_child);
        kept.point=gp[esi];
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::root_point::Outcome;
        Memory<RootPoint> memory{*this};
        const auto outcome=logic::root_point::find(memory,*this,gp[eax],gp[ecx]);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::copied) {
            gp[eax]=gp[esi];gp[esi]=this->pop(0xbbfdau);gp[ebx]=this->pop(0xbbfdbu);
            this->ret(0xbbfdcu,0);this->publish(0xbbfdcu);return;
        }
        gp[edi]=this->pop(0xbbfe2u);gp[eax]=gp[esi];gp[esi]=this->pop(0xbbfe5u);gp[ebx]=this->pop(0xbbfe6u);
        this->ret(0xbbfe7u,0);this->publish(0xbbfe7u);
    }
};
template<class State,class Reference,class Fallback> bool bridge(State &s,Reference reference,Fallback fallback) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xbbf20u,0xbbfe7u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return RootPoint<State,Fallback>(v,c,t,fallback);});
    return true;
}
} // namespace halo2::native_root_point_detail
