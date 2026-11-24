#pragma once
// Boundary adapter for the perpendicular, entered at 11FA50
// (native_tu15_perpendicular_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: ECX the vector, EDX the result (returned in
// EAX), a 16-byte frame of floats; the x87 steps on the live stack (two
// registers, so a stack with fewer than two free is refused at admission)
// with the original's C1 clears; ESI saved around the default axis; XMM0
// to XMM5 as the cross product leaves them; the flags of the final ADD ESP;
// RET. Each access publishes its original instruction.
// HALO2_REFERENCE_PERPENDICULAR=1 restores the translated body.
// HALO2_VERIFY_PERPENDICULAR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_perpendicular_systems.hpp"
#include "physics/runtime/physics_solver_internal.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include <bit>
#include <cmath>

namespace halo2::native_perpendicular_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_perpendicular;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PERPENDICULAR");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PERPENDICULAR");return p && *p;}();
    return value;
}
enum Branch : unsigned {horizontal, short_length, zero_length, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Perpendicular verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr std::uint16_t c1=0x200u;
inline constexpr std::uint32_t frame_bytes=0x10u;
// FLD m32 / an x87 operand: the float, extended.
inline long double extended(std::uint32_t bits){return physics_step_detail::ext(physics_step_detail::f(bits));}
template<class State> struct Perpendicular : Port<State> {
    using Access=logic::perpendicular::Access;
    using Local=logic::perpendicular::Local;
    std::uint32_t depth{};
    Perpendicular(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t local(Local l)const{return this->gp[esp]+depth+4u*std::uint32_t(l);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case x_read:return 0x11fa53u;
        case y_read:return 0x11fa55u;
        case y_store:return 0x11fa5au;
        case negated_store:return 0x11fa5eu;
        case negated_square:return 0x11fa62u;
        case y_load:return 0x11fa66u;
        case y_square:return 0x11fa6au;
        case length_store:return 0x11fa72u;
        case epsilon:return 0x11fa77u;
        case length_read:return 0x11fa83u;
        case one:return 0x11fa8bu;
        case scale_y:return 0x11faa1u;
        case scale_x:return 0x11faaau;
        case axis_global:return 0x11fab6u;
        case axis_x:return 0x11fabcu;
        case axis_x_store:return 0x11fabeu;
        case axis_y:return 0x11fac2u;
        case axis_z:return 0x11fac5u;
        case axis_x_load:return 0x11fac8u;
        case axis_y_store:return 0x11faceu;
        case axis_y_load:return 0x11fad2u;
        case axis_z_store:return 0x11fad8u;
        case axis_z_load:return 0x11fadcu;
        case cross_x:return 0x11fae3u;
        case cross_z:return 0x11fae7u;
        case cross_y:return 0x11faefu;
        case cross_x_again:return 0x11fafcu;
        case cross_z_again:return 0x11fb04u;
        case cross_y_again:return 0x11fb0du;
        case result_x:return 0x11fb1eu;
        case result_y:return 0x11fb22u;
        case result_z:return 0x11fb27u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fp=this->s.fp;
        using enum Access;
        switch(a) {
        case x_read:case y_load:case epsilon:fp.push(extended(v));fp.status&=~c1;break;          // FLD m32
        case negated_square:case y_square:fp.st(0)=physics_step_detail::mul(fp.st(0),extended(v));break;   // FMUL m32
        case y_read:case axis_global:case axis_z:gp[eax]=v;break;
        case axis_x:case axis_y:gp[esi]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void negate(){auto &fp=this->s.fp;fp.st(0)=-fp.st(0);fp.status&=~c1;}                       // FCHS
    // FST m32 (this unit's translation leaves C1 alone).
    std::uint32_t spill(){return std::bit_cast<std::uint32_t>(physics_step_detail::spill(this->s.fp.st(0)));}
    // FADDP; FSQRT.
    void length() {
        auto &fp=this->s.fp;
        fp.st(1)=physics_step_detail::add(fp.st(1),fp.st(0));fp.pop();fp.st(0)=physics_solver_detail::root(fp.st(0));
    }
    void magnitude(){auto &fp=this->s.fp;fp.st(0)=std::fabs(fp.st(0));fp.status&=~c1;}            // FABS
    // FCOMIP ST,ST1; FSTP ST0; JA: the epsilon above the length.
    bool within_epsilon() {
        auto &fp=this->s.fp;
        this->s.flags=this->flags;this->s.compare_and_pop(fp.st(1));this->flags=this->s.flags;
        fp.st(0)=fp.st(0);fp.status&=~c1;fp.pop();
        return !this->flags.cf && !this->flags.zf;
    }
    void save_esi(){this->push(0x11fabbu,this->gp[esi]);depth+=4u;}                               // PUSH ESI
    void restore_esi(){this->gp[esi]=this->pop(0x11fae2u);depth-=4u;}                              // POP ESI
    void crossed(const logic::perpendicular::Cross &c) {
        auto &x=this->s.xmm;
        x[0]=logic::sse::scalar(c.xmm0);x[1]=logic::sse::scalar(c.xmm1);x[2]=logic::sse::scalar(c.xmm2);
        x[3]=logic::sse::scalar(c.xmm3);x[4]=logic::sse::scalar(c.xmm4);x[5]=logic::sse::scalar(c.xmm5);
    }
    void run() {
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],frame_bytes,4);                                          // SUB ESP,10
        Memory<Perpendicular> memory{*this};
        const auto direction=logic::perpendicular::of(memory,*this,gp[ecx],gp[edx]);
        using logic::perpendicular::Direction;
        this->reached(direction==Direction::horizontal?Branch::horizontal:
                      direction==Direction::short_length?Branch::short_length:Branch::zero_length);
        gp[eax]=gp[edx];gp[esp]=this->flags.plus(gp[esp],frame_bytes,4);                           // MOV EAX,EDX; ADD ESP,10
        this->ret(0x11fb31u,0);this->publish(0x11fb31u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.fp.count>6u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x11fa50u,0x11fb31u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Perpendicular<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_perpendicular_detail
