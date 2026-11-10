#pragma once
#include <cstdint>

namespace halo2::native_fog_membership_detail {
// A cluster's signed authored designator either names its fog palette directly
// or selects a fog plane that supplies the palette. Validation and lazy data
// acquisition belong to the view; the source datum remains an actual input.
template<class View>bool designator_member(View&view,std::int32_t designator){
    if(view.no_designator(designator))return view.finish(false);
    if(view.any_fog())return view.finish(true);
    const auto palette=view.palette(designator);
    if(view.no_palette(palette))return view.finish(false);
    return view.finish(view.matches(palette));
}
template<class View>bool contains(View&view){
    if(!view.available())return view.finish(false);
    if(view.override_membership())return view.finish(true);
    return designator_member(view,view.designator());
}
} // namespace halo2::native_fog_membership_detail
