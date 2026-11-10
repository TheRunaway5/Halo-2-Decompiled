#include "physics/runtime/native_tu15_host_fp_io.hpp"
#pragma once
#include "halo2/native_tu15_bsp_leaf_surface_kernel.hpp"
#include "physics/runtime/native_tu15_bsp_surface_2d.hpp"
#include "physics/runtime/native_tu15_bsp_surface_polygon.hpp"

namespace halo2::native_bsp_leaf_surface_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_BSP_LEAF_SURFACE");return p&&*p;}();return enabled;}
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State,class Call,class Before>struct Adapter {
    State&s;Call&call;Before&before;std::uint32_t sp,frame;bool compare_y_live{};
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;native_host_fp_io_detail::preserve([&]{before(s,pc);});}
    std::uint32_t word(std::uint32_t address,unsigned width=4)const{
        std::array<std::byte,4>bytes{};native_host_fp_io_detail::preserve([&]{s.memory.read(address,std::span(bytes).first(width));});std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8*n);return value;
    }
    void push80(const long double&value){
        if(s.fp.count>=8)throw std::runtime_error("Native BSP surface temporary x87 capacity unavailable");
        std::memmove(s.fp.lanes.data()+1,s.fp.lanes.data(),s.fp.count*sizeof(long double));std::memcpy(s.fp.lanes.data(),&value,10);
        ++s.fp.count;s.fp.top=(s.fp.top+7u)&7u;s.fp.status&=~0x200u;
    }
    void pop80(){
        if(!s.fp.count)throw std::runtime_error("Native BSP surface x87 temporary missing");
        --s.fp.count;std::memmove(s.fp.lanes.data(),s.fp.lanes.data()+1,s.fp.count*sizeof(long double));s.fp.top=(s.fp.top+1u)&7u;
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4,bool stack=false,bool pop_fp=false){
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},std::vector<std::byte>(width)};
        if(s.observe){event.before.resize(width);native_host_fp_io_detail::preserve([&]{s.memory.read(address,event.before);});}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));native_host_fp_io_detail::preserve([&]{s.memory.write(address,event.written);});
        if(stack)s.gp[4]=address;if(pop_fp){pop80();s.fp.status&=~0x200u;}if(s.observe)native_host_fp_io_detail::preserve([&]{s.observe(event);});
    }
    void push(std::uint32_t pc,std::uint32_t value){store(pc,s.gp[4]-4u,value,4,true);}
    std::uint32_t pop(std::uint32_t pc){at(pc);const auto value=word(s.gp[4]);s.gp[4]+=4u;return value;}
    std::uint32_t descriptor()const{return s.gp[3];}
    std::uint32_t candidate()const{return s.gp[1];}
    std::uint32_t fraction()const{return word(sp+16u);}
    std::uint32_t cursor(std::uint32_t)const{return word(frame+4u);}
    std::uint32_t plane_index()const{return word(sp+12u);}
    std::uint32_t abs_x(std::uint32_t)const{return word(frame+0x10u);}
    std::uint32_t abs_y(std::uint32_t)const{return word(frame+8u);}
    std::uint32_t abs_z(std::uint32_t)const{return word(sp+16u);}
    std::uint32_t axis(std::uint32_t)const{return word(sp+16u,2);}
    std::uint32_t displacement()const{return s.gp[6];}
    std::uint32_t origin()const{return s.gp[7];}
    std::uint32_t component(std::uint32_t index,const std::array<std::uint32_t,3>&)const{return word(frame+0x24u+index*4u);}
    std::uint32_t verify()const{return word(sp+20u,1);}
    std::uint32_t retry_index(std::uint32_t)const{return word(frame+0xcu);}
    std::uint32_t retry_end(std::uint32_t)const{return word(frame+0x18u);}
    std::uint32_t tree(const Progress&){
        push(0x1e6e0fu,0x1e6e14u);at(0x2a0230u);
        if(!native_bsp_surface_2d_detail::bridge(s,[&](auto&state,auto pc){before(state,pc);}))call(s,0x2a0230u,0x1e6e0fu,false);
        return s.gp[0];
    }
    bool polygon(const Progress&){
        auto&g=s.gp;at(0x1e6e1eu);g[0]=word(sp+16u);g[1]=g[2];at(0x1e6e24u);g[2]=word(frame+0x14u);
        push(0x1e6e28u,g[1]);at(0x1e6e29u);g[1]=word(sp+4u);push(0x1e6e2du,g[2]);push(0x1e6e2eu,g[0]);
        at(0x1e6e2fu);g[0]=word(sp+8u);push(0x1e6e33u,g[5]);push(0x1e6e34u,g[1]);g[2]=g[3];push(0x1e6e37u,0x1e6e3cu);
        if(!native_bsp_surface_polygon_detail::bridge(s,[&](auto&state,auto pc){before(state,pc);}))call(s,0x1e6e80u,0x1e6e37u,false);
        at(0x1e6e3cu);s.flags.logic(g[0]&255u,1);return(g[0]&255u)!=0;
    }
    void close_comparison(const Progress&d){
        if(compare_y_live){push80(d.compare_z);pop80();pop80();compare_y_live=false;s.flags.load(d.yz.flags);}
    }
    void progress(Phase phase,const Progress&d){
        auto&g=s.gp;
        switch(phase){
        case Phase::leaves:at(0x1e6cb3u);break;
        case Phase::first_index:g[0]=d.leaf;at(0x1e6cb9u);break;
        case Phase::count:g[1]=d.index;at(0x1e6cbdu);break;
        case Phase::initialize:g[0]=d.end;s.flags.minus(d.index,d.end,4);push(0x1e6cc5u,g[5]);store(0x1e6cc6u,frame+0xcu,d.index);store(0x1e6ccau,frame+0x18u,d.end);break;
        case Phase::entries:at(0x1e6cd4u);break;
        case Phase::fraction:g[2]=d.entries;at(0x1e6cd7u);break;
        case Phase::cache_cursor:s.xmm[7]=d.fraction;g[2]=d.cursor;store(0x1e6ce0u,frame+4u,d.cursor);break;
        case Phase::cursor:at(0x1e6ce4u);break;
        case Phase::plane:g[2]=d.cursor;at(0x1e6ce8u);break;
        case Phase::match_plane:g[5]=d.plane;g[2]=s.flags.logic(d.plane&0x7fffu,4);at(0x1e6cf3u);break;
        case Phase::plane_index:s.flags.minus(d.plane&0x7fffu,d.plane_index,4);at(0x1e6cfdu);break;
        case Phase::planes:g[0]=d.plane_index;at(0x1e6d01u);break;
        case Phase::normal_x:g[1]=d.planes;g[0]=d.plane_offset;s.flags.load(native_bsp_surface_polygon_detail::shifted_flags(d.plane_index,4u,s.flags.bits()));at(0x1e6d07u);break;
        case Phase::store_abs_x:g[0]=d.coefficients;s.flags.plus(d.plane_offset,d.planes,4);push80(d.normal_x.value);store(0x1e6d0eu,frame+0x10u,d.normal_x.rounded,4,false,true);break;
        case Phase::normal_y:at(0x1e6d12u);break;
        case Phase::read_abs_x:push80(d.normal_y.loaded);at(0x1e6d15u);break;
        case Phase::store_abs_y:s.xmm[0]=d.abs_x;std::memcpy(s.fp.lanes.data(),&d.normal_y.value,10);store(0x1e6d1du,frame+8u,d.normal_y.rounded,4,false,true);break;
        case Phase::normal_z:at(0x1e6d21u);break;
        case Phase::store_abs_z:push80(d.normal_z.value);store(0x1e6d26u,sp+16u,d.normal_z.rounded,4,false,true);break;
        case Phase::compare_y:at(0x1e6d2au);break;
        case Phase::compare_z:push80(d.compare_y);compare_y_live=true;at(0x1e6d2eu);break;
        case Phase::axis_z:close_comparison(d);at(0x1e6d38u);break;
        case Phase::axis_y:close_comparison(d);if(!d.yz.below){s.xmm[1]=d.abs_other;s.flags.load(d.axis_comparison.flags);}at(0x1e6d4du);break;
        case Phase::store_axis_z:s.xmm[1]=d.abs_other;s.flags.load(d.axis_comparison.flags);store(0x1e6d43u,sp+16u,2);break;
        case Phase::store_axis_y:s.xmm[1]=d.abs_other;s.flags.load(d.axis_comparison.flags);store(0x1e6d56u,sp+16u,1);break;
        case Phase::store_axis_x:store(0x1e6d60u,sp+16u,0);break;
        case Phase::axis:at(0x1e6d68u);break;
        case Phase::coefficient:g[1]=d.axis;at(0x1e6d6du);break;
        case Phase::zero:s.xmm[0]=d.coefficient;at(0x1e6d72u);break;
        case Phase::displacement_x:if(d.positive){g[0]=1;s.flags.load(d.coefficient_comparison.flags);}else g[0]=s.flags.logic(0,4);at(0x1e6d84u);break;
        case Phase::store_side:s.xmm[0]=d.x;g[2]=d.plane_sign;s.flags.minus(d.positive,d.plane_sign,1);g[0]=d.side;store(0x1e6d95u,frame+0x14u,d.side,1);break;
        case Phase::origin_x:s.xmm[0]=d.x;at(0x1e6d9du);break;
        case Phase::store_x:s.xmm[0]=d.x;g[0]=d.side;store(0x1e6da4u,frame+0x24u,d.values[0]);break;
        case Phase::displacement_y:at(0x1e6daau);break;
        case Phase::origin_y:s.xmm[0]=d.y;g[0]=d.side+d.axis*2u;at(0x1e6db6u);break;
        case Phase::store_y:s.xmm[0]=d.y;g[0]=(d.side+d.axis*2u)*3u;store(0x1e6dbeu,frame+0x28u,d.values[1]);break;
        case Phase::displacement_z:at(0x1e6dc4u);break;
        case Phase::first_component:s.xmm[0]=d.z;g[0]=d.table_index;s.flags.load(native_bsp_surface_polygon_detail::shifted_flags((d.side+d.axis*2u)*3u,1u,s.flags.bits()));at(0x1e6dcbu);break;
        case Phase::second_component:g[1]=d.first_component;at(0x1e6dd2u);break;
        case Phase::node_cursor:g[2]=d.second_component;at(0x1e6dd9u);break;
        case Phase::node:g[0]=d.cursor;at(0x1e6dddu);break;
        case Phase::origin_z:g[0]=d.node;s.xmm[0]=d.z;at(0x1e6de5u);break;
        case Phase::store_z:s.xmm[0]=d.z;store(0x1e6deau,frame+0x2cu,d.values[2]);break;
        case Phase::project_x:at(0x1e6df0u);break;
        case Phase::store_project_x:s.xmm[0]=scalar(d.point[0]);store(0x1e6df6u,frame+0x1cu,d.point[0]);break;
        case Phase::project_y:at(0x1e6dfcu);break;
        case Phase::store_project_y:s.xmm[0]=scalar(d.point[1]);g[1]=g[3]+0x20u;g[2]=frame+0x1cu;store(0x1e6e09u,frame+0x20u,d.point[1]);break;
        case Phase::tree:break;
        case Phase::verify:at(0x1e6e14u);g[5]=d.result;at(0x1e6e16u);break;
        case Phase::polygon:g[0]=(g[0]&0xffffff00u)|d.verify;s.flags.logic(d.verify,1);break;
        case Phase::retry_index:at(0x1e6e40u);break;
        case Phase::retry_end:g[1]=d.index;at(0x1e6e44u);break;
        case Phase::next_cursor:
            if(!d.matched)s.flags.minus(d.plane&0x7fffu,d.plane_index,4);else g[0]=d.end;
            at(0x1e6e48u);break;
        case Phase::store_index:g[1]=d.next_index;g[5]=d.next_cursor;s.flags.minus(d.next_index,d.end,4);store(0x1e6e52u,frame+0xcu,d.next_index);break;
        case Phase::store_cursor:store(0x1e6e56u,frame+4u,d.next_cursor);break;
        case Phase::finished:
            if(d.accepted){if(!d.verify){g[0]=(g[0]&0xffffff00u)|d.verify;s.flags.logic(d.verify,1);}g[0]=g[5];g[5]=pop(0x1e6e6cu);}
            else{g[0]=s.flags.logic(0xffffffffu,4);g[5]=pop(0x1e6e63u);}
            g[4]=s.flags.plus(g[4],0x2cu,4);pop(d.accepted?0x1e6e70u:0x1e6e67u);g[4]+=20u;break;
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
    std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));if((cw&0x3fu)!=0x3fu||(_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
        if(s.observe&&!c->observation_only_stores)return false;
        for(const auto site:c->service_sites)if(site>=0x1e6cb0u&&site<=0x1e6e70u)return false;
    }
    const auto sp=s.gp[4];Adapter<State,Call,Before>adapter{s,call,before,sp,sp-0x30u};adapter.at(0x1e6cb0u);s.gp[4]=s.flags.minus(sp,0x2cu,4);
    query(adapter,adapter,[&](auto phase,const auto&data){adapter.progress(phase,data);});return true;
#endif
}
} // namespace halo2::native_bsp_leaf_surface_detail
