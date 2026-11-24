#pragma once
// Boundary adapter for the frame projection, entered at 1452F0
// (native_tu15_orientation_frame_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: ECX the frame, EAX the vector, EDX the out
// vector, returned in EAX; AH meanwhile the LAHF of the scale test; XMM0 the
// up dot product, XMM1 the vector's x (scaled), XMM2 and XMM3 the up axis'
// x and y products, XMM4 the left axis' x product, their upper lanes zero;
// the flags of the TEST AH,44 (the SSE arithmetic sets none); RET. Each
// access publishes its original instruction.
// HALO2_REFERENCE_ORIENTATION_FRAME=1 restores the translated body.
// HALO2_VERIFY_ORIENTATION_FRAME=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_orientation_frame_systems.hpp"

namespace halo2::native_frame_projection_detail {
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
enum Branch : unsigned {unit_scale, scaled, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Frame projection verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct FrameProjection : Port<State> {
    using Access=logic::frame_projection::Access;
    FrameProjection(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case scale:return 0x1452f0u;
        case one:return 0x1452f4u;
        case x:return 0x1452fcu;
        case y:return 0x145300u;
        case z:return 0x145305u;
        case forward_z:return 0x145332u;
        case forward_y:return 0x145337u;
        case forward_x:return 0x145348u;
        case forward_store:return 0x145355u;
        case left_z:return 0x145359u;
        case left_y:return 0x14535eu;
        case left_x:return 0x14536fu;
        case left_store:return 0x14537cu;
        case up_z:return 0x145381u;
        case up_y:return 0x14538au;
        case up_x:return 0x145393u;
        case up_store:return 0x1453a4u;
        }
        return 0;
    }
    void loaded(Access,std::uint32_t,std::uint32_t){}
    void stored(Access,std::uint32_t,std::uint32_t){}
    // UCOMISS XMM4,XMM0; LAHF; TEST AH,44.
    void compared(std::uint32_t scale_value,std::uint32_t one_value) {
        auto &gp=this->gp;auto &fl=this->flags;
        fl.scalar_compare(logic::sse::scalar(scale_value),logic::sse::scalar(one_value),true);
        const auto ah=(fl.bits()|2u)&0xffu;
        gp[eax]=(gp[eax]&0xffff00ffu)|(ah<<8);
        fl.logic(ah&0x44u,1);
        this->reached(native_perpendicular::unequal(scale_value,one_value)?Branch::scaled:Branch::unit_scale);
    }
    void built(const logic::frame_projection::Registers &r) {
        auto &x=this->s.xmm;
        x[0]=logic::sse::scalar(r.xmm0);x[1]=logic::sse::scalar(r.xmm1);x[2]=logic::sse::scalar(r.xmm2);
        x[3]=logic::sse::scalar(r.xmm3);x[4]=logic::sse::scalar(r.xmm4);
    }
    void run() {
        auto &gp=this->gp;
        Memory<FrameProjection> memory{*this};
        logic::frame_projection::project(memory,*this,gp[ecx],gp[eax],gp[edx]);
        gp[eax]=gp[edx];this->ret(0x1453abu,0);this->publish(0x1453abu);                            // MOV EAX,EDX
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1452f0u,0x1453abu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return FrameProjection<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_frame_projection_detail
