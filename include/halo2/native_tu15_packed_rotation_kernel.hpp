#pragma once

namespace halo2::native_packed_rotation_detail {
template<class Vector>struct Norm{Vector opposite,alternate,value;};
template<class View,class Vector>Norm<Vector> squared_norm(View&view,Vector quaternion){
    const auto squared=view.multiply(quaternion,quaternion);
    const auto opposite=view.opposite(squared);
    const auto pair=view.add(squared,opposite);
    const auto alternate=view.alternate(pair);
    return {opposite,alternate,view.add(pair,alternate)};
}
// Four signed codec words share one normalization policy. The view owns the
// source/public seed convention, retained storage and escaped SIMD values.
template<class View>void decode(View&view){
    const auto quaternion=view.input();
    const auto norm=squared_norm(view,quaternion);
    const auto inverse=view.inverse(norm.value,quaternion,norm.opposite,norm.alternate);
    view.output(view.scale(quaternion,inverse),inverse);
}
} // namespace halo2::native_packed_rotation_detail
