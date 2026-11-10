#pragma once
#include <cstdint>
#include <cstdlib>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_triangle_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_simplex_projection_kernel.hpp"

namespace halo2::native_simplex_triangle_detail {
using Data=NativeTu15SimplexTriangle;
using P=NativeTu15SimplexTrianglePhase;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<bool Actual,class State,class Before>class Frame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::word;
    static constexpr std::uint32_t site(std::uint32_t pc){return Actual?pc:pc-0x3af0u;}
    void at(std::uint32_t pc){Base::at(site(pc));}
    void push(std::uint32_t pc,std::uint32_t value){Base::push(site(pc),value);}
    std::uint32_t pop(std::uint32_t pc){return Base::pop(site(pc));}
    native_raw_x87_detail::Scope<State>&fp_;
    void publish(__m128 a,__m128 b,__m128 c,__m128 d,__m128 e,__m128 f,__m128 g,__m128 h){auto&x=s_.xmm;x[0]=a;x[1]=b;x[2]=c;x[3]=d;x[4]=e;x[5]=f;x[6]=g;x[7]=h;}
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);if(address&15u)throw native_mopp_ray_caller_detail::AlignedLoadFault{site(pc),address,s_.other_flags|s_.flags.bits(),s_.other_flags|s_.flags.bits()|0x10000u};
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16);
        for(unsigned offset:{0u,8u}){
            SourcePhysicsContactRecordStore event{site(pc),address+offset,{},{}};
            if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}
            const auto written=std::span(bytes).subspan(offset,8);fp_.io([&]{s_.memory.write(event.address,written);});
            if(s_.observe){event.written.assign(written.begin(),written.end());fp_.io([&]{s_.observe(event);});}
        }
    }
public:
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp){}
    void begin(Data&d){auto&g=s_.gp;d.work=g[1];push(0x30d3f0u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x1cu,4);push(0x30d3f9u,g[6]);}
    std::uint32_t argument(P p,const Data&d){
        std::uint32_t pc,offset;unsigned reg;
        if(p==P::vertices_argument){pc=0x30d3fau;offset=12u;reg=2;}
        else if(p==P::point_argument){pc=0x30d408u;offset=8u;reg=0;}
        else if(p==P::orientation_argument){
            pc=0x30d507u;offset=16u;reg=0;
            publish(d.normal,d.normal_x,d.z_term,d.x_rows,d.pa_low,d.y_term,d.normal_z,d.pa_high);
        }else throw std::runtime_error("Invalid triangle argument acquisition");
        at(pc);return s_.gp[reg]=word(s_.gp[5]+offset);
    }
    std::uint32_t scratch_address(const Data&)const{return s_.gp[4]+0x10u;}
    __m128 vector_read(P p,const Data&d,std::uint32_t address){
        std::uint32_t pc{};unsigned reg=8;
        switch(p){
        case P::first:pc=0x30d3fdu;reg=3;break;
        case P::second:pc=0x30d400u;reg=7;break;
        case P::third:pc=0x30d404u;reg=2;break;
        case P::point:pc=0x30d44du;reg=4;publish(d.positive_normal,d.negative_normal,d.ba,d.cb_zxy,d.ac_zxy,d.ac_yzx,d.cb_yzx,d.b);break;
        case P::third_fresh:pc=0x30d46du;reg=6;publish(d.normal,d.pb_yzx,d.ba,d.cb_zxy,d.p,d.ac_yzx,d.pb_yzx,d.negative_pb);break;
        case P::scratch_read:pc=0x30d48bu;publish(d.normal,d.positive_pb,d.ba,d.pc,d.p,d.pc_yzx,d.negative_pc,d.negative_pb);break;
        case P::first_fresh:pc=0x30d490u;reg=3;publish(d.normal,d.positive_pb,d.ba,d.pc,d.p,d.positive_pc,d.negative_pc,d.negative_pb);break;
        case P::orientation_first:pc=0x30d53bu;reg=1;break;
        case P::orientation_second:pc=0x30d53fu;reg=2;break;
        case P::sign:pc=0x30d56eu;reg=1;break;
        case P::swap_first:pc=0x30d57fu;reg=0;break;
        case P::swap_second:pc=0x30d582u;reg=1;break;
        case P::payload_first:pc=0x30d58du;reg=0;break;
        case P::payload_second:pc=0x30d591u;reg=1;break;
        default:throw std::runtime_error("Invalid triangle vector acquisition");
        }
        const auto value=Base::vector(site(pc),address);if(reg!=8u)s_.xmm[reg]=value;return value;
    }
    void vector_write(P p,const Data&d,std::uint32_t address,__m128 value){
        std::uint32_t pc;
        switch(p){
        case P::scratch_store:pc=0x30d445u;publish(d.cb_yzx,d.negative_normal,d.ba,d.cb_zxy,d.ac_zxy,d.ac_yzx,d.cb_yzx,d.b);break;
        case P::weights:pc=0x30d51eu;break;
        case P::normal:pc=0x30d52eu;break;
        case P::flipped_normal:pc=0x30d578u;s_.xmm[0]=value;break;
        case P::publish_first:pc=0x30d586u;break;
        case P::publish_second:pc=0x30d589u;break;
        case P::publish_payload_first:pc=0x30d595u;break;
        case P::publish_payload_second:pc=0x30d599u;break;
        default:throw std::runtime_error("Invalid triangle vector publication");
        }
        store_vector(pc,address,value);
    }
    void prepare_weights(const Data&d){publish(d.normal,d.weights,d.z_term,d.x_term,d.pa_low,d.y_term,d.normal_z,d.pa_high);s_.gp[6]=s_.flags.logic(d.mask,4);s_.flags.logic(d.orient&d.orient,4);}
    void prepare_normal(const Data&d){s_.flags.minus(d.mask,7u,4);}
    void prepare_orientation(const Data&d){s_.xmm[1]=d.products;s_.xmm[2]=d.xy;s_.xmm[3]=d.dot;s_.gp[0]=std::uint32_t(_mm_movemask_ps(d.dot));s_.flags.logic(s_.gp[0]&1u,1);s_.gp[0]=d.mask;}
    void load_weight(const Data&d){at(0x30d59du);const auto value=word(d.work+0x160u);fp_.require_capacity(1u);native_simplex_projection_numeric::load(value);}
    std::uint32_t word_read(P,const Data&,std::uint32_t address){at(0x30d5a3u);return s_.gp[2]=word(address);}
    void word_write(P,const Data&,std::uint32_t address,std::uint32_t value){Base::store(site(0x30d5a9u),address,value,[]{});}
    void store_weight(const Data&d){at(0x30d5afu);const auto pending=fp_.stage_single(true);Base::store(site(0x30d5afu),d.work+0x164u,pending.bits,[&]{fp_.commit(pending);},true);}
    void finish(const Data&d,bool flipped){
        auto&g=s_.gp;g[0]=d.mask;g[6]=pop(flipped?0x30d5b5u:0x30d5beu);g[4]=g[5];g[5]=pop(flipped?0x30d5b8u:0x30d5c1u);pop(flipped?0x30d5b9u:0x30d5c2u);g[4]+=12u;
    }
};
template<bool Actual=true,class State,class Before=Noop>
std::uint32_t run_private_owner(State&s,Before before={}){
    native_raw_x87_detail::Scope<State>fp{s};Frame<Actual,State,Before>memory{s,before,fp};Data data;
    return native_tu15_classify_simplex_triangle(memory,data);
}
} // namespace halo2::native_simplex_triangle_detail
#endif

