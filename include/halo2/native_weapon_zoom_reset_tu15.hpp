#pragma once
#include <cstdint>
#include <functional>

namespace halo2 {
// Committed CABC0 reset at CAC96, before its producer-local sound gate.
// Full datums and the actual106040 result belong to this completed call.
// A recipient must use its own local-player binding; source_local_user only
// describes the producer. This is not an inferred numeric zoom transition.
struct NativeWeaponZoomResetTu15 {
    std::uint32_t unit=0xffffffffu,player=0xffffffffu,weapon=0xffffffffu;
    std::uint8_t previous_selector=0xff,request{};
    bool weapon_scoped{};
    std::int16_t source_local_user=-1;
    bool operator==(const NativeWeaponZoomResetTu15 &)const=default;
};
using NativeWeaponZoomResetObserverTu15=std::function<void(const NativeWeaponZoomResetTu15 &)>;
}
