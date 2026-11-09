#pragma once
#include "halo2/native_tu15_camera_collision_publication_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "physics/runtime/native_tu15_sse_mmx_scope.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_camera_collision_publication_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
template<class Scope>struct Io {Scope*scope;template<class Callback>void operator()(Callback&&callback){scope->io(std::forward<Callback>(callback));}};
template<class State,class Before,class Scope>class Frame:public native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    void normal_pair(const Progress&d){s_.xmm[0]=d.normal;s_.xmm[4]=d.term;}
public:
    Frame(State&s,Before&before,Scope&scope):Base{s,before,Io<Scope>{&scope}}{}
    void begin(Progress&d){d.output=s_.gp[0];d.transform=s_.gp[1];d.collision=s_.gp[2];s_.flags.logic(d.transform,4);push(0x16b9a2u,s_.gp[6]);}
    void copy_begin(const Progress&){push(0x16baf1u,s_.gp[7]);}
    void copy_end(const Progress&){s_.gp[7]=pop(0x16bb0bu);}
    void finish(const Progress&){s_.gp[6]=pop(0x16bb32u);pop(0x16bb33u);}
    std::uint32_t integer(Read field,const Progress&d){
        std::uint32_t pc{},address{};unsigned width=4,reg=1,register_width=4;
        switch(field){
        case Read::fraction:pc=0x16b9a3u;address=s_.gp[2];reg=6;break;
        case Read::plane:
            pc=d.transformed?0x16b9aeu:d.flipped?0x16bab6u:0x16baeeu;address=s_.gp[2]+4u;reg=d.transformed||!d.flipped?6:1;break;
        case Read::index:pc=d.transformed?0x16ba6eu:0x16baafu;address=s_.gp[2]+16u;break;
        case Read::copied_value:{constexpr std::array sites{0x16baf2u,0x16baf9u,0x16baffu,0x16bb05u};pc=sites[d.lane];address=s_.gp[6]+d.lane*4u;reg=d.lane==3?6:7;break;}
        case Read::point:{constexpr std::array sites{0x16bb0cu,0x16bb12u,0x16bb18u};pc=sites[d.lane];address=s_.gp[2]+8u+d.lane*4u;break;}
        case Read::kind:pc=0x16bb1eu;address=s_.gp[2]+20u;width=1;register_width=1;break;
        case Read::surface:pc=0x16bb24u;address=s_.gp[2]+21u;width=1;register_width=1;break;
        case Read::material:pc=0x16bb2au;address=s_.gp[2]+22u;width=2;register_width=2;reg=2;break;
        default:throw std::runtime_error("Invalid raw camera publication integer phase");
        }
        at(pc);const auto value=word(address,width);s_.set(reg,value,register_width,0);
        if(field==Read::index)s_.flags.logic(s_.reg(1,1,8),1);
        if(field==Read::copied_value&&d.lane==0)s_.gp[1]=s_.gp[0]+40u;
        return value;
    }
    Scalar scalar(Read field,const Progress&d){
        std::uint32_t pc{},address{};int destination=-1;
        switch(field){
        case Read::plane_y:pc=0x16b9b1u;address=s_.gp[6]+4u;destination=2;break;
        case Read::plane_z:pc=0x16b9b6u;address=s_.gp[6]+8u;destination=3;break;
        case Read::row_x_z:pc=0x16b9bbu;address=s_.gp[1]+28u;destination=0;break;
        case Read::row_x_y:pc=0x16b9c0u;address=s_.gp[1]+16u;destination=4;break;
        case Read::plane_x:pc=0x16b9c5u;address=s_.gp[6];destination=1;break;
        case Read::row_x_x:normal_pair(d);pc=0x16b9d5u;address=s_.gp[1]+4u;destination=4;break;
        case Read::row_y_z:pc=0x16b9e7u;address=s_.gp[1]+32u;destination=0;break;
        case Read::row_y_y:pc=0x16b9ecu;address=s_.gp[1]+20u;destination=4;break;
        case Read::row_y_x:normal_pair(d);pc=0x16b9fdu;address=s_.gp[1]+8u;destination=4;break;
        case Read::row_z_z:pc=0x16ba0fu;address=s_.gp[1]+36u;destination=0;break;
        case Read::row_z_y:s_.xmm[0]=d.normal;pc=0x16ba18u;address=s_.gp[1]+24u;destination=3;break;
        case Read::row_z_x:s_.xmm[0]=d.normal;s_.xmm[3]=d.term;pc=0x16ba21u;address=s_.gp[1]+12u;destination=2;break;
        case Read::distance_z:pc=0x16ba37u;address=s_.gp[1]+48u;destination=1;break;
        case Read::plane_w:s_.xmm[1]=d.distance;pc=0x16ba40u;address=s_.gp[6]+12u;destination=0;break;
        case Read::scale:s_.xmm[0]=d.term;pc=0x16ba45u;address=s_.gp[1];break;
        case Read::distance_y:s_.xmm[0]=d.term;s_.xmm[1]=d.distance;pc=0x16ba4du;address=s_.gp[1]+44u;destination=0;break;
        case Read::stored_y:s_.xmm[0]=d.term;pc=0x16ba52u;address=s_.gp[0]+44u;break;
        case Read::stored_x:s_.xmm[0]=d.term;s_.xmm[1]=d.distance;pc=0x16ba5bu;address=s_.gp[0]+40u;destination=0;break;
        case Read::distance_x:s_.xmm[0]=d.term;pc=0x16ba60u;address=s_.gp[1]+40u;break;
        case Read::negated_value:{
            if(d.lane==0)s_.xmm[0]=Scalar{};if(d.lane<3)s_.xmm[1]=Scalar{};
            constexpr std::array transformed{0x16ba7fu,0x16ba8cu,0x16ba99u,0x16baa3u};
            constexpr std::array plain{0x16babfu,0x16bacbu,0x16bad8u,0x16bae2u};
            pc=d.transformed?transformed[d.lane]:plain[d.lane];address=(d.transformed?s_.gp[0]+40u:s_.gp[1])+d.lane*4u;break;
        }
        default:throw std::runtime_error("Invalid raw camera publication scalar phase");
        }
        at(pc);const auto value=N::scalar(word(address));if(destination>=0)s_.xmm[unsigned(destination)]=value;return value;
    }
    void store(Write field,const Progress&d,std::uint32_t value){
        std::uint32_t pc{},address{};unsigned width=4;
        switch(field){
        case Write::fraction:pc=0x16b9a5u;address=s_.gp[0]+4u;break;
        case Write::normal:{
            constexpr std::array sites{0x16b9e2u,0x16ba0au,0x16ba32u,0x16ba69u};pc=sites[d.lane];address=s_.gp[0]+40u+d.lane*4u;
            if(d.lane<2)normal_pair(d);else if(d.lane==2){s_.xmm[0]=d.normal;s_.xmm[3]=d.term;s_.xmm[2]=d.other_term;}else{s_.xmm[0]=d.term;s_.xmm[1]=d.distance;}
            break;
        }
        case Write::negated_value:{
            constexpr std::array transformed{0x16ba84u,0x16ba91u,0x16ba9eu,0x16baa8u};constexpr std::array plain{0x16bac3u,0x16bad0u,0x16baddu,0x16bae7u};
            pc=d.transformed?transformed[d.lane]:plain[d.lane];address=s_.gp[0]+40u+d.lane*4u;s_.xmm[d.lane==3?0:1]=d.negative;break;
        }
        case Write::copied_value:{constexpr std::array sites{0x16baf7u,0x16bafcu,0x16bb02u,0x16bb08u};pc=sites[d.lane];address=s_.gp[1]+d.lane*4u;break;}
        case Write::point:{constexpr std::array sites{0x16bb0fu,0x16bb15u,0x16bb1bu};pc=sites[d.lane];address=s_.gp[0]+76u+d.lane*4u;break;}
        case Write::kind:pc=0x16bb21u;address=s_.gp[0]+88u;width=1;break;
        case Write::surface:pc=0x16bb27u;address=s_.gp[0]+89u;width=1;break;
        case Write::material:pc=0x16bb2eu;address=s_.gp[0]+90u;width=2;break;
        }
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);Base::io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));
        Base::io([&]{s_.memory.write(address,std::span(bytes).first(width));});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);Base::io([&]{s_.observe(event);});}
    }
};
template<class State,class Before=Noop>void run_private(State&s,Before before={}){
    const auto execute=[&](auto&scope){Frame frame{s,before,scope};Progress d;publish(frame,d);};
    if constexpr(requires{s.active_mmx;})if(s.active_mmx){native_sse_mmx_detail::Scope scope{s};execute(scope);return;}
    native_raw_x87_detail::Scope scope{s};execute(scope);
}
} // namespace halo2::native_camera_collision_publication_detail
#endif
