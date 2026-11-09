#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_mopp_ray_resume_kernel.hpp"

namespace halo2::native_mopp_ray_caller_detail {
template<class State,class Before>class ResumeFrame:public IntegerFrame<State,Before,ScopeIo<State>> {
    using Base=IntegerFrame<State,Before,ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;
    using Progress=native_mopp_ray_resume_detail::Progress;
    using Scalar=native_mopp_ray_resume_detail::Scalar;
    using Vectors=native_mopp_ray_resume_detail::Vectors;
    using Store=native_mopp_ray_resume_detail::Store;
    using Arithmetic=native_mopp_ray_resume_detail::Arithmetic;
    using Comparison=native_mopp_ray_resume_detail::Comparison;
    using Vec=native_mopp_ray_resume_detail::Vector;
    native_raw_x87_detail::Scope<State>&scope_;
    static auto packed(Vec value){return native_mopp_ray_detail::packed(value);}
    static Vec words(__m128 value){return native_mopp_ray_detail::words(value);}
public:
    ResumeFrame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope)
        :Base{s,before,ScopeIo<State>{&scope}},scope_(scope){}
    std::uint32_t scalar(Scalar kind,const Progress&d){
        const bool inc=d.increasing;const auto sp=s_.gp[4];std::uint32_t pc{},address{};int reg=-1;bool single{};
        switch(kind){
        case Scalar::product_end:pc=inc?0x30af2du:0x30b082u;address=sp+(inc?0x18u:0x30u);break;
        case Scalar::product_start:pc=inc?0x30af31u:0x30b086u;address=sp+(inc?0x2cu:0x34u);break;
        case Scalar::epsilon:pc=inc?0x30af35u:0x30b08au;address=0x46a0e8u;break;
        case Scalar::fraction_start:pc=inc?0x30af42u:0x30b097u;address=sp+(inc?0x2cu:0x34u);break;
        case Scalar::fraction_end:pc=inc?0x30af4au:0x30b09fu;address=sp+(inc?0x18u:0x30u);break;
        case Scalar::fraction_numerator:pc=inc?0x30af55u:0x30b0aau;address=sp+(inc?0x2cu:0x34u);break;
        case Scalar::interpolation_fraction:pc=inc?0x30af5du:0x30b0b2u;address=sp+(inc?0x14u:0x30u);single=true;break;
        case Scalar::current_fraction:pc=inc?0x30af80u:0x30b0d5u;address=s_.gp[1]+0x54u;break;
        case Scalar::unit_fraction:pc=inc?0x30af83u:0x30b0d8u;address=0x46a0d0u;break;
        case Scalar::shortening_fraction:pc=inc?0x30af94u:0x30b0e9u;address=s_.gp[1]+0x54u;reg=inc?2:0;break;
        case Scalar::saved_fraction:pc=inc?0x30afb3u:0x30b101u;address=sp+(inc?0x70u:0x74u);single=true;break;
        case Scalar::scale:pc=inc?0x30afd7u:0x30b125u;address=s_.gp[3]+0x14u;reg=inc?1:0;break;
        case Scalar::saved_scale:pc=inc?0x30afe2u:0x30b130u;address=sp+(inc?0x7cu:0x78u);single=true;break;
        case Scalar::axis_endpoint:pc=inc?0x30b00au:0x30b15bu;address=s_.gp[7]+0x10u+s_.gp[0]*4u;break;
        case Scalar::axis_plane:pc=inc?0x30b00eu:0x30b15fu;address=sp+(inc?0x1cu:0x20u);break;
        }
        at(pc);const auto value=word(address);if(reg>=0)s_.gp[reg]=value;
        if(single)s_.xmm[0]=packed(Vec{value,0,0,0});return value;
    }
    Vec vector(Vectors kind,const Progress&d){
        const bool inc=d.increasing;std::uint32_t pc{},address{},reg{};
        switch(kind){
        case Vectors::bounds_end:pc=inc?0x30af46u:0x30b09bu;address=s_.gp[7]+0x10u;reg=1;break;
        case Vectors::bounds_start:pc=inc?0x30af6du:0x30b0c2u;address=s_.gp[7];reg=1;break;
        case Vectors::interpolation_one:pc=inc?0x30af4eu:0x30b0a3u;address=0x41c500u;reg=3;break;
        case Vectors::original_end:pc=inc?0x30af97u:0x30b0ecu;address=s_.gp[1]+0x30u;reg=2;break;
        case Vectors::shortening_one:pc=inc?0x30af9eu:0x30b0f0u;address=0x41c500u;reg=4;break;
        case Vectors::code_info:pc=inc?0x30afa5u:0x30b0fau;address=s_.gp[inc?0:2];reg=1;break;
        case Vectors::original_start:pc=inc?0x30afc3u:0x30b111u;address=s_.gp[1]+0x20u;reg=2;break;
        case Vectors::scaled_end:pc=inc?0x30afdau:0x30b128u;address=s_.gp[7]+0x10u;reg=1;break;
        case Vectors::translation:pc=inc?0x30aff6u:0x30b14bu;address=s_.gp[3];reg=0;break;
        case Vectors::translated_end:pc=inc?0x30aff9u:0x30b14eu;address=s_.gp[7]+0x10u;reg=1;break;
        }
        const auto value=Base::vector(pc,address);s_.xmm[reg]=value;return words(value);
    }
    void cache(const Progress&d){at(d.increasing?0x30af7cu:0x30b0d1u);s_.gp[1]=word(s_.gp[4]+0x28u);}
    void code(const Progress&d){at(d.increasing?0x30af9bu:0x30b0f7u);s_.gp[d.increasing?0:2]=word(s_.gp[1]+0x10u);}
    std::uint32_t axis(const Progress&d){at(d.increasing?0x30afa8u:0x30b136u);const auto value=word(s_.gp[4]+0x3cu);s_.gp[0]=value;s_.flags.minus(value,3u,4);return value;}
    void store_scalar(Store kind,std::uint32_t value,const Progress&d){
        const bool fraction=kind==Store::saved_fraction,inc=d.increasing;
        const auto pc=fraction?(inc?0x30afafu:0x30b0fdu):(inc?0x30afdeu:0x30b12cu);
        const auto offset=fraction?(inc?0x70u:0x74u):(inc?0x7cu:0x78u);
        Base::store(pc,s_.gp[4]+offset,value,[]{});
    }
    void spill(Store,const Progress&d){
        const auto pc=d.increasing?0x30af59u:0x30b0aeu;at(pc);const auto pending=scope_.stage_single(true);
        Base::store(pc,s_.gp[4]+(d.increasing?0x14u:0x30u),pending.bits,[&]{scope_.commit(pending);},true);
    }
    void store_vector(Store kind,Vec value,const Progress&d){
        const bool inc=d.increasing;std::uint32_t pc{};
        if(kind==Store::bounds_start)pc=inc?0x30af79u:0x30b0ceu;
        else if(kind==Store::shortened_end)pc=inc?0x30afd3u:0x30b121u;
        else if(kind==Store::scaled_end)pc=inc?0x30aff2u:0x30b147u;
        else pc=inc?0x30b000u:0x30b155u;
        Base::vector_store(pc,s_.gp[7]+(kind==Store::bounds_start?0u:0x10u),packed(value));
    }
    void publish_comparison(Comparison kind,native_mopp_ray_detail::Comparison value,const Progress&d){
        s_.gp[0]=(s_.gp[0]&0xffff0000u)|value.status;
        s_.flags.logic((value.status>>8u)&(kind==Comparison::axis&&!d.increasing?0x41u:5u),1);
    }
    void floating(std::uint32_t value){scope_.require_capacity(1);native_mopp_ray_detail::push(value);}
    void multiply_scalar(std::uint32_t value){native_mopp_ray_detail::multiply(value);}
    void subtract_scalar(std::uint32_t value){native_mopp_ray_detail::subtract(value);}
    void reverse_divide(std::uint32_t value){native_mopp_ray_detail::reverse_divide(value);}
    auto compare(std::uint32_t value){return native_mopp_ray_detail::compare(value);}
    Vec broadcast(std::uint32_t value,Arithmetic kind,const Progress&){
        const Vec result{value,value,value,value};s_.xmm[kind==Arithmetic::scale?2:0]=packed(result);return result;
    }
    Vec multiply(Vec left,Vec right,Arithmetic kind,const Progress&){
        const auto value=native_mopp_ray_detail::multiply(packed(left),packed(right));
        s_.xmm[kind==Arithmetic::interpolation_right||kind==Arithmetic::scale?2:kind==Arithmetic::interpolation_left||kind==Arithmetic::shortening_right?3:4]=value;return words(value);
    }
    Vec subtract(Vec left,Vec right,Arithmetic kind,const Progress&){
        const auto value=native_mopp_ray_detail::subtract(packed(left),packed(right));s_.xmm[kind==Arithmetic::translation?1:kind==Arithmetic::interpolation_left?3:4]=value;return words(value);
    }
    Vec add(Vec left,Vec right,Arithmetic kind,const Progress&){
        const auto value=native_mopp_ray_detail::add(packed(left),packed(right));s_.xmm[kind==Arithmetic::interpolation_left?3:4]=value;return words(value);
    }
    void advance(const Progress&d){
        if(!d.increasing){at(0x30b16eu);s_.gp[0]=word(s_.gp[4]+0x24u);at(0x30b172u);s_.gp[0]=s_.flags.minus(s_.gp[0],word(s_.gp[4]+0x14u),4);s_.gp[6]=s_.flags.plus(s_.gp[6],s_.gp[0],4);}
    }
    void after_resume(bool finished){at(finished?0x30b6afu:0x30ae02u);}
};
}
