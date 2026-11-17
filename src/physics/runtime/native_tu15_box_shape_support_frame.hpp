#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_box_shape_support_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
namespace halo2::native_box_shape_support_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
// The retained API publishes State::push's predecrement on provider failure;
// unchanged-i386 proofs keep the CPU's after-write commit as a separate mode.
enum class Publication { physical,retained };
template<class State,class Before,Publication Mode>class Frame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;using Base::vector;
    native_raw_x87_detail::Scope<State>&fp_;
    void saved_bp(){
        constexpr auto pc=0x306790u;const auto value=s_.gp[5];
        if constexpr(Mode==Publication::retained){at(pc);s_.gp[4]-=4u;Base::store(pc,s_.gp[4],value,[]{},true);}
        else push(pc,value);
    }
    template<class Vector>void store_vector(std::uint32_t pc,std::uint32_t address,Vector value){
        at(pc);if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw native_mopp_ray_caller_detail::AlignedLoadFault{pc,address,flags,flags|0x10000u};}
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16);
        for(unsigned offset:{0u,8u}){SourcePhysicsContactRecordStore event{pc,address+offset,{},{}};
            if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}
            const auto data=std::span(bytes).subspan(offset,8);fp_.io([&]{s_.memory.write(event.address,data);});
            if(s_.observe){event.written.assign(data.begin(),data.end());fp_.io([&]{s_.observe(event);});}
        }
    }
public:
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope):Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&scope}},fp_(scope){}
    void run(){auto&g=s_.gp;saved_bp();g[5]=g[4];s_.xmm[0]=vector(0x306793u,g[1]+16u);
        at(0x306797u);g[0]=word(g[5]+12u);at(0x30679au);g[1]=word(g[5]+8u);store_vector(0x30679du,g[0],s_.xmm[0]);
        s_.xmm[1]=vector(0x3067a0u,0x461960u);s_.xmm[2]=vector(0x3067a7u,g[1]);
        std::array<std::uint32_t,4>extent{},sign{},direction{};std::memcpy(extent.data(),&s_.xmm[0],16);std::memcpy(sign.data(),&s_.xmm[1],16);std::memcpy(direction.data(),&s_.xmm[2],16);
        const auto point=native_tu15_box_shape_support_point(extent,sign,direction);std::memcpy(&s_.xmm[2],point.masked_direction.data(),16);std::memcpy(&s_.xmm[0],point.point.data(),16);
        g[2]=point.feature;g[2]=s_.flags.logic(g[2]&7u,4);g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[2]=s_.flags.logic(g[2]|0x3f000000u,4);
        store_vector(0x3067bfu,g[0],s_.xmm[0]);Base::store(0x3067c2u,g[0]+12u,g[2],[]{});
        g[4]=g[5];g[5]=pop(0x3067c7u);pop(0x3067c8u);g[4]+=8u;
    }
};
template<Publication Mode=Publication::physical,class State,class Before=Noop>void run_private_owner(State&s,Before before={}){native_raw_x87_detail::Scope scope{s};Frame<State,Before,Mode>frame{s,before,scope};frame.run();}
} // namespace halo2::native_box_shape_support_detail
#endif
