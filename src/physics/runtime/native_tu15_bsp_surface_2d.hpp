#include "physics/runtime/native_tu15_host_fp_io.hpp"
#pragma once
#include "halo2/native_tu15_bsp_surface_2d_kernel.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <cstdlib>

namespace halo2::native_bsp_surface_2d_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto* p=std::getenv("HALO2_REFERENCE_BSP_SURFACE_2D");return p&&*p;}();return enabled;}
template<class State>void position(State& state,std::uint32_t site){if(state.instruction_cursor&&state.instruction_cursor->position)*state.instruction_cursor->position=site;}
struct Memory {
    const SourcePhysicsContactMemory& memory;
    std::uint32_t word(std::uint32_t at,unsigned width=4)const {
        std::array<std::byte,4> bytes{};native_host_fp_io_detail::preserve([&]{memory.read(at,std::span(bytes).first(width));});std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
    }
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline std::uint32_t index_flags(std::uint32_t node){
    std::uint32_t flags{},overflow{};
    asm volatile("andl $32767,%%ecx;shll $4,%%ecx;seto %%dl;lahf;movzbl %%ah,%%eax;movzbl %%dl,%%edx"
        :"+c"(node),"=a"(flags),"=d"(overflow)::"cc");
    return flags|((overflow&1u)<<11u);
}
#endif
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Before=Noop>bool bridge(State& state,Before before={}){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference_mode()||state.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{state.selected_observation_pcs;})if(!state.selected_observation_pcs)return false;
    if constexpr(requires{state.active_mmx;})if(state.active_mmx)return false;
    if(state.services.instruction){
        if(!state.literal_service_sites||!state.instruction_cursor||!state.instruction_cursor->position)return false;
        const auto* cursor=state.instruction_cursor;
        if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))return false;
        if(state.observe&&!cursor->observation_only_stores)return false;
        for(const auto site:cursor->service_sites)if(site>=0x2a0230u&&site<=0x2a028du)return false;
    }
    auto& gp=state.gp;const auto node=gp[0],descriptor=gp[1],point_address=gp[2];Memory memory{state.memory};
    const auto at=[&](std::uint32_t site){position(state,site);native_host_fp_io_detail::preserve([&]{before(state,site);});};
    const auto push=[&](std::uint32_t pc,std::uint32_t value){
        // Preserve the existing State::push pre-store stack commitment.
        gp[4]-=4u;SourcePhysicsContactRecordStore event{pc,gp[4],{},std::vector<std::byte>(4)};
        if(state.observe){event.before.resize(4);native_host_fp_io_detail::preserve([&]{state.memory.read(event.address,event.before);});}
        for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(8u*n));
        native_host_fp_io_detail::preserve([&]{state.memory.write(event.address,event.written);});
        if(state.observe)native_host_fp_io_detail::preserve([&]{state.observe(event);});
    };
    const auto pop=[&](){const auto value=memory.word(gp[4]);gp[4]+=4u;return value;};
    struct Point {const Memory& memory;std::uint32_t address;std::uint32_t word(unsigned lane)const{return memory.word(address+lane*4u);}}point{memory,point_address};
    state.flags.logic((node>>8u)&255u,1);
    const auto progress=[&](Phase phase,const Progress& data){
        switch(phase){
        case Phase::begin:state.xmm[0]=Scalar{};at(0x2a0237u);push(0x2a0237u,gp[6]);break;
        case Phase::nodes:at(0x2a0238u);break;
        case Phase::row:gp[6]=data.nodes;gp[0]=data.offset;state.flags.load(index_flags(data.node));at(0x2a0248u);break;
        case Phase::y_loaded:state.xmm[1]=data.y;at(0x2a024eu);break;
        case Phase::y_product:state.xmm[1]=data.y;at(0x2a0253u);break;
        case Phase::x_loaded:state.xmm[2]=data.x;at(0x2a0258u);break;
        case Phase::products:state.xmm[2]=data.x;gp[0]=state.flags.plus(data.offset,data.nodes,4);break;
        case Phase::sum:state.xmm[1]=data.y;at(0x2a0262u);break;
        case Phase::child:
            state.xmm[1]=data.y;gp[1]=data.side;
            if(data.side)state.flags.load(data.comparison.flags);else state.flags.logic(0,4);
            at(0x2a0275u);break;
        case Phase::next:gp[0]=data.node;state.flags.logic((data.node>>8u)&255u,1);break;
        case Phase::finish:at(0x2a027eu);gp[6]=pop();break;
        }
    };
    gp[0]=find(memory,descriptor,node,point,progress);state.flags.logic(gp[0],4);
    at(gp[0]==0xffffffffu?0x2a028du:0x2a0289u);pop();return true;
#endif
}
} // namespace halo2::native_bsp_surface_2d_detail
