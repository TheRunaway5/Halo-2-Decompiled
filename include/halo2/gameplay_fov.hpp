#pragma once
#include "halo2/display_settings.hpp"
#include "halo2/source_camera.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
inline constexpr float gameplay_camera_angle_scale=.8949999809265137f;
// MCC 7E0F35/7E0F3D replaces the world camera's vertical angle in the
// first-person pass, independently of the FOV slider and scope magnification.
inline constexpr float gameplay_first_person_projection_angle=.8655884265899658f;
inline float gameplay_field_of_view(unsigned degrees) {
    return float(std::clamp(degrees,minimum_field_of_view,maximum_field_of_view))*.01745329238474369f;
}
// Keep the authoritative observer/vehicle packets in source units. Rebase its
// current lens onto the user's unzoomed FOV at presentation time; this preserves
// tangent-space magnification and the existing source zoom transition owner.
inline float gameplay_observer_field_of_view(float current,float authored,unsigned degrees) {
    const float selected=gameplay_field_of_view(degrees);
    if(current==authored)return selected;
    if(!(std::isfinite(current) && current>0 && current<3.141592654f &&
         std::isfinite(authored) && authored>0 && authored<3.141592654f))
        throw FormatError("invalid source gameplay field of view");
    return float(2*std::atan(std::tan(double(current)*.5)*
        std::tan(double(selected)*.5)/std::tan(double(authored)*.5)));
}
// MCC halo2.dll 6F4490 and 7DFF10 (aspect-aware branch): the slider is the
// observer's horizontal angle, converted using at most a 16:9 reference aspect.
// The physical viewport then reveals additional horizontal view on ultrawide.
// Preserve the source safe-frustum stage after this render-camera conversion.
inline SourceCameraAngles gameplay_camera_angles(float horizontal,float unzoomed,float aspect) {
    if(!(std::isfinite(horizontal) && horizontal>0 && horizontal<3.141592654f &&
         std::isfinite(unzoomed) && unzoomed>0 && std::isfinite(aspect) && aspect>0))
        throw FormatError("invalid gameplay field of view");
    const float reference_aspect=aspect<.20000000298023224f?1.7777777910232544f:
        std::min(aspect,1.7777777910232544f);
    const float tangent=float(std::tan(double(horizontal*.5f)));
    const float vertical=float(std::atan2(double(tangent/reference_aspect),1.0))*2.f;
    return {vertical*gameplay_camera_angle_scale,(horizontal/unzoomed)*gameplay_camera_angle_scale};
}
} // namespace halo2
