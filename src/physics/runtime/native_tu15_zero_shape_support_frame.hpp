#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_zero_shape_support_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
namespace halo2::native_zero_shape_support_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
// The retained API publishes State::push's predecrement on provider failure;
// unchanged-i386 proofs keep the CPU's after-write commit as a separate mode.
enum class Publication { physical,retained };
template<class State,class Before,Publication Mode>class Frame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;
    void saved_bp(){
        constexpr auto pc=0x2e8d20u;const auto value=s_.gp[5];
        if constexpr(Mode==Publication::retained){at(pc);s_.gp[4]-=4u;Base::store(pc,s_.gp[4],value,[]{},true);}
        else push(pc,value);
    }
    void store(std::uint32_t address){
        constexpr std::uint32_t pc=0x2e8d2cu;at(pc);
        if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw native_mopp_ray_caller_detail::AlignedLoadFault{pc,address,flags,flags|0x10000u};}
        const auto point=native_tu15_zero_shape_support_point();const auto data=std::as_bytes(std::span(point));
        for(unsigned offset:{0u,8u}){SourcePhysicsContactRecordStore event{pc,address+offset,{},{}};
            if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}
            const auto bytes=data.subspan(offset,8);fp_.io([&]{s_.memory.write(event.address,bytes);});
            if(s_.observe){event.written.assign(bytes.begin(),bytes.end());fp_.io([&]{s_.observe(event);});}
        }
    }
public:
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope):Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&scope}},fp_(scope){}
    void run(){auto&g=s_.gp;saved_bp();g[5]=g[4];at(0x2e8d23u);g[0]=word(g[5]+12u);
        const auto point=native_tu15_zero_shape_support_point();std::memcpy(&s_.xmm[0],point.data(),16);
        g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);store(g[0]);g[4]=g[5];g[5]=pop(0x2e8d31u);pop(0x2e8d32u);g[4]+=8u;
    }
};
template<Publication Mode=Publication::physical,class State,class Before=Noop>void run_private_owner(State&s,Before before={}){native_raw_x87_detail::Scope scope{s};Frame<State,Before,Mode>frame{s,before,scope};frame.run();}
} // namespace halo2::native_zero_shape_support_detail
#endif
