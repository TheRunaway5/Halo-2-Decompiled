#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_mopp_primitive_key_kernel.hpp"

namespace halo2::native_mopp_ray_caller_detail {
// Private and unbound. This transports encoded primitive-key DATA through the
// raw caller view; leaf fields and filter/shape outcomes have separate owners.
template<class State,class Before>class PrimitiveKeyFrame:public IntegerFrame<State,Before,ScopeIo<State>> {
    using Base=IntegerFrame<State,Before,ScopeIo<State>>;
    using Progress=native_mopp_primitive_key_detail::Progress;
    using Base::s_;
    void combine(unsigned destination,unsigned low){
        auto&g=s_.gp;g[destination]=native_mopp_aabb_abi_detail::shifted_immediate<8>(s_,g[destination]);
        g[destination]=s_.flags.plus(g[destination],g[low],4);
    }
public:
    using Base::encoded_byte;using Base::decoded;
    PrimitiveKeyFrame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope)
        :Base{s,before,ScopeIo<State>{&scope}}{}
    void before_leaf_fields(){Base::at(0x30b5f5u);}
    std::uint32_t encoded_byte(const Progress&d){
        const auto n=d.index,address=d.program+n+1u;
        if(d.count==1u)return Base::load(7,0x30b5a3u,address,1);
        if(d.count==2u)return Base::load(n?2:7,n?0x30b5adu:0x30b5a9u,address,1);
        if(d.count==3u)return Base::load(n==0?7:n==1?0:1,std::array{0x30b5b8u,0x30b5bcu,0x30b5c0u}[n],address,1);
        if(n==3u)combine(0,2);
        return Base::load(n==0?0:n==2?1:2,std::array{0x30b5d0u,0x30b5d4u,0x30b5d8u,0x30b5e1u}[n],address,1);
    }
    void decoded(const Progress&d){
        auto&g=s_.gp;
        if(!d.count)g[7]=d.encoded;
        else if(d.count==2u)combine(7,2);
        else if(d.count==3u){combine(7,0);combine(7,1);}
        else if(d.count==4u){
            g[0]=native_mopp_aabb_abi_detail::shifted_immediate<16>(s_,g[0]);
            g[1]=native_mopp_aabb_abi_detail::shifted_immediate<8>(s_,g[1]);
            g[2]=s_.flags.plus(g[2],g[0],4);g[7]=g[2]+g[1];
        }
    }
};
}
