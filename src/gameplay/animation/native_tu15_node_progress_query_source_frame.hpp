#pragma once
#include "halo2/native_tu15_node_progress_query_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>

namespace halo2::native_node_progress_query_detail {
using namespace physics_world_runtime_detail;
template<bool Literal,class State,class Child,bool PhysicalStack=false> class SourceFrame {
    State& s;
    Child& child;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s.instruction_literal(pc);
        else if(s.services.instruction)s.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t p,unsigned width=4) {return r(s.memory,p,width);}
    __m128 scalar(std::uint32_t p) {return _mm_set_ss(f(read(p)));}
    bool below_or_equal() const {return s.flags.cf||s.flags.zf;}
    void push(std::uint32_t pc,std::uint32_t value) {
        if constexpr(PhysicalStack){
            // The i386 provider sees the unretired stack pointer.
            const auto destination=s.gp[4]-4u;
            w(s.memory,s.observe,pc,destination,4,value);
            s.gp[4]=destination;
        }else s.push(pc,value);
    }
    std::uint32_t shift_descriptor_index(std::uint32_t value) {
        if constexpr(PhysicalStack){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
            // Preserve the measured x86 AF/OF residue of SHL EAX,4. The
            // authored source flags model deliberately has a different rule.
            std::uint32_t result,flags;
            asm volatile("shll $4,%%eax; movl %%eax,%%ecx; seto %%dh; lahf; movb %%ah,%%dl"
                :"+a"(value),"=c"(result),"=d"(flags)::"cc");
            s.flags.load((flags&0xd5u)|((flags&0x100u)<<3u));
            return result;
#endif
        }
        return s.flags.shift(value,4u,4);
    }
public:
    SourceFrame(State& state,Child& lookup):s(state),child(lookup) {}
    bool begin() {
        step(0x1cf0e0u);push(0x1cf0e0u,s.reg(1,4,0));
        step(0x1cf0e1u);s.flags.minus(read(s.gp[6]+6u,2),0xffffffffu,2);
        step(0x1cf0e6u);s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
        step(0x1cf0e9u);w(s.memory,s.observe,0x1cf0e9u,s.gp[4],4,u(_mm_cvtss_f32(s.xmm[0])));
        step(0x1cf0eeu);return !s.flags.zf;
    }
    void lookup_descriptor() {
        auto&g=s.gp;
        step(0x1cf0f0u);s.set(0,read(g[6]),4,0);
        step(0x1cf0f2u);s.set(1,read(0x4f0414u),4,0);
        step(0x1cf0f8u);s.set(0,s.flags.logic(s.reg(0,4,0)&0xffffu,4),4,0);
        step(0x1cf0fdu);s.set(0,shift_descriptor_index(s.reg(0,4,0)),4,0);
        step(0x1cf100u);s.set(2,read(g[0]+g[1]+8u),4,0);
        step(0x1cf104u);s.set(0,read(g[6]+4u),4,0);
        step(0x1cf107u);push(0x1cf107u,s.reg(0,4,0));
        step(0x1cf108u);push(0x1cf108u,0x1cf10du);child(s);
    }
    bool positive_divisor() {
        step(0x1cf10du);
        const auto word=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(read(s.gp[0]+0x14u,2)));
        s.set(1,std::bit_cast<std::uint32_t>(std::int32_t(word)),4,0);
        step(0x1cf111u);s.xmm[2]=_mm_xor_ps(s.xmm[2],s.xmm[2]);
        step(0x1cf114u);s.xmm[1]=_mm_cvtsi32_ss(s.xmm[1],si(s.reg(1,4,0)));
        step(0x1cf118u);s.flags.scalar_compare(s.xmm[1],s.xmm[2],false);
        step(0x1cf11bu);return !below_or_equal();
    }
    void reload_scratch() {step(0x1cf142u);s.xmm[0]=scalar(s.gp[4]);}
    Return normalized_ratio() {
        step(0x1cf11du);s.xmm[0]=scalar(s.gp[6]+0x1cu);
        step(0x1cf122u);s.xmm[0]=_mm_div_ss(s.xmm[0],s.xmm[1]);
        step(0x1cf126u);s.flags.scalar_compare(s.xmm[2],s.xmm[0],false);
        step(0x1cf129u);
        if(!below_or_equal()){
            step(0x1cf12bu);s.xmm[0]=s.xmm[2];return Return::lower;
        }
        step(0x1cf130u);s.xmm[1]=scalar(0x46a0d0u);
        step(0x1cf138u);s.flags.scalar_compare(s.xmm[0],s.xmm[1],false);
        step(0x1cf13bu);if(below_or_equal())return Return::ordinary;
        step(0x1cf13du);s.xmm[0]=s.xmm[1];return Return::upper;
    }
    std::uint32_t finish(Return route) {
        step(route==Return::lower?0x1cf12eu:route==Return::upper?0x1cf140u:0x1cf147u);
        s.set(1,s.pop(),4,0);
        step(route==Return::lower?0x1cf12fu:route==Return::upper?0x1cf141u:0x1cf148u);
        s.pop();return s.gp[0];
    }
};

template<bool Literal,class State,class Child> std::uint32_t run_source_frame(State& state,Child lookup) {
    SourceFrame<Literal,State,Child> view{state,lookup};return query(view);
}
template<class State,class Child> std::uint32_t run_physical_frame(State& state,Child lookup) {
    SourceFrame<false,State,Child,true> view{state,lookup};return query(view);
}
} // namespace halo2::native_node_progress_query_detail
