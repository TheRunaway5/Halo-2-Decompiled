#include "physics/runtime/native_tu15_host_fp_io.hpp"
#pragma once
#include "halo2/native_tu15_bsp_surface_polygon_kernel.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <cstdlib>

namespace halo2::native_bsp_surface_polygon_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_BSP_SURFACE_POLYGON");return p&&*p;}();return enabled;}
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// Restore the exact preceding arithmetic context before shifts whose AF/OF
// result is undefined. SAHF leaves OF from the seeded ADD intact. No native
// PUSHF/red-zone writes or guest instruction callbacks are introduced.
inline std::uint32_t shifted_flags(std::uint32_t value,unsigned count,std::uint32_t context,bool right=false){
    auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);auto shift=count;
    if(right)asm volatile("addb $127,%%al;sahf;shrl %%cl,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(shift),"+r"(value)::"cc");
    else asm volatile("addb $127,%%al;sahf;shll %%cl,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(shift),"+r"(value)::"cc");
    return (flags&0xd5u)|((shift&1u)<<11u);
}
struct Memory {
    const SourcePhysicsContactMemory&memory;
    std::uint32_t word(std::uint32_t at,unsigned width=4)const{
        std::array<std::byte,4>bytes{};native_host_fp_io_detail::preserve([&]{memory.read(at,std::span(bytes).first(width));});std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8*n);return value;
    }
};
#endif
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if((_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
        if(s.observe&&!c->observation_only_stores)return false;
        for(const auto site:c->service_sites)if(site>=0x1e6e80u&&site<=0x1e6facu)return false;
    }
    auto&gp=s.gp;const auto entry_sp=gp[4],descriptor=gp[2],bitmap=gp[0];Memory memory{s.memory};
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;native_host_fp_io_detail::preserve([&]{before(s,pc);});};
    const auto store=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value,bool stack=false){
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},std::vector<std::byte>(4)};
        if(s.observe){event.before.resize(4);native_host_fp_io_detail::preserve([&]{s.memory.read(address,event.before);});}
        for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(8*n));native_host_fp_io_detail::preserve([&]{s.memory.write(address,event.written);});
        if(stack)gp[4]=address;if(s.observe)native_host_fp_io_detail::preserve([&]{s.observe(event);});
    };
    const auto push=[&](std::uint32_t pc,std::uint32_t value){store(pc,gp[4]-4u,value,true);};
    const auto pop=[&](std::uint32_t pc){at(pc);const auto value=memory.word(gp[4]);gp[4]+=4u;return value;};
    at(0x1e6e80u);const auto surface=gp[1]=memory.word(entry_sp+8u);
    gp[4]=s.flags.minus(gp[4],8u,4);push(0x1e6e87u,gp[3]);push(0x1e6e88u,gp[6]);push(0x1e6e89u,gp[7]);
    gp[7]=bitmap;s.flags.logic(bitmap,4);bool first_loop=true;
    struct Inputs {
        Memory&memory;std::uint32_t sp;
        std::uint32_t limit()const{return memory.word(sp+4u,2);}
        std::uint32_t axis()const{return memory.word(sp+12u,2);}
        std::uint32_t winding()const{return memory.word(sp+16u,1);}
        std::uint32_t edges(std::uint32_t)const{return memory.word(sp+4u);}
        std::uint32_t vertices(std::uint32_t)const{return memory.word(sp-8u);}
        std::uint32_t surface(std::uint32_t)const{return memory.word(sp+8u);}
        std::uint32_t first_edge(std::uint32_t)const{return memory.word(sp-4u);}
    }inputs{memory,entry_sp};
    struct Point {
        Memory&memory;std::uint32_t sp;
        std::uint32_t address()const{return memory.word(sp+20u);}
        std::uint32_t word(std::uint32_t address,unsigned lane)const{return memory.word(address+lane*4u);}
    }point{memory,entry_sp};
    const auto simd=[&](const Progress&d){s.xmm[0]=d.product_x;s.xmm[1]=d.cross;s.xmm[2]=d.edge_x;s.xmm[3]=d.dx;s.xmm[4]=d.first_x;s.xmm[5]=d.first_y;};
    const auto finish=[&](const Progress&d){
        if(d.exit==Exit::bitmap){gp[0]=d.record;s.flags.logic(d.word&d.mask,4);gp[7]=pop(0x1e6ebfu);gp[6]=pop(0x1e6ec0u);gp[0]&=0xffffff00u;s.flags.logic(0,1);gp[3]=pop(0x1e6ec3u);}
        else if(d.exit==Exit::outside){simd(d);s.flags.load(d.comparison.flags);gp[5]=pop(0x1e6fa3u);gp[7]=pop(0x1e6fa4u);gp[6]=pop(0x1e6fa5u);gp[0]&=0xffffff00u;s.flags.logic(0,1);gp[3]=pop(0x1e6fa8u);}
        else {s.flags.minus(d.next,d.loop_limit,4);gp[5]=pop(0x1e6f97u);gp[7]=pop(0x1e6f98u);gp[6]=pop(0x1e6f99u);gp[0]=(gp[0]&0xffffff00u)|1u;gp[3]=pop(0x1e6f9cu);}
        gp[4]=s.flags.plus(gp[4],8u,4);pop(d.exit==Exit::bitmap?0x1e6ec7u:(d.exit==Exit::outside?0x1e6facu:0x1e6fa0u));gp[4]+=20u;
    };
    const auto progress=[&](Phase phase,const Progress&d){
        switch(phase){
        case Phase::surfaces:at(0x1e6e8eu);break;
        case Phase::surface_flags:gp[0]=d.record;at(0x1e6e96u);break;
        case Phase::material:s.flags.logic(d.flags,1);at(0x1e6e9cu);break;
        case Phase::material_limit:gp[1]=d.material;at(0x1e6ea0u);break;
        case Phase::material_reload:s.flags.minus(d.material,d.limit,2);at(0x1e6ea7u);break;
        case Phase::material_word:{
            gp[1]=d.current_material&31u;gp[3]=d.mask;gp[6]=d.word_index;
            s.flags.logic(gp[1],4);const auto mask_flags=shifted_flags(1u,gp[1],s.flags.bits());
            s.flags.load(shifted_flags(d.current_material,5u,mask_flags,true));at(0x1e6ebau);break;}
        case Phase::edges:
            gp[0]=d.record;
            if(!d.bitmap)s.flags.logic(0,4);
            else if(!d.flags)s.flags.logic(0,1);
            else if(std::bit_cast<std::int32_t>(signed_half(d.material))>=std::bit_cast<std::int32_t>(signed_half(d.limit)))s.flags.minus(d.material,d.limit,2);
            else s.flags.logic(d.word&d.mask,4);
            at(0x1e6ecau);break;
        case Phase::vertices:gp[1]=d.edges;at(0x1e6ecdu);break;
        case Phase::axis:gp[3]=d.vertices;at(0x1e6ed0u);break;
        case Phase::first_edge:gp[2]=d.axis;at(0x1e6ed5u);break;
        case Phase::cache_edges:gp[0]=d.first_edge;s.xmm[6]=Scalar{};store(0x1e6edcu,entry_sp+4u,d.edges);break;
        case Phase::winding:at(0x1e6ee0u);break;
        case Phase::first_component:gp[1]=d.table_index;s.flags.load(shifted_flags((d.winding+d.axis*2u)*3u,1u,s.flags.bits()));at(0x1e6eedu);break;
        case Phase::save_bp:gp[7]=d.first_component;push(0x1e6ef4u,gp[5]);break;
        case Phase::second_component:at(0x1e6ef5u);break;
        case Phase::save_first_edge:gp[5]=d.second_component;gp[7]=d.first_offset;s.flags.load(shifted_flags(d.first_component,2u,s.flags.bits()));store(0x1e6effu,entry_sp-4u,d.first_edge);break;
        case Phase::save_vertices:store(0x1e6f03u,entry_sp-8u,d.vertices);break;
        case Phase::edge_cursor:
            if(first_loop){gp[5]=d.second_offset;s.flags.load(shifted_flags(d.second_component,2u,s.flags.bits()));first_loop=false;}
            else s.flags.minus(d.index,d.loop_limit,4);
            gp[2]=d.index*3u;at(0x1e6f13u);break;
        case Phase::edge_surface:gp[0]=d.current_edges;at(0x1e6f17u);break;
        case Phase::surface_compare:gp[1]=d.edge_surface;gp[6]=d.edge;at(0x1e6f1fu);break;
        case Phase::first_vertex:s.flags.minus(d.edge_surface,d.current_surface,4);gp[1]=(d.edge_surface&0xffffff00u)|d.selected;gp[2]=d.selected;at(0x1e6f29u);break;
        case Phase::first_x:gp[0]=d.first_position;s.flags.plus(d.first_vertex*16u,d.vertices,4);at(0x1e6f32u);break;
        case Phase::first_y:s.xmm[4]=d.first_x;at(0x1e6f37u);break;
        case Phase::point_source:s.xmm[5]=d.first_y;at(0x1e6f3cu);break;
        case Phase::point_x:gp[0]=d.point;at(0x1e6f40u);break;
        case Phase::point_y:s.xmm[3]=d.point_x;at(0x1e6f44u);break;
        case Phase::second_vertex:s.xmm[3]=d.dx;s.xmm[0]=d.dy;gp[3]=d.selected?0u:1u;s.flags.logic(d.selected,1);at(0x1e6f58u);break;
        case Phase::second_vertices:gp[1]=d.second_vertex;at(0x1e6f5cu);break;
        case Phase::second_x:gp[3]=d.vertices;gp[1]=d.second_position;s.flags.plus(d.second_vertex*16u,d.vertices,4);at(0x1e6f65u);break;
        case Phase::second_y:s.xmm[2]=d.second_x;at(0x1e6f6au);break;
        case Phase::next_edge:simd(d);s.flags.load(d.comparison.flags);at(0x1e6f88u);break;
        case Phase::loop_limit:gp[0]=d.next;at(0x1e6f8du);break;
        case Phase::finished:finish(d);break;
        }
    };
    contains(memory,descriptor,bitmap,surface,inputs,point,progress);return true;
#endif
}
} // namespace halo2::native_bsp_surface_polygon_detail
