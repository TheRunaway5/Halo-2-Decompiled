#pragma once
#include "halo2/source_geometry_resource.hpp"
#include "rendering/native_tu15_geometry_request_abi.hpp"

namespace halo2::native_geometry_publish_detail {
// The descriptor algorithm is the shared typed resource DATA owner. This
// adapter retains only the live caller frame, delayed allocation/descriptor
// inputs, iteration cells and final integer ABI. Arbitrary memory-fault and
// division-fault partial ABI remain a separate qualification.
template<class State> bool run(State &s) {
    if(!native_geometry_request_detail::admitted(s))return false;
    using namespace physics_step_detail;
    auto &g=s.gp;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto read=[&](std::uint32_t pc,std::uint32_t p){at(pc);return r(s.memory,p,4);};
    const auto store=[&](std::uint32_t pc,std::uint32_t p,std::uint32_t value){at(pc);w(s.memory,s.observe,pc,p,4,value);};
    g[4]-=8u;
    std::array<std::byte,4> input;
    SourceGeometryResourceMemory memory;
    memory.read=[&](std::uint32_t p,std::size_t n)->std::span<const std::byte>{
        if(n>input.size())throw std::runtime_error("Native geometry publication operand extent");
        s.memory.read(p,std::span(input).first(n));return std::span(input).first(n);
    };
    memory.store_tu15=[&](std::uint32_t pc,std::uint32_t p,std::span<const std::byte> bytes){
        std::uint32_t value{};for(unsigned i=0;i<bytes.size();++i)value|=std::uint32_t(std::to_integer<unsigned>(bytes[i]))<<(8u*i);
        at(pc);w(s.memory,s.observe,pc,p,bytes.size(),value);
    };
    bool framed=false;
    SourceGeometryResourcePublishDriver driver;
    driver.initial_allocation=[&](std::uint32_t root){
        s.push(0x22a687u,g[3]);g[3]=read(0x22a688u,g[4]+16u);g[0]=root;g[3]+=8u;return g[3]-8u;
    };
    driver.begin=[&](std::int32_t count){
        g[1]=std::uint32_t(count);g[0]=0;store(0x22a6a1u,g[4]+8u,0);
        if(count<=0)return false;
        s.push(0x22a6abu,g[5]);store(0x22a6acu,g[4]+8u,0);s.push(0x22a6b0u,g[6]);framed=true;return true;
    };
    driver.descriptor=[&](std::uint32_t base){g[0]=base;g[6]=read(0x22a6b4u,g[4]+12u);return base+g[6];};
    driver.current_allocation=[&]{g[1]=read(0x22a6ccu,g[4]+24u);return g[1];};
    driver.advance=[&]{
        g[0]=read(0x22a765u,g[4]+16u);g[2]=read(0x22a769u,g[4]+12u);g[1]=read(0x22a76du,g[7]+16u);
        ++g[0];g[2]+=16u;const bool more=std::bit_cast<std::int32_t>(g[0])<std::bit_cast<std::int32_t>(g[1]);
        store(0x22a776u,g[4]+16u,g[0]);store(0x22a77au,g[4]+12u,g[2]);return more;
    };
    source_geometry_resource_publish_driven(g[7],0,memory,driver);
    if(framed){g[6]=s.pop();g[5]=s.pop();}
    g[3]=s.pop();g[4]=s.flags.plus(g[4],8u,4);at(0x22a78au);s.pop();g[4]+=4u;
    return true;
}
} // namespace halo2::native_geometry_publish_detail
