#pragma once
#include "halo2/native_tu15_effect_reference_element_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_host_fp_io.hpp"

namespace halo2::native_effect_reference_element_detail {
struct HostIo {
    template<class Function>void operator()(Function&&function)const{
        native_host_fp_io_detail::preserve(std::forward<Function>(function));
    }
};
// Hardware integer flags include the architecturally unspecified flag bits
// observed at an interrupted original instruction. FP/MMX state stays opaque.
template<bool Multiply,class State>std::uint32_t scaled(State&s,std::uint32_t value){
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);unsigned overflow{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(Multiply)
        asm volatile("addb $127,%%al;sahf;imull $56,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+a"(flags),"+c"(overflow),"+r"(value)::"cc");
    else
        asm volatile("addb $127,%%al;sahf;shll $4,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+a"(flags),"+c"(overflow),"+r"(value)::"cc");
    s.flags.load((flags&0xd5u)|((overflow&1u)<<11u));return value;
#else
    static_assert(sizeof(State)==0,"Original effect-reference ABI requires x86 integer flags");
#endif
}
template<class State,class Before,class Dispatch>class ResolverFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,HostIo> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,HostIo>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    Dispatch&dispatch_;
    void child(std::uint32_t pc,std::uint32_t target,std::uint32_t returned){push(pc,returned);dispatch_(s_,target,pc);}
    void finish(std::uint32_t restore,std::uint32_t returned){s_.gp[6]=pop(restore);pop(returned);}
public:
    ResolverFrame(State&s,Before&before,Dispatch&dispatch):Base{s,before,HostIo{}},dispatch_(dispatch){}
    std::uint32_t group(){
        auto&g=s_.gp;at(0x178070u);g[2]=word(0x4f0414u);push(0x178076u,g[6]);g[6]=g[1];
        at(0x178079u);g[0]=word(g[6]+0x10u);
        g[1]=std::uint32_t(std::int32_t(std::int16_t(g[0]&65535u)));
        g[1]=scaled<false>(s_,g[1]);at(0x178082u);return g[1]=word(g[1]+g[2]);
    }
    void classified(std::uint32_t group){
        s_.flags.minus(group,0x65666665u,4);
        if(group>0x65666665u)s_.flags.minus(group,0x70727433u,4);
        else if(group!=0x65666665u){s_.flags.minus(group,0x5052544du,4);if(group!=0x5052544du)s_.flags.minus(group,0x62736474u,4);}
    }
    void static_element(){
        auto&g=s_.gp;g[0]=s_.flags.logic(g[0]&65535u,4);g[0]=scaled<false>(s_,g[0]);
        at(0x1780a7u);g[2]=word(g[0]+g[2]+8u);at(0x1780abu);g[0]=word(g[6]+14u,2);
        at(0x1780afu);g[1]=word(g[2]+0x18u);g[0]=scaled<true>(s_,g[0]);g[0]=s_.flags.plus(g[0],g[1],4);
        finish(0x1780b7u,0x1780b8u);
    }
    void effect_element(){
        auto&g=s_.gp;at(0x1780b9u);g[1]=word(g[6]+0x14u);g[0]=s_.flags.logic(g[0]&65535u,4);
        g[1]=scaled<true>(s_,g[1]);g[0]=scaled<false>(s_,g[0]);at(0x1780c7u);g[0]=word(g[0]+g[2]+8u);
        at(0x1780cbu);g[2]=word(g[0]+0x18u);at(0x1780ceu);g[0]=word(g[6]+14u,2);
        at(0x1780d2u);g[6]=word(g[2]+g[1]+0x34u);g[0]=scaled<true>(s_,g[0]);g[0]=s_.flags.plus(g[0],g[6],4);
        finish(0x1780dbu,0x1780dcu);
    }
    void parent_element(){
        auto&g=s_.gp;at(0x1780e9u);g[1]=word(g[6]+0x48u);child(0x1780ecu,0x178070u,0x1780f1u);
        at(0x1780f1u);g[2]=word(g[0]+4u);child(0x1780f4u,0x13a6b0u,0x1780f9u);
        at(0x1780f9u);g[1]=word(g[6]+14u,2);at(0x1780fdu);g[2]=word(g[0]);push(0x1780ffu,g[1]);g[1]=g[0];
        at(0x178102u);const auto target=word(g[2]+0x1cu);const auto sp=g[4]-4u;
        Base::store(0x178102u,sp,0x178105u,[&]{g[4]=sp;},true);dispatch_(s_,target,0x178102u);
        finish(0x178105u,0x178106u);
    }
    void missing(){s_.gp[0]=s_.flags.logic(0u,4);finish(0x1780e7u,0x1780e8u);}
};
template<class State,class Before,class Dispatch>class AccessorFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,HostIo> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,HostIo>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    Dispatch&dispatch_;
public:
    AccessorFrame(State&s,Before&before,Dispatch&dispatch):Base{s,before,HostIo{}},dispatch_(dispatch){}
    void run(){
        auto&g=s_.gp;g[1]=std::uint32_t(std::int32_t(std::int16_t(g[2]&65535u)));push(0x13a6b3u,g[6]);
        at(0x13a6b4u);g[6]=word(0x4f0414u);g[1]=scaled<false>(s_,g[1]);at(0x13a6bdu);g[1]=word(g[1]+g[6]);
        g[0]=s_.flags.logic(0u,4);s_.flags.minus(g[1],0x5052544du,4);const bool model=g[1]==0x5052544du;
        if(!model)s_.flags.minus(g[1],0x70727433u,4);
        if(model||g[1]==0x70727433u){
            g[6]=model?0x486048u:0x48603cu;at(model?0x13a6e8u:0x13a6d7u);g[0]=word(g[6]);
            push(model?0x13a6eau:0x13a6d9u,g[2]);g[1]=g[6];const auto pc=model?0x13a6edu:0x13a6dcu;
            at(pc);const auto target=word(g[0]+0x50u);const auto sp=g[4]-4u;
            Base::store(pc,sp,model?0x13a6f0u:0x13a6dfu,[&]{g[4]=sp;},true);dispatch_(s_,target,pc);g[0]=g[6];
            if(!model){g[6]=pop(0x13a6e1u);pop(0x13a6e2u);return;}
        }
        g[6]=pop(0x13a6f2u);pop(0x13a6f3u);
    }
};
template<class State,class Before>class DefinitionFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,HostIo> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,HostIo>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
public:
    DefinitionFrame(State&s,Before&before):Base{s,before,HostIo{}}{}
    std::uint32_t handle(){at(0x22b790u);return s_.gp[0]=word(s_.gp[4]+4u);}
    std::uint32_t table(){at(0x22b794u);return s_.gp[2]=word(0x4f0414u);}
    void publish_handle(std::uint32_t handle){Base::store(0x22b79au,s_.gp[1]+4u,handle,[]{});}
    std::uint32_t definition(std::uint32_t,std::uint32_t){
        auto&g=s_.gp;g[0]=s_.flags.logic(g[0]&65535u,4);g[0]=scaled<false>(s_,g[0]);
        at(0x22b7a5u);return g[0]=word(g[0]+g[2]+8u);
    }
    void publish_definition(std::uint32_t definition){Base::store(0x22b7a9u,s_.gp[1]+8u,definition,[]{});}
    void finish(){pop(0x22b7acu);s_.gp[4]+=4u;}
};
template<class State,class Before>class ElementFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,HostIo> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,HostIo>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
    bool model_;
