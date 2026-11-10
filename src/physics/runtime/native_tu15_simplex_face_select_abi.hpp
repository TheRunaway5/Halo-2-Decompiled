#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_face_select_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
namespace halo2::native_simplex_face_select_detail {
using Data=NativeTu15SimplexFaceSelect;
using P=NativeTu15SimplexFaceSelectPhase;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<bool Actual,class State,class Call,class Before>class Frame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::word;
    static constexpr std::uint32_t site(std::uint32_t pc){return Actual?pc:pc-0x3af0u;}
    native_raw_x87_detail::Scope<State>&fp_;Call&call_;P last_{};
    void at(std::uint32_t pc){Base::at(site(pc));}
    void push(std::uint32_t pc,std::uint32_t value){Base::push(site(pc),value);}
    std::uint32_t pop(std::uint32_t pc){return Base::pop(site(pc));}
    void publish(__m128 a,__m128 b,__m128 c,__m128 d,__m128 e,__m128 f,__m128 g,__m128 h){auto&x=s_.xmm;x[0]=a;x[1]=b;x[2]=c;x[3]=d;x[4]=e;x[5]=f;x[6]=g;x[7]=h;}
    void cross_complete(const Data&d){publish(d.negative_third,d.normal_third,d.apex_fresh,d.delta,d.normal_first,d.normal_second,d.negative_second,d.negative_first);}
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw native_mopp_ray_caller_detail::AlignedLoadFault{site(pc),address,flags,flags|0x10000u};}
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16);
        for(unsigned offset:{0u,8u}){SourcePhysicsContactRecordStore event{site(pc),address+offset,{},{}};if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}const auto data=std::span(bytes).subspan(offset,8);fp_.io([&]{s_.memory.write(event.address,data);});if(s_.observe){event.written.assign(data.begin(),data.end());fp_.io([&]{s_.observe(event);});}}
    }
    void single_store(std::uint32_t pc,std::uint32_t address){at(pc);const auto pending=fp_.stage_single(true);Base::store(site(pc),address,pending.bits,[&]{fp_.commit(pending);},true);}
