#pragma once
#include "physics/runtime/native_tu15_mopp_ray_integer_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "halo2/native_tu15_mopp_ray_scale_kernel.hpp"
#include "halo2/native_tu15_mopp_ray_traversal_kernel.hpp"

namespace halo2::native_mopp_ray_caller_detail {
template<class State>struct ScopeIo {
    native_raw_x87_detail::Scope<State>*scope;
    template<class Callback>void operator()(Callback&&callback)const{
        scope->io(std::forward<Callback>(callback));
    }
};

// Private caller view for the shared authored-ray quantization controller.
// Geometry stays in that value controller. This view publishes retained GP,
// XMM and actual x87 state at lazy DATA boundaries, and owns virtual spills.
// Construct Scope before this view and keep it alive until the view returns.
template<class State,class Before>class ScaleFrame:public IntegerFrame<State,Before,ScopeIo<State>> {
    using Base=IntegerFrame<State,Before,ScopeIo<State>>;
    using Read=native_mopp_ray_scale_detail::Read;
    using Write=native_mopp_ray_scale_detail::Write;
    using Progress=native_mopp_ray_scale_detail::Progress;
    using Vec=native_mopp_ray_scale_detail::Vector;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>&scope_;
    Read last_read_{};
    static auto packed(Vec value){return native_mopp_ray_detail::packed(value);}
    static Vec words(__m128 value){return native_mopp_ray_detail::words(value);}
    static std::pair<std::uint32_t,unsigned>destination(Write kind){
        switch(kind){
        case Write::scratch_x:return{0x30b375u,0x14u};
        case Write::scratch_y:return{0x30b381u,0x14u};
        case Write::scratch_z:return{0x30b39au,0x14u};
        case Write::scratch_scale:return{0x30b3bdu,0x14u};
        case Write::translation_x:return{0x30b38au,0x60u};
        case Write::translation_y:return{0x30b3a2u,0x64u};
        case Write::translation_z:return{0x30b3aau,0x68u};
        case Write::translation_w:return{0x30b38eu,0x6cu};
        case Write::scale_word:return{0x30b3cfu,0x14u};
        case Write::variable_shift:return{0x30b412u,0x50u};
        case Write::variable_scale:return{0x30b41cu,0x54u};
        case Write::variable_property:return{0x30b423u,0x5cu};
        case Write::variable_key:return{0x30b42au,0x58u};
        default:throw std::logic_error("Non-scalar ray quantization destination");
        }
    }
public:
    using Base::advance;using Base::finish;
    ScaleFrame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope)
        :Base{s,before,ScopeIo<State>{&scope}},scope_(scope){}
    std::uint32_t scalar(Read kind,const Progress&d){
        last_read_=kind;auto&g=s_.gp;
        switch(kind){
        case Read::x:return Base::load(1,0x30b36au,g[6]+1u,1);
        case Read::y:return Base::load(2,0x30b36eu,g[6]+2u,1);
        case Read::z:return Base::load(1,0x30b379u,g[6]+3u,1);
        case Read::scratch_x:at(0x30b37du);return word(g[4]+0x14u);
        case Read::scratch_y:at(0x30b396u);return word(g[4]+0x14u);
        case Read::scratch_z:at(0x30b3a6u);return word(g[4]+0x14u);
        case Read::scratch_scale:at(0x30b3c1u);return word(g[4]+0x14u);
        case Read::scratch_broadcast:{
            at(0x30b3d3u);const auto value=word(g[4]+0x14u);
            s_.xmm[0]=packed(Vec{value,0,0,0});return value;
        }
        case Read::variable_shift:return Base::load(1,0x30b40du,g[3]+0x10u);
        case Read::variable_scale:at(0x30b416u);return word(g[3]+0x14u);
        case Read::variable_property:return Base::load(2,0x30b420u,g[3]+0x1cu);
        case Read::variable_key:return Base::load(0,0x30b427u,g[3]+0x18u);
        default:throw std::logic_error("Non-scalar ray quantization source");
        }
    }
    Vec vector(Read kind,const Progress&){
        last_read_=kind;auto&g=s_.gp;unsigned reg{};std::uint32_t pc{},address{};
        switch(kind){
        case Read::start_initial:pc=0x30b372u;address=g[7];break;
        case Read::end_initial:pc=0x30b3b9u;address=g[7]+0x10u;break;
        case Read::translation:reg=1;pc=0x30b3aeu;address=g[4]+0x60u;break;
        case Read::start_scaled:reg=2;pc=0x30b3ccu;address=g[7];break;
        case Read::end_scaled:reg=2;pc=0x30b3e6u;address=g[7]+0x10u;break;
        case Read::variable_translation:reg=2;pc=0x30b3f8u;address=g[3];break;
        default:throw std::logic_error("Non-vector ray quantization source");
        }
        const auto value=Base::vector(pc,address);s_.xmm[reg]=value;return words(value);
    }
    void store(Write kind,std::uint32_t value,const Progress&d){
        if(kind==Write::variable_shift)s_.gp[1]=s_.flags.plus(s_.gp[1],d.opcode,4);
        const auto[pc,offset]=destination(kind);
        Base::store(pc,s_.gp[4]+offset,value,[]{});
    }
    void spill(Write kind,bool pop,const Progress&d){
        auto&g=s_.gp;
        if(kind==Write::translation_x)g[2]=1u;
        else if(kind==Write::translation_y){
            g[1]=d.opcode;g[2]=native_mopp_aabb_abi_detail::shifted(s_,g[2],g[1]);
        }
        const auto[pc,offset]=destination(kind);at(pc);
        const auto pending=scope_.stage_single(pop);
        Base::store(pc,g[4]+offset,pending.bits,[&]{scope_.commit(pending);},true);
    }
    void store_vector(Write kind,Vec value,const Progress&){
        auto&g=s_.gp;std::uint32_t pc{},address{};
        switch(kind){
        case Write::start_translated:pc=0x30b3b6u;address=g[7];break;
        case Write::end_translated:pc=0x30b3c8u;address=g[7]+0x10u;break;
        case Write::start_scaled:pc=0x30b3e3u;address=g[7];break;
        case Write::end_scaled:pc=0x30b3f4u;address=g[7]+0x10u;break;
        case Write::variable_translation:pc=0x30b408u;address=g[4]+0x40u;break;
        default:throw std::logic_error("Non-vector ray quantization destination");
        }
        Base::vector_store(pc,address,packed(value));
    }
    void integer(std::uint32_t value){
        scope_.require_capacity(1);native_mopp_ray_detail::integer(std::bit_cast<std::int32_t>(value));
    }
    void multiply_scalar(std::uint32_t value){native_mopp_ray_detail::multiply(value);}
    Vec subtract(Vec left,Vec right){
        const auto value=native_mopp_ray_detail::subtract(packed(left),packed(right));s_.xmm[0]=value;return words(value);
    }
    Vec add(Vec left,Vec right){
        const auto value=native_mopp_ray_detail::add(packed(left),packed(right));s_.xmm[2]=value;return words(value);
    }
    Vec multiply(Vec left,Vec right){
        const auto value=native_mopp_ray_detail::multiply(packed(left),packed(right));
        s_.xmm[last_read_==Read::variable_translation?1:3]=value;return words(value);
    }
    void advance(std::uint32_t program,const Progress&){s_.gp[6]=s_.flags.plus(s_.gp[6],4u,4);(void)program;}
    void finish(const Progress&){s_.gp[3]=s_.gp[4]+0x40u;}
};
}
