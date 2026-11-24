#pragma once
// Boundary adapter for the flat axes, entered at 152D60
// (native_tu15_flat_axes_systems.hpp). Its caller and the child keep the
// original ABI, which the adapter owns: EAX the facing, EDI the fallback,
// ESI the axes (RET); 30BD0 gets EAX the forward axis and ECX the axes and
// answers the length in XMM0, every register it returns coming back; EAX,
// ECX and EDX as the original leaves them (UCOMISS's LAHF in AH); XMM0 zero,
// XMM1 the forward x and XMM2 the left x; the flags of the last length's
// TEST AH,44. Each access publishes its original instruction.
// HALO2_REFERENCE_FLAT_AXES=1 restores the translated body.
// HALO2_VERIFY_FLAT_AXES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_flat_axes_systems.hpp"

namespace halo2::native_flat_axes_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_flat_axes;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_FLAT_AXES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_FLAT_AXES");return p && *p;}();
    return value;
}
enum Branch : unsigned {facing, fallback, default_axis, branch_count};
static_assert(unsigned(logic::flat_axes::Source::default_axis)==Branch::default_axis);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Flat axes verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
// Each copy's instructions and the registers its three reads land in.
struct CopySites {std::uint32_t x,x_store,y,y_store,z,z_store;unsigned x_into,y_into,z_into;std::uint32_t flatten,call;};
inline constexpr CopySites copies[]{
    {0x152d60u,0x152d67u,0x152d69u,0x152d6cu,0x152d6fu,0x152d72u,edx,edx,eax,0x152d77u,0x152d7cu},
    {0x152d90u,0x152d94u,0x152d96u,0x152d99u,0x152d9cu,0x152d9fu,edx,eax,edx,0x152da4u,0x152da9u},
    {0x152dc3u,0x152dc7u,0x152dc9u,0x152dccu,0x152dcfu,0x152dd2u,edx,edx,eax,0u,0u}};
template<class State,class Normalize> struct FlatAxes : Port<State> {
    using Access=logic::flat_axes::Access;
    using Source=logic::flat_axes::Source;
    Normalize &normalize;
    Source source{};
    std::uint32_t zero_value{};
    FlatAxes(State &s,std::uint32_t *c,Trace<State> *t,Normalize &n):Port<State>(s,c,t,{}),normalize(n){}
    const CopySites &sites()const{return copies[unsigned(source)];}
    void copying(Source next){source=next;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case x:return sites().x;
        case x_store:return sites().x_store;
        case y:return sites().y;
        case y_store:return sites().y_store;
        case z:return sites().z;
        case z_store:return sites().z_store;
        case flatten:this->gp[eax]=this->gp[esi];return sites().flatten;                          // MOV EAX,ESI
        case zero:return 0x152daeu;
        case Access::default_axis:return 0x152dbeu;
        case forward_x:return 0x152dd5u;
        case forward_y:return 0x152ddcu;
        case left_x:return 0x152de1u;
        case left_y:return 0x152de6u;
        case left_z:return 0x152debu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case x:gp[sites().x_into]=v;gp[ecx]=gp[esi];break;                                         // MOV ECX,ESI
        case y:gp[sites().y_into]=v;break;
        case z:gp[sites().z_into]=v;break;
        case zero:zero_value=v;break;
        case Access::default_axis:gp[eax]=v;break;
        case forward_x:this->s.xmm[1]=lane0_only(v);break;                                         // MOVSS XMM1,[ESI]
        default:break;
        }
    }
    // MOVAPS XMM2,XMM0; SUBSS XMM2,[ESI+4] (XMM0 zero).
    void stored(Access a,std::uint32_t,std::uint32_t v){if(a==Access::left_x)this->s.xmm[2]=lane0_only(v);}
    std::uint32_t normalized(Source) {
        const auto &site=sites();
        this->call(site.call,site.call+5u,0x30bd0u,normalize);
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(this->s.xmm[0])));
    }
    // (MOVAPS XMM1,XMM0; XORPS XMM0,XMM0; UCOMISS XMM1,XMM0) or (UCOMISS XMM0,[46A0E8]; XORPS XMM0,XMM0);
    // LAHF; TEST AH,44.
    void compared(Source tested) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        if(tested==Source::facing){x[1]=x[0];x[0]=_mm_setzero_ps();this->flags.scalar_compare(x[1],x[0],true);}
        else{this->flags.scalar_compare(x[0],lane0_only(zero_value),true);x[0]=_mm_setzero_ps();}
        const auto ah=(this->flags.bits()|2u)&0xffu;
        gp[eax]=(gp[eax]&0xffff00ffu)|(ah<<8);this->test8(ah&0x44u);
    }
    void run() {
        this->s.xmm[0]=_mm_setzero_ps();                                                          // XORPS XMM0,XMM0
        Memory<FlatAxes> memory{*this};
        const auto outcome=logic::flat_axes::build(memory,*this,this->gp[eax],this->gp[edi],this->gp[esi]);
        this->reached(unsigned(outcome));
        this->ret(0x152df0u,0u);this->publish(0x152df0u);
    }
};
template<class State,class Reference,class Normalize> bool bridge(State &s,Reference reference,Normalize normalize) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x152d60u,0x152df0u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return FlatAxes<State,Normalize>(v,c,t,normalize);});
    return true;
}
} // namespace halo2::native_flat_axes_detail
