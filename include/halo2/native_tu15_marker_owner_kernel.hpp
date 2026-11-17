#pragma once
#include <cstdint>

namespace halo2::native_marker_owner_detail {
// Inherited definitions select the parent again. An admitted pickup uses its
// authored parent-unit datum; all other records keep the current object.
// The view owns live caller storage, real child continuations and lazy reads.
template<class View>std::uint32_t lookup(View&view){
    for(;;){
        view.open_record();
        const auto parent=view.parent();
        if(parent!=0xffffffffu&&view.inherits_marker()){
            view.promote(parent);continue;
        }
        if(view.unit_candidate()&&view.supported_kind()&&view.pickup_parent())
            return view.finish(true);
        return view.finish(false);
    }
}
} // namespace halo2::native_marker_owner_detail
