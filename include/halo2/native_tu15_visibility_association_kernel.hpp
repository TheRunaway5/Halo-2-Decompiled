#pragma once
#include <cstdint>

namespace halo2::native_visibility_association_detail {
struct BitLocation {std::int32_t word{};std::uint32_t mask{};};
inline BitLocation locate(std::int32_t ordinal){return{ordinal>>5u,1u<<(std::uint32_t(ordinal)&31u)};}
// Reserve one visibility association and initialize its four-word mask before
// marking the caller's cluster-entry ordinal. Raw storage owns the count's
// intermediate publication and lazy ordinal read; host storage owns its vector.
template<class View>auto reserve(View&view){
    const auto index=view.reserve_index();
    if(!view.valid_index(index)){
        view.saturate();
        return view.unavailable();
    }
    view.clear_mask(index);
    const auto ordinal=view.ordinal();
    view.mark(index,ordinal,locate(std::int32_t(ordinal)));
    return view.completed(index);
}
} // namespace halo2::native_visibility_association_detail
