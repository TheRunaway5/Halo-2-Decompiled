#pragma once
#include <cstdint>

namespace halo2::native_object_body_detail {
// A salted datum must identify a live slot before the caller's type mask is
// read. The view owns each lazy read and the original saved-register boundary.
template<class View>std::uint32_t lookup(View&view){
    const auto datum=view.begin();
    if(view.present(datum)){
        const auto objects=view.objects();
        const auto limit=view.limit(objects);
        if(view.in_bounds(datum,limit)){
            const auto record=view.record(objects);
            const auto salt=view.salt(record);
            if(view.live(salt)&&view.matches(datum,salt))view.admit(record);
        }
    }
    return view.finish();
}
} // namespace halo2::native_object_body_detail
