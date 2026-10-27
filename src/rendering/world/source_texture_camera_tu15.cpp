#include "halo2/source_texture_camera_tu15.hpp"
#include "halo2/physics_contact_record.hpp"
#include "halo2/cache.hpp"
#include <span>

namespace halo2 {
namespace {
template<class T>T read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    T value;memory.read(address,std::as_writable_bytes(std::span(&value,1)));return value;
}
bool available(const SourceTextureCameraTargetTu15 &target) {
    return target.allocation && !target.retiring;
}
}
SourceTextureCameraInputsTu15 source_texture_camera_inputs_tu15(const SourcePhysicsContactMemory &memory) {
    SourceTextureCameraInputsTu15 result;
    result.camera_words=read<std::array<std::uint32_t,12>>(memory,0x4c6190u);
    result.cinematic_auxiliary=read<std::uint32_t>(memory,0x51df50u);
    if(result.cinematic_auxiliary) {
        if(result.cinematic_auxiliary>0xfffffffau)throw FormatError("Texture camera cinematic owner escapes source address space");
        result.cinematic=read<std::uint8_t>(memory,result.cinematic_auxiliary+5u);
    }
    result.texture_camera_view=read<std::uint8_t>(memory,0x491e27u);
    result.copy_complete=read<std::uint8_t>(memory,0x515edcu);
    constexpr std::uint32_t targets=0x4c1378u,stride=0x98u;
    for(const auto [index,target]:std::array{std::pair{18u,&result.target18},std::pair{20u,&result.target20}}) {
        target->allocation=read<std::uint32_t>(memory,targets+index*stride+0x88u);
        target->retiring=read<std::uint8_t>(memory,targets+index*stride+0x95u);
    }
    return result;
}
SourceTextureCameraLifecycleTu15 source_texture_camera_lifecycle_tu15(const SourceTextureCameraInputsTu15 &input) {
    const bool needed=(input.cinematic_auxiliary && input.cinematic) ||
        (input.camera_words[0]!=0xffffffffu && input.camera_words[0]!=0);
    SourceTextureCameraLifecycleTu15 result{needed};
    if(needed && !input.target18.allocation) {
        result.action=SourceTextureCameraTargetActionTu15::allocate18;
        result.width=640;result.height=480;
    } else if(!needed && available(input.target18)) {
        result.action=SourceTextureCameraTargetActionTu15::retire18;
    }
    return result;
}
SourceTextureCameraBindingTu15 source_texture_camera_binding_tu15(const SourceTextureCameraInputsTu15 &input) {
    if(source_texture_camera_lifecycle_tu15(input).action!=SourceTextureCameraTargetActionTu15::none)
        throw FormatError("Texture camera target lifecycle requires its actual allocation or retirement owner");
    auto texture=SourceTextureCameraTextureTu15::default2d;
    if(!input.texture_camera_view) {
        if(available(input.target20))texture=SourceTextureCameraTextureTu15::target20;
        else if(available(input.target18))texture=SourceTextureCameraTextureTu15::target18;
    }
    return SourceTextureCameraBindingTu15(texture);
}
}
