#include "halo2/source_object_light_attribution.hpp"
#include "halo2/bsp_surface_lighting.hpp"
#include <bit>

namespace halo2 {
SourceObjectLightPosition source_object_light_render_position(
    const SourceObjectLightPositionStream& stream,std::span<const float,6> bounds,
    std::uint32_t vertex) {
    if(vertex>=stream.vertex_count)throw FormatError("object light CPU vertex escapes resident stream");
    std::size_t stride{};
    switch(stream.format) {
    case 1:stride=12;break;
    case 2:stride=6;break;
    case 3:stride=16;break;
    case 4:stride=8;break;
    default:throw FormatError("object light CPU position format requires original default-vector bank");
    }
    const Bytes bytes(stream.bytes);const auto at=std::size_t(vertex)*stride;
    bytes.require(at,stride);SourceObjectLightPosition out;
    if(stream.format==1 || stream.format==3)
        out.point={bytes.f32(at),bytes.f32(at+4),bytes.f32(at+8)};
    else {
        const auto axis=[&](unsigned i) {
            const float value=float(std::bit_cast<std::int16_t>(bytes.u16(at+i*2)));
            const float scaled=value*2.f;
            const float midpoint=scaled+1.f;
            const float normalized=midpoint*std::bit_cast<float>(0x37000080u)+.5f;
            const float extent=bounds[i*2+1]-bounds[i*2];
            const float expanded=normalized*extent;
            return expanded+bounds[i*2];
        };
        out.point={axis(0),axis(1),axis(2)};
    }
    if(stream.format==3 || stream.format==4)
        out.node_hint=std::to_integer<std::uint8_t>(bytes.slice(at+(stream.format==3?12:6),1)[0]);
    return out;
}
Vec3 source_object_light_inverse_point(const Transform& transform,Vec3 point) {
    if(transform.scale==0.f)return {};
    Vec3 delta{point.x-transform.position.x,point.y-transform.position.y,
        point.z-transform.position.z};
    if(transform.scale!=1.f) {
        const float inverse=1.f/transform.scale;
        delta={inverse*delta.x,inverse*delta.y,inverse*delta.z};
    }
    const auto axis=[&](Vec3 basis) {
        return (basis.z*delta.z+basis.y*delta.y)+basis.x*delta.x;
    };
    return {axis(transform.basis[0]),axis(transform.basis[1]),axis(transform.basis[2])};
}
std::optional<SourceObjectLightAttribution> source_object_light_attribution(
    const SourceObjectLightAttributionModel& model,const SourceObjectLightAttributionInput& input) {
    const auto& hit=input.contact;
    if(hit.region<0 || std::size_t(hit.region)>=input.active_permutations.size() ||
       std::size_t(hit.region)>=model.highest_sections.size())
        throw FormatError("object light contact region escapes actual active bank");
    const auto permutation=input.active_permutations[hit.region];
    if(permutation==255)return {};
    if(permutation>=128 || permutation>=model.highest_sections[hit.region].size())
        throw FormatError("object light active permutation escapes source render region");
    const auto selected=model.highest_sections[hit.region][permutation];
    if(selected==255)return {};
    if(selected<0 || std::size_t(selected)>=model.sections.size())
        throw FormatError("object light highest section escapes resident bank");
    const auto& section=model.sections[selected];
    if(!section.resident)return {};
    Vec3 point=input.world_point;
    if(!input.world_nodes.empty()) {
        if(hit.node<0 || std::size_t(hit.node)>=input.world_nodes.size() ||
           std::size_t(hit.node)>=model.inverse_binds.size())
            throw FormatError("object light collision node escapes current transform banks");
        point=source_object_light_inverse_point(input.world_nodes[hit.node],point);
        point=source_object_light_inverse_point(model.inverse_binds[hit.node],point);
    }
    if(!input.collision)return {};
    const auto region=hit.packed_bsp>>24,permutation_index=(hit.packed_bsp>>16)&255u,
        bsp_index=(hit.packed_bsp>>8)&255u;
    const auto& collision=*input.collision;
    if(region>=collision.regions.size() || permutation_index>=collision.regions[region].size() ||
       bsp_index>=collision.regions[region][permutation_index].size())
        throw FormatError("object light packed BSP escapes resolved collision bank");
    const auto& bsp=collision.regions[region][permutation_index][bsp_index];
    std::vector<std::uint16_t> planes;planes.reserve(bsp.nodes.size());
    for(const auto& node:bsp.nodes)planes.push_back(std::bit_cast<std::uint16_t>(node.plane));
    float weight_b{},weight_c{};std::uint32_t node_hint{};
    const auto reference=source_object_light_render_reference(section.leaves,hit.node,
        std::uint32_t(bsp.leaves.size()),std::uint32_t(hit.leaf),
        std::uint32_t(hit.signed_plane),planes,[&](const ObjectLightRenderReference& ref) {
            if(std::size_t(ref.strip)+2>=section.indices.size())
                throw FormatError("object light reference strip escapes resident indices");
            std::array<Vec3,3> positions;
            for(unsigned i=0;i<3;++i) {
                const auto decoded=source_object_light_render_position(section.positions,
                    model.position_bounds,section.indices[std::size_t(ref.strip)+i]);
                positions[i]=decoded.point;node_hint=decoded.node_hint;
            }
            return original_render_barycentrics(positions[0],positions[1],positions[2],
                point,weight_b,weight_c);
        });
    if(!reference)return {};
    //14AC00 reads output+24 before14AC1E writes the accepted strip: it is
    //still−1, so no unsigned part range matches and the source keeps part0.
    return SourceObjectLightAttribution{input.owner,model.render_model,std::uint32_t(selected),
        hit.region,reference->strip,reference->lightmap_triangle,node_hint,0,weight_b,weight_c};
}
} // namespace halo2
