#include "halo2/source_weather_exclusion.hpp"
#include <stdexcept>

namespace halo2 {
bool source_weather_point_in_exclusion(const std::array<float,3> &point,
    const SourceWeatherExclusionVolume &volume) {
    const float dx=point[0]-volume.center[0];
    const float dz=point[2]-volume.center[2];
    const float dy=point[1]-volume.center[1];
    const float distance=(dx*dx+dz*dz)+dy*dy;
    const float radius_squared=volume.radius*volume.radius;
    if(!(radius_squared>distance))return false;
    if(volume.plane_count<=0)return true;
    if(std::uint32_t(volume.plane_count)>volume.planes.size())
        throw std::invalid_argument("source weather exclusion needs its complete owned plane bank");
    for(std::int32_t i=0;i<volume.plane_count;++i) {
        const auto &plane=volume.planes[std::size_t(i)];
        const float plane_distance=((plane[1]*point[1]+plane[2]*point[2])+
            plane[0]*point[0])-plane[3];
        if(plane_distance>.0001f)return false;
    }
    return true;
}
} // namespace halo2
