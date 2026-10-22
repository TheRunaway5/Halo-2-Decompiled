#include "halo2/source_renderer_request_tu15.hpp"
namespace halo2 {
namespace {
template<std::size_t N> auto encode(const std::array<std::uint32_t,N>& words) {
    std::array<std::byte,N*4> bytes{};
    for(unsigned i=0;i<N;++i)for(unsigned j=0;j<4;++j)bytes[4*i+j]=std::byte(words[i]>>(8*j));
    return bytes;
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t p) {
    const auto bytes=memory.view(p,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
}
void mount_native_tu15_renderer_request_data(SourceRuntimeMemory& memory) {
    memory.mount(0x448e40,encode(std::array<std::uint32_t,3>{0x10000,3,1}),SourceRuntimeMemory::Access::ReadOnly,"source12560 authored display options");
    memory.mount(0x473574,encode(std::array<std::uint32_t,2>{0x3c800000,0x44800000}),SourceRuntimeMemory::Access::ReadOnly,"source169D0 authored display operands");
    memory.mount(0x46a28c,encode(std::array<std::uint32_t,2>{0x43f00000,0x44200000}),SourceRuntimeMemory::Access::ReadOnly,"source16A10 logical height and width operands");
    memory.mount(0x4922a8,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,"source12560 cold initialized byte");
    memory.mount(0x4922aa,std::array<std::byte,16>{},SourceRuntimeMemory::Access::ReadWrite,"source16A10 outer and safe rectangles");
    memory.mount(0x4922c0,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,"source169D0 cold display fields");
    memory.mount(0x4922e0,std::array<std::byte,2>{},SourceRuntimeMemory::Access::ReadWrite,"source169D0 refresh field");
    memory.mount(0x4922e8,std::array<std::byte,20>{},SourceRuntimeMemory::Access::ReadWrite,"source display option copies and operands");
    memory.mount(0x515e90,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source cold graphics-device binding");
    memory.mount(0x412728,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,"source12560 SDK allocation-size inputs");
}
NativeTu15RendererDeviceFrame native_tu15_renderer_device_frame(const SourceRuntimeMemory& memory,
    const SourcePhysicsContactRuntimeCall& call,const SourcePhysicsWorldRuntimeAbi& abi) {
    const auto sp=abi.gp[4];
    if(call.instruction!=0x129adu || call.target!=0x3ff320u || call.tail || word(memory,sp)!=0x129b2u ||
       abi.gp[0]!=0x40u || abi.gp[1]!=0x515e90u || word(memory,sp+4)!=sp+0x14u)
        throw std::invalid_argument("Source renderer device-creation caller frame differs");
    NativeTu15RendererDeviceFrame result;
    result.output_token=abi.gp[1];result.parameters_token=word(memory,sp+4);result.flags=abi.gp[0];
    result.output_backing=memory.native_address(result.output_token,4);
    result.parameters_backing=memory.native_address(result.parameters_token,68);
    for(unsigned i=0;i<17;++i)result.presentation[i]=word(memory,result.parameters_token+4*i);
    return result;
}
}
