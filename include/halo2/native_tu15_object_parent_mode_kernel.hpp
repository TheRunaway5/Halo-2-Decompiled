#pragma once
#include <cstdint>

namespace halo2::native_object_parent_mode_detail {
inline bool pickup_parent(std::uint8_t flags){return(flags&1u)!=0u&&(flags&2u)==0u;}
// The view owns lazy object-table acquisition and the narrow ABI boundary.
// A sentinel datum still selects slot65535, exactly like the source owner.
template<class View>auto lookup(View&view,std::uint32_t datum){
    const auto objects=view.objects();
    const auto rows=view.rows(objects);
    const auto record=view.record(rows,datum&65535u);
    const auto flags=view.flags(record);
    return view.finish(flags,pickup_parent(flags));
}
} // namespace halo2::native_object_parent_mode_detail
