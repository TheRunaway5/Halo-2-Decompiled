#pragma once
#include "halo2/math.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <optional>

namespace halo2 {
struct SourceObjectAudioViewTu15 {
    Vec3 position;
    std::int16_t cluster;
    struct Orientation {Vec3 forward,up;};
    std::optional<Orientation> orientation{};
    std::int32_t leaf=-1;
    // Also stand in for this view's original 127400 listener record at the
    // 18E970 background-sound owner, so it selects and requests the BSP
    // ambience loops. A caller that plays BSP ambience through its own lane
    // leaves this false; that owner then reads the (inactive) record.
    bool background_sound=false;
};
using SourceObjectAudioViewsTu15=std::array<std::optional<SourceObjectAudioViewTu15>,4>;

// Native camera inputs to the original18F900/190460 algorithms, and to
// 18E970 for views that opt in with background_sound. Presence is an actual
// local view, not sound-driver readiness. No legacy listener record, source
// availability byte, visibility result or callee result is written. An
// unbound call continues to read its original sound-system records.
inline std::optional<std::uint32_t> source_object_audio_view_operand_tu15(
    const SourceObjectAudioViewsTu15* views,std::uint32_t sound_state,
    std::uint32_t instruction,std::uint32_t address,unsigned size) {
    if(!views)return {};
    unsigned offset;
    bool background=false;
    switch(instruction) {
    case 0x18f947u:case 0x190475u:offset=0x8eu;if(size!=1)return {};break;
    case 0x18f94eu:case 0x18f960u:offset=0x8cu;if(size!=2)return {};break;
    case 0x19047cu:offset=0xb8u;if(size!=4)return {};break;
    case 0x190485u:offset=0xbcu;if(size!=4)return {};break;
    case 0x19048fu:offset=0xc0u;if(size!=4)return {};break;
    // 18E970's listener loads: active, cluster, then position x/y/z.
    case 0x18ea19u:offset=0x8eu;background=true;if(size!=1)return {};break;
    case 0x18ea24u:offset=0x8cu;background=true;if(size!=2)return {};break;
    case 0x18ea34u:offset=0xb8u;background=true;if(size!=4)return {};break;
    case 0x18ea39u:offset=0xbcu;background=true;if(size!=4)return {};break;
    case 0x18ea3cu:offset=0xc0u;background=true;if(size!=4)return {};break;
    default:return {};
    }
    const auto base=std::uint64_t(sound_state)+offset;
    if(address<base || (address-base)%0x48u)return {};
    const auto index=(address-base)/0x48u;
    if(index>=views->size())return {};
    const auto &view=(*views)[index];
    if(background && !(view && view->background_sound))return {};
    if(offset==0x8e)return view?1u:0u;
    if(offset==0x8c)return view?std::uint16_t(view->cluster):0xffffu;
    if(!view)return {}; // The original active test excludes these reads.
    return std::bit_cast<std::uint32_t>(offset==0xb8?view->position.x:
        offset==0xbc?view->position.y:view->position.z);
}
}
