#include "halo2/pstencil/lightmap.hpp"
#include <cmath>

namespace halo2::pstencil {
Vec3 lightmap_light_origin(Vec3 incident) {
    const double x=incident.x,y=incident.y,z=incident.z;
    const auto magnitude=std::sqrt((x*x+y*y)+z*z);
    if(!std::isfinite(magnitude) || magnitude<.0001)
        throw FormatError("pStencil lightmap shadow requires a valid incident sample");
    const auto inverse=float(1.0/magnitude);
    return {(incident.x*inverse)*20000.f,(incident.y*inverse)*20000.f,(incident.z*inverse)*20000.f};
}
}
