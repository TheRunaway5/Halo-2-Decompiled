#pragma once
#include "halo2/native_tu15_object_definition_mode_query_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_definition_mode_query_detail {
template<class State,class Before> class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;using Base::push;

    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {
        at(pc);return word(address,width);
    }
    std::uint32_t index16(std::uint32_t value) {
        // The original is SHL EDX,4. Use the measured x86 flag residue.
        std::uint32_t result;
        asm volatile("shll $4,%%edx; movl %%edx,%%ecx; seto %%dh; lahf; movb %%ah,%%dl"
            :"+d"(value),"=c"(result)::"eax","cc");
        s_.flags.load((value&0xd5u)|((value&0x100u)<<3u));
        return result;
    }
public:
    Frame(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}} {}
    bool mode_is_nine() {
        auto& g=s_.gp;
        g[2]=read(0x165590u,0x4f3218u);
        push(0x165596u,g[6]);
        g[6]=read(0x165597u,g[2]+0x180u);
        s_.set(0,s_.flags.logic(0u,1),1,0);
        s_.flags.minus(g[6],9u,4);
        g[6]=pop(0x1655a2u);
        return s_.flags.zf;
    }
    bool has_datum() {s_.flags.minus(s_.gp[1],0xffffffffu,4);return !s_.flags.zf;}
    bool definition_word_is_one() {
        auto& g=s_.gp;
        g[2]=read(0x1655aau,0x4ecbd0u);
        g[2]=read(0x1655b0u,g[2]+0x44u);
        g[1]=s_.flags.logic(g[1]&0xffffu,4);
        g[1]+=g[1]*2u;
        g[1]=read(0x1655bcu,g[2]+g[1]*4u+8u);
        g[2]=read(0x1655c0u,g[1]);
        g[1]=read(0x1655c2u,0x4f0414u);
        g[2]=index16(s_.flags.logic(g[2]&0xffffu,4));
        g[2]=read(0x1655d1u,g[2]+g[1]+8u);
        s_.flags.minus(read(0x1655d5u,g[2]+0x290u,2),1u,2);
        return s_.flags.zf;
    }
    void accept() {s_.set(0,1u,1,0);}
    bool finish() {pop(0x1655e1u);return s_.reg(0,1,0)!=0;}
};

template<class State,class Before> bool run_private(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Frame<State,Before> view{state,before,fp};
    return query(view);
}
} // namespace halo2::native_object_definition_mode_query_detail
#endif
