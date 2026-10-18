#include "halo2/mcc_model_vertex_layout.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
MccModelVertexLayout project_mcc_rigid_model_vertex_layout(unsigned geometry_class,
    unsigned nodes_per_vertex,unsigned model_nodes,unsigned count,
    std::span<const std::byte> raw_model_header,std::span<const std::byte> descriptors,std::span<const std::byte> positions,
    std::span<const std::byte> uv,std::span<const std::byte> basis,
    std::span<const std::byte> node_map) {
    if(raw_model_header.size()!=120)
        throw FormatError("MCC rigid model requires its complete retained source header");
    if((geometry_class!=1 && geometry_class!=2) || nodes_per_vertex>1 ||
       (geometry_class==2 && nodes_per_vertex!=1) || model_nodes>256 || node_map.size()>256 ||
       (nodes_per_vertex && !model_nodes))
        throw FormatError("MCC model vertex projection requires verified rigid or one-influence rigid-boned geometry");
    if(descriptors.size()<96 || descriptors.size()>16*32 || descriptors.size()%32)
        throw FormatError("MCC rigid model descriptor bank is incomplete");
    const auto type=std::to_integer<unsigned>(descriptors[0]);
    const unsigned stride=geometry_class==1?12:16;
    if((geometry_class==1?type!=1:(type!=2 && type!=3)) ||
       std::to_integer<unsigned>(descriptors[1])!=stride || !count || count>65535 ||
       positions.size()!=std::size_t(count)*stride)
        throw FormatError("MCC rigid model position declaration/extent differs from its actual class");
    for(auto value:node_map)if(std::to_integer<unsigned>(value)>=model_nodes)
        throw FormatError("MCC rigid model node map escapes its actual model hierarchy");
    if(geometry_class==2)for(unsigned i=0;i<count;++i) {
        const auto index=std::to_integer<unsigned>(positions[i*stride+12]);
        if(index>=(node_map.empty()?model_nodes:node_map.size()))
            throw FormatError("MCC rigid-boned vertex has no actual node-map owner");
    }
    // Reuse the independently frozen BSP UV/basis projection. Its temporary
    // dense POSITION view only validates the same literal FLOAT3 components;
    // the actual interleaved source bank and unused index bytes stay intact.
    auto declarations=std::vector<std::byte>(descriptors.begin(),descriptors.end());
    declarations[0]=std::byte{1};declarations[1]=std::byte{12};
    std::vector<std::byte> dense;
    std::span<const std::byte> xyz=positions;
    if(geometry_class==2) {
        dense.resize(std::size_t(count)*12);
        for(unsigned i=0;i<count;++i)std::copy_n(positions.begin()+i*16,12,dense.begin()+i*12);
        xyz=dense;
    }
    auto converted=project_mcc_bsp_vertex_layout(count,declarations,xyz,uv,basis);
    if(geometry_class==2) {
        // Whole genuine TU1.5 1C310: type3 binds FLOAT3 at0 and the single
        // actual UBYTE influence at12; its declared stride remains16.
        converted.descriptors[0]=std::byte{3};converted.descriptors[1]=std::byte{16};
    }
    MccModelVertexLayout out{converted.descriptors,std::move(converted.basis),{}};
    out.node_map.reserve(node_map.size());
    for(auto value:node_map)out.node_map.push_back(std::to_integer<std::uint8_t>(value));
    return out;
}
}
