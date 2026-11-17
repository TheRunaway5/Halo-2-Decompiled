#pragma once
#include "halo2/native_tu15_ray_filter_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_host_fp_io.hpp"
namespace halo2::native_ray_filter_detail {
struct HostIo {
    template<class Callback>void operator()(Callback&&callback)const{
        native_host_fp_io_detail::preserve(std::forward<Callback>(callback));
    }
};
// Private actual2E9920 caller view. Integer policy keeps the State software
// FP/MMX image opaque; only host callback state is fenced at ordinary I/O.
template<class State,class Before,class Dispatch>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,HostIo> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,HostIo>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;using Base::io;
    Dispatch&dispatch_;
    void byte_store(std::uint32_t pc,std::uint8_t value){
        at(pc);const auto address=s_.gp[0];SourcePhysicsContactRecordStore event{pc,address,{},{std::byte(value)}};
        if(s_.observe){event.before.resize(1);io([&]{s_.memory.read(address,event.before);});}
        io([&]{s_.memory.write(address,event.written);});if(s_.observe)io([&]{s_.observe(event);});
    }
    void finish(std::uint32_t pc){pop(pc);s_.gp[4]+=16u;}
public:
    Frame(State&s,Before&before,Dispatch&dispatch):Base{s,before,HostIo{}},dispatch_(dispatch){}
    void begin(){
        auto&g=s_.gp;at(0x2e9920u);g[2]=word(g[4]+0x10u);push(0x2e9924u,g[6]);g[6]=g[1];
    }
    std::uint32_t collection_info(){
        auto&g=s_.gp;at(0x2e9927u);g[1]=word(g[4]+0x10u);at(0x2e992bu);g[0]=word(g[1]);
        push(0x2e992du,g[2]);at(0x2e992eu);const auto owner=word(g[0]+0x38u);
        const auto address=g[4]-4u;Base::store(0x2e992eu,address,0x2e9931u,[&]{g[4]=address;},true);
        // Genuine child execution is not an observation/provider callback.
        dispatch_(s_,owner,0x2e992eu);return g[0];
    }
    std::uint32_t ray_info(){
        auto&g=s_.gp;at(0x2e9931u);g[1]=word(g[4]+12u);at(0x2e9935u);return g[2]=word(g[1]+0x20u);
    }
    void groups(std::uint32_t first,std::uint32_t second){
        s_.gp[1]=first^second;s_.flags.logic(s_.gp[1]&0xffff0000u,4);
        if(((first^second)&0xffff0000u)==0u)s_.flags.logic(first&0xffff0000u,4);
    }
    void reject(){
        auto&g=s_.gp;at(0x2e994cu);g[0]=word(g[4]+8u);byte_store(0x2e9950u,0u);
        g[6]=pop(0x2e9953u);finish(0x2e9954u);
    }
    std::uint32_t mask(std::uint32_t first,std::uint32_t second){
        auto&g=s_.gp;g[1]=second&31u;g[0]=1u<<g[1];g[2]=s_.flags.logic(first&31u,4);
        at(0x2e9966u);g[1]=word(g[6]+g[2]*4u+12u);const auto value=g[1];g[6]=pop(0x2e996au);return value;
    }
    void accept(std::uint32_t mask,std::uint32_t bit){
        auto&g=s_.gp;s_.flags.logic(mask&bit,4);at(0x2e996du);g[0]=word(g[4]+4u);
        g[1]=(g[1]&0xffffff00u)|std::uint32_t((mask&bit)!=0u);byte_store(0x2e9974u,std::uint8_t(g[1]));finish(0x2e9976u);
    }
};
template<class State,class Before,class Dispatch>void run_private(State&s,Before&before,Dispatch&dispatch){
    Frame view{s,before,dispatch};filter(view);
}
template<class State,class Before>class EmptyInfoFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,HostIo> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,HostIo>;
public:
    EmptyInfoFrame(State&s,Before&before):Base{s,before,HostIo{}}{}
    void run(){this->at(0x212330u);this->s_.gp[0]=this->s_.flags.logic(0u,4);this->pop(0x212332u);this->s_.gp[4]+=4u;}
};
template<class State,class Before>void empty_info_private(State&s,Before&before){EmptyInfoFrame view{s,before};view.run();}
} // namespace halo2::native_ray_filter_detail