public:
    Frame(State&s,Call&call,Before&before,native_raw_x87_detail::Scope<State>&fp):Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    void begin(Data&d){auto&g=s_.gp;push(0x30d5d0u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x5cu,4);push(0x30d5d9u,g[7]);d.vertices=g[7]=g[0];d.point_address=g[3];d.scratch=g[4];}
    void require_capacity(unsigned n){fp_.require_capacity(n);}
    __m128 vector_read(P phase,const Data&d,std::uint32_t address){
        std::uint32_t pc;unsigned reg;
        switch(phase){
        case P::first:pc=0x30d5dcu;reg=6;break;
        case P::origin:pc=0x30d5e0u;reg=4;break;
        case P::apex:pc=0x30d5e3u;reg=5;break;
        case P::second:pc=0x30d5e7u;reg=7;break;
        case P::point:pc=0x30d678u;reg=3;publish(d.negative_third,d.third_zxy,d.third_yzx,d.third_yzx,d.positive_first,d.positive_second,d.negative_second,d.negative_first);break;
        case P::apex_fresh:pc=0x30d681u;reg=2;publish(d.negative_third,d.positive_third,d.positive_third,d.point,d.positive_first,d.positive_second,d.negative_second,d.negative_first);break;
        case P::absolute_mask:pc=0x30d6fdu;reg=1;publish(d.scaled,d.scale_x,d.product_x,d.low_third,d.product_z,d.product_y,d.scale_z,d.high_third);break;
        case P::normal:pc=0x30d724u;reg=1;break;
        default:throw std::runtime_error("Invalid face-select vector acquisition");
        }
        const auto value=Base::vector(site(pc),address);s_.xmm[reg]=value;return value;
    }
    void vector_write(P phase,const Data&d,std::uint32_t address,__m128 value){
        std::uint32_t pc;
        switch(phase){
        case P::normal_first:pc=0x30d6abu;publish(d.scaled,d.normal_third,d.normal_first,d.delta,d.normal_first,d.normal_second,d.negative_second,d.negative_first);break;
        case P::normal_second:pc=0x30d6c2u;publish(d.scaled,d.normal_third,d.normal_first,d.low_third,d.high_pair,d.normal_second,d.negative_second,d.high_third);break;
        case P::normal_third:pc=0x30d6c7u;break;
        case P::signed_products:pc=0x30d719u;publish(d.triples,d.signed_products,d.absolute_triples,d.low_third,d.product_z,d.product_y,d.scale_z,d.high_third);break;
        default:throw std::runtime_error("Invalid face-select vector publication");
        }
        store_vector(pc,address,value);
    }
    std::uint32_t word_read(P phase,const Data&d){
        last_=phase;std::uint32_t pc,address;
        switch(phase){
        case P::scale_argument:pc=0x30d61du;address=s_.gp[5]+8u;publish(d.second_first,d.edge_second,d.edge_first,d.apex_origin,d.apex_first,d.apex_second,native_simplex_direction_numeric::shuffle<0xc9>(d.apex_first),d.negative_first);break;
        case P::scale_load:pc=0x30d695u;address=d.scratch+0x10u;break;
        case P::zero:pc=0x30d727u;address=0x46a0e8u;break;
        case P::squared_length_load:pc=0x30d74eu;address=d.scratch+0x10u;break;
        case P::face_product:pc=0x30d75bu;address=d.scratch+0x20u+d.face*4u;break;
        case P::denominator:pc=0x30d75fu;address=d.scratch+0x10u;break;
        case P::rank_second:pc=0x30d77cu;address=d.scratch+0x18u;break;
        case P::rank_first:pc=0x30d785u;address=d.scratch+0x14u;break;
        case P::rank_first_word:pc=0x30d792u;address=d.scratch+0x14u;break;
        case P::rank_reload:pc=0x30d79eu;address=d.scratch+0x10u;break;
        case P::rank_third_compare:pc=0x30d7a4u;address=d.scratch+0x1cu;break;
        case P::rank_temporary_read:pc=0x30d7b3u;address=d.scratch+0x10u;break;
        case P::rank_third_load:pc=0x30d7b7u;address=d.scratch+0x1cu;break;
        case P::rank_first_reload:pc=0x30d7bfu;address=d.scratch+0x14u;break;
        case P::rank_first_retry:pc=0x30d7ceu;address=d.scratch+0x14u;break;
        case P::threshold_third:pc=0x30d7d9u;address=d.scratch+0x1cu;break;
        case P::threshold_constant:pc=0x30d7ddu;address=0x46a454u;break;
        case P::rank_factor:pc=0x30d7f6u;address=0x46a190u;break;
        case P::rank_third_again:pc=0x30d7feu;address=d.scratch+0x1cu;break;
        default:throw std::runtime_error("Invalid face-select scalar acquisition");
        }
        at(pc);const auto value=word(address);
        if(phase==P::scale_argument)s_.gp[0]=value;
        if(phase==P::scale_load){s_.xmm[0]=_mm_setzero_ps();std::memcpy(&s_.xmm[0],&value,4);}
        if(phase==P::rank_first_word)s_.gp[1]=value;
        if(phase==P::rank_temporary_read)s_.gp[2]=value;
        return value;
    }
    void word_write(P phase,const Data&d,std::uint32_t value){
        std::uint32_t pc,address;
        switch(phase){
        case P::scale_store:pc=0x30d691u;address=d.scratch+0x10u;cross_complete(d);break;
        case P::squared_length:pc=0x30d74au;address=d.scratch+0x10u;s_.gp[0]=address;s_.xmm[1]=d.squares;s_.xmm[2]=d.squared_xy;s_.xmm[3]=d.squared_length;break;
        case P::rank_old_store:pc=0x30d79au;address=d.scratch+0x10u;break;
        case P::rank_third_store:pc=0x30d7bbu;address=d.scratch+0x1cu;break;
        default:throw std::runtime_error("Invalid face-select scalar publication");
        }
        Base::store(site(pc),address,value,[]{});
    }
    void begin_scores(const Data&d){s_.gp[2]=d.scratch+0x30u;s_.gp[1]=s_.flags.logic(s_.gp[1]^s_.gp[1],4);}
    void relation(const Data&d,std::uint32_t mask){s_.set(0,d.relation,2,0);s_.flags.logic((d.relation>>8u)&mask,1);}
    void maximum_score(const Data&d){Base::store(site(0x30d769u),d.scratch+0x14u+d.face*4u,0x7f7fffffu,[]{});}
    void store_score(const Data&d){single_store(0x30d763u,d.scratch+0x14u+d.face*4u);}
    void advance_score(const Data&){auto&g=s_.gp;g[1]=s_.flags.plus(g[1],4u,4);g[2]=s_.flags.plus(g[2],16u,4);s_.flags.minus(g[1],12u,4);}
    void choice(const Data&d){s_.gp[1]=d.choice?d.choice:s_.flags.logic(s_.gp[1]^s_.gp[1],4);}
    void store_rank(P phase,const Data&d){single_store(phase==P::rank_first_store?0x30d796u:0x30d7afu,d.scratch+(phase==P::rank_first_store?0x14u:0x10u));}
    void ranking_compared(const Data&d){s_.set(0,d.relation,2,0);}
    void ranking_discarded(const Data&d){s_.flags.logic((d.relation>>8u)&0x41u,1);}
    void finish(const Data&,std::uint32_t result){auto&g=s_.gp;g[0]=result;const bool rejected=result==0xffffffffu;g[7]=pop(rejected?0x30d7f1u:0x30d80fu);g[4]=g[5];g[5]=pop(rejected?0x30d7f4u:0x30d812u);pop(rejected?0x30d7f5u:0x30d813u);}
    std::uint32_t refine(const Data&d){
        auto&g=s_.gp;g[0]=g[4]+0x20u;push(0x30d818u,g[0]);push(0x30d819u,g[3]);store_vector(0x30d81au,g[4]+0x28u,d.triples);push(0x30d81fu,0x30d824u);
        fp_.child([&]{call_(s_,0x30d1a0u,0x309d2fu);});g[4]=s_.flags.plus(g[4],8u,4);g[7]=pop(0x30d827u);g[4]=g[5];g[5]=pop(0x30d82au);pop(0x30d82bu);return g[0];
    }
};
template<bool Actual=true,class State,class Call,class Before=Noop>
std::uint32_t run_private_owner(State&s,Call call,Before before={}){native_raw_x87_detail::Scope<State>fp{s};Frame<Actual,State,Call,Before>memory{s,call,before,fp};Data data;return native_tu15_select_simplex_faces(memory,data);}
} // namespace halo2::native_simplex_face_select_detail
#endif
