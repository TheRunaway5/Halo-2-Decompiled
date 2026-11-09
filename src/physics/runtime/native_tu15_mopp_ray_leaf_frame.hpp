#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_mopp_ray_leaf_kernel.hpp"

namespace halo2::native_mopp_ray_caller_detail {
template<class State,class Before,class Dispatch>class LeafFrame:public IntegerFrame<State,Before,ScopeIo<State>> {
    using Base=IntegerFrame<State,Before,ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    using Progress=native_mopp_ray_leaf_detail::Progress;
    using Read=native_mopp_ray_leaf_detail::Read;using Write=native_mopp_ray_leaf_detail::Write;
    native_raw_x87_detail::Scope<State>&scope_;Dispatch&dispatch_;
    void call(std::uint32_t pc,std::uint32_t target_address,std::uint32_t returned){
        at(pc);const auto target=word(target_address);push_positioned(pc,returned);
        scope_.child([&]{dispatch_(s_,target,pc);});
    }
    void push_positioned(std::uint32_t pc,std::uint32_t value){
        const auto address=s_.gp[4]-4u;Base::store(pc,address,value,[&]{s_.gp[4]=address;},true);
    }
    bool response(std::uint32_t pc){at(pc);const auto value=word(s_.gp[0],1);s_.flags.minus(value,0u,1);return value!=0u;}
public:
    LeafFrame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope,Dispatch&dispatch)
        :Base{s,before,ScopeIo<State>{&scope}},scope_(scope),dispatch_(dispatch){}
    std::uint32_t scalar(Read kind,const Progress&){
        auto&g=s_.gp;std::uint32_t pc{},address{};unsigned reg{};
        switch(kind){
        case Read::cache_initial:pc=0x30b5f5u;address=g[4]+0x28u;reg=0;break;
        case Read::key_base:pc=0x30b5f9u;address=g[3]+0x18u;reg=6;break;
        case Read::filter:pc=0x30b5fcu;address=g[0]+0x44u;reg=1;break;
        case Read::filter_collection:pc=0x30b605u;address=g[0]+0x64u;reg=6;break;
        case Read::filter_table:pc=0x30b608u;address=g[1];reg=2;break;
        case Read::cache_after_filter:pc=0x30b620u;address=g[4]+0x28u;reg=0;break;
        case Read::collection:pc=0x30b624u;address=g[0]+0x64u;reg=0;break;
        case Read::collection_table:pc=0x30b627u;address=g[0];reg=2;break;
        case Read::cache_after_shape:pc=0x30b637u;address=g[4]+0x28u;reg=6;break;
        case Read::output:pc=0x30b63bu;address=g[6]+0x58u;reg=1;break;
        case Read::shape_table:s_.flags.logic(g[1]&g[1],4);pc=0x30b640u;address=g[0];reg=2;break;
        case Read::output_after_cast:pc=0x30b658u;address=g[6]+0x58u;reg=0;break;
        case Read::output_fraction:pc=0x30b65fu;address=g[0]+0x14u;reg=2;break;
        case Read::parent:pc=0x30b671u;address=g[6]+0x60u;reg=1;break;
        case Read::parent_field:pc=0x30b678u;address=g[1]+8u;reg=1;break;
        case Read::collector:pc=0x30b67fu;address=g[6]+0x5cu;reg=1;break;
        case Read::collector_after_cast:pc=0x30b699u;address=g[6]+0x5cu;reg=2;break;
        case Read::collector_fraction:pc=0x30b69cu;address=g[2]+4u;reg=0;break;
        }
        at(pc);return g[reg]=word(address);
    }
    void key(const Progress&d){auto&g=s_.gp;g[7]=s_.flags.plus(g[7],d.key_base,4);s_.flags.logic(g[1]&g[1],4);}
    bool filter(const Progress&){
        auto&g=s_.gp;push(0x30b60au,g[7]);push(0x30b60bu,g[6]);g[0]=s_.flags.plus(g[0],0x20u,4);
        push(0x30b60fu,g[0]);g[0]=g[4]+0x47u;push(0x30b614u,g[0]);call(0x30b615u,g[2],0x30b617u);
        return response(0x30b617u);
    }
    std::uint32_t shape(const Progress&){
        auto&g=s_.gp;g[1]=g[4]+0xa0u;push(0x30b630u,g[1]);push(0x30b631u,g[7]);g[1]=g[0];
        call(0x30b634u,g[2]+0x34u,0x30b637u);return g[0];
    }
    bool cast_direct(const Progress&){
        auto&g=s_.gp;push(0x30b644u,g[1]);g[1]=g[6]+0x20u;push(0x30b648u,g[1]);g[1]=g[4]+0x43u;
        push(0x30b64du,g[1]);g[1]=g[0];call(0x30b650u,g[2]+0x20u,0x30b653u);return response(0x30b653u);
    }
    void cast_collector(const Progress&){
        auto&g=s_.gp;push(0x30b682u,g[1]);g[1]=g[4]+0x64u;push(0x30b687u,g[1]);g[1]=g[6]+0x20u;
        push(0x30b68bu,g[1]);g[1]=g[0];Base::store(0x30b68eu,g[4]+0x6cu,g[0],[]{});
        Base::store(0x30b692u,g[4]+0x70u,g[7],[]{});call(0x30b696u,g[2]+0x24u,0x30b699u);
    }
    void store(Write kind,std::uint32_t value,const Progress&d){
        auto&g=s_.gp;std::uint32_t pc{},address{};
        switch(kind){
        case Write::hit:{
            pc=0x30b65bu;address=g[6]+0x50u;at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{std::byte{1}}};
            if(s_.observe){event.before.resize(1);Base::io([&]{s_.memory.read(address,event.before);});}
            const std::array bytes{std::byte{1}};Base::io([&]{s_.memory.write(address,bytes);});
            if(s_.observe)Base::io([&]{s_.observe(event);});return;
        }
        case Write::cache_fraction:pc=d.direct?0x30b662u:0x30b69fu;address=g[6]+0x54u;break;
        // A genuine child may have changed its saved EDI through an alias.
        // The retained source writes the returned register, not an entry key.
        case Write::output_key:pc=0x30b665u;address=g[0]+0x10u;value=g[7];break;
        case Write::parent:pc=0x30b674u;address=g[4]+0x6cu;break;
        case Write::parent_field:pc=0x30b67bu;address=g[4]+0x68u;break;
        }
        Base::store(pc,address,value,[]{});
    }
    void finish(native_mopp_ray_leaf_detail::Result result){
        using Result=native_mopp_ray_leaf_detail::Result;
        if(result!=Result::direct_hit&&result!=Result::collected){Base::finish();return;}
        const bool direct=result==Result::direct_hit;auto&g=s_.gp;
        g[7]=pop(direct?0x30b668u:0x30b6a2u);g[6]=pop(direct?0x30b669u:0x30b6a3u);g[3]=pop(direct?0x30b66au:0x30b6a4u);
        g[4]=g[5];g[5]=pop(direct?0x30b66du:0x30b6a7u);pop(direct?0x30b66eu:0x30b6a8u);g[4]+=12u;
    }
};
}
