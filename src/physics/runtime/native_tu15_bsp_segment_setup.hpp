#pragma once
#include "halo2/native_tu15_bsp_segment_setup_kernel.hpp"
#include "physics/runtime/native_tu15_bsp_segment.hpp"

namespace halo2::native_bsp_segment_setup_detail {
using Noop=native_bsp_segment_detail::Noop;
inline bool reference_mode(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_BSP_SEGMENT_SETUP");return p&&*p;}();return value;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State,class Call,class Before>struct Adapter:native_bsp_leaf_surface_detail::Adapter<State,Call,Before>{
    using Base=native_bsp_leaf_surface_detail::Adapter<State,Call,Before>;using Base::s;using Base::sp;using Base::frame;
    using Base::at;using Base::word;using Base::push;using Base::pop;
    Adapter(State&state,Call&call,Before&before,std::uint32_t stack):Base{state,call,before,stack,stack-40u}{}
    std::uint32_t context()const{return frame;}
    std::uint32_t flags()const{return s.gp[1];}
    std::uint32_t descriptor()const{return s.gp[2];}
    std::uint32_t output()const{return s.gp[0];}
    Scalar epsilon()const{return s.xmm[0];}
    std::uint32_t limit()const{return word(sp+4u,2);}
    std::uint32_t bitmap()const{return word(sp+8u);}
    std::uint32_t origin()const{return word(sp+12u);}
    std::uint32_t displacement()const{return word(sp+16u);}
    std::uint32_t maximum(std::uint32_t)const{return word(sp+4u);}
    void store(Write kind,std::uint32_t address,std::uint32_t value,unsigned width){
        std::uint32_t pc{};
        switch(kind){
        case Write::flags:pc=0x1e68e9u;break;case Write::descriptor:pc=0x1e68f1u;break;
        case Write::limit:pc=0x1e68f9u;break;case Write::bitmap:pc=0x1e6902u;break;
        case Write::origin:pc=0x1e690au;break;case Write::displacement:pc=0x1e690eu;break;
        case Write::output:pc=0x1e6912u;break;case Write::fraction:pc=0x1e6920u;break;
        case Write::count:pc=0x1e6924u;break;case Write::previous_leaf:pc=0x1e692du;break;
        case Write::previous_kind:pc=0x1e6931u;break;case Write::partition:pc=0x1e6935u;break;
        case Write::maximum_zero:pc=0x1e693bu;address=sp+4u;break;
        case Write::maximum_one:pc=0x1e694eu;address=sp+4u;break;
        case Write::maximum_epsilon:pc=0x1e6956u;address=sp+4u;break;
        }
        Base::store(pc,address,value,width);
    }
    bool traverse(const Progress&d){
        auto&g=s.gp;g[0]=d.maximum;push(0x1e6960u,g[0]);push(0x1e6961u,g[1]);push(0x1e6962u,g[1]);g[1]=g[4]+12u;
        push(0x1e6967u,g[1]);push(0x1e6968u,0x1e696du);
        if(!native_bsp_segment_detail::bridge(s,this->call,this->before))this->call(s,0x1e6980u,0x1e6968u,false);
        at(0x1e696du);return(g[0]&255u)!=0;
    }
    void progress(Phase phase,const Progress&d){
        auto&g=s.gp;
        switch(phase){
        case Phase::begin:at(0x1e68e0u);g[4]=s.flags.minus(g[4],40u,4);s.xmm[2]=d.zero;break;
        case Phase::context_flags:s.flags.load(d.first.flags);break;
        case Phase::argument_limit:at(0x1e68ecu);break;
        case Phase::context_descriptor:g[1]=(g[1]&0xffff0000u)|(d.limit&65535u);break;
        case Phase::argument_bitmap:at(0x1e68f5u);break;
        case Phase::context_limit:g[2]=d.bitmap;break;
        case Phase::argument_origin:at(0x1e68feu);break;
        case Phase::context_bitmap:g[1]=d.origin;break;
        case Phase::argument_displacement:at(0x1e6906u);break;
        case Phase::context_origin:g[2]=d.displacement;break;
        case Phase::context_displacement:case Phase::context_output:break;
        case Phase::select_initial:s.xmm[1]=d.initial;break;
        case Phase::clear_cursor:g[1]=s.flags.logic(0,4);break;
        case Phase::output_fraction:case Phase::output_count:break;
        case Phase::second_comparison:g[0]=0xffffffffu;s.flags.load(d.second.flags);break;
        case Phase::previous_leaf:case Phase::previous_kind:case Phase::partition:break;
        case Phase::constant_one:at(0x1e6943u);break;
        case Phase::cap_comparison:s.xmm[1]=scalar(d.one);s.flags.load(d.cap.flags);break;
        case Phase::maximum_zero:case Phase::maximum_one:case Phase::maximum_epsilon:break;
        case Phase::maximum:at(0x1e695cu);break;
        case Phase::traverse:break;
        case Phase::finished:g[4]=s.flags.plus(g[4],40u,4);pop(0x1e6970u);g[4]+=16u;break;
        }
    }
};
#endif
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call call,Before before={}){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15||s.fp.count>6u||(s.fp.status&0x8080u))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));if((cw&63u)!=63u||(_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    // An observer without the owned instruction-cursor contract has no
    // read-only witness, so its literal continuation must remain available.
    if(s.observe&&!s.services.instruction)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
        if(s.observe&&!c->observation_only_stores)return false;
        for(const auto site:c->service_sites)if(site>=0x1e68e0u&&site<=0x1e6970u)return false;
    }
    Adapter<State,Call,Before>adapter{s,call,before,s.gp[4]};setup(adapter,adapter,[&](auto phase,const auto&data){adapter.progress(phase,data);});return true;
#endif
}
} // namespace halo2::native_bsp_segment_setup_detail
