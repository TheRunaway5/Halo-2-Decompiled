#pragma once
// Boundary adapter for the world bounds, entered at 1751B0
// (native_tu15_world_bounds_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the point; AL the result; PUSH ECX makes
// the slot each coordinate passes through, and ECX is popped from it; the
// exponent tests leave their AND in ECX, EDX and EAX by coordinate; XMM0
// the coordinate, XMM2 and XMM1 the limits; the flags of the last test;
// RET. Each access publishes its original instruction.
// HALO2_REFERENCE_WORLD_BOUNDS=1 restores the translated body.
// HALO2_VERIFY_WORLD_BOUNDS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_world_bounds_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_world_bounds_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_world_bounds;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_WORLD_BOUNDS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_WORLD_BOUNDS");return p && *p;}();
    return value;
}
enum Branch : unsigned {not_finite, below_least, above_most, inside, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"World bounds verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr bool backend=logic::sse::available;
template<class State> struct Bounds : Port<State> {
    using Access=logic::world_bounds::Access;
    using Local=logic::world_bounds::Local;
    std::uint32_t current{};
    Bounds(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t local(Local)const{return this->gp[esp];}
    void axis(std::uint32_t a){current=a;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        static constexpr std::uint32_t coordinates[]{0x1751b1u,0x1751e5u,0x17520au},stores[]{0x1751b5u,0x1751eau,0x17520fu},
            bits[]{0x1751bau,0x1751efu,0x175214u};
        using enum Access;
        switch(a) {
        case coordinate:return coordinates[current];
        case coordinate_store:return stores[current];
        case coordinate_bits:return bits[current];
        case least:return 0x1751cbu;
        case most:return 0x1751d8u;
        }
        return 0;
    }
    // The comparisons after a coordinate's exponent test and limits.
    void compared(std::uint32_t value) {
        auto &x=this->s.xmm;
        this->flags.scalar_compare(x[0],x[2],false);
        if(!logic::sse::below(value,logic::sse::bits(x[2]))) {
            this->flags.scalar_compare(x[1],x[0],false);
            if(logic::sse::below(logic::sse::bits(x[1]),value))this->reached(Branch::above_most);
        } else this->reached(Branch::below_least);
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case coordinate:x[0]=lane0_only(v);break;
        // AND reg,7F800000; CMP reg,7F800000 (ECX, EDX, EAX by coordinate).
        case coordinate_bits: {
            static constexpr unsigned regs[]{ecx,edx,eax};
            gp[regs[current]]=this->flags.logic(v&logic::exponent_bits,4);this->compare32(gp[regs[current]],logic::exponent_bits);
            if((v&logic::exponent_bits)==logic::exponent_bits)this->reached(Branch::not_finite);
            else if(current)compared(logic::sse::bits(x[0]));
            break;
        }
        case least:x[2]=lane0_only(v);this->flags.scalar_compare(x[0],x[2],false);
            if(logic::sse::below(logic::sse::bits(x[0]),v))this->reached(Branch::below_least);
            break;
        case most:x[1]=lane0_only(v);this->flags.scalar_compare(x[1],x[0],false);
            if(logic::sse::below(v,logic::sse::bits(x[0])))this->reached(Branch::above_most);
            break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        this->push(0x1751b0u,gp[ecx]);                                                            // PUSH ECX
        Memory<Bounds> memory{*this};
        const bool result=logic::world_bounds::inside(memory,*this,gp[eax]);
        if(result){set_low_byte(gp[eax],1u);this->reached(Branch::inside);}
        else set_low_byte(gp[eax],this->flags.logic(0u,1));                                       // XOR AL,AL
        const auto pc=result?0x17522fu:0x175233u;
        gp[ecx]=this->pop(pc);this->ret(pc+1u,0);this->publish(pc+1u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1751b0u,0x175234u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Bounds<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_world_bounds_detail
