#include "halo2/source_water_reflection_tu15.hpp"
#include <algorithm>
#include <cstring>

namespace halo2 {
SourceWaterReflectionTu15 source_water_reflection_tu15(
    std::span<const std::byte,0x74> camera,const SourceViewProjectionBank &rendering,float water_height) {
    SourceWaterReflectionTu15 out;
    std::copy(camera.begin(),camera.end(),out.submitting_camera.begin());
    out.rendering=rendering;
    // Exact3BDA5..3BE11: binary32 subtraction, including zero signs.
    auto &first=out.rendering.inverse_view;
    first.basis[2].x=0.f-first.basis[2].x;
    first.basis[2].y=0.f-first.basis[2].y;
    first.basis[2].z=0.f-first.basis[2].z;
    first.position.z=water_height*2.f-first.position.z;
    out.rendering.camera_to_world=inverse_transform(first); // Whole144090.
    const std::array<std::int16_t,4> rectangle{0,0,256,512};
    std::memcpy(out.submitting_camera.data()+0x30,rectangle.data(),sizeof(rectangle));
    Vec3 original_eye;float original_far;
    std::memcpy(&original_eye,camera.data(),sizeof(original_eye));
    std::memcpy(&original_far,camera.data()+0x44,sizeof(original_far));
    //3BE1C: flag0 and actual target21. This does not inherit texture-camera
    // render scaling, and it does not manufacture a reflected raw eye.
    out.constants=source_view_shader_bank(out.rendering,original_far,original_eye,
        {0,0,512,256,1.f,false,false,21},false);
    return out;
}
}
