#include "halo2/mcc_bsp_vertex_layout.hpp"
#include "halo2/bsp_lighting.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
MccBspVertexLayout project_mcc_bsp_vertex_layout(unsigned count,
    std::span<const std::byte> descriptors,std::span<const std::byte> positions,
    std::span<const std::byte> uv,std::span<const std::byte> basis) {
    if(!count || count>65535 || descriptors.size()<96 || descriptors.size()>16*32 || descriptors.size()%32)
        throw FormatError("Unsupported MCC BSP source vertex/descriptor bank");
    constexpr std::array<unsigned,3> types{1,23,25},strides{12,8,36};
    for(unsigned d=0;d<3;++d) {
        if(std::to_integer<unsigned>(descriptors[d*32])!=types[d] ||
           std::to_integer<unsigned>(descriptors[d*32+1])!=strides[d])
            throw FormatError("Unsupported MCC BSP source vertex declaration");
        for(unsigned p=2;p<32;++p)if(descriptors[d*32+p]!=std::byte{})
            throw FormatError("MCC BSP vertex declaration has unowned relocation/offset fields");
    }
    if(positions.size()!=std::size_t(count)*12 || uv.size()!=std::size_t(count)*8 || basis.size()!=std::size_t(count)*36)
        throw FormatError("MCC BSP vertex streams differ from complete declared extents");
    for(const auto stream:{positions,uv}) {
        const Bytes bytes(stream);
        for(std::size_t p=0;p<stream.size();p+=4)if(!std::isfinite(bytes.f32(p)))
            throw FormatError("Nonfinite MCC BSP position/UV input");
    }
    MccBspVertexLayout out;
    std::copy_n(descriptors.begin(),96,out.descriptors.begin());
    out.descriptors[32]=std::byte{24};
    out.descriptors[64]=std::byte{27};out.descriptors[65]=std::byte{12};
    out.basis.resize(std::size_t(count)*12);const Bytes source(basis);
    for(unsigned v=0;v<count;++v)for(unsigned slot=0;slot<3;++slot) {
        const auto p=v*36+slot*12;
        // Genuine TU1.5 183AF0: preserve physical normal/tangent/binormal
        // order, clamp and pack with the already CPU-proven original rule.
        const auto word=pack_bsp_incident_tu15({source.f32(p),source.f32(p+4),source.f32(p+8)});
        for(unsigned j=0;j<4;++j)out.basis[v*12+slot*4+j]=std::byte(word>>(j*8));
    }
    return out;
}
}
