#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_camera_instance_query_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include <stdexcept>

namespace halo2::native_camera_instance_query_detail {
template<class Scope>struct Io {Scope*scope;template<class Function>void operator()(Function&&f){scope->io(std::forward<Function>(f));}};
template<unsigned Factor,class State>std::uint32_t product(State&s,std::uint32_t value){
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);unsigned overflow{};
    asm volatile("addb $127,%%al;sahf;imull %3,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(overflow),"+r"(value):"i"(Factor):"cc");
    s.flags.load((flags&0xd5u)|((overflow&1u)<<11u));return value;
}
// Private full-controller adapter in progress. The qualified DATA algorithm
// is complete; later ABI stages deliberately stop until independently proved.
template<class State,class Before,class Scope,class Dispatch>class Frame:public native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    Scope&scope_;Dispatch&dispatch_;
    void child(std::uint32_t pc,std::uint32_t target,std::uint32_t returned){push(pc,returned);scope_.child([&]{dispatch_(s_,target,pc);});}
    [[noreturn]]static void pending(){throw std::runtime_error("Native16C000 controller ABI stage is unqualified");}
public:
    using Scalar=__m128;using Data=Progress<Scalar>;
    Frame(State&s,Before&before,Scope&scope,Dispatch&dispatch):Base{s,before,Io<Scope>{&scope}},scope_(scope),dispatch_(dispatch){}
    std::uint32_t begin(const Data&){auto&g=s_.gp;g[4]=s_.flags.minus(g[4],0x468u,4);push(0x16c006u,g[5]);push(0x16c007u,g[6]);push(0x16c008u,g[7]);g[6]=g[0];return g[6];}
    std::uint32_t word(Read kind,const Data&){
        auto&g=s_.gp;std::uint32_t pc{},address{};unsigned width=4,reg{};
        switch(kind){
        case Read::scenario:pc=0x16c00bu;address=0x4ecc18u;reg=0;break;
        case Read::instances:pc=0x16c010u;address=g[0]+0x144u;reg=2;break;
        case Read::definitions:pc=0x16c016u;address=g[0]+0x13cu;reg=1;break;
        case Read::mask:pc=0x16c01cu;address=g[4]+0x478u;reg=0;break;
        case Read::definition_index:g[7]=product<0x58u>(s_,g[6]);pc=0x16c028u;address=g[2]+g[7]+0x34u;reg=5;width=2;break;
        default:pending();
        }
        at(pc);const auto value=Base::word(address,width);
        g[reg]=kind==Read::definition_index?std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value)))):value;
        return value;
    }
    bool admit(const Data&){auto&g=s_.gp;g[7]=s_.flags.plus(g[7],g[2],4);g[5]=product<0xc8u>(s_,g[5]);g[5]=s_.flags.plus(g[5],g[1],4);push(0x16c037u,g[0]);g[2]=g[5];child(0x16c03au,0x16b810u,0x16c03fu);s_.flags.logic(g[0]&255u,1);return(g[0]&255u)!=0;}
    void resume(Child kind,Data&d){if(kind!=Child::admission)pending();d.index=s_.gp[6];d.instance=s_.gp[7];d.definition=s_.gp[5];}
    bool near_sphere(const Data&){auto&g=s_.gp;at(0x16c047u);g[1]=Base::word(g[7]+0x48u);at(0x16c04au);g[2]=Base::word(g[4]+0x484u);push(0x16c051u,g[1]);at(0x16c052u);g[1]=Base::word(g[4]+0x484u);g[0]=g[7]+0x3cu;child(0x16c05cu,0x120fa0u,0x16c061u);pending();}
    bool finish(bool found,const Data&){if(found)pending();auto&g=s_.gp;g[7]=pop(0x16c280u);g[6]=pop(0x16c281u);s_.set(0,s_.flags.logic(0u,1),1,0);g[5]=pop(0x16c284u);g[4]=s_.flags.plus(g[4],0x468u,4);pop(0x16c28bu);g[4]+=16u;return false;}
    void inverse(const Data&){pending();}
    void phase(Phase,const Data&){pending();}
    Scalar scalar(Read,const Data&){pending();}
    Scalar multiply(Scalar,Scalar){pending();}
    Scalar add(Scalar,Scalar){pending();}
    bool different(Scalar,Scalar,Comparison,const Data&){pending();}
    void store(Write,Scalar,const Data&){pending();}
    void prepare_bitmap(const Data&){pending();}
    std::uint32_t bitmap(const Data&){pending();}
    bool bsp(const Data&){pending();}
    std::uint32_t default_material(const Data&){pending();}
    void store_word(Write,std::uint32_t,const Data&){pending();}
    void publish(const Data&){pending();}
};
template<class State,class Before,class Dispatch>void run_private(State&s,Before before,Dispatch dispatch){
    native_raw_x87_detail::Scope scope{s};Frame frame{s,before,scope,dispatch};Progress<__m128>d;query(frame,d);
}
}
#endif
