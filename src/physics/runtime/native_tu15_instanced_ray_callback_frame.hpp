#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_instanced_ray_callback_kernel.hpp"

namespace halo2::native_instanced_ray_callback_detail {
template<class State,class Before,class Dispatch>
class ForwardFrame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&scope_;Dispatch&dispatch_;
    static auto scalar(std::uint32_t value){return native_mopp_ray_detail::packed({value,0,0,0});}
public:
    ForwardFrame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope,Dispatch&dispatch)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&scope}},scope_(scope),dispatch_(dispatch){}
    void begin(){auto&g=s_.gp;push(0x24def0u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x20u,4);}
    std::uint32_t collector(){at(0x24def9u);return s_.gp[1]=word(s_.gp[5]+0x10u);}
    std::uint32_t one(){at(0x24defcu);const auto value=word(0x46a0d0u);s_.xmm[0]=scalar(value);return value;}
    std::uint32_t table(const Forward&){at(0x24df04u);return s_.gp[0]=word(s_.gp[1]);}
    std::uint32_t packet(){return s_.gp[2]=s_.gp[4];}
    void push_packet(const Forward&){push(0x24df09u,s_.gp[2]);}
    std::uint32_t body(){at(0x24df0au);return s_.gp[2]=word(s_.gp[5]+12u);}
    void push_body(const Forward&){push(0x24df0du,s_.gp[2]);}
    void fraction(const Forward&d){Base::store(0x24df0eu,s_.gp[4]+0x1cu,d.one,[]{});}
    std::uint32_t owner(const Forward&){at(0x24df14u);return word(s_.gp[0]);}
    void invoke(const Forward&d){const auto address=s_.gp[4]-4u;Base::store(0x24df14u,address,0x24df16u,[&]{s_.gp[4]=address;},true);scope_.child([&]{dispatch_(s_,d.owner,0x24df14u,false);});}
    void finish(){auto&g=s_.gp;g[4]=g[5];g[5]=pop(0x24df18u);pop(0x24df19u);g[4]+=12u;}
};

template<class State,class Before,class Dispatch>
class CollectorFrame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&scope_;Dispatch&dispatch_;bool any_hit_{};
    static auto scalar(std::uint32_t value){return native_mopp_ray_detail::packed({value,0,0,0});}
    void narrow(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);Base::io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));
        Base::io([&]{s_.memory.write(address,std::span(bytes).first(width));});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);Base::io([&]{s_.observe(event);});}
    }
public:
    CollectorFrame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope,Dispatch&dispatch)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&scope}},scope_(scope),dispatch_(dispatch){}
    std::uint32_t body(){at(0x24df20u);return s_.gp[0]=word(s_.gp[4]+4u);}
    std::uint32_t instance(const Collector&){at(0x24df24u);return s_.gp[0]=word(s_.gp[0]+4u);}
    std::uint32_t epoch(){at(0x24df27u);return s_.gp[2]=word(0x4f44ecu);}
    std::uint32_t begin(const Collector&){push(0x24df2du,s_.gp[3]);push(0x24df2eu,s_.gp[6]);return s_.gp[6]=s_.gp[1];}
    void index(const Collector&d){s_.gp[1]=d.index;}
    std::uint32_t read(Read kind,const Collector&d){
        auto&g=s_.gp;std::uint32_t pc{},address{};int reg=-1;bool simd{};
        switch(kind){
        case Read::stamp:pc=0x24df34u;address=0x4f44f0u+g[1]*4u;break;
        case Read::result_before:pc=0x24df3du;address=g[6]+0x1cu;reg=3;break;
        case Read::displacement:pc=0x24df47u;address=g[6]+0x14u;reg=1;break;
        case Read::origin:pc=0x24df4au;address=g[6]+0x10u;reg=2;break;
        case Read::filter:pc=0x24df4eu;address=g[6]+0xcu;reg=1;break;
        case Read::mask:pc=0x24df52u;address=g[6]+8u;reg=2;break;
        case Read::accepted_mask:pc=0x24df60u;address=g[6]+8u;break;
        case Read::result_after:pc=0x24df7au;address=g[6]+0x1cu;reg=0;break;
        case Read::current:pc=0x24df7du;address=g[6]+4u;simd=true;break;
        case Read::candidate:pc=0x24df82u;address=g[0]+4u;break;
        case Read::selected:pc=d.keep?0x24df9bu:0x24df88u;address=d.keep?g[6]+4u:g[0]+4u;simd=true;break;
        }
        at(pc);const auto value=word(address);if(reg>=0)g[unsigned(reg)]=value;if(simd)s_.xmm[0]=scalar(value);
        if(kind==Read::accepted_mask)s_.flags.logic(value&0x10000000u,4);return value;
    }
    void compare_stamp(const Collector&d){s_.flags.minus(d.stamp,d.epoch,4);}
    void write(Write kind,std::uint32_t value,const Collector&d){
        if(kind==Write::stamp){Base::store(0x24df40u,0x4f44f0u+s_.gp[1]*4u,value,[]{});return;}
        const auto pc=kind==Write::fraction?(any_hit_?0x24df6cu:d.keep?0x24dfa0u:0x24df8du):any_hit_?0x24df71u:d.keep?0x24dfa5u:0x24df92u;
        narrow(pc,s_.gp[6]+(kind==Write::fraction?4u:0x18u),value,kind==Write::fraction?4u:1u);
    }
    void push_displacement(const Collector&){push(0x24df4du,s_.gp[1]);}
    void push_origin(const Collector&){push(0x24df51u,s_.gp[2]);}
    void push_filter(const Collector&){push(0x24df55u,s_.gp[1]);}
    void push_mask(const Collector&){push(0x24df56u,s_.gp[2]);}
    bool query(const Collector&){push(0x24df57u,0x24df5cu);scope_.child([&]{dispatch_(s_,0x16c000u,0x24df57u,false);});s_.flags.logic(s_.gp[0]&255u,1);return (s_.gp[0]&255u)!=0u;}
    void resumed(Collector&d){d.collector=s_.gp[6];}
    void zero(const Collector&){s_.xmm[0]=_mm_setzero_ps();any_hit_=true;}
    bool compare(const Collector&d){s_.flags.scalar_compare(s_.xmm[0],scalar(d.candidate),false);return s_.flags.cf||s_.flags.zf;}
    void finish(Return result){
        const auto pc=result==Return::any_hit?0x24df75u:result==Return::replace?0x24df96u:0x24dfa9u;
        s_.gp[6]=pop(pc);s_.gp[3]=pop(pc+1u);pop(pc+2u);s_.gp[4]+=8u;
    }
};

template<class State,class Before,class Dispatch>
void collect_retained(State&s,Before&before,Dispatch&dispatch){
    native_raw_x87_detail::Scope scope{s};CollectorFrame frame{s,before,scope,dispatch};Collector data;
    const auto result=collect(frame,data);frame.finish(result);
}
template<class State,class Before,class Dispatch>
void forward_retained(State&s,Before&before,Dispatch&dispatch){
    native_raw_x87_detail::Scope scope{s};ForwardFrame frame{s,before,scope,dispatch};Forward data;forward(frame,data);
}
} // namespace halo2::native_instanced_ray_callback_detail
