#pragma once
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
struct NativeWeaponAudioViewTu15 {
    std::uint32_t unit=0xffffffffu,primary=0xffffffffu,secondary=0xffffffffu;
};
// Only an actual zoom caller may route by unit. Other unit audio retains its
// existing owners; FP animation sounds route by carried weapon. Compare full
// datums in both cases, including their generation.
inline std::optional<unsigned> native_weapon_audio_view_tu15(std::uint32_t owner,
    std::span<const NativeWeaponAudioViewTu15> views,bool unit_zoom=false) {
    if(owner==0xffffffffu)return {};
    for(unsigned view=0;view<views.size();++view)
        if((unit_zoom && owner==views[view].unit) || owner==views[view].primary || owner==views[view].secondary)return view;
    return {};
}
}
