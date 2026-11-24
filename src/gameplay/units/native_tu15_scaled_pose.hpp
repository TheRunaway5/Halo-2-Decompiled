#pragma once
// Boundary adapter for the scaled reference pose, entered at 1CEFB0
// (native_tu15_scaled_pose_systems.hpp). Its callers and 1CF070 keep the
// original ABI, which the adapter owns: ECX the reference, EDI the vector,
// the value pointer on the stack (RET 4), its slot reused for the floats;
// 16 bytes of stack for the pose (SUB ESP,10 ... ADD ESP,10); EBP, ESI and,
// past the reference's test, EBX saved; 1CF070 gets EAX the reference, ESI
// and EBX the pose's vector and value and the count pushed (and pops it),
// and every register it returns comes back; EAX and EDX as the original
// leaves them; XMM0 the factor (zero without a reference), XMM1 the last
// product; the flags of the last instruction that sets them. Each access
// publishes its original instruction.
// HALO2_REFERENCE_SCALED_POSE=1 restores the translated body.
// HALO2_VERIFY_SCALED_POSE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_scaled_pose_systems.hpp"

namespace halo2::native_scaled_pose_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_scaled_pose;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_SCALED_POSE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_SCALED_POSE");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_reference, scaled, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Scaled pose verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr std::uint32_t frame_bytes=0x10u;
template<class State,class Pose> struct ScaledPose : Port<State> {
    using Access=logic::scaled_pose::Access;
    using Kept=logic::scaled_pose::Kept;
    Pose &pose_child;
    std::uint32_t k{},depth{};   // bytes pushed past the reserved 16
    ScaledPose(State &s,std::uint32_t *c,Trace<State> *t,Pose &p):Port<State>(s,c,t,{}),pose_child(p){}
    // The value pointer's slot past the reserved bytes and the pushes.
    std::uint32_t argument_slot()const{return this->gp[esp]+depth+frame_bytes+4u;}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        static constexpr std::uint32_t copies[]{0x1cefc1u,0x1cefc7u,0x1cefcdu},copy_stores[]{0x1cefc5u,0x1cefcau,0x1cefd0u},
            pose_vectors[]{0x1cf01eu,0x1cf02cu,0x1cf03bu},vector_stores[]{0x1cf028u,0x1cf036u,0x1cf045u};
        switch(a) {
        case base:return 0x1cefb0u;
        case value_pointer:return 0x1cefbcu;
        case copy:return copies[k];
        case copy_store:return copy_stores[k];
        case value_clear:return 0x1cefd3u;
        case high:return 0x1cefd8u;
        case rate:return 0x1cefdfu;
        case rate_store:return 0x1cefe5u;
        case rate_reload:return 0x1cefebu;
        case factor:return 0x1ceff5u;
        case scale:return 0x1ceffau;
        case factor_store:return 0x1cf00du;
        case factor_reload:return 0x1cf018u;
        case pose_vector:return pose_vectors[k];
        case vector_store:return vector_stores[k];
        case pose_value:return 0x1cf04au;
        case value_store:return 0x1cf054u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV EAX,[474D04]; XORPS XMM0,XMM0; SUB ESP,10; PUSH EBP.
        case base:gp[eax]=v;this->s.xmm[0]=_mm_setzero_ps();gp[esp]=fl.minus(gp[esp],frame_bytes,4);
            this->push(0x1cefbbu,gp[ebp]);depth=4u;break;
        case value_pointer:gp[ebp]=v;this->push(0x1cefc0u,gp[esi]);depth=8u;break;                   // MOV EBP; PUSH ESI
        // MOV ESI,[EAX]; MOV EDX,EDI / MOV ESI,[EAX+4] / MOV EAX,[EAX+8].
        case copy:if(k==2)gp[eax]=v;else gp[esi]=v;if(k==0)gp[edx]=gp[edi];break;
        case high:fl.minus(v&0xffffu,0xffffu,2);break;                                              // CMP word [ECX+6],-1
        case rate:this->s.xmm[0]=logic::sse::scalar(v);break;
        // MOVSS XMM0,[ESP+20]; CVTTSS2SI EAX,XMM0.
        case rate_reload:this->s.xmm[0]=logic::sse::scalar(v);gp[eax]=std::uint32_t(_mm_cvttss_si32(this->s.xmm[0]));break;
        case factor:this->s.xmm[0]=logic::sse::scalar(v);break;
        case factor_reload:this->s.xmm[0]=logic::sse::scalar(v);break;
        default:break;
        }
    }
    // MOVSS [ESP+24],XMM0: the factor, scaled, is XMM0 then.
    void stored(Access a,std::uint32_t,std::uint32_t v){if(a==Access::factor_store)this->s.xmm[0]=logic::sse::scalar(v);}
    void component(std::uint32_t index){k=index;}
    void saved(){this->push(0x1cefe4u,this->gp[ebx]);depth=12u;}                                     // PUSH EBX
    // MULSS XMM0,[46A20C] (the scaled factor in XMM0); PUSH EAX.
    void truncated(std::uint32_t count){this->push(0x1cf002u,count);depth=16u;}
    // LEA EBX,[ESP+10]; LEA ESI,[ESP+14]; MOV EAX,ECX; CALL 1CF070.
    void pose(Kept &kept,std::uint32_t reference_address,std::uint32_t) {
        auto &gp=this->gp;
        gp[ebx]=gp[esp]+0x10u;gp[esi]=gp[esp]+0x14u;gp[eax]=reference_address;
        this->call(0x1cf013u,0x1cf018u,0x1cf070u,pose_child);
        depth=12u;kept={gp[ebp],gp[edi]};
    }
    // MOVSS XMM1,[local]; MULSS XMM1,XMM0.
    void scaled(std::uint32_t product,std::uint32_t applied){this->s.xmm[1]=logic::sse::scalar(product);this->s.xmm[0]=logic::sse::scalar(applied);}
    void run() {
        auto &gp=this->gp;
        Memory<ScaledPose> memory{*this};
        const bool posed=logic::scaled_pose::compute(memory,*this,gp[ecx],gp[edi]);
        this->reached(posed?Branch::scaled:Branch::no_reference);
        if(posed)gp[ebx]=this->pop(0x1cf059u);                                                       // POP EBX
        gp[esi]=this->pop(0x1cf05au);gp[ebp]=this->pop(0x1cf05bu);
        gp[esp]=this->flags.plus(gp[esp],frame_bytes,4);this->ret(0x1cf05fu,4u);this->publish(0x1cf05fu);   // ADD ESP,10
    }
};
template<class State,class Reference,class Pose> bool bridge(State &s,Reference reference,Pose pose) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1cefb0u,0x1cf05fu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ScaledPose<State,Pose>(v,c,t,pose);});
    return true;
}
} // namespace halo2::native_scaled_pose_detail
