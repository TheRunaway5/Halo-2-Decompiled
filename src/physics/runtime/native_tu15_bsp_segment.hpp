#include "physics/runtime/native_tu15_host_fp_io.hpp"
#pragma once
#include "halo2/native_tu15_bsp_segment_kernel.hpp"
#include "physics/runtime/native_tu15_bsp_leaf_surface.hpp"

namespace halo2::native_bsp_segment_detail {
using Noop=native_bsp_leaf_surface_detail::Noop;
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_BSP_SEGMENT");return p&&*p;}();return enabled;}
inline std::uint32_t spill_reference(long double value,std::uint16_t&status){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto converted=single_store(value);status=(status&~0x200u)|(converted.status&0x200u);return converted.value;
#else
    return physics_step_detail::u(physics_step_detail::spill(value));
#endif
}
template<class State,class Call,class Before=Noop>bool bridge(State&,Call,Before={});
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline std::uint32_t arithmetic_shift_flags(std::uint32_t value,unsigned count,std::uint32_t context){
    auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);auto shift=count;
    asm volatile("addb $127,%%al;sahf;sarl %%cl,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(shift),"+r"(value)::"cc");
    return(flags&0xd5u)|((shift&1u)<<11u);
}
template<class State,class Call,class Before>struct Adapter:native_bsp_leaf_surface_detail::Adapter<State,Call,Before> {
    using Base=native_bsp_leaf_surface_detail::Adapter<State,Call,Before>;
    using Base::s;using Base::sp;using Base::at;using Base::word;using Base::push;using Base::pop;using Base::push80;using Base::pop80;
    bool leaf_stored{},kind_stored{},cross_live{};unsigned filter_mask{};
    Adapter(State&state,Call&call,Before&before,std::uint32_t stack):Base{state,call,before,stack,stack-20u}{}
    std::uint32_t node()const{return word(sp+8u);}
    std::uint32_t context()const{return word(sp+4u);}
    std::uint32_t maximum(std::uint32_t)const{return word(sp+16u);}
    std::uint32_t minimum(std::uint32_t)const{return word(sp+12u);}
    std::uint32_t distance(std::uint32_t)const{return word(sp+4u);}
    std::uint32_t slope(std::uint32_t)const{return word(sp-4u);}
    std::uint32_t crossing(std::uint32_t)const{return word(sp+8u);}
    std::uint32_t leaf(std::uint32_t)const{return word(sp+16u);}
    std::uint32_t kind(std::uint32_t)const{return word(sp+8u,1);}
    std::uint32_t candidate(std::uint32_t)const{return word(sp+4u);}
    std::uint32_t verify(std::uint32_t)const{return word(sp-8u);}
    std::uint32_t flags(std::uint32_t)const{return word(sp-4u,1);}
    void raw_store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width,bool packet=true){
        if(packet)at(pc);SourcePhysicsContactRecordStore event{pc,address,{},std::vector<std::byte>(width)};
        if(s.observe){event.before.resize(width);native_host_fp_io_detail::preserve([&]{s.memory.read(address,event.before);});}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));native_host_fp_io_detail::preserve([&]{s.memory.write(address,event.written);});if(s.observe)native_host_fp_io_detail::preserve([&]{s.observe(event);});
    }
    void store_crossing(long double value){
        constexpr auto pc=0x1e6a6fu;at(pc);const auto converted=single_store(value);
        s.fp.status=(s.fp.status&~0x200u)|(converted.status&0x200u);
        SourcePhysicsContactRecordStore event{pc,sp+8u,{},std::vector<std::byte>(4)};
        if(s.observe){event.before.resize(4);native_host_fp_io_detail::preserve([&]{s.memory.read(event.address,event.before);});}
        for(unsigned n=0;n<4;++n)event.written[n]=std::byte(converted.value>>(n*8u));native_host_fp_io_detail::preserve([&]{s.memory.write(event.address,event.written);});
        pop80();cross_live=false;if(s.observe)native_host_fp_io_detail::preserve([&]{s.observe(event);});
    }
    RecursiveContinuation recurse(const Progress&d,std::uint32_t child_node){
        auto&g=s.gp;g[2]=child_node;s.flags.load(arithmetic_shift_flags(d.packed,8u,s.flags.bits()));
        push(0x1e6a86u,g[2]);push(0x1e6a87u,g[5]);push(0x1e6a88u,0x1e6a8du);
        if(!native_bsp_segment_detail::bridge(s,this->call,this->before))this->call(s,0x1e6980u,0x1e6a88u,false);
        return{(g[0]&255u)!=0,g[5],g[6],g[3]&255u};
    }
    SurfaceContinuation surface(const Progress&d){
        auto&g=s.gp;g[2]=d.bitmap_limit;push(0x1e6bcau,g[0]);push(0x1e6bcbu,g[2]);push(0x1e6bccu,0x1e6bd1u);
        if(!native_bsp_leaf_surface_detail::bridge(s,this->call,this->before))this->call(s,0x1e6cb0u,0x1e6bccu,false);
        return{g[0],g[5],g[3]};
    }
    void store(Write kind,std::uint32_t address,std::uint32_t value,unsigned width){
        std::uint32_t pc{};bool packet=true;
        switch(kind){
        case Write::distance:pc=0x1e6a24u;address=sp+4u;break;
        case Write::slope:pc=0x1e6a34u;address=sp-4u;break;
        case Write::minimum:pc=0x1e6ab7u;address=sp+12u;break;
        case Write::leaf:pc=leaf_stored?0x1e6afcu:0x1e6adfu;address=sp+16u;leaf_stored=true;break;
        case Write::kind:pc=kind_stored?0x1e6b0cu:0x1e6ae3u;address=sp+8u;kind_stored=true;break;
        case Write::verify:pc=value?0x1e6b98u:0x1e6ae7u;address=sp-8u;break;
        case Write::flags:pc=0x1e6b1bu;address=sp-4u;break;
        case Write::candidate:pc=candidate_pc;address=sp+4u;break;
        case Write::far_plane:pc=0x1e6ab0u;break;
        case Write::fraction:pc=0x1e6c05u;break;
        case Write::plane:pc=0x1e6c1au;break;
        case Write::hit_candidate:pc=0x1e6c24u;break;
        case Write::surface:pc=0x1e6c2au;break;
        case Write::material:pc=0x1e6c33u;break;
        case Write::hit_flags:pc=0x1e6c3cu;break;
        case Write::brush:pc=0x1e6c46u;break;
        case Write::partition:pc=0x1e6c52u;break;
        case Write::record:pc=0x1e6c7au;break;
        case Write::count:pc=0x1e6c81u;packet=false;break;
        case Write::overflow:pc=0x1e6c96u;break;
        case Write::previous_leaf:pc=recorded?0x1e6c85u:0x1e6c9du;break;
        case Write::previous_kind:pc=recorded?0x1e6c89u:0x1e6ca1u;break;
        case Write::crossing:throw std::runtime_error("Crossing store requires its extended value");
        }
        raw_store(pc,address,value,width,packet);
    }
    std::uint32_t candidate_pc{};bool recorded{};
    void low(unsigned reg,std::uint32_t value){s.gp[reg]=(s.gp[reg]&0xffffff00u)|(value&255u);}
    void branch_values(const Progress&d){low(0,d.minimum_side);low(1,d.maximum_side);s.flags.minus(d.minimum_side,d.maximum_side,1);}
    void finish(const Progress&d){
        auto&g=s.gp;
        if(d.exit==Exit::near_hit||d.exit==Exit::pruned){
            const bool hit=d.exit==Exit::near_hit;g[6]=pop(hit?0x1e6b3au:0x1e6b45u);g[5]=pop(hit?0x1e6b3bu:0x1e6b46u);
            if(hit)low(0,1u);else{low(0,0u);s.flags.logic(0,1);}g[3]=pop(hit?0x1e6b3eu:0x1e6b49u);g[4]=s.flags.plus(g[4],8u,4);pop(hit?0x1e6b42u:0x1e6b4du);g[4]+=16u;return;
        }
        if(d.exit==Exit::hit){low(0,1u);g[3]=pop(0x1e6c58u);g[4]=s.flags.plus(g[4],8u,4);pop(0x1e6c5cu);g[4]+=16u;return;}
        g[5]=pop(recorded?0x1e6c8cu:0x1e6ca4u);low(0,0);s.flags.logic(0,1);g[3]=pop(recorded?0x1e6c8fu:0x1e6ca7u);g[4]=s.flags.plus(g[4],8u,4);pop(recorded?0x1e6c93u:0x1e6cabu);g[4]+=16u;
    }
    void progress(Phase phase,const Progress&d){
        auto&g=s.gp;
        switch(phase){
        case Phase::node_argument:at(0x1e6980u);break;
        case Phase::context_argument:g[0]=d.node;g[4]=s.flags.minus(g[4],8u,4);s.flags.logic(d.node&0x800000u,4);push(0x1e698cu,g[3]);push(0x1e698du,g[5]);at(0x1e698eu);break;
        case Phase::save_index:g[5]=d.context;push(0x1e6992u,g[6]);break;
        case Phase::node:break;
        case Phase::descriptor:at(0x1e69a0u);break;
        case Phase::nodes:g[1]=d.descriptor;at(0x1e69a3u);break;
        case Phase::plane_index:g[2]=d.nodes;g[0]=s.flags.logic(d.node&0x7fffffu,4);g[6]=d.record;at(0x1e69aeu);break;
        case Phase::planes:g[0]=d.plane_index;at(0x1e69b1u);break;
        case Phase::origin:g[2]=d.planes;at(0x1e69b4u);break;
        case Phase::origin_z:g[1]=d.origin;at(0x1e69b7u);break;
        case Phase::origin_y:s.xmm[0]=d.distance;at(0x1e69bcu);break;
        case Phase::plane_y:s.xmm[1]=d.y;g[0]=d.plane_index*16u;s.flags.load(native_bsp_surface_polygon_detail::shifted_flags(d.plane_index,4u,s.flags.bits()));at(0x1e69c4u);break;
        case Phase::plane_z:s.xmm[1]=d.y;at(0x1e69cau);break;
        case Phase::plane_x:g[0]=d.plane;s.flags.plus(d.plane_index*16u,d.planes,4);s.xmm[0]=d.distance;at(0x1e69d6u);break;
        case Phase::origin_x:s.xmm[1]=d.x;at(0x1e69dau);break;
        case Phase::displacement:s.xmm[1]=d.x;at(0x1e69deu);break;
        case Phase::displacement_y:g[1]=d.displacement;at(0x1e69e1u);break;
        case Phase::displacement_plane_y:s.xmm[2]=d.dy;at(0x1e69e6u);break;
        case Phase::displacement_z:s.xmm[2]=d.dy;s.xmm[0]=d.distance;at(0x1e69efu);break;
        case Phase::displacement_plane_z:s.xmm[1]=d.slope;at(0x1e69f4u);break;
        case Phase::plane_distance:s.xmm[1]=d.slope;at(0x1e69f9u);break;
        case Phase::displacement_x:s.xmm[0]=d.distance;s.xmm[1]=d.slope;at(0x1e6a02u);break;
        case Phase::displacement_plane_x:s.xmm[2]=d.dx;at(0x1e6a06u);break;
        case Phase::maximum:s.xmm[1]=d.slope;s.xmm[2]=d.slope;at(0x1e6a11u);break;
        case Phase::minimum:s.xmm[2]=d.at_max;s.xmm[3]=d.slope;at(0x1e6a1au);break;
        case Phase::distance_store:s.xmm[3]=d.at_min;break;
        case Phase::slope_store:s.xmm[2]=d.at_max;s.xmm[0]=d.zero;s.flags.load(d.min_comparison.flags);break;
        case Phase::same_child:branch_values(d);g[0]=d.side;g[1]=d.record+d.side*2u;at(0x1e6ac5u);break;
        case Phase::cross_distance:branch_values(d);s.flags.load(d.slope_comparison.flags);low(3,d.slope_positive);if(!d.slope_positive)s.flags.logic(0,1);at(0x1e6a5cu);break;
        case Phase::cross_minimum:push80(d.crossing_distance);cross_live=true;at(0x1e6a60u);break;
        case Phase::cross_slope:g[1]=d.minimum;at(0x1e6a64u);break;
        case Phase::cross_store:s.flags.logic(d.slope_positive,1);low(2,!d.slope_positive);std::memcpy(s.fp.lanes.data(),&d.crossing_extended,10);s.fp.status&=~0x200u;break;
        case Phase::cross_reload:at(0x1e6a73u);break;
        case Phase::near_child:g[0]=d.crossing;push(0x1e6a77u,g[0]);g[0]=g[2]&255u;push(0x1e6a7bu,g[1]);g[1]=d.record+d.side*2u;at(0x1e6a7fu);break;
        case Phase::recurse:break;
        case Phase::recursed:at(0x1e6a8du);s.flags.logic(g[0]&255u,1);break;
        case Phase::near_output:at(0x1e6a95u);break;
        case Phase::near_cross:g[0]=d.output;at(0x1e6a98u);break;
        case Phase::near_limit:s.xmm[0]=d.cross_scalar;at(0x1e6a9eu);break;
        case Phase::far_plane:s.flags.load(d.limit_comparison.flags);at(0x1e6aa7u);break;
        case Phase::far_plane_store:g[1]=d.far_plane;g[0]=d.side;g[2]=d.record+d.side*2u;break;
        case Phase::far_child:at(0x1e6ab3u);break;
        case Phase::minimum_store:g[0]=d.packed;break;
        case Phase::next_node:g[0]=d.node;s.flags.logic(d.node&0x800000u,4);break;
        case Phase::leaf:g[6]=s.flags.logic(0xffffffffu,4);s.flags.minus(d.node,0xffffffffu,4);low(3,3);break;
        case Phase::leaf_descriptor:at(0x1e6aeeu);break;
        case Phase::leaves:g[2]=d.descriptor;at(0x1e6af1u);break;
        case Phase::leaf_flags:g[1]=d.nodes;g[0]=s.flags.logic(d.node,4);at(0x1e6af9u);break;
        case Phase::leaf_store:if(leaf_stored)low(2,d.leaf_flags);break;
        case Phase::leaf_reload:at(0x1e6b00u);break;
        case Phase::kind_store:if(kind_stored){g[6]=d.leaf;s.flags.logic(d.leaf_flags&1u,1);low(0,s.flags.increment(d.kind-1u,1,false));}break;
        case Phase::flags:low(3,d.kind);at(0x1e6b12u);break;
        case Phase::save_flags:g[1]=d.flags;g[2]=s.flags.logic(d.first,4);push(0x1e6b1au,g[7]);break;
        case Phase::enter_previous:at(0x1e6b21u);break;
        case Phase::consider_enter:low(0,d.previous_kind);s.flags.minus(d.previous_kind,d.previous_kind==1u?1u:2u,1);if(d.previous_kind==1u||d.previous_kind==2u)s.flags.minus(d.kind,3u,1);break;
        case Phase::enter_leaf:at(0x1e6b31u);break;
        case Phase::exit_previous:s.flags.logic(d.flags&2u,1);at(0x1e6b55u);break;
        case Phase::consider_exit:s.flags.minus(d.previous_kind,3u,1);if(d.previous_kind==3u)s.flags.minus(d.kind,d.kind==1u?1u:2u,1);break;
        case Phase::inside_previous:s.flags.logic(d.flags&4u,1);at(0x1e6b74u);break;
        case Phase::consider_inside:s.flags.minus(d.previous_kind,2u,1);if(d.previous_kind==2u)s.flags.minus(d.kind,2u,1);break;
        case Phase::inside_leaf:s.flags.logic(d.first,4);at(0x1e6b8bu);break;
        case Phase::candidate_store:
            if(d.selection==Candidate::enter){g[2]=d.candidate;candidate_pc=0x1e6b34u;}
            else if(d.selection==Candidate::exit)candidate_pc=0x1e6b65u;
            else{if(d.first){g[0]=d.candidate;candidate_pc=0x1e6b8eu;}else{s.flags.logic(d.first,4);candidate_pc=0x1e6b94u;}}break;
        case Phase::verify_store:break;
        case Phase::candidate:at(0x1e6b9du);break;
        case Phase::verify:g[1]=d.candidate;s.flags.minus(d.candidate,0xffffffffu,4);at(0x1e6baau);break;
        case Phase::query_minimum:g[2]=d.verify;at(0x1e6baeu);break;
        case Phase::query_descriptor:g[0]=d.minimum;at(0x1e6bb2u);break;
        case Phase::query_displacement:g[3]=d.descriptor;at(0x1e6bb5u);break;
        case Phase::query_origin:g[6]=d.displacement;at(0x1e6bb8u);break;
        case Phase::push_verify:g[7]=d.origin;push(0x1e6bbbu,g[2]);break;
        case Phase::query_plane:at(0x1e6bbcu);break;
        case Phase::push_minimum:g[2]=d.plane_index;push(0x1e6bbfu,g[0]);break;
        case Phase::query_bitmap:at(0x1e6bc0u);break;
        case Phase::push_plane:g[0]=d.bitmap;push(0x1e6bc3u,g[2]);break;
        case Phase::query_limit:g[2]=s.flags.logic(0,4);at(0x1e6bc6u);break;
        case Phase::surface:break;
        case Phase::surface_returned:at(0x1e6bd1u);s.flags.minus(d.surface,0xffffffffu,4);break;
        case Phase::surfaces:at(0x1e6bdau);break;
        case Phase::surface_flags:g[1]=d.surfaces;at(0x1e6bddu);break;
        case Phase::filter_first:low(2,d.surface_flags);g[1]=d.surface_row;s.flags.logic(d.surface_flags&2u,1);filter_mask=0;break;
        case Phase::saved_flags_first:filter_mask=8;at(0x1e6be9u);break;
        case Phase::filter_second:s.flags.logic(d.surface_flags&8u,1);filter_mask=0;break;
        case Phase::saved_flags_second:filter_mask=16;at(0x1e6bf5u);break;
        case Phase::hit_output:if(filter_mask)s.flags.logic(d.saved_flags&filter_mask,1);at(0x1e6bfcu);break;
        case Phase::hit_minimum:g[2]=d.output;at(0x1e6bffu);break;
        case Phase::fraction_store:s.xmm[0]=scalar(d.minimum);break;
        case Phase::hit_descriptor:at(0x1e6c09u);break;
        case Phase::hit_planes:g[6]=d.descriptor;at(0x1e6c0cu);break;
        case Phase::hit_plane_index:g[7]=d.current_planes;at(0x1e6c0fu);break;
        case Phase::plane_output:g[2]=d.current_plane;at(0x1e6c12u);break;
        case Phase::plane_store:g[6]=d.output;g[2]=d.current_planes+d.current_plane*16u;s.flags.load(native_bsp_surface_polygon_detail::shifted_flags(d.current_plane,4u,s.flags.bits()));s.flags.plus(d.current_plane*16u,d.current_planes,4);break;
        case Phase::candidate_output:at(0x1e6c1du);break;
        case Phase::hit_candidate:g[2]=d.output;at(0x1e6c20u);break;
        case Phase::candidate_output_store:g[6]=d.candidate;break;
        case Phase::surface_output:at(0x1e6c27u);break;
        case Phase::surface_store:g[2]=d.output;break;
        case Phase::material:at(0x1e6c2du);break;
        case Phase::material_output:g[0]=d.material;at(0x1e6c30u);break;
        case Phase::material_store:g[2]=d.output;break;
        case Phase::flags_output:at(0x1e6c36u);break;
        case Phase::hit_flags:g[0]=d.output;at(0x1e6c39u);break;
        case Phase::flags_store:low(2,d.flags_byte);break;
        case Phase::brush_output:at(0x1e6c3fu);break;
        case Phase::brush:g[0]=d.output;at(0x1e6c42u);break;
        case Phase::brush_store:low(2,d.brush);g[7]=pop(0x1e6c45u);break;
        case Phase::partition_output:at(0x1e6c49u);break;
        case Phase::partition:g[0]=d.output;at(0x1e6c4cu);break;
        case Phase::partition_store:g[1]=(g[1]&0xffff0000u)|d.partition;g[6]=pop(0x1e6c50u);g[5]=pop(0x1e6c51u);break;
        case Phase::cached_kind:if(filter_mask)s.flags.logic(d.saved_flags&filter_mask,1);at(0x1e6c5fu);break;
        case Phase::cached_leaf:low(3,d.kind);at(0x1e6c63u);break;
        case Phase::record_leaf:g[6]=d.leaf;if(d.selection!=Candidate::none&&!d.queried&&d.candidate==0xffffffffu)g[1]=d.candidate;s.flags.minus(d.leaf,0xffffffffu,4);break;
        case Phase::record_output:at(0x1e6c6cu);break;
        case Phase::record_count:g[0]=d.output;at(0x1e6c6fu);break;
        case Phase::record_store:g[1]=d.count;s.flags.minus(d.count,0x100u,4);break;
        case Phase::counter_output:at(0x1e6c7eu);break;
        case Phase::counter_read:g[0]=d.output;at(0x1e6c81u);break;
        case Phase::counter_store:s.flags.increment(d.current_count,4,false);recorded=true;break;
        case Phase::overflow_store:g[1]=d.count;s.flags.minus(d.count,0x100u,4);break;
        case Phase::previous_leaf_store:g[7]=pop(recorded?0x1e6c84u:0x1e6c9cu);break;
        case Phase::previous_kind_store:g[6]=pop(recorded?0x1e6c88u:0x1e6ca0u);break;
        case Phase::finished:if(d.exit==Exit::pruned)s.flags.load(d.limit_comparison.flags);finish(d);break;
        }
    }
};
#endif
template<class State,class Call,class Before>bool bridge(State&s,Call call,Before before){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15||s.fp.count>6u||(s.fp.status&0x8080u))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));if((cw&0x3fu)!=0x3fu||(_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
        if(s.observe&&!c->observation_only_stores)return false;
        for(const auto site:c->service_sites)if(site>=0x1e6980u&&site<=0x1e6cabu)return false;
    }
    Adapter<State,Call,Before>adapter{s,call,before,s.gp[4]};traverse(adapter,adapter,[&](auto phase,const auto&data){adapter.progress(phase,data);});return true;
#endif
}
} // namespace halo2::native_bsp_segment_detail
