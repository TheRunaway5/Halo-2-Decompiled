#pragma once
#include "halo2/native_tu15_diagnostic_stream_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <utility>

// Typed controller/frame ownership. Formatter, writer/copy, singleton/factory
// and platform service dependencies retain their separate qualification scope.
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"

namespace halo2::native_diagnostic_stream_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
template<class Scope>struct Io {
    Scope*scope;
    template<class Callback>void operator()(Callback&&callback){scope->io(std::forward<Callback>(callback));}
};

template<class State,class Before,class Scope>class TextView:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
public:
    TextView(State&s,Before&before,Scope&scope):Base{s,before,Io<Scope>{&scope}}{}
    void open_text(){
        at(0x2e2e00u);s_.gp[0]=word(s_.gp[4]+4u);s_.gp[2]=s_.gp[0]+1u;
    }
    std::uint32_t next_byte(){
        at(0x2e2e07u);const auto value=word(s_.gp[0],1);s_.set(1,value,1,0);
        s_.gp[0]=s_.flags.increment(s_.gp[0],4,false);s_.flags.logic(value,1);return value;
    }
    void finish_text(std::uint32_t bytes){
        s_.flags.minus(s_.gp[0],s_.gp[2],4);s_.gp[0]=bytes;pop(0x2e2e10u);
    }
};

template<class State,class Before,class Call,class Scope>class StreamView:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::before_;using Base::at;using Base::word;
    using Base::push;using Base::pop;using Base::store;
    Call&call_;Scope&scope_;
    void handoff(std::uint32_t target,std::uint32_t site,bool tail){
        scope_.child([&]{call_(s_,target,site,tail);});
    }
public:
    StreamView(State&s,Before&before,Call&call,Scope&scope):
        Base{s,before,Io<Scope>{&scope}},call_(call),scope_(scope){}
    void reserve_message(){
        auto&g=s_.gp;
        at(0x2e3080u);g[1]=word(g[4]+8u);
        g[4]=s_.flags.minus(g[4],1024u,4);
        push(0x2e308au,g[6]);push(0x2e308bu,g[7]);
        g[0]=g[4]+0x414u;push(0x2e3093u,g[0]);push(0x2e3094u,g[1]);
        g[2]=g[4]+16u;push(0x2e3099u,1024u);push(0x2e309eu,g[2]);
    }
    void format_message(){
        push(0x2e309fu,0x2e30a4u);
        format_tail();
    }
    void format_tail(){
        // Actual2E2D10 is a one-instruction tail to the genuine CRT owner.
        at(0x2e2d10u);handoff(0x32ba80u,0x2e2d10u,true);
    }
    void capture_output(){
        auto&g=s_.gp;
        at(0x2e30a4u);g[0]=word(g[4]+0x41cu);
        at(0x2e30abu);g[6]=word(g[0]+8u);
        at(0x2e30aeu);g[7]=word(g[6]);
    }
    std::uint32_t measure_message(){
        auto&g=s_.gp;g[1]=g[4]+0x18u;
        push(0x2e30b4u,g[1]);push(0x2e30b5u,0x2e30bau);
        TextView text{s_,before_,scope_};const auto bytes=native_diagnostic_stream::measure(text);
        g[4]=s_.flags.plus(g[4],20u,4);return bytes;
    }
    void append_message(std::uint32_t bytes){
        auto&g=s_.gp;push(0x2e30bdu,bytes);g[2]=g[4]+12u;
        push(0x2e30c2u,g[2]);g[1]=g[6];
        at(0x2e30c5u);const auto target=word(g[7]+8u);
        const auto address=g[4]-4u;
        store(0x2e30c5u,address,0x2e30c8u,[&]{g[4]=address;},true);
        handoff(target,0x2e30c5u,false);
    }
    std::uint32_t release_message(){
        auto&g=s_.gp;g[7]=pop(0x2e30c8u);g[6]=pop(0x2e30c9u);
        g[4]=s_.flags.plus(g[4],1024u,4);pop(0x2e30d0u);return g[0];
    }
    void flush_output(){
        auto&g=s_.gp;at(0x2e30e0u);g[1]=word(g[1]+8u);
        at(0x2e30e3u);g[0]=word(g[1]);
        at(0x2e30e5u);const auto target=word(g[0]+12u);
        handoff(target,0x2e30e5u,true);
    }
};

template<class State,class Call,class Before=Noop>
void print(State&s,Call call,Before before={}){
    native_raw_x87_detail::Scope scope{s};StreamView view{s,before,call,scope};
    native_diagnostic_stream::print(view);
}
template<class State,class Call,class Before=Noop>
void flush(State&s,Call call,Before before={}){
    native_raw_x87_detail::Scope scope{s};StreamView view{s,before,call,scope};
    native_diagnostic_stream::flush(view);
}
template<class State,class Call,class Before=Noop>
void format_tail(State&s,Call call,Before before={}){
    native_raw_x87_detail::Scope scope{s};StreamView view{s,before,call,scope};view.format_tail();
}
template<class State,class Before=Noop>void measure(State&s,Before before={}){
    native_raw_x87_detail::Scope scope{s};TextView view{s,before,scope};
    native_diagnostic_stream::measure(view);
}
} // namespace halo2::native_diagnostic_stream_detail
#endif
