#include "halo2/source_sound_wet_returns_tu15.hpp"
#include <stdexcept>
namespace halo2 {
SourceSoundWetReturnRouteTu15 source_sound_wet_return_route_tu15(unsigned index){
    if(index>=8)throw std::out_of_range("Original wet return voice index");
    return {15u+index,6u+index%4u,0x2a70u+index*4u,0x100000u,0};
}
void source_sound_driver_wet_returns_tu15(SoundDriverStartupState& state,const SoundDriverStartupServices& services){
    const auto read=[&](std::uint32_t address){
        if(!services.memory)throw std::runtime_error("Original wet return memory owner unavailable");
        const auto raw=services.memory(address,4);
        if(raw.size()!=4)throw std::runtime_error("Original wet return memory extent unavailable");
        std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return value;
    };
    for(unsigned i=0;i<8;++i){
        const auto route=source_sound_wet_return_route_tu15(i);
        const auto slot=state.driver_address+route.driver_slot_offset;
        if(!services.native_buffer_create)throw std::runtime_error("Native source wet return constructor unavailable");
        const auto result=services.native_buffer_create({read(state.driver_address+0x2ab0u),slot,0,
            {0x18u,route.flags,0,0,0,route.input_bin}});
        if(result<0)continue;
        if(!services.native_buffer_operation)throw std::runtime_error("Native source wet return operation unavailable");
        services.native_buffer_operation({SoundDriverBufferOperation::Volume,read(slot),{route.volume,0,0},{}});
        const std::array<std::uint32_t,2> bins{route.output_bin,0};
        services.native_buffer_operation({SoundDriverBufferOperation::MixBins,read(slot),{1,0,0},bins});
        services.native_buffer_operation({SoundDriverBufferOperation::Play,read(slot),{0,0,0},{}});
    }
}
}
