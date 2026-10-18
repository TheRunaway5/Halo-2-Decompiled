#include "halo2/mcc_skinned_model_vertex_layout.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
MccSkinnedModelVertexLayout project_mcc_skinned_model_vertex_layout(
    unsigned opaque,unsigned transparent,unsigned model_nodes,unsigned count,
    std::span<const std::byte> header,std::span<const std::byte> descriptors,
    std::span<const std::byte> positions,std::span<const std::byte> uv,
    std::span<const std::byte> basis,std::span<const std::byte> node_map,
    std::span<const std::byte> parts,std::span<const std::byte> indices) {
    const auto maximum=std::max(opaque,transparent);
    if(header.size()!=120 || !count || count>65535 || !maximum || maximum>4 ||
       !model_nodes || model_nodes>256 || node_map.size()>256 ||
       descriptors.size()<96 || descriptors.size()>16*32 || descriptors.size()%32)
        throw FormatError("MCC skinned model lacks a complete typed source owner");
    const auto type=std::to_integer<unsigned>(descriptors[0]);
    const unsigned source_stride=maximum==1?16:20;
    if(type!=maximum*2 && type!=maximum*2+1)
        throw FormatError("MCC skinned model format differs from maximum stored influences");
    if(std::to_integer<unsigned>(descriptors[1])!=source_stride ||
       positions.size()!=std::size_t(count)*source_stride)
        throw FormatError("MCC skinned model position declaration/extent is unsupported");
    const Bytes h(header),p(parts),ib(indices);
    if(parts.empty() || parts.size()%72 || h.u32(0)!=parts.size()/72 ||
       h.u32(0x20)>65535 || indices.size()!=std::size_t(h.u32(0x20))*2)
        throw FormatError("MCC skinned model part/index ranges differ from source header");
    for(auto value:node_map)if(std::to_integer<unsigned>(value)>=model_nodes)
        throw FormatError("MCC skinned node map escapes its actual model hierarchy");
    const auto node_domain=node_map.empty()?model_nodes:node_map.size();
    for(std::size_t part=0;part<parts.size();part+=72) {
        // Authenticated TU1.5 1CA80/1CB00 (older image1CAA0/1CB20)
        // selects +15 only for part types4/5, otherwise +14.
        const auto kind=p.u16(part);
        const auto active=kind==4 || kind==5?transparent:opaque;
        const auto first=p.u16(part+6),length=p.u16(part+8);
        if(!active || active>maximum || first>h.u32(0x20) || length>h.u32(0x20)-first)
            throw FormatError("MCC skinned part has no supported selected influence interval");
        for(unsigned i=0;i<length;++i) {
            const auto vertex=ib.u16(std::size_t(first+i)*2);
            if(vertex>=count)throw FormatError("MCC skinned part index escapes actual vertices");
            for(unsigned lane=0;lane<active;++lane)
                if(std::to_integer<unsigned>(positions[std::size_t(vertex)*source_stride+12+lane])>=node_domain)
                    throw FormatError("MCC skinned active index lacks its actual node-map owner");
        }
    }
    // Validate literal FLOAT3/UV/basis through the existing proven projection.
    // Dense XYZ is private validation input; no source position is quantized.
    std::vector<std::byte> dense(std::size_t(count)*12);
    for(unsigned i=0;i<count;++i)
        std::copy_n(positions.begin()+i*source_stride,12,dense.begin()+i*12);
    auto declarations=std::vector<std::byte>(descriptors.begin(),descriptors.end());
    declarations[0]=std::byte{1};declarations[1]=std::byte{12};
    auto converted=project_mcc_bsp_vertex_layout(count,declarations,dense,uv,basis);
    // Whole genuine1C310: FLOAT types3/5/7/9 fetch respectively1/2/3/4
    // UB_OGL indices, and types5/7/9 fetch the same stored weight width.
    // Missing fetch lanes belong to the original declaration, not new bytes.
    const unsigned output_stride=maximum<=2?16:20;
    converted.descriptors[0]=std::byte(maximum*2+1);
    converted.descriptors[1]=std::byte(output_stride);
    MccSkinnedModelVertexLayout out{converted.descriptors,{},std::move(converted.basis),{}};
    if(maximum==2) {
        out.positions.resize(std::size_t(count)*16);
        for(unsigned i=0;i<count;++i) {
            const auto source=positions.begin()+i*20;
            const auto target=out.positions.begin()+i*16;
            std::copy_n(source,14,target);
            std::copy_n(source+16,2,target+14);
        }
    } else out.positions.assign(positions.begin(),positions.end());
    out.node_map.reserve(node_map.size());
    for(auto value:node_map)out.node_map.push_back(std::to_integer<std::uint8_t>(value));
    return out;
}
}
