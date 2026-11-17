#pragma once
#include "halo2/native_tu15_geometry_leaf_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_integer_fp_scope.hpp"
#include "physics/runtime/native_tu15_mopp_ray_integer_frame.hpp"

namespace halo2::native_geometry_leaf_detail {
enum class Kind{direction,position};
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class Scope>struct Io{Scope*scope;template<class Call>void operator()(Call&&call)const{scope->io(std::forward<Call>(call));}};
template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,Io<native_integer_fp_detail::Scope<State>>>{
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::vector;using Base::vector_store;using Base::push;using Base::pop;
    Kind kind_;bool canonical_;
    std::uint32_t site(std::uint32_t value)const{return canonical_?value:value+0x3af0u;}
public:
    Frame(State&s,Before&before,Scope&scope,Kind kind,bool canonical):Base{s,before,{&scope}},kind_(kind),canonical_(canonical){}
    void begin(){
        auto&g=s_.gp;push(site(kind_==Kind::direction?0x2e0cc0u:0x2e0ba0u),g[5]);g[5]=g[4];
        if(kind_==Kind::direction){
            g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);
            at(site(0x2e0cc6u));g[0]=word(g[5]+8u);
        }
    }
    Vector basis_column(unsigned axis){
        if(kind_==Kind::direction){
            const auto pc=axis==2?0x2e0cc9u:axis==1?0x2e0ccdu:0x2e0cd1u;
            const auto lane=axis==2?3u:axis==1?5u:2u;
            s_.xmm[lane]=vector(site(pc),s_.gp[0]+axis*16u);return words(s_.xmm[lane]);
        }
        if(axis==3){at(site(0x2e0ba9u));s_.gp[0]=word(s_.gp[5]+8u);}
        const auto pc=axis==3?0x2e0bacu:axis==2?0x2e0bb0u:axis==1?0x2e0bbeu:0x2e0bd6u;
        const auto lane=axis==3?1u:axis==2?3u:axis==1?2u:0u;
        s_.xmm[lane]=vector(site(pc),s_.gp[0]+axis*16u);return words(s_.xmm[lane]);
    }
    Vector source_column(unsigned){
        at(site(kind_==Kind::direction?0x2e0cd4u:0x2e0ba3u));s_.gp[0]=word(s_.gp[5]+12u);
        const auto lane=kind_==Kind::direction?1u:0u;
        s_.xmm[lane]=vector(site(kind_==Kind::direction?0x2e0cd7u:0x2e0ba6u),s_.gp[0]);return words(s_.xmm[lane]);
    }
    void computed(const Direction&d){
        s_.xmm[0]=d.result;s_.xmm[1]=d.input;s_.xmm[2]=d.z_term;s_.xmm[3]=d.x_broadcast;
        s_.xmm[4]=d.z_low;s_.xmm[5]=d.y_term;s_.xmm[6]=d.z_broadcast;s_.xmm[7]=d.z_high;
    }
    void z_term_ready(const Position&d){s_.xmm[2]=_mm_shuffle_ps(d.input,d.input,0xaa);s_.xmm[3]=d.z_term;}
    void y_term_ready(const Position&d){s_.xmm[1]=d.x_broadcast;s_.xmm[2]=d.y_term;s_.xmm[3]=d.translated_z;}
    void computed(const Position&d){
        s_.xmm[0]=d.result;s_.xmm[1]=d.x_broadcast;s_.xmm[2]=d.y_term;s_.xmm[3]=d.translated_z;
        s_.gp[4]=s_.flags.logic(s_.gp[4]&0xfffffff0u,4);
    }
    void destination_column(unsigned,const Vector&value,NativeTu15GeometryPublication){
        vector_store(site(kind_==Kind::direction?0x2e0d26u:0x2e0be5u),s_.gp[1],lanes(value));
    }
    void finish(){
        s_.gp[4]=s_.gp[5];s_.gp[5]=pop(site(kind_==Kind::direction?0x2e0d2bu:0x2e0beau));
        pop(site(kind_==Kind::direction?0x2e0d2cu:0x2e0bebu));s_.gp[4]+=8u;
    }
};
// Private architectural transport; no source-owner admission/default. Before
// is an I/O boundary observer here, not a pure-CPU instruction-stop protocol.
template<class State,class Before=Noop>void run_private(State&s,Kind kind,Before before={},bool canonical=false){
    native_integer_fp_detail::Scope scope{s};Frame view{s,before,scope,kind,canonical};view.begin();
    if(kind==Kind::direction)transposed_direction(view);else transformed_position(view);
    view.finish();
}
} // namespace halo2::native_geometry_leaf_detail
#endif
