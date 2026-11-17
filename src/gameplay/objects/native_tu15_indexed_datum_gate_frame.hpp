#pragma once
#include "halo2/native_tu15_indexed_datum_gate_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_indexed_datum_gate_detail {
template<Globals Addresses,class State,class Before> class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;

    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {
        at(pc);return word(address,width);
    }
public:
    Frame(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}} {}

    bool valid_datum() {
        s_.set(0,s_.flags.logic(0u,1),1,0);
        s_.flags.minus(s_.gp[2],0xffffffffu,4);return !s_.flags.zf;
    }
    bool enabled() {
        s_.set(1,read(0x3eb17u,Addresses.enabled,1),1,0);
        s_.flags.logic(s_.reg(1,1,0),1);return !s_.flags.zf;
    }
    void save_index_register() {push(0x3eb21u,s_.gp[6]);}
    bool has_index() {
        s_.gp[6]=read(0x3eb22u,Addresses.index);
        s_.flags.minus(s_.gp[6],0xffffffffu,4);return !s_.flags.zf;
    }
    bool has_selection() {
        s_.flags.minus(read(0x3eb2du,Addresses.selection,2),0xffffu,2);return !s_.flags.zf;
    }
    bool has_rows() {
        auto& g=s_.gp;
        g[1]=read(0x3eb37u,Addresses.header);
        push(0x3eb3du,g[7]);
        g[7]=read(0x3eb3eu,g[1]+0xacu);
        s_.flags.logic(g[7],4);g[7]=pop(0x3eb46u);
        return !s_.flags.zf&&s_.flags.sf==s_.flags.of;
    }
    bool matches_row() {
        auto& g=s_.gp;
        g[1]=read(0x3eb49u,g[1]+0xb0u);
        g[1]=(read(0x3eb4fu,g[1]+g[6]*2u,2)^0x8000u)-0x8000u;
        s_.flags.minus(g[1],g[2],4);return s_.flags.zf;
    }
    void accept() {s_.set(0,1u,1,0);}
    bool finish(bool saved) {
        if(saved)s_.gp[6]=pop(0x3eb59u);
        pop(0x3eb5au);return s_.reg(0,1,0)!=0;
    }
};

template<Globals Addresses=title_update_15,class State,class Before> bool run_private(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Frame<Addresses,State,Before> view{state,before,fp};
    return query(view);
}
} // namespace halo2::native_indexed_datum_gate_detail
#endif
