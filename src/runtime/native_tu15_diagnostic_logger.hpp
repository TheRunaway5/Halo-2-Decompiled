#pragma once
#include "halo2/native_tu15_diagnostic_logger_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "runtime/native_tu15_diagnostic_stream.hpp"

namespace halo2::native_diagnostic_logger_detail {
// Actual2E16D0 raw transport; production route admission is separate.
template<class State,class Before,class Scope>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_diagnostic_stream_detail::Io<Scope>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_diagnostic_stream_detail::Io<Scope>>;
public:
    Frame(State&s,Before&before,Scope&scope):Base{s,before,{&scope}}{}
    void resolve(){
        Base::at(0x2e16d0u);Base::s_.gp[0]=native_diagnostic_logger::stream(Base::s_.gp[1]);
        Base::pop(0x2e16d3u);
    }
};
template<class State,class Before=native_diagnostic_stream_detail::Noop>
void run_private(State&s,Before before={}){
    native_raw_x87_detail::Scope scope{s};Frame view{s,before,scope};view.resolve();
}
}
#endif
