#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_basis_kernel.hpp"
#include <bit>
#include <cstdint>
#include <xmmintrin.h>

namespace halo2::native_solver_hull_basis_detail {
// I/O methods commit the actual instruction before publishing a source
// continuation. The schedule never captures stack or child-result pointers
// across an I/O that precedes their authored acquisition.
template<class State,class Io>class Schedule {
    State&s_;
    Io&io_;
    static std::uint32_t low(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
    template<unsigned mask>static __m128 shuffle(__m128 value){return _mm_shuffle_ps(value,value,mask);}
public:
    Schedule(State&s,Io&io):s_(s),io_(io){}
    void open(){
        auto&g=s_.gp;io_.push(0x313790u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);
        g[4]=s_.flags.minus(g[4],0xb8u,4);io_.push(0x31379cu,g[6]);g[6]=g[1];
    }
    void measure_axes(){
        auto&g=s_.gp;auto&x=s_.xmm;
        io_.load_gp(0x31379fu,0u,g[6]+0x50u);
        io_.load_vector(0x3137a2u,1u,g[0]+0x10u);io_.load_vector(0x3137a6u,0u,g[0]);
        x[0]=io_.sub(x[0],x[1]);io_.vector_store(0x3137acu,g[4]+0xcu,x[0]);
        io_.load_float(0x3137b1u,g[4]+0xcu);io_.absolute();g[1]=s_.flags.logic(0,4);
        io_.load_float(0x3137b9u,g[4]+0x10u);io_.absolute();io_.push(0x3137bfu,g[7]);
        io_.save_float(0x3137c0u,g[4]+0x2cu,false);
        io_.load_float(0x3137c4u,g[4]+0x18u);g[2]=1u;io_.absolute();g[7]=2u;
        io_.save_float(0x3137d4u,g[4]+0xcu,true);
    }
    bool first_axis_kept(){io_.compare_stack();io_.status_to_ax();s_.flags.logic((s_.gp[0]>>8u)&5u,1);return s_.flags.pf;}
    void promote_y(){
        io_.drop();auto&g=s_.gp;g[2]=s_.flags.logic(0,4);io_.load_float(0x3137e5u,g[4]+0x2cu);g[1]=1u;
    }
    bool middle_axis_kept(){
        io_.load_float(0x3137eeu,s_.gp[4]+0xcu);io_.compare_stack();io_.status_to_ax();io_.drop();
        s_.flags.logic((s_.gp[0]>>8u)&5u,1);return s_.flags.pf;
    }
    void promote_z(){s_.gp[7]=s_.gp[1];s_.gp[1]=2u;}
    void build_perpendiculars(){
        auto&g=s_.gp;auto&x=s_.xmm;
        io_.load_float(0x313804u,g[4]+0x10u+g[2]*4u);io_.load_gp(0x313808u,0u,g[4]+0x10u+g[7]*4u);
        x[3]=x[0];io_.negate();
        x[3]=shuffle<210>(x[3]);x[4]=shuffle<201>(x[0]);
        io_.store(0x31381cu,g[4]+0x30u+g[1]*4u,0u);io_.store(0x313824u,g[4]+0x30u+g[2]*4u,g[0]);
        io_.save_float(0x313828u,g[4]+0x30u+g[7]*4u,true);
        io_.load_vector(0x31382cu,1u,g[4]+0x30u);
        x[2]=shuffle<201>(x[1]);x[3]=io_.mul(x[3],x[2]);x[2]=shuffle<210>(x[1]);
        x[0]=io_.mul(x[4],x[2]);x[2]=io_.sub(x[0],x[3]);
    }
    void normalize_first(){
        auto&g=s_.gp;auto&x=s_.xmm;
        x[0]=io_.mul(x[1],x[1]);x[3]=io_.add_scalar(shuffle<85>(x[0]),x[0]);x[4]=shuffle<170>(x[0]);
        io_.load_gp(0x313866u,2u,g[6]+0x50u);
        x[0]=io_.add_scalar(x[4],x[3]);io_.vector_store(0x313870u,g[4]+0x10u,x[0]);
        io_.estimate(0x313875u,3u);io_.store(0x313879u,g[4]+0x10u,low(x[3]));
        io_.load_vector(0x31387fu,5u,g[4]+0x10u);x[0]=io_.mul_scalar(io_.mul_scalar(x[0],x[5]),x[5]);
        io_.store(0x31388cu,g[4]+0xcu,0x40400000u);io_.load_scalar(0x313894u,3u,g[4]+0xcu);
        x[6]=io_.sub_scalar(x[3],x[0]);io_.store(0x3138a1u,g[4]+0xcu,0x3f000000u);
        io_.load_scalar(0x3138a9u,4u,g[4]+0xcu);x[0]=io_.mul_scalar(io_.mul_scalar(x[4],x[5]),x[6]);
        x[5]=io_.mul(shuffle<0>(x[0]),x[1]);
    }
    void normalize_second_and_evaluate_first(){
        auto&g=s_.gp;auto&x=s_.xmm;
        x[0]=io_.mul(x[2],x[2]);x[1]=io_.add_scalar(shuffle<85>(x[0]),x[0]);
        io_.vector_store(0x3138d5u,g[4]+0x30u,x[5]);
        x[5]=shuffle<170>(x[0]);x[0]=io_.add_scalar(x[5],x[1]);
        io_.vector_store(0x3138e8u,g[4]+0x10u,x[0]);
        io_.estimate(0x3138edu,1u);io_.store(0x3138f1u,g[4]+0x10u,low(x[1]));
        io_.load_vector(0x3138f7u,1u,g[4]+0x10u);
        x[0]=io_.mul_scalar(io_.mul_scalar(x[0],x[1]),x[1]);x[3]=io_.sub_scalar(x[3],x[0]);
        g[1]=g[4]+0x40u;io_.push(0x31390cu,g[1]);
        x[4]=io_.mul_scalar(io_.mul_scalar(x[4],x[1]),x[3]);x[0]=x[4];
        io_.push(0x313918u,g[2]);g[0]=g[4]+0x38u;x[0]=io_.mul(shuffle<0>(x[0]),x[2]);
        io_.push(0x313924u,g[0]);g[1]=g[6];io_.vector_store(0x313927u,g[4]+0x1cu,x[0]);
        io_.child(0x31392cu,0x313470u,0x313931u);
    }
    void evaluate_second(){
        auto&g=s_.gp;io_.load_gp(0x313931u,2u,g[6]+0x50u);
        g[1]=g[4]+0x80u;io_.push(0x31393bu,g[1]);io_.push(0x31393cu,g[2]);g[0]=g[4]+0x18u;
        io_.push(0x313941u,g[0]);g[1]=g[6];io_.child(0x313944u,0x313470u,0x313949u);
    }
    bool select_second(){
        auto&g=s_.gp;io_.load_float(0x313949u,g[4]+0x70u);io_.compare_float(0x31394du,g[4]+0xb0u);io_.status_to_ax();
        s_.flags.logic((g[0]>>8u)&0x41u,1);return !s_.flags.zf;
    }
    void append(bool second){
        auto&g=s_.gp;
        if(second){g[2]=g[4]+0x80u;io_.push(0x313974u,g[2]);g[1]=g[6];io_.child(0x313977u,0x313290u,0x31397cu);}
        else{g[1]=g[4]+0x40u;io_.push(0x31395fu,g[1]);g[1]=g[6];io_.child(0x313962u,0x313290u,0x313967u);}
    }
    void finish(bool second){
        auto&g=s_.gp;io_.pop(second?0x31397cu:0x313967u,7u);io_.pop(second?0x31397du:0x313968u,6u);
        g[4]=g[5];io_.pop(second?0x313980u:0x31396bu,5u);io_.ret(second?0x313981u:0x31396cu);
    }
};
} // namespace halo2::native_solver_hull_basis_detail
#endif
