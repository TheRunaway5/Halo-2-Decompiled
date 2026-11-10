#pragma once
#include "halo2/native_tu15_model_marker_lookup_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "physics/runtime/native_tu15_sse_mmx_scope.hpp"

namespace halo2::native_model_marker_lookup_detail {
enum class Observation {actual,canonical};
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class Scope>struct Io{
    Scope*scope;
    template<class Callback>void operator()(Callback&&callback){scope->io(std::forward<Callback>(callback));}
};
inline std::uint32_t offset_flags(std::uint32_t datum){
    std::uint32_t flags{},overflow{};
    // Original AND/SHL producers retain the CPU's undefined shift flags.
    asm volatile("andl $65535,%%ecx;shll $4,%%ecx;seto %%dl;lahf;movzbl %%ah,%%eax;movzbl %%dl,%%edx"
        :"+c"(datum),"=a"(flags),"=d"(overflow)::"cc");
    return flags|((overflow&1u)<<11u);
}
template<class State,class Before,class Scope>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>{
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::word;
    Observation observation_;bool positive_count_{};
    std::uint32_t pc(std::uint32_t actual)const{return observation_==Observation::actual?actual:actual-0x48c0u;}
    std::uint32_t read(std::uint32_t actual,std::uint32_t address){Base::at(pc(actual));return word(address);}
    std::uint32_t pop(std::uint32_t actual){return Base::pop(pc(actual));}
    std::uint32_t finish(bool saved,bool exhausted=false){
        if(saved)s_.gp[6]=pop(exhausted?0x1e11edu:0x1e11f1u);
        pop(exhausted?0x1e11eeu:0x1e11f2u);return s_.gp[0];
    }
public:
    Frame(State&s,Before&before,Scope&scope,Observation observation):
        Base{s,before,Io<Scope>{&scope}},observation_(observation){}
    std::uint32_t invalid(Invalid reason){
        auto&g=s_.gp;g[0]=0xffffffffu;
        if(reason==Invalid::datum)s_.flags.minus(g[1],0xffffffffu,4);
        else if(reason==Invalid::name)s_.flags.minus(g[7],0xffffffffu,4);
        else s_.flags.logic(g[7],4);
        return finish(false);
    }
    void open(std::uint32_t datum,std::uint32_t name){
        auto&g=s_.gp;g[0]=0xffffffffu;s_.flags.logic(name,4);
        const auto address=s_.image==SourcePhysicsContactImage::TitleUpdate15?0x4f0414u:0x4eb114u;
        g[2]=read(0x1e11c1u,address);g[1]=(datum&65535u)<<4u;s_.flags.load(offset_flags(datum));
        g[1]=read(0x1e11d0u,g[1]+g[2]+8u);
        Base::push(pc(0x1e11d4u),g[6]);
    }
    std::uint32_t count(){
        auto&g=s_.gp;g[6]=read(0x1e11d5u,g[1]+0x58u);g[2]=0u;s_.flags.logic(g[6],4);
        positive_count_=std::bit_cast<std::int32_t>(g[6])>0;return positive_count_?g[6]:0u;
    }
    void open_rows(){s_.gp[1]=read(0x1e11deu,s_.gp[1]+0x5cu);}
    std::uint32_t item(std::uint32_t){
        const auto value=read(0x1e11e1u,s_.gp[1]);s_.flags.minus(value,s_.gp[7],4);return value;
    }
    std::uint32_t key()const{return s_.gp[7];}
    std::uint32_t found(std::uint32_t index,std::uint32_t){s_.gp[0]=index;return finish(true);}
    void advance(std::uint32_t,std::uint32_t){
        auto&g=s_.gp;g[2]=s_.flags.increment(g[2],4,false);g[1]=s_.flags.plus(g[1],12u,4);
        s_.flags.minus(g[2],g[6],4);
    }
    std::uint32_t absent(){return finish(true,positive_count_);}
};
template<class State,class Before,class Scope>
std::uint32_t run_scoped(State&s,Before&before,Scope&scope,Observation observation){
    const auto datum=s.gp[1],name=s.gp[7];Frame view{s,before,scope,observation};return lookup(view,datum,name);
}
template<class State,class Before=Noop>
std::uint32_t run_private(State&s,Before before={},Observation observation=Observation::actual){
    if(native_sse_mmx_detail::supported(s)){
        native_sse_mmx_detail::Scope scope{s};return run_scoped(s,before,scope,observation);
    }
    native_raw_x87_detail::Scope scope{s};return run_scoped(s,before,scope,observation);
}
} // namespace halo2::native_model_marker_lookup_detail
#endif
