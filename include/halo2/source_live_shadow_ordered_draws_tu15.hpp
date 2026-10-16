#pragma once
#include "halo2/source_shadow.hpp"
#include "halo2/source_live_shadow_observer_tu15.hpp"

namespace halo2 {
class PhysicsLiveWorldTu15;
struct SourceLiveShadowOrderedDrawsTu15 {
    SourceShadowPhase phase{};
    std::uint32_t owner{},bank{},category{},layer{};
    std::uint32_t caller_stack{},source_address{};
    std::uint32_t capacity=2560;
    // Complete original24-byte descriptors, including retained unused bytes.
    std::vector<std::array<std::uint32_t,6>> records;
};
// Whole TU1.5 3BAA0 selection/sort on the existing source ABI stack. Accepted
// routes are Generate/bank4CE298/category2/layer10 and Apply/4CE280/1/11.
// The caller already owns the completed commands and phase inputs. This does
// not bind a shader, submit GPU work or replace the original sorting owner.
// The observer receives entry3BAA0 and the actual root owner. Copying records
// completes before a later source call can reuse the caller's stack storage.
SourceLiveShadowOrderedDrawsTu15 source_live_shadow_ordered_draws_tu15(
    PhysicsLiveWorldTu15&,SourceShadowPhase,std::uint32_t command_bank,
    std::uint32_t category,std::uint32_t layer,std::uint32_t actual_owner,
    const SourceShadowAdmissionObserverTu15& = {});
}
