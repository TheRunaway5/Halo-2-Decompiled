#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_mopp_ray_crossing_kernel.hpp"

namespace halo2::native_mopp_ray_caller_detail {
template<class State,class Before>class CrossingFrame:public IntegerFrame<State,Before,ScopeIo<State>> {
    using Base=IntegerFrame<State,Before,ScopeIo<State>>;
    using Progress=native_mopp_ray_crossing_detail::Progress;
    using Scalar=native_mopp_ray_crossing_detail::Scalar;
    using Vectors=native_mopp_ray_crossing_detail::Vectors;
    using Store=native_mopp_ray_crossing_detail::Store;
    using Result=native_mopp_ray_crossing_detail::Result;
    using Vec=native_mopp_ray_crossing_detail::Vector;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>&scope_;
    static auto packed(Vec value){return native_mopp_ray_detail::packed(value);}
    static Vec words(__m128 value){return native_mopp_ray_detail::words(value);}
public:
    CrossingFrame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope)
        :Base{s,before,ScopeIo<State>{&scope}},scope_(scope){}
    std::uint32_t scalar(Scalar kind,const Progress&d){
        std::uint32_t pc{},address{};const auto sp=s_.gp[4];
        switch(kind){
        case Scalar::low_initial:pc=0x30ae75u;address=sp+0x20u;break;
        case Scalar::low_end:pc=0x30aea2u;address=sp+0x20u;break;
        case Scalar::high_start:pc=0x30ae96u;address=sp+0x1cu;break;
        case Scalar::high_end:pc=0x30aeaeu;address=sp+0x1cu;break;
        case Scalar::end_for_low:pc=0x30ae9eu;address=sp+0x18u;break;
        case Scalar::end_for_high:pc=0x30aeaau;address=sp+0x18u;break;
        case Scalar::start_low:pc=0x30aeb6u;address=sp+0x34u;break;
        case Scalar::direction_end_low:pc=0x30aebau;address=sp+0x30u;break;
        case Scalar::product_end:pc=d.increasing?0x30aec9u:0x30b022u;address=sp+(d.increasing?0x30u:0x18u);break;
        case Scalar::product_start:pc=d.increasing?0x30aecdu:0x30b026u;address=sp+(d.increasing?0x34u:0x2cu);break;
        case Scalar::epsilon:pc=d.increasing?0x30aed1u:0x30b02au;address=0x46a0e8u;break;
        case Scalar::fraction_start:pc=d.increasing?0x30aedeu:0x30b037u;address=sp+(d.increasing?0x34u:0x2cu);break;
        case Scalar::fraction_end:pc=d.increasing?0x30aee2u:0x30b03bu;address=sp+(d.increasing?0x30u:0x18u);break;
        case Scalar::fraction_numerator:pc=d.increasing?0x30aee6u:0x30b03fu;address=sp+(d.increasing?0x34u:0x2cu);break;
        case Scalar::fraction_bits:pc=d.increasing?0x30aeeeu:0x30b047u;address=sp+(d.increasing?0x14u:0x2cu);break;
        }
        at(pc);const auto value=word(address);
        if(kind==Scalar::fraction_bits)s_.xmm[0]=packed(Vec{value,0,0,0});return value;
    }
    Vec vector(Vectors kind,const Progress&d){
        const auto reg=kind==Vectors::start?1u:2u;
        const auto pc=kind==Vectors::start?0x30ae79u:kind==Vectors::end?0x30ae7eu:d.increasing?0x30aefeu:0x30b057u;
        const auto address=kind==Vectors::one?0x41c500u:s_.gp[7]+(kind==Vectors::end?16u:0u);
        const auto value=Base::vector(pc,address);s_.xmm[reg]=value;return words(value);
    }
    void store_vector(Store kind,Vec value,const Progress&d){
        const bool initial=!d.clipped;
        const auto pc=kind==Store::child_start?0x30ae82u:initial?0x30ae8au:d.increasing?0x30af0eu:0x30b067u;
        Base::vector_store(pc,s_.gp[4]+(kind==Store::child_start?0x80u:0x90u),packed(value));
    }
    void spill(Store kind,const Progress&d){
        std::uint32_t pc{},offset{};
        switch(kind){
        case Store::start_low:pc=0x30ae92u;offset=0x34u;break;
        case Store::start_high:pc=0x30ae9au;offset=0x2cu;break;
        case Store::end_low:pc=0x30aea6u;offset=0x30u;break;
        case Store::end_high:pc=0x30aeb2u;offset=0x18u;break;
        case Store::fraction:pc=d.increasing?0x30aeeau:0x30b043u;offset=d.increasing?0x14u:0x2cu;break;
        default:throw std::logic_error("Unknown crossing scalar destination");
        }
        at(pc);const auto pending=scope_.stage_single(true);
        Base::store(pc,s_.gp[4]+offset,pending.bits,[&]{scope_.commit(pending);},true);
    }
    void publish_comparison(native_mopp_ray_crossing_detail::Comparison,native_mopp_ray_detail::Comparison value){
        s_.gp[0]=(s_.gp[0]&0xffff0000u)|value.status;s_.flags.logic((value.status>>8u)&5u,1);
    }
    void floating(std::uint32_t value){scope_.require_capacity(1);native_mopp_ray_detail::push(value);}
    void reverse_subtract_second(){native_mopp_ray_detail::reverse_subtract_second();}
    void subtract_scalar(std::uint32_t value){native_mopp_ray_detail::subtract(value);}
    void multiply_scalar(std::uint32_t value){native_mopp_ray_detail::multiply(value);}
    void reverse_divide(std::uint32_t value){native_mopp_ray_detail::reverse_divide(value);}
    auto compare(std::uint32_t value){return native_mopp_ray_detail::compare(value);}
    Vec broadcast(std::uint32_t value){const Vec result{value,value,value,value};s_.xmm[0]=packed(result);return result;}
    Vec multiply(Vec left,Vec right,Result result){const auto value=native_mopp_ray_detail::multiply(packed(left),packed(right));s_.xmm[result==Result::right?3:2]=value;return words(value);}
    Vec subtract(Vec left,Vec right,Result){const auto value=native_mopp_ray_detail::subtract(packed(left),packed(right));s_.xmm[2]=value;return words(value);}
    Vec add(Vec left,Vec right,Result){const auto value=native_mopp_ray_detail::add(packed(left),packed(right));s_.xmm[2]=value;return words(value);}
    void before_recursion(const Progress&d){at(d.increasing?0x30af16u:0x30b06fu);}
};
}
