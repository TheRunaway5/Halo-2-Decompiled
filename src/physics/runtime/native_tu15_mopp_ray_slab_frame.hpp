#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_mopp_ray_slab_kernel.hpp"

namespace halo2::native_mopp_ray_caller_detail {
template<class State,class Before>class SlabFrame:public IntegerFrame<State,Before,ScopeIo<State>> {
    using Base=IntegerFrame<State,Before,ScopeIo<State>>;
    using Read=native_mopp_ray_slab_detail::Read;
    using Write=native_mopp_ray_slab_detail::Write;
    using Data=native_mopp_ray_slab_detail::Progress;
    using Comparison=native_mopp_ray_slab_detail::Comparison;
    using VectorRead=native_mopp_ray_slab_detail::VectorRead;
    using VectorMath=native_mopp_ray_slab_detail::VectorMath;
    using Vec=native_mopp_ray_slab_detail::Vector;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>&scope_;
    static auto packed(Vec value){return native_mopp_ray_detail::packed(value);}
    static Vec words(__m128 value){return native_mopp_ray_detail::words(value);}
    void combine(unsigned destination,unsigned other){
        auto&g=s_.gp;g[destination]=native_mopp_aabb_abi_detail::shifted_immediate<8>(s_,g[destination]);
        g[destination]=s_.flags.plus(g[destination],g[other],4);
    }
    void publish(Comparison kind,native_mopp_ray_detail::Comparison comparison){
        s_.gp[0]=(s_.gp[0]&0xffff0000u)|comparison.status;
        const auto mask=kind==Comparison::upper_gate?0x41u:5u;
        s_.flags.logic((comparison.status>>8u)&mask,1);
    }
public:
    using Base::begin;using Base::finish;using Base::publish_program_class;
    SlabFrame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope)
        :Base{s,before,ScopeIo<State>{&scope}},scope_(scope){}
    std::uint32_t scalar(Read kind,const Data&d){
        auto&g=s_.gp;std::uint32_t pc{},address{};
        switch(kind){
        case Read::low_high:return Base::load(1,d.wide?0x30b17du:0x30b1dbu,g[6]+1u,1);
        case Read::low_middle:return Base::load(2,d.wide?0x30b181u:0x30b1dfu,g[6]+2u,1);
        case Read::low_low:combine(1,2);return Base::load(2,0x30b18au,g[6]+3u,1);
        case Read::high_middle:combine(1,2);return Base::load(2,0x30b193u,g[6]+5u,1);
        case Read::high_high:return Base::load(1,0x30b1a9u,g[6]+4u,1);
        case Read::high_low:
            g[1]=native_mopp_aabb_abi_detail::shifted_immediate<8>(s_,g[1]);g[2]=s_.flags.plus(g[2],g[1],4);
            return Base::load(1,0x30b1b2u,g[6]+6u,1);
        case Read::cache_low:return Base::load(1,0x30b19bu,g[4]+0x28u);
        case Read::cache_high:return Base::load(2,0x30b1c2u,g[4]+0x28u);
        case Read::decode_low:pc=d.wide?0x30b1a2u:0x30b1edu;address=g[4]+0x14u;break;
        case Read::decode_high:pc=d.wide?0x30b1ccu:0x30b1f5u;address=g[4]+0x14u;break;
        case Read::cache_scale_low:pc=0x30b1a6u;address=g[1]+0x14u;break;
        case Read::cache_scale_high:pc=0x30b1d0u;address=g[2]+0x14u;break;
        case Read::variable_scale_low:combine(2,1);pc=0x30b1bbu;address=g[3]+0x14u;break;
        case Read::variable_scale_high:pc=0x30b1d3u;address=g[3]+0x14u;break;
        case Read::translation_low:pc=0x30b1c6u;address=g[3]+g[0]*4u;break;
        case Read::translation_high:pc=0x30b1d6u;address=g[3]+g[0]*4u;break;
        case Read::start:pc=0x30b1f9u;address=g[7]+g[0]*4u;break;
        case Read::end:pc=0x30b200u;address=g[7]+0x10u+g[0]*4u;break;
        case Read::direction_start:pc=0x30b208u;address=g[4]+0x24u;break;
        case Read::direction_end:pc=0x30b20cu;address=g[4]+0x18u;break;
        case Read::lower_gate:pc=d.reverse?0x30b23du:0x30b217u;address=g[4]+(d.reverse?0x24u:0x18u);break;
        case Read::upper_gate:pc=d.reverse?0x30b24eu:0x30b228u;address=g[4]+(d.reverse?0x18u:0x24u);break;
        case Read::clip_start:pc=d.clipping?0x30b2c9u:0x30b264u;address=g[4]+0x24u;break;
        case Read::clip_end:pc=d.clipping?0x30b2d3u:0x30b275u;address=g[4]+0x18u;break;
        case Read::delta_test:pc=d.clipping?0x30b2d7u:0x30b279u;address=g[4]+0x14u;break;
        case Read::delta_ratio:pc=d.clipping?0x30b2eeu:0x30b28cu;address=g[4]+0x14u;break;
        case Read::epsilon:pc=d.clipping?0x30b2ddu:0x30b27fu;address=0x46a0e8u;break;
        case Read::numerator:pc=d.clipping?0x30b2f7u:0x30b2a0u;address=g[4]+0x14u;break;
        case Read::ratio:{
            at(d.clipping?0x30b2ffu:0x30b2aau);const auto value=word(g[4]+0x14u);
            s_.xmm[0]=packed(Vec{value,0,0,0});return value;
        }
        }
        at(pc);return word(address);
    }
    void store(Write kind,std::uint32_t value,const Data&d){
        if(kind!=Write::decode_low&&kind!=Write::decode_high)throw std::logic_error("Unknown slab integer store");
        const auto pc=d.wide?(kind==Write::decode_low?0x30b197u:0x30b1beu):(kind==Write::decode_low?0x30b1e3u:0x30b1f1u);
        Base::store(pc,s_.gp[4]+0x14u,value,[]{});
    }
    void spill(Write kind,bool pop,const Data&d){
        std::uint32_t pc{},offset{};
        switch(kind){
        case Write::start:pc=0x30b1fcu;offset=0x24u;break;
        case Write::end:pc=0x30b204u;offset=0x18u;break;
        case Write::delta:pc=d.clipping?0x30b2cfu:0x30b271u;offset=0x14u;break;
        case Write::ratio:pc=d.clipping?0x30b2fbu:0x30b2a6u;offset=0x14u;break;
        default:throw std::logic_error("Unknown slab spill");
        }
        at(pc);const auto pending=scope_.stage_single(pop);
        Base::store(pc,s_.gp[4]+offset,pending.bits,[&]{scope_.commit(pending);},true);
    }
    Vec vector(VectorRead kind,const Data&d){
        const auto pc=kind==VectorRead::start?0x30b268u:kind==VectorRead::end?0x30b26bu:d.clipping?0x30b30fu:0x30b290u;
        const auto address=kind==VectorRead::one?0x41c500u:s_.gp[7]+(kind==VectorRead::end?0x10u:0u);
        const auto reg=kind==VectorRead::start?1u:kind==VectorRead::end?2u:d.clipping?2u:4u;
        const auto value=Base::vector(pc,address);s_.xmm[reg]=value;return words(value);
    }
    Vec broadcast(std::uint32_t value){const auto result=Vec{value,value,value,value};s_.xmm[0]=packed(result);return result;}
    Vec subtract(Vec a,Vec b,VectorMath,const Data&d){const auto value=native_mopp_ray_detail::subtract(packed(a),packed(b));s_.xmm[d.clipping?2:4]=value;return words(value);}
    Vec multiply(Vec a,Vec b,VectorMath kind,const Data&d){const auto value=native_mopp_ray_detail::multiply(packed(a),packed(b));s_.xmm[kind==VectorMath::last_term?3:d.clipping?2:4]=value;return words(value);}
    Vec add(Vec a,Vec b,VectorMath,const Data&d){const auto value=native_mopp_ray_detail::add(packed(a),packed(b));s_.xmm[d.clipping?2:4]=value;return words(value);}
    void store_vector(Write,Vec value,const Data&d){
        const auto address=d.clipping?s_.gp[7]+s_.gp[1]:s_.gp[2]+0x10u;
        Base::vector_store(d.clipping?0x30b31fu:0x30b2c3u,address,packed(value));
    }
    void axis(const Data&d){s_.gp[0]=s_.flags.plus(s_.gp[0],d.wide?0xffffffd7u:0xffffffdau,4);}
    void advance(std::uint32_t,const Data&d){s_.gp[6]=s_.flags.plus(s_.gp[6],d.wide?7u:3u,4);}
    void direction(const Data&d){if(d.reverse)s_.gp[1]=1;else s_.gp[1]=s_.flags.logic(0,4);}
    void prepare_ratio(const Data&d){
        if(d.clipping)s_.gp[1]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,s_.gp[1]);
        else{s_.gp[0]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,s_.gp[1]);s_.gp[2]=s_.gp[7];}
    }
    void ratio_target(const Data&d){if(!d.clipping)s_.gp[2]=s_.flags.minus(s_.gp[2],s_.gp[0],4);}
    native_mopp_ray_detail::Comparison compare_scalar(std::uint32_t value,Comparison kind,const Data&){auto result=native_mopp_ray_detail::compare(value);publish(kind,result);return result;}
    native_mopp_ray_detail::Comparison compare_stack(bool third,Comparison kind,const Data&){auto result=native_mopp_ray_detail::compare_second(third);publish(kind,result);return result;}
    void integer(std::uint32_t value){scope_.require_capacity(1);native_mopp_ray_detail::integer(std::bit_cast<std::int32_t>(value));}
    void floating(std::uint32_t value){scope_.require_capacity(1);native_mopp_ray_detail::push(value);}
    void multiply_scalar(std::uint32_t value){native_mopp_ray_detail::multiply(value);}
    void subtract_scalar(std::uint32_t value){native_mopp_ray_detail::subtract(value);}
    void reverse_subtract(std::uint32_t value){native_mopp_ray_detail::reverse_subtract(value);}
    void reverse_divide(std::uint32_t value){native_mopp_ray_detail::reverse_divide(value);}
    void subtract_second(){native_mopp_ray_detail::subtract_second();}
    void multiply_second(){native_mopp_ray_detail::multiply_second();}
    void discard(){native_mopp_ray_detail::discard();}
    void before_next(){at(0x30ae02u);}
};
template<class State,class Before>
bool slab(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope,std::uint32_t opcode,std::uint32_t&program){
    SlabFrame frame{s,before,scope};native_mopp_ray_slab_detail::Progress progress;
    return native_mopp_ray_slab_detail::clip(frame,program,opcode,progress)==native_mopp_ray_slab_detail::Decision::rejected;
}
} // namespace halo2::native_mopp_ray_caller_detail
