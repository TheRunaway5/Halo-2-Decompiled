#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_mopp_ray_setup_kernel.hpp"

namespace halo2::native_mopp_ray_setup_detail {
// Private actual-PC 30B890 caller view. No production/default binding.
template<class State,class Before,class Dispatch>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::io;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&scope_;Dispatch&dispatch_;
    static auto packed(Vector value){return native_mopp_ray_detail::packed(value);}
    static Vector words(__m128 value){return native_mopp_ray_detail::words(value);}
public:
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope,Dispatch&dispatch)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&scope}},scope_(scope),dispatch_(dispatch){}
    void begin(){
        auto&g=s_.gp;push(0x30b890u,g[5]);g[5]=g[4];
        g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x48u,4);
    }
    std::uint32_t scalar(Read kind,const Progress&){
        auto&g=s_.gp;std::uint32_t pc{},address{};bool set_eax{};
        switch(kind){
        case Read::collection:pc=0x30b899u;address=g[5]+8u;set_eax=true;break;
        case Read::context:pc=0x30b8a5u;address=g[5]+0x14u;set_eax=true;break;
        case Read::collector:pc=0x30b8abu;address=g[5]+0x18u;set_eax=true;break;
        case Read::fraction:pc=0x30b8b6u;address=g[0]+4u;set_eax=true;break;
        case Read::code:pc=0x30b8bcu;address=g[5]+12u;set_eax=true;break;
        case Read::inverse_scale:pc=0x30b8cau;address=g[0]+12u;break;
        case Read::scale:pc=0x30b8d5u;address=g[0]+12u;break;
        case Read::normalization:pc=0x30b8dbu;address=0x46a1bcu;break;
        case Read::ray:pc=0x30b8e1u;address=g[5]+16u;set_eax=true;break;
        case Read::vector_scale:{
            at(0x30b8fcu);const auto value=word(g[4]+12u);s_.xmm[0]=packed({value,0,0,0});return value;
        }
        case Read::filter_info:pc=0x30b91fu;address=g[0]+0x20u;break;
        case Read::filter:pc=0x30b925u;address=g[0]+0x24u;set_eax=true;break;
        }
        at(pc);const auto value=word(address);
        if(set_eax)g[0]=value;else if(kind==Read::filter_info)g[7]=value;
        return value;
    }
    void unit(){at(0x30b89cu);scope_.require_capacity(1);native_mopp_ray_detail::push(word(0x46a0d0u));}
    void zero_output(){s_.gp[2]=s_.flags.logic(0u,4);}
    void zero_translation(){s_.xmm[0]=packed({0u,0u,0u,0u});Base::vector_store(0x30b8c5u,s_.gp[4]+8u,s_.xmm[0]);}
    void program(){auto&g=s_.gp;push(0x30b8cdu,g[6]);g[6]=g[0]+0x30u;push(0x30b8d1u,g[7]);}
    void store(Store kind,std::uint32_t value,const Progress&){
        auto&g=s_.gp;std::uint32_t pc{},address{};
        switch(kind){
        case Store::collection:pc=0x30b8a2u;address=g[1]+0x64u;break;
        case Store::context:pc=0x30b8a8u;address=g[1]+0x60u;break;
        case Store::collector:pc=0x30b8aeu;address=g[1]+0x5cu;break;
        case Store::output:pc=0x30b8b3u;address=g[1]+0x58u;value=g[2];break;
        case Store::fraction:pc=0x30b8b9u;address=g[1]+0x54u;break;
        case Store::code:pc=0x30b8bfu;address=g[1]+0x10u;break;
        case Store::filter_info:pc=0x30b922u;address=g[1]+0x40u;break;
        case Store::filter:pc=0x30b928u;address=g[1]+0x44u;break;
        case Store::hit:{
            pc=0x30b92bu;address=g[1]+0x50u;at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{std::byte(g[2])}};
            if(s_.observe){event.before.resize(1);io([&]{s_.memory.read(address,event.before);});}
            io([&]{s_.memory.write(address,event.written);});if(s_.observe)io([&]{s_.observe(event);});return;
        }
        default:throw std::logic_error("Invalid ray-setup scalar destination");
        }
        Base::store(pc,address,value,[]{});
    }
    void spill(Store kind,bool pop,const Progress&){
        const auto pc=kind==Store::inverse_scale?0x30b8d2u:kind==Store::variable_scale?0x30b8eeu:0x30b8f8u;
        at(pc);const auto pending=scope_.stage_single(pop);
        const auto address=kind==Store::inverse_scale?s_.gp[1]+0x14u:s_.gp[4]+(pop?12u:0x24u);
        Base::store(pc,address,pending.bits,[&]{scope_.commit(pending);},true);
    }
    Vector vector(Vectors kind,const Progress&d){
        auto&g=s_.gp;unsigned reg{};std::uint32_t pc{},address{};
        switch(kind){
        case Vectors::translation:pc=0x30b8d8u;address=g[0];break;
        case Vectors::start:pc=0x30b8e4u;address=g[0];reg=3;break;
        case Vectors::end:pc=0x30b8e7u;address=g[0]+16u;reg=2;break;
        case Vectors::current_end:s_.xmm[1]=packed({d.scale,d.scale,d.scale,d.scale});pc=0x30b917u;address=g[0]+16u;break;
        }
        const auto value=Base::vector(pc,address);s_.xmm[reg]=value;return words(value);
    }
    Vector subtract(Vector left,Vector right,Arithmetic kind,const Progress&){
        const auto value=native_mopp_ray_detail::subtract(packed(left),packed(right));
        s_.xmm[kind==Arithmetic::end_delta?2:1]=value;return words(value);
    }
    Vector multiply(std::uint32_t scale,Vector value,Arithmetic kind,const Progress&){
        const auto weight=packed({scale,scale,scale,scale});
        const auto result=native_mopp_ray_detail::multiply(weight,packed(value));
        s_.xmm[kind==Arithmetic::start_scaled?4:1]=result;return words(result);
    }
    void store_vector(Store kind,Vector value,const Progress&){
        Base::vector_store(kind==Store::start?0x30b90cu:0x30b91bu,s_.gp[1]+(kind==Store::start?0x20u:0x30u),packed(value));
    }
    void floating(std::uint32_t value){scope_.require_capacity(1);native_mopp_ray_detail::push(value);}
    void divide_scalar(std::uint32_t value){native_mopp_ray_detail::divide(value);}
    void multiply_scalar(std::uint32_t value){native_mopp_ray_detail::multiply(value);}
    void variables(){
        auto&g=s_.gp;Base::store(0x30b92eu,g[4]+0x28u,g[2],[]{});
        Base::store(0x30b932u,g[4]+0x20u,g[2],[]{});Base::store(0x30b936u,g[4]+0x2cu,g[2],[]{});
        g[2]=g[4]+0x30u;push(0x30b93eu,g[2]);push(0x30b93fu,g[6]);g[0]=g[4]+0x18u;
    }
    void arguments(){push(0x30b947u,s_.gp[0]);}
    void bounds(Vector start,Vector end){
        Base::vector_store(0x30b948u,s_.gp[4]+0x3cu,packed(start));
        Base::vector_store(0x30b94du,s_.gp[4]+0x4cu,packed(end));
    }
    void traverse(){
        push(0x30b952u,0x30b957u);scope_.child([&]{dispatch_(s_,0x30aa70u,0x30b952u);});
    }
    void finish(){auto&g=s_.gp;g[7]=pop(0x30b957u);g[6]=pop(0x30b958u);g[4]=g[5];g[5]=pop(0x30b95bu);pop(0x30b95cu);g[4]+=20u;}
};
template<class State,class Before,class Dispatch>void run_private(State&s,Before&before,Dispatch&dispatch){
    native_raw_x87_detail::Scope scope{s};Frame frame{s,before,scope,dispatch};prepare(frame);
}
} // namespace halo2::native_mopp_ray_setup_detail
