#include "halo2/pstencil/projected_light_material.hpp"

namespace halo2::pstencil {
unsigned projected_light_implementation(std::uint32_t flags,std::uint32_t mask,
    bool material_specular) {
    if(mask&1u)return 1;
    if(flags&0x20u)return 4;
    if((mask&2u) && material_specular)return 0;
    return mask&4u?2u:3u;
}
bool projected_light_specular_mask(std::uint32_t flags,std::uint32_t mask,
    std::uint32_t layers) {
    return !(flags&2u) && (mask&2u) && (layers&0x2000u);
}
std::uint32_t projected_light_feature_mask(Vec3 specular,float epsilon,
    bool shadow,float volume,std::uint16_t kind) {
    std::uint32_t mask=(specular.x>epsilon || specular.y>epsilon || specular.z>epsilon)?2u:0u;
    if(shadow) {
        mask|=0x24u;
        if(volume>0 && kind!=2)mask|=8u;
    }
    return mask;
}
}
