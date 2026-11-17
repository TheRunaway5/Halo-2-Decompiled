#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_paired_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
namespace halo2::native_simplex_paired_detail {
using Data=NativeTu15SimplexPaired;using P=NativeTu15SimplexPairedPhase;
using Promotion=NativeTu15SimplexPairedPromotion;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
// Private adapter: there is no admission/default route before independent
// whole-owner/child-closure and retained-prefix qualification.
template<bool Actual,class State,class Call,class Before>class Frame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::word;native_raw_x87_detail::Scope<State>&fp_;Call&call_;
    bool cached_compared_{};unsigned weight_step_{};
    static constexpr std::uint32_t site(std::uint32_t pc){return Actual?pc:pc-0x3af0u;}
    void at(std::uint32_t pc){Base::at(site(pc));}
    void push(std::uint32_t pc,std::uint32_t value){Base::push(site(pc),value);}
    std::uint32_t pop(std::uint32_t pc){return Base::pop(site(pc));}
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw native_mopp_ray_caller_detail::AlignedLoadFault{site(pc),address,flags,flags|0x10000u};}
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16);
        for(unsigned offset:{0u,8u}){SourcePhysicsContactRecordStore event{site(pc),address+offset,{},{}};if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}const auto data=std::span(bytes).subspan(offset,8);fp_.io([&]{s_.memory.write(event.address,data);});if(s_.observe){event.written.assign(data.begin(),data.end());fp_.io([&]{s_.observe(event);});}}
    }
    void adopt(Data&d){d.triangle=s_.gp[6];d.work=s_.gp[1];if(d.derive_plane){d.segment=s_.gp[7];d.selected_point=s_.gp[7];d.tip=s_.gp[3];}else{d.segment=s_.gp[3];d.selected_point=s_.gp[7];}}
