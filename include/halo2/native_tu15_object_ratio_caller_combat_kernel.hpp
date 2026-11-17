#pragma once

namespace halo2::native_object_ratio_caller_detail {
template<class View> bool run_combat_ratio(View& view,bool middle_key) {
    if(!view.allow_ratio(middle_key)||!view.has_definition_flag(middle_key))return false;
    view.call_mode_ratio(middle_key);
    if(middle_key){view.copy_ratio();return true;}
    view.pick_low_ratio();
    return false;
}
} // namespace halo2::native_object_ratio_caller_detail
