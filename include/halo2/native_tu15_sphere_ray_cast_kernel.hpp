#pragma once

namespace halo2::native_sphere_ray_cast_detail {
// Native sphere/ray policy. The view supplies lazy shape/ray DATA and owns
// staged result publication, numerical temporaries and caller ABI transport.
// No child response, instruction interpreter or precomputed hit is supplied.
template<class View>bool cast(View&view){
    view.begin();view.initial_geometry();
    if(!view.approaching()){view.finish(false,1);return false;}
    view.direction_length();
    if(view.needs_recentering()){
        if(!view.recenter_possible()){view.finish(false,0);return false;}
        view.recenter();
    }else view.original_origin();
    view.discriminant();
    if(!view.intersects()){view.finish(false,2);return false;}
    if(!view.root_before_endpoint()){view.finish(false,0);return false;}
    if(!view.root_nonnegative()){view.finish(false,0);return false;}
    view.publish_hit();view.finish(true,0);return true;
}
} // namespace halo2::native_sphere_ray_cast_detail