public:
    ElementFrame(State&s,Before&before,bool model):Base{s,before,HostIo{}},model_(model){}
    std::uint32_t index(){at(model_?0x22b710u:0x22aca0u);return s_.gp[0]=word(s_.gp[4]+4u);}
    std::uint32_t definition(){at(model_?0x22b714u:0x22aca4u);return s_.gp[1]=word(s_.gp[1]+8u);}
    std::uint32_t scale(std::uint32_t){return s_.gp[0]=scaled<true>(s_,s_.gp[0]);}
    std::uint32_t elements(std::uint32_t){at(model_?0x22b71au:0x22acaau);return word(s_.gp[1]+(model_?0x7cu:0x88u));}
    void finish(std::uint32_t,std::uint32_t elements){s_.gp[0]=s_.flags.plus(s_.gp[0],elements,4);pop(model_?0x22b71du:0x22acb0u);s_.gp[4]+=4u;}
};
template<class State,class Before,class Dispatch>void run_private(State&s,std::uint32_t entry,Before&before,Dispatch&dispatch){
    switch(entry){
    case 0x178070u:{ResolverFrame view{s,before,dispatch};resolve(view);break;}
    case 0x13a6b0u:{AccessorFrame view{s,before,dispatch};view.run();break;}
    case 0x22b790u:{DefinitionFrame view{s,before};bind_definition(view);break;}
    case 0x22aca0u:case 0x22b710u:{ElementFrame view{s,before,entry==0x22b710u};element(view);break;}
    default:throw std::runtime_error("Unknown native effect-reference owner");
    }
}
} // namespace halo2::native_effect_reference_element_detail
