#pragma once

namespace halo2 {
// The two views keep their respective scratch and observer transports.
// Bounds are acquired in the owning comparison/store phase, not cached.
template<class View>auto native_tu15_solver_hull_clip_kernel(View&view){
    view.project_and_compare_lower();
    if(view.use_upper()){
        if(view.update_upper())view.publish_upper();
    }else{
        if(view.update_lower())view.publish_lower();
        else view.discard_lower();
        view.mark_lower();
    }
    return view.finish();
}
} // namespace halo2
