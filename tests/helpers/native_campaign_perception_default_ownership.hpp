#pragma once
#include "halo2/native_campaign_ai_perception_group_storage_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <array>
#include <stdexcept>
namespace halo2::test {
inline void perception_default_ownership(SourceRuntimeMemory &memory) {
    const auto check=[](bool value,const char *message){if(!value)throw std::runtime_error(message);};
    const auto word=[](const SourceRuntimeMemory &source,std::uint32_t address) {
        std::array<std::byte,4> bytes{};source.read(address,bytes);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
        return value;
    };
    const auto cold=memory.checkpoint();
    const auto available=memory.find_unowned_range(0x70000000u,0x7f000000u,0x70u,16u);
    check(available.has_value(),"Perception reset ownership test has no free data span");
    const auto point=*available;
    memory.mount(point,std::vector<std::byte>(0x70,std::byte{0x5a}),
        SourceRuntimeMemory::Access::ReadWrite,"native perception reset ownership test point");
    native_campaign_ai_perception_point_reset_tu15(memory,point);
    for(unsigned axis=0;axis<3;++axis)
        check(word(memory,point+0x48u+axis*4u)==0,
            "Actual native perception reset lacks its original default-point owner");
    const std::array<std::uint32_t,1> changed_default{0x44ac28u};
    memory.write(0x474ce8u,std::as_bytes(std::span(changed_default)));
    SourceRuntimeMemory restored;restored.restore(memory.checkpoint());
    native_campaign_ai_perception_point_reset_tu15(restored,point);
    for(unsigned axis=0;axis<3;++axis)
        check(word(restored,point+0x48u+axis*4u)==word(restored,0x44ac28u+axis*4u),
            "Perception reset ignored the real writable/restored default-point pointer");
    memory.restore(cold);
}
}
