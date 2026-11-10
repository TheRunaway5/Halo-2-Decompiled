#pragma once
#include "halo2/native_tu15_fog_membership_kernel.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <cstddef>

namespace halo2::native_fog_membership_detail {
struct AtmosphereView {
    const BspAtmosphere&atmosphere;std::uint32_t datum;
    bool no_designator(std::int32_t value)const{return value==-1;}
    bool any_fog()const{return datum==0xffffffffu;}
    std::int32_t palette(std::int32_t designator)const{
        if(designator>=0)return designator;
        const auto plane=std::uint32_t(designator)&127u;
        if(plane>=atmosphere.fog_planes.size())throw FormatError("containing fog plane outside BSP");
        return atmosphere.fog_planes[plane].palette;
    }
    bool no_palette(std::int32_t palette)const{return palette<0||std::size_t(palette)>=atmosphere.fog_palette.size();}
    bool matches(std::int32_t palette)const{return atmosphere.fog_palette[palette].datum==datum;}
    bool finish(bool contains)const{return contains;}
};
inline bool planar_member(const BspAtmosphere&atmosphere,int cluster,std::uint32_t datum){
    AtmosphereView view{atmosphere,datum};
    return designator_member(view,atmosphere.clusters[cluster].planar_designator);
}
} // namespace halo2::native_fog_membership_detail