public:
    Frame(State&s,Call&call,Before&before,native_raw_x87_detail::Scope<State>&fp):Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    void begin(Data&d){
        auto&g=s_.gp;d.work=g[1];push(0x30d830u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0xa4u,4);
        at(0x30d83cu);s_.set(0,word(g[5]+0x18u,1),1,0);s_.flags.logic(g[0]&255u,1);d.derive_plane=(g[0]&255u)!=0u;
        push(0x30d841u,g[3]);push(0x30d842u,g[6]);at(0x30d843u);d.triangle=g[6]=word(g[5]+0xcu);push(0x30d846u,g[7]);d.scratch=g[4];
    }
    void require_capacity(unsigned n){fp_.require_capacity(n);}
    std::uint32_t argument(P phase,const Data&d){
        const auto pc=phase==P::exit_flag?0x30dafdu:d.derive_plane?0x30d858u:0x30da2bu;
        const auto offset=phase==P::exit_flag?0x1cu:8u;const auto reg=phase==P::exit_flag?2u:d.derive_plane?7u:3u;
        at(pc);const auto value=word(s_.gp[5]+offset);s_.gp[reg]=value;return value;
    }
    void capture_tip(Data&d){d.tip=s_.gp[3]=s_.gp[6]+0x20u;}
    void select_second_endpoint(Data&d){d.selected_point=s_.gp[7]=s_.gp[3]+0x10u;}
    __m128 vector_read(P phase,const Data&d,std::uint32_t address){
        auto&x=s_.xmm;std::uint32_t pc;unsigned reg;
        if(d.derive_plane){switch(phase){
        case P::triangle_first:pc=0x30d84du;reg=2;break;
        case P::triangle_tip:pc=0x30d850u;reg=3;break;
        case P::triangle_second:pc=0x30d854u;reg=0;break;
        case P::segment_first:pc=0x30d893u;reg=3;x[0]=d.edge_second;x[1]=d.positive;x[2]=d.triangle_first;x[3]=native_simplex_direction_numeric::shuffle<0xd2>(d.edge_first);x[4]=d.negative;break;
        case P::segment_second:pc=0x30d8c1u;reg=4;x[0]=d.distance_first;x[1]=d.normal;x[2]=d.triangle_first;x[3]=d.segment_first;x[4]=d.xy_first;x[5]=d.z_first;break;
        case P::ones:pc=0x30d8feu;reg=2;address=0x41c500u;x[0]=d.ratio;x[1]=d.distance_second;x[2]=d.denominator;x[3]=d.segment_first;x[4]=d.segment_second;x[5]=d.z_second;break;
        case P::selected_endpoint:pc=0x30d966u;reg=0;break;
        case P::payload_first:pc=0x30d971u;reg=0;break;
        case P::payload_second:pc=0x30d975u;reg=1;break;
        case P::closest_segment_first:pc=0x30d99fu;reg=0;break;
        case P::closest_segment_second:pc=0x30d9a2u;reg=1;break;
        case P::triangle_tip_fresh:pc=0x30d9feu;reg=0;break;
        case P::tip_payload:pc=0x30da0du;reg=0;break;
        default:throw std::runtime_error("Invalid derived paired vector acquisition");
        }}else{switch(phase){
        case P::cached_weights:pc=0x30da2eu;reg=0;break;
        case P::triangle_first:pc=0x30da35u;reg=3;break;
        case P::segment_second:pc=0x30da38u;reg=4;break;
        case P::axis:pc=0x30da3cu;reg=1;break;
        case P::segment_first_fresh:pc=0x30da73u;reg=5;x[0]=d.product_second;x[1]=d.axis;x[2]=d.distance_second;x[3]=d.triangle_first;x[4]=d.segment_second;x[5]=d.xy_second;break;
        case P::ones:pc=0x30daaeu;reg=2;address=0x41c500u;x[0]=d.distance_first;x[1]=d.denominator;x[2]=d.distance_second;x[3]=d.z_first;x[4]=d.segment_second;x[5]=d.segment_first;break;
        case P::endpoint_inside_point:pc=0x30dafau;reg=0;break;
        case P::endpoint_inside_payload:pc=0x30db03u;reg=0;break;
        case P::triangle_tip_fresh:pc=d.promotion==Promotion::first?0x30dbebu:0x30dbc8u;reg=0;break;
        case P::tip_payload:pc=d.promotion==Promotion::first?0x30dbf2u:0x30dbd0u;reg=0;break;
        default:throw std::runtime_error("Invalid cached paired vector acquisition");
        }}
        // Complete World resolver20de37... maps418A60 to41C500; the unchanged
        // TU1.5 image contains four exact3F800000 words at that address.
        const auto value=Base::vector(site(pc),address);x[reg]=value;return value;
    }
    void vector_write(P phase,const Data&d,std::uint32_t address,__m128 value){
        auto&x=s_.xmm;std::uint32_t pc;
        switch(phase){
        case P::edge_first:pc=0x30d87fu;x[0]=d.edge_second;x[1]=d.edge_first;x[2]=d.triangle_first;x[3]=native_simplex_direction_numeric::shuffle<0xd2>(d.edge_first);x[4]=d.negative;break;
        case P::edge_second:pc=0x30d88bu;x[1]=native_simplex_direction_numeric::shuffle<0xc9>(d.edge_second);break;
        case P::intersection:pc=d.derive_plane?0x30d920u:0x30dad4u;x[0]=d.fraction;x[1]=d.scaled_second;x[2]=d.intersection;push(d.derive_plane?0x30d91fu:0x30dad3u,s_.gp[0]);address=s_.gp[4]+(d.derive_plane?0x1cu:0x3cu);break;
        case P::cached_weights_store:pc=0x30da43u;break;
        case P::swap_endpoint_first:pc=0x30d96au;break;
        case P::swap_endpoint_second:pc=0x30d96du;break;
        case P::swap_payload_first:pc=0x30d979u;break;
        case P::swap_payload_second:pc=0x30d97du;break;
        case P::indexed_tip:pc=d.derive_plane?0x30da09u:d.promotion==Promotion::first?0x30dbefu:0x30dbccu;break;
        case P::indexed_payload:pc=d.derive_plane?0x30da14u:d.promotion==Promotion::first?0x30dbf7u:0x30dbd4u;break;
        case P::endpoint_inside_point_store:pc=0x30db00u;break;
        case P::endpoint_inside_payload_store:pc=0x30db07u;break;
        default:throw std::runtime_error("Invalid paired vector publication");
        }
        store_vector(pc,address,value);
    }
    void begin_intersection(const Data&d){
        s_.xmm[0]=d.ratio;push(d.derive_plane?0x30d905u:0x30dab9u,0u);
    }
    void prepare_intersection_argument(const Data&d){
        s_.xmm[0]=d.fraction;s_.xmm[1]=d.fraction;s_.xmm[2]=d.complement;push(d.derive_plane?0x30d911u:0x30dac5u,s_.gp[6]);s_.gp[0]=s_.gp[4]+(d.derive_plane?0x18u:0x38u);
    }
    std::uint32_t triangle_child(P phase,Data&d){
        std::uint32_t pc;
        if(phase==P::intersection_child)pc=d.derive_plane?0x30d925u:0x30dad9u;
        else if(phase==P::endpoint_child){pc=d.derive_plane?0x30d985u:0x30daebu;push(d.derive_plane?0x30d981u:0x30dae7u,d.derive_plane?1u:0u);push(d.derive_plane?0x30d983u:0x30dae9u,s_.gp[6]);push(d.derive_plane?0x30d984u:0x30daeau,s_.gp[7]);}
        else throw std::runtime_error("Invalid paired triangle child phase");
        push(pc,pc+5u);fp_.child([&]{call_(s_,0x30d3f0u,pc-0x3af0u);});adopt(d);return s_.gp[0];
    }
    void prepare_closest_first(const Data&){auto&g=s_.gp;g[1]=g[4]+0x50u;push(0x30d9aau,g[1]);g[2]=g[4]+0x44u;push(0x30d9afu,g[2]);push(0x30d9b0u,g[3]);g[0]=g[4]+0x1cu;push(0x30d9b5u,g[0]);}
    void closest_child(P phase,Data&d){
        auto&g=s_.gp;std::uint32_t pc;
        if(phase==P::closest_first_child){s_.xmm[1]=d.delta_first;push(0x30d9b9u,g[7]);store_vector(0x30d9bau,g[4]+0x24u,d.delta_first);pc=0x30d9bfu;}
        else if(phase==P::closest_second_child){g[1]=g[4]+0x94u;push(0x30d9cbu,g[1]);g[2]=g[4]+0x48u;push(0x30d9d0u,g[2]);push(0x30d9d1u,g[3]);g[0]=g[4]+0x30u;push(0x30d9d6u,g[0]);push(0x30d9d7u,g[7]);pc=0x30d9d8u;}
        else throw std::runtime_error("Invalid paired closest child phase");
        push(pc,pc+5u);fp_.child([&]{call_(s_,0x312620u,pc-0x3af0u);});adopt(d);
    }
    void crossing_decision(const Data&d,bool){
        auto&g=s_.gp;
        if(d.derive_plane){g[0]=std::uint32_t(_mm_movemask_ps(d.distance_second));g[2]=std::uint32_t(_mm_movemask_ps(d.distance_first));g[0]=s_.flags.logic(g[0]^g[2],4);s_.flags.logic(g[0]&1u,1);}
        else{auto&x=s_.xmm;x[0]=d.distance_first;x[1]=d.positive;x[2]=d.distance_second;x[3]=d.z_first;x[4]=d.segment_second;x[5]=d.segment_first;g[2]=std::uint32_t(_mm_movemask_ps(d.positive));s_.flags.logic(g[2]&1u,1);}
    }
    void classification(const Data&){cached_compared_=false;s_.flags.minus(s_.gp[0],7u,4);}
    void nearest_endpoint(const Data&d){s_.set(0,d.relation,2,0);s_.flags.logic((d.relation>>8u)&5u,1);}
    void cached_selection(const Data&d){cached_compared_=true;nearest_endpoint(d);}
    void distance_choice(const Data&d){auto&g=s_.gp;g[4]=s_.flags.plus(g[4],0x28u,4);s_.set(0,d.relation,2,0);s_.flags.logic((d.relation>>8u)&0x41u,1);g[0]=d.edge_index?1u:s_.flags.logic(0u,4);}
    void prepare_indexed_tip(const Data&){auto&g=s_.gp;g[1]=g[0];g[0]=s_.flags.plus(g[0],4u,4);g[1]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,g[1]);}
    void prepare_indexed_payload(const Data&){s_.gp[0]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,s_.gp[0]);}
    std::uint32_t scalar_read(P phase,const Data&d){
        std::uint32_t pc,address;
        if(phase==P::distance_first_load){pc=0x30d945u;address=s_.gp[4]+0x2cu;}
        else if(phase==P::distance_second_load){pc=0x30d953u;address=s_.gp[4]+0x28u;}
        else if(phase==P::closest_distance_first){pc=0x30d9ddu;address=s_.gp[4]+0x98u;}
        else if(phase==P::closest_distance_second){pc=0x30d9e4u;address=s_.gp[4]+0xc8u;}
        else{
            if(phase==P::fresh_weight_first){weight_step_=0;s_.flags.minus(s_.gp[0],d.mask,4);}
            constexpr std::array<std::uint32_t,6>one{0x30db35u,0x30db3bu,0x30db3fu,0x30db43u,0x30db49u,0x30db4du};
            constexpr std::array<std::uint32_t,6>two{0x30db61u,0x30db67u,0x30db6bu,0x30db6fu,0x30db75u,0x30db79u};
            constexpr std::array<std::uint32_t,6>four{0x30db9eu,0x30dba4u,0x30dba8u,0x30dbacu,0x30dbb2u,0x30dbb6u};
            if(weight_step_>=6u)throw std::runtime_error("Paired cached-weight read phase overflow");
            const auto step=weight_step_++;pc=(d.mask==1u?one:d.mask==2u?two:four)[step];const bool fresh=step==0u||step==3u;
            const auto index=step==0u||step==1u||step==5u?d.weight_first:d.weight_second;
            address=fresh?s_.gp[1]+0x160u+index*4u:s_.gp[4]+0x10u+index*4u;
        }
        at(pc);return word(address);
    }
    void word_write(P phase,const Data&d,std::uint32_t value){
        std::uint32_t pc,address;
        if(phase==P::distance_first_store){pc=0x30d941u;address=s_.gp[2]=s_.gp[4]+0x2cu;s_.xmm[0]=d.distance_first;s_.xmm[1]=d.distance_second;s_.xmm[2]=d.xy_second;s_.xmm[3]=d.segment_first;s_.xmm[4]=d.segment_second;s_.xmm[5]=d.z_second;}
        else if(phase==P::distance_second_store){pc=0x30d94fu;address=s_.gp[0]=s_.gp[4]+0x28u;}
        else if(phase==P::work_flag){pc=0x30db0bu;address=s_.gp[1]+0x10u;}
        else throw std::runtime_error("Invalid paired word publication phase");
        Base::store(site(pc),address,value,[]{});
    }
    void decrement_count(const Data&d,bool first){
        std::uint32_t acquire,store;
        if(d.derive_plane){acquire=first?0x30d98fu:0x30da18u;store=first?0x30d992u:0x30da1bu;}
        else if(first){acquire=0x30daf5u;store=0x30daf8u;}
        else{
            acquire=d.promotion==Promotion::keep?0x30db86u:d.promotion==Promotion::second?0x30dbc3u:0x30dbe6u;store=acquire+3u;
            if(!cached_compared_)s_.flags.minus(s_.gp[0],d.mask==3u?3u:d.mask==5u?5u:d.mask==6u?6u:4u,4);
        }
        at(acquire);s_.gp[0]=word(s_.gp[5]+(first?0x10u:0x14u));at(store);const auto address=s_.gp[0];auto flags=s_.flags;const auto value=flags.increment(word(address),4,true);Base::store(site(store),address,value,[&]{s_.flags=flags;},true);
    }
    void before_payload_copy(const Data&d){if(!d.derive_plane&&d.promotion==Promotion::first)s_.gp[7]=pop(0x30dbf6u);}
    void finish(P phase,const Data&,std::uint32_t result){
        auto&g=s_.gp;std::uint32_t di,si,bx,bp,ret;
        switch(phase){
        case P::overlap_exit:di=0x30d934u;si=0x30d935u;bx=0x30d936u;bp=0x30d939u;ret=0x30d93au;g[0]=result;break;
        case P::derived_endpoint_exit:di=0x30d996u;si=0x30d997u;bx=0x30d998u;bp=0x30d99bu;ret=0x30d99cu;g[0]=s_.flags.logic(0u,4);break;
        case P::derived_reduce_exit:di=0x30da22u;si=0x30da23u;bx=0x30da24u;bp=0x30da27u;ret=0x30da28u;g[0]=result;break;
        case P::cached_endpoint_exit:di=0x30db10u;si=0x30db11u;bx=0x30db12u;bp=0x30db15u;ret=0x30db16u;g[0]=s_.flags.logic(0u,4);break;
        case P::cached_keep_exit:di=0x30db90u;si=0x30db91u;bx=0x30db92u;bp=0x30db95u;ret=0x30db96u;g[0]=result;break;
        case P::cached_second_exit:di=0x30dbddu;si=0x30dbdeu;bx=0x30dbdfu;bp=0x30dbe2u;ret=0x30dbe3u;g[0]=result;break;
        case P::cached_first_exit:di=0u;si=0x30dbfbu;bx=0x30dc01u;bp=0x30dc04u;ret=0x30dc05u;break;
        default:throw std::runtime_error("Invalid paired exit phase");
        }
        if(di)g[7]=pop(di);g[6]=pop(si);if(!di)g[0]=result;g[3]=pop(bx);g[4]=g[5];g[5]=pop(bp);pop(ret);g[4]+=24u;
    }
};
template<bool Actual=true,class State,class Call,class Before=Noop>
std::uint32_t run_private_owner(State&s,Call call,Before before={}){native_raw_x87_detail::Scope<State>fp{s};Frame<Actual,State,Call,Before>memory{s,call,before,fp};Data data;return native_tu15_reduce_paired_simplex(memory,data);}
} // namespace halo2::native_simplex_paired_detail
#endif
