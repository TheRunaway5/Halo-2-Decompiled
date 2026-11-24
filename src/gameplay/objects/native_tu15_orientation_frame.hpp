#pragma once
// Boundary adapter for the orientation frame, entered at 144210
// (native_tu15_orientation_frame_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: ECX the frame, EDX the forward vector, EAX
// the up vector; EBX, ESI and EDI saved; EAX (the up vector's z), EDX (the
// frame's up) as the original leaves them; XMM0 zero, XMM1 the left y, XMM2
// the left x, XMM3 the up z times the forward y, their upper lanes zero;
// the flags as the caller left them; RET. Each access publishes its
// original instruction.
// HALO2_REFERENCE_ORIENTATION_FRAME=1 restores the translated body.
// HALO2_VERIFY_ORIENTATION_FRAME=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_orientation_frame_systems.hpp"

namespace halo2::native_orientation_frame_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_orientation_frame;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ORIENTATION_FRAME");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ORIENTATION_FRAME");return p && *p;}();
    return value;
}
enum Branch : unsigned {built, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Orientation frame verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct OrientationFrame : Port<State> {
    using Access=logic::orientation_frame::Access;
    std::uint32_t k{};
    OrientationFrame(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        static constexpr std::uint32_t forward_loads[]{0x144221u,0x144228u,0x14422eu},forward_stores[]{0x144226u,0x14422bu,0x144231u},
            up_loads[]{0x14428au,0x144291u,0x144297u},up_stores[]{0x14428fu,0x144294u,0x14429bu},
            position_stores[]{0x14429fu,0x1442a4u,0x1442a9u};
        switch(a) {
        case one:return 0x144210u;
        case scale_store:return 0x144218u;
        case forward:return forward_loads[k];
        case forward_store:return forward_stores[k];
        case forward_y_z:return 0x144234u;
        case up_x_z:return 0x144239u;
        case forward_x_z:return 0x14423du;
        case up_y_z:return 0x144241u;
        case forward_z_y:return 0x144246u;
        case up_x_y:return 0x14424bu;
        case up_z_x:return 0x14424fu;
        case forward_y_x:return 0x144254u;
        case up_z_y:return 0x14425du;
        case forward_x_y:return 0x144262u;
        case forward_z_x:return 0x14426au;
        case up_y_x:return 0x14426fu;
        case left_z_store:return 0x144274u;
        case left_y_store:return 0x14427cu;
        case left_x_store:return 0x144285u;
        case up:if(k==0)this->gp[edx]=this->gp[ecx]+logic::layout::up;return up_loads[k];           // LEA EDX,[ECX+1C]
        // POP EDI before the last up store.
        case up_store:if(k==2)this->gp[edi]=this->pop(0x14429au);return up_stores[k];
        // POP ESI before the position.
        case position_store:if(k==0)this->gp[esi]=this->pop(0x14429eu);return position_stores[k];
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // MOV EBX,[EDI]; LEA ESI,[ECX+4] / MOV EBX,[EDI+4] / MOV EDI,[EDI+8].
        case forward:if(k==2)gp[edi]=v;else gp[ebx]=v;if(k==0)gp[esi]=gp[ecx]+logic::layout::forward;break;
        // MOV ESI,[EAX] / MOV ESI,[EAX+4] / MOV EAX,[EAX+8].
        case up:if(k==2)gp[eax]=v;else gp[esi]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saved() {
        auto &gp=this->gp;
        this->push(0x14421cu,gp[ebx]);this->push(0x14421du,gp[esi]);this->push(0x14421eu,gp[edi]);gp[edi]=gp[edx];
    }
    void component(std::uint32_t index){k=index;}
    void built(const logic::orientation_frame::Registers &r) {
        auto &x=this->s.xmm;
        x[0]=_mm_setzero_ps();x[1]=logic::sse::scalar(r.xmm1);x[2]=logic::sse::scalar(r.xmm2);x[3]=logic::sse::scalar(r.xmm3);
    }
    void run() {
        auto &gp=this->gp;
        Memory<OrientationFrame> memory{*this};
        logic::orientation_frame::build(memory,*this,gp[ecx],gp[edx],gp[eax]);
        this->reached(Branch::built);
        gp[ebx]=this->pop(0x1442aeu);this->ret(0x1442afu,0);this->publish(0x1442afu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x144210u,0x1442afu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return OrientationFrame<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_orientation_frame_detail
