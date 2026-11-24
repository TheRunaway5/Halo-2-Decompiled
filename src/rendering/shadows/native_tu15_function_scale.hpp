#pragma once
// Boundary adapter for the object function scale, entered at C0700
// (native_tu15_function_scale_systems.hpp). Its callers and the child keep
// the original ABI, which the adapter owns: EDX the handle, ECX the index;
// XMM0 the result (XMM1 the scale); PUSH ECX makes the scale's slot; ESI
// and EDI saved (the tag and the object); the slot is addressed from the
// live ESP, so a child that leaves the stack unbalanced moves it as it does
// for the original; BCB70 (through the unit's route) gets (EDX, function,
// slot), pops them, and every register it returns comes back; the flags of
// the last instruction that sets them; RET. Each access publishes its
// original instruction.
// HALO2_REFERENCE_FUNCTION_SCALE=1 restores the translated body.
// HALO2_VERIFY_FUNCTION_SCALE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "rendering/shadows/native_tu15_function_scale_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_function_scale_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_function_scale;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_FUNCTION_SCALE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_FUNCTION_SCALE");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_function, evaluated, object_scaled, unscaled, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Function scale verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge scale_edges[]{
    {0xc0748u,0xc0763u,no_function},{0xc0748u,0xc0753u,evaluated},{0xc0763u,0xc076fu,object_scaled},
    {0xc0763u,0xc077fu,unscaled},
};
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr bool backend=logic::sse::available;
template<class State,class Route> struct Scale : Port<State> {
    using Access=logic::function_scale::Access;
    using Local=logic::function_scale::Local;
    using Kept=logic::function_scale::Kept;
    Route &route;
    // The bytes pushed below the return address (ECX's slot first).
    std::uint32_t depth{};bool multiplied{};
    Scale(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,scale_edges),route(r){}
    std::uint32_t local(Local)const{return this->gp[esp]+depth-4u;}
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case one:return 0xc0701u;
        case object_table:save(0xc0709u,gp[esi]);return 0xc070au;                                  // PUSH ESI
        case object_datums:return 0xc0710u;
        // MOV EAX,EDX; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]; PUSH EDI.
        case object_address:gp[eax]=this->flags.logic(gp[edx]&0xffffu,4)*3u;save(0xc071du,gp[edi]);return 0xc071eu;
        case object_tag:return 0xc0722u;
        case instances:return 0xc0724u;
        // AND EAX,FFFF; SHL EAX,4.
        case tag:gp[eax]=this->flags.shift(this->flags.logic(gp[eax]&0xffffu,4),logic::instance_shift,4);return 0xc0732u;
        case functions:return 0xc0736u;
        case scale_init:return 0xc0742u;
        case function:return 0xc0748u;
        case scale_read:return 0xc075bu;
        // XOR EDX,EDX; MOV DX,[ESI+2].
        case flags:gp[edx]=this->flags.logic(0u,4);return 0xc0763u;
        case object_scale:return 0xc076fu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case one:case scale_read:x[1]=lane0_only(v);break;
        case object_table:case object_datums:gp[esi]=v;break;
        case object_address:gp[edi]=v;break;
        case object_tag:case function:gp[eax]=v;if(a==function)this->test32(v);break;
        case instances:gp[esi]=v;break;
        case tag:gp[esi]=v;break;
        // LEA ECX,[ECX+ECX*2]; LEA EAX,[EAX+ECX*8].
        case functions:gp[ecx]*=3u;gp[eax]=v+gp[ecx]*8u;break;
        // SHR EDX,9; TEST DL,1.
        case flags:
            gp[edx]=this->flags.right(v&0xffffu,logic::scaled_bit,4,false);this->test8(gp[edx]&1u);
            multiplied=(gp[edx]&1u)!=0;break;
        case object_scale:x[0]=lane0_only(v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // LEA ECX,[ESP+8]; PUSH ECX; PUSH EAX; PUSH EDX; CALL BCB70.
    void evaluate(std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        gp[ecx]=local(Local::scale);save(0xc0753u,gp[ecx]);save(0xc0754u,gp[eax]);save(0xc0755u,gp[edx]);
        this->call_routed(route,0xc0756u,0xc075bu,0xbcb70u);depth-=12u;
        kept={gp[esi],gp[edi]};
    }
    // MULSS XMM0,XMM1, or MOVAPS XMM0,XMM1.
    void scaled(std::uint32_t value) {
        auto &x=this->s.xmm;
        if(multiplied)x[0]=lane0_only(value);else x[0]=x[1];
    }
    void run() {
        auto &gp=this->gp;
        save(0xc0700u,gp[ecx]);                                                                     // PUSH ECX
        Memory<Scale> memory{*this};
        (void)logic::function_scale::scale(memory,*this,gp[edx],gp[ecx]);
        const auto pc=multiplied?0xc0777u:0xc077fu;
        gp[edi]=this->pop(pc);
        gp[esi]=this->pop(multiplied?0xc077cu:0xc0783u);gp[ecx]=this->pop(multiplied?0xc077du:0xc0784u);
        const auto end=multiplied?0xc077eu:0xc0785u;
        this->ret(end,0);this->publish(end);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xc0700u,0xc0785u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Scale<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_function_scale_detail
