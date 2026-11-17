#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_boundary_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
namespace halo2::native_simplex_boundary_detail {
using Data=NativeTu15SimplexBoundary;using P=NativeTu15SimplexBoundaryPhase;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
// Private actual-PC owner; no admission or production route is installed.
template<class State,class Call,class Before>class Frame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::word;using Base::at;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;Call&call_;bool first_scan_{};
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw native_mopp_ray_caller_detail::AlignedLoadFault{pc,address,flags,flags|0x10000u};}
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16);
        for(unsigned offset:{0u,8u}){SourcePhysicsContactRecordStore event{pc,address+offset,{},{}};if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}const auto written=std::span(bytes).subspan(offset,8);fp_.io([&]{s_.memory.write(event.address,written);});if(s_.observe){event.written.assign(written.begin(),written.end());fp_.io([&]{s_.observe(event);});}}
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){Base::store(pc,address,value,[]{});}
    std::uint32_t load(std::uint32_t pc,unsigned reg,std::uint32_t address,unsigned width=4){at(pc);return s_.gp[reg]=word(address,width);}
public:
    Frame(State&s,Call&call,Before&before,native_raw_x87_detail::Scope<State>&fp):Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    void begin(Data&d){auto&g=s_.gp;push(0x30dc10u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x74u,4);push(0x30dc19u,g[3]);push(0x30dc1au,g[6]);d.work=g[6]=g[1];}
    std::uint32_t count(P phase,const Data&d,bool first){
        std::uint32_t pc;unsigned reg=0;
        switch(phase){case P::initial_second_count:pc=0x30dc1du;break;case P::first_scan_count:pc=0x30dc24u;break;case P::first_scan_count_again:pc=0x30dc4au;break;case P::initial_first_count:pc=0x30dc54u;break;case P::second_scan_count:pc=0x30dc59u;break;case P::second_scan_count_again:pc=0x30dc7fu;break;case P::pair_first_count:pc=0x30dc8au;break;case P::pair_second_count:pc=0x30dc8cu;reg=3;break;case P::first_reduction_count:pc=0x30dd98u;reg=7;break;case P::second_reduction_count:pc=0x30ddc3u;break;case P::second_count_again:pc=0x30ddebu;reg=1;break;default:throw std::runtime_error("Invalid boundary count acquisition");}
        at(pc);const auto value=word(s_.gp[6]+(first?0u:4u));
        if(phase==P::initial_second_count){s_.flags.minus(value,3u,4);push(0x30dc21u,s_.gp[7]);}
        else if(phase==P::initial_first_count)s_.flags.minus(value,3u,4);
        else s_.gp[reg]=value;
        return value;
    }
    void prepare_triangle_scan(Data&d,bool first){first_scan_=first;auto&g=s_.gp;g[3]=s_.flags.logic(0u,4);s_.flags.logic(g[0],4);if(std::bit_cast<std::int32_t>(g[0])>0)g[7]=g[6]+(first?0x20u:0xa0u);d.index=g[3];d.row=g[7];}
    std::uint32_t triangle_child(Data&d,bool first){auto&g=s_.gp;push(first?0x30dc30u:0x30dc68u,0u);g[0]=g[6]+(first?0xa0u:0x20u);push(first?0x30dc38u:0x30dc6du,g[0]);push(first?0x30dc39u:0x30dc6eu,g[7]);g[1]=g[6];const auto pc=first?0x30dc3cu:0x30dc71u;push(pc,pc+5u);fp_.child([&]{call_(s_,0x30d3f0u,pc);});d.work=g[6];d.index=g[3];d.row=g[7];d.scratch=g[4];return g[0];}
    void classify_triangle(const Data&){s_.flags.minus(s_.gp[0],7u,4);}
    bool advance_triangle_scan(Data&d,bool,std::uint32_t limit){auto&g=s_.gp;g[3]=s_.flags.increment(g[3],4,false);g[7]=s_.flags.plus(g[7],0x10u,4);s_.flags.minus(g[3],limit,4);d.index=g[3];d.row=g[7];return s_.flags.sf!=s_.flags.of;}
    void prepare_early_source(Data&d,bool first){auto&g=s_.gp;if(first){g[3]=s_.flags.plus(g[3],2u,4);g[3]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[3]);d.source=g[3]+g[6];}else{g[0]=g[3]+0xeu;g[0]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[0]);d.source=g[0]+g[6];}}
    void prepare_early_relative(Data&d){s_.gp[3]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,s_.gp[3]);d.source=s_.gp[3]+s_.gp[6];}
    void before_early_relative_store(const Data&){s_.gp[7]=pop(0x30de3fu);}
    void initialize_pairs(Data&d){auto&g=s_.gp;g[1]=s_.flags.logic(0u,4);s_.flags.minus(g[0],3u,4);s_.set(1,s_.flags.zf,1,0);g[2]=s_.flags.logic(0u,4);s_.flags.minus(g[3],3u,4);s_.set(2,s_.flags.zf,1,0);g[0]=s_.flags.logic(0u,4);store(0x30dca1u,g[4]+0x1cu,0x7f7fffffu);store(0x30dca9u,g[4]+0x24u,g[0]);g[1]=1u+g[1]*2u;s_.flags.minus(g[1],g[0],4);store(0x30dcb3u,g[4]+0x2cu,g[1]);g[2]=1u+g[2]*2u;store(0x30dcbbu,g[4]+0x20u,g[2]);store(0x30dcbfu,g[4]+0x28u,g[0]);store(0x30dcc3u,g[4]+0x14u,g[0]);d.first_edges=g[1];d.second_edges=g[2];d.outer_index=g[0];d.scratch=g[4];}
    void initialize_outer_point(Data&d){s_.gp[0]=s_.gp[6]+0x20u;store(0x30dcd0u,s_.gp[4]+0x18u,s_.gp[0]);d.outer_point=s_.gp[0];}
    void begin_outer_pair(Data&d){auto&g=s_.gp;g[7]=s_.flags.logic(0u,4);s_.flags.logic(g[2],4);if(std::bit_cast<std::int32_t>(g[2])>0)g[3]=g[6]+0xa0u;d.inner_index=g[7];d.second_edges=g[2];d.second_point=g[3];}
    void acquire_outer_index(Data&d){d.outer_index=load(0x30dce4u,1,s_.gp[4]+0x14u);}
    std::uint32_t index_read(P phase,const Data&,std::uint32_t index){std::uint32_t pc,address;unsigned reg;switch(phase){case P::first_endpoint:pc=0x30dce8u;address=0x41d762u+index;reg=2;break;case P::second_endpoint:pc=0x30dcefu;address=0x41d762u+index;reg=1;break;case P::first_reduction_endpoint:pc=0x30dda1u;address=0x41d760u+index;reg=1;break;case P::second_reduction_endpoint:s_.gp[1]=s_.gp[0];pc=0x30ddd0u;address=0x41d760u+index;reg=0;break;default:throw std::runtime_error("Invalid boundary signed endpoint table");}at(pc);return s_.gp[reg]=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(word(address,1)))));}
    void prepare_first_endpoint(Data&d){auto&g=s_.gp;g[2]=s_.flags.plus(g[2],2u,4);g[2]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[2]);d.source=g[2]+g[6];}
    void prepare_second_endpoint(Data&d){auto&g=s_.gp;g[1]=s_.flags.plus(g[1],0xau,4);g[1]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[1]);d.source=g[1]+g[6];}
    __m128 vector_read(P phase,const Data&d,std::uint32_t address){std::uint32_t pc;unsigned reg=0;switch(phase){case P::first_point:pc=0x30dcf6u;break;case P::first_end:pc=0x30dcffu;reg=1;break;case P::second_point:pc=0x30dd06u;s_.xmm[1]=d.first_delta;break;case P::second_end:pc=0x30dd14u;reg=1;break;case P::first_reduction_source:pc=0x30ddb0u;break;case P::second_position_source:pc=0x30ddddu;break;case P::second_relative_source:pc=0x30ddf4u;break;case P::early_source:pc=first_scan_?0x30de0fu:0x30de2au;break;case P::early_relative:pc=0x30de3bu;break;default:throw std::runtime_error("Invalid boundary vector read");}const auto value=Base::vector(pc,address);s_.xmm[reg]=value;return value;}
    void vector_write(P phase,const Data&d,std::uint32_t address,__m128 value){std::uint32_t pc;switch(phase){case P::first_delta_store:pc=0x30dd0fu;break;case P::first_reduction_store:pc=0x30ddbau;break;case P::second_position_store:pc=0x30dde7u;break;case P::second_relative_store:pc=0x30ddfeu;break;case P::early_position:pc=first_scan_?0x30de13u:0x30de31u;if(!first_scan_)s_.gp[3]=s_.flags.plus(s_.gp[3],0xau,4);break;case P::early_relative_store:pc=0x30de40u;break;default:throw std::runtime_error("Invalid boundary vector write");}store_vector(pc,address,value);}
    void prepare_closest_arguments(const Data&){auto&g=s_.gp;g[2]=g[4]+0x50u;push(0x30dd1cu,g[2]);g[1]=g[4]+0x34u;push(0x30dd21u,g[1]);push(0x30dd22u,g[3]);g[2]=g[4]+0x4cu;push(0x30dd27u,g[2]);}
    void closest_child(Data&d){auto&g=s_.gp;s_.xmm[1]=d.second_delta;push(0x30dd2bu,g[0]);store_vector(0x30dd2cu,g[4]+0x44u,d.second_delta);push(0x30dd31u,0x30dd36u);fp_.child([&]{call_(s_,0x312620u,0x30dd31u);});d.work=g[6];d.inner_index=g[7];d.second_point=g[3];}
    void require_capacity(unsigned n){fp_.require_capacity(n);}
    std::uint32_t scalar_read(P phase,const Data&){const auto pc=phase==P::distance?0x30dd36u:0x30dd3du,address=s_.gp[4]+(phase==P::distance?0x84u:0x30u);at(pc);return word(address);}
    void distance_classification(Data&d){s_.gp[4]=s_.flags.plus(s_.gp[4],0x14u,4);s_.set(0,d.relation,2,0);s_.flags.logic((d.relation>>8u)&5u,1);d.scratch=s_.gp[4];}
    void publish_minimum(const Data&){auto&g=s_.gp;load(0x30dd4bu,0,g[4]+0x70u);load(0x30dd4fu,1,g[4]+0x14u);store(0x30dd53u,g[4]+0x1cu,g[0]);store(0x30dd57u,g[4]+0x24u,g[1]);store(0x30dd5bu,g[4]+0x28u,g[7]);}
    bool advance_inner_pair(Data&d){auto&g=s_.gp;load(0x30dd5fu,0,g[4]+0x20u);g[7]=s_.flags.increment(g[7],4,false);g[3]=s_.flags.plus(g[3],0x10u,4);s_.flags.minus(g[7],g[0],4);load(0x30dd69u,0,g[4]+0x18u);d.inner_index=g[7];d.second_point=g[3];d.outer_point=g[0];return s_.flags.sf!=s_.flags.of;}
    void reload_pair_limits(Data&d){d.second_edges=load(0x30dd73u,2,s_.gp[4]+0x20u);d.first_edges=load(0x30dd77u,1,s_.gp[4]+0x2cu);}
    bool advance_outer_pair(Data&d){auto&g=s_.gp;load(0x30dd7bu,7,g[4]+0x14u);g[7]=s_.flags.increment(g[7],4,false);g[0]=s_.flags.plus(g[0],0x10u,4);s_.flags.minus(g[7],g[1],4);store(0x30dd85u,g[4]+0x14u,g[7]);store(0x30dd89u,g[4]+0x18u,g[0]);d.outer_index=g[7];d.outer_point=g[0];return s_.flags.sf!=s_.flags.of;}
    void classify_first_edges(const Data&){s_.flags.minus(s_.gp[1],3u,4);}
    void classify_second_edges(const Data&){s_.flags.minus(s_.gp[2],3u,4);}
    std::uint32_t selected_index(P phase,const Data&){return phase==P::first_reduction_choice?load(0x30dd9au,1,s_.gp[4]+0x24u):load(0x30ddc6u,2,s_.gp[4]+0x28u);}
    void write_count(P phase,const Data&,bool first,std::uint32_t value){std::uint32_t pc;switch(phase){case P::first_reduction_count:s_.gp[7]=s_.flags.increment(s_.gp[7],4,true);pc=0x30dd9fu;break;case P::second_reduction_count:s_.gp[0]=s_.flags.increment(s_.gp[0],4,true);pc=0x30ddcbu;break;case P::first_singleton:pc=0x30de17u;break;case P::second_singleton:pc=0x30de47u;break;default:throw std::runtime_error("Invalid boundary count publication");}store(pc,s_.gp[6]+(first?0u:4u),value);}
    void prepare_first_reduction_source(Data&d){auto&g=s_.gp;g[0]=g[7];g[0]=s_.flags.plus(g[0],2u,4);g[0]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[0]);d.source=g[0]+g[6];}
    void prepare_first_reduction_destination(Data&d){auto&g=s_.gp;g[1]=s_.flags.plus(g[1],2u,4);g[1]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[1]);d.destination=g[1]+g[6];}
    void prepare_second_position_source(Data&d){auto&g=s_.gp;g[1]=s_.flags.plus(g[1],0xeu,4);g[1]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[1]);d.source=g[1]+g[6];}
    void prepare_second_position_destination(Data&d){auto&g=s_.gp;g[2]=0xeu+g[0];g[2]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[2]);d.destination=g[2]+g[6];}
    void prepare_second_relative_source(Data&d){auto&g=s_.gp;g[1]=s_.flags.plus(g[1],0xau,4);g[1]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[1]);d.source=g[1]+g[6];}
    void prepare_second_relative_destination(Data&d){auto&g=s_.gp;g[0]=s_.flags.plus(g[0],0xau,4);g[0]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[0]);d.destination=g[0]+g[6];}
    void finish(P phase,const Data&){auto&g=s_.gp;const bool first=phase==P::first_overlap_exit,second=phase==P::second_overlap_exit;if(!second)g[7]=pop(first?0x30de1du:0x30de02u);g[6]=pop(first?0x30de1eu:second?0x30de4eu:0x30de03u);g[3]=pop(first?0x30de1fu:second?0x30de4fu:0x30de04u);g[4]=g[5];g[5]=pop(first?0x30de22u:second?0x30de52u:0x30de07u);pop(first?0x30de23u:second?0x30de53u:0x30de08u);}
};
template<class State,class Call,class Before=Noop>
std::uint32_t run_private_owner(State&s,Call call,Before before={}){native_raw_x87_detail::Scope<State>fp{s};Frame<State,Call,Before>memory{s,call,before,fp};Data data;native_tu15_reduce_simplex_boundary(memory,data);return s.gp[0];}
} // namespace halo2::native_simplex_boundary_detail
#endif