namespace halo2::native_simplex_triangle_detail {
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_SIMPLEX_TRIANGLE");return p&&*p;}();return enabled;}
struct Activity{std::uint64_t actual_calls{},canonical_calls{},world_owner_deferrals{},world_reference_deferrals{},legacy_handoff_fallbacks{};};
inline Activity&activity(){thread_local Activity value;return value;}
template<class State>bool admitted(const State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>7u)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!s.services.instruction&&(s.instruction_cursor||s.observe))return false;
    if(s.services.instruction){
        const auto*p=s.instruction_cursor;
        if(!s.literal_service_sites||!p||p!=&s.services.instruction_cursor||!p->position||!p->callback_type||
           s.services.instruction.target_type()!=*p->callback_type||p->executor!=std::this_thread::get_id()||
           !p->literal_service_sites||p->service_sites.size()!=3u||p->service_sites[0]!=0x38846eu||
           p->service_sites[1]!=0x21f590u||p->service_sites[2]!=0x33b595u)return false;
        if((p->first_trace&&*p->first_trace)||(p->second_trace&&*p->second_trace))return false;
    }
    if(s.observe&&(!s.instruction_cursor||!s.instruction_cursor->observation_only_stores))return false;
    return true;
#endif
}
template<class State>bool bridge(State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(!admitted(s))return false;
    if constexpr(requires{s.selected_observation_pcs;}){++activity().actual_calls;run_private_owner<true>(s);}
    else{++activity().canonical_calls;run_private_owner<false>(s);}
    return true;
#endif
}
// SceneConstructor owns actual TU1.5 PCs while borrowing the base WorldState
// ABI, so its authenticated source overlay selects the namespace explicitly.
template<class State>bool bridge_actual(State&s){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(!admitted(s))return false;
    ++activity().actual_calls;run_private_owner<true>(s);return true;
#endif
}
} // namespace halo2::native_simplex_triangle_detail
