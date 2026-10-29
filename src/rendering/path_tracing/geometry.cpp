#include "halo2/path_tracing/geometry.hpp"
#include "halo2/path_tracing/scene.hpp"
#include "rendering/models/model_skin_cpu_materialization.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <map>
#include <set>

namespace halo2::path_tracing {
namespace {
struct Record {
    std::uint32_t destination{},palette{},palette_count{},post_identity_placement{};
    std::array<std::uint32_t,4> bones{};
    std::array<float,4> weights{};
};
using BodyTriangleKey=std::array<std::uint32_t,10>;
BodyTriangleKey body_key(std::uint32_t material,Vec3 a,Vec3 b,Vec3 c) {
    return {material,std::bit_cast<std::uint32_t>(a.x),std::bit_cast<std::uint32_t>(a.y),std::bit_cast<std::uint32_t>(a.z),
        std::bit_cast<std::uint32_t>(b.x),std::bit_cast<std::uint32_t>(b.y),std::bit_cast<std::uint32_t>(b.z),
        std::bit_cast<std::uint32_t>(c.x),std::bit_cast<std::uint32_t>(c.y),std::bit_cast<std::uint32_t>(c.z)};
}
}
std::vector<SecondaryBodyCount> secondary_body_counts(const Scene& scene,std::span<const Vertex> native_bank,
    std::span<const SecondaryBodyOwner> owners,std::span<const Material> materials,
    std::span<const MaterialRange> replaced) {
    if(native_bank.size()%3 || scene.materials.size()!=materials.size())
        throw FormatError("Path Tracing body bank requires complete triangles and current scene materials");
    const auto valid_range=[&](const MaterialRange& r) {
        return r.count && r.first<materials.size() && r.count<=materials.size()-r.first;
    };
    const auto contains=[](const MaterialRange& r,std::uint32_t material) {
        return material>=r.first && material-r.first<r.count;
    };
    std::set<std::uint32_t> roots;
    std::vector<SecondaryBodyCount> result;result.reserve(owners.size());
    for(unsigned i=0;i<owners.size();++i) {
        const auto& body=owners[i];
        if(body.root==~0u || body.definition==~0u || body.render_model==~0u || body.lod>=6 ||
           !roots.insert(body.root).second || !valid_range(body.materials) ||
           !body.vertex_count || body.first_vertex%3 || body.vertex_count%3 ||
           body.first_vertex>native_bank.size() || body.vertex_count>native_bank.size()-body.first_vertex ||
           body.sections.empty())throw FormatError("Path Tracing body provenance lost its full model/owner/range");
        std::set<unsigned> sections;
        for(auto section:body.sections) {
            if(section>=512)throw FormatError("Path Tracing body provenance has invalid selected sections");
            sections.insert(section);
        }
        if(!std::any_of(replaced.begin(),replaced.end(),[&](const auto& range) {
            return valid_range(range) && body.materials.first>=range.first &&
                body.materials.first-range.first<range.count &&
                body.materials.count<=range.count-(body.materials.first-range.first);
        }))throw FormatError("Path Tracing body bank lacks its primary replacement owner");
        for(unsigned prior=0;prior<i;++prior) {
            const auto& other=owners[prior];
            if((body.first_vertex<other.first_vertex+std::uint64_t(other.vertex_count) &&
                other.first_vertex<body.first_vertex+std::uint64_t(body.vertex_count)) ||
               (body.materials.first<other.materials.first+std::uint64_t(other.materials.count) &&
                other.materials.first<body.materials.first+std::uint64_t(body.materials.count)))
                throw FormatError("Path Tracing body owners overlap their vertex/material banks");
        }
        std::map<BodyTriangleKey,std::uint32_t> remaining;
        const auto first=std::size_t(body.first_vertex),end=first+body.vertex_count;
        for(std::size_t vertex=0;vertex<native_bank.size();++vertex) {
            const auto& v=native_bank[vertex];
            if(vertex<first || vertex>=end) {
                if(contains(body.materials,v.material))
                    throw FormatError("Path Tracing body material bank is shared by an independent owner");
            } else if(!contains(body.materials,v.material) || !sections.contains(v.source_section))
                throw FormatError("Path Tracing body vertex lost its material/selected-section owner");
        }
        std::uint64_t eligible{};
        for(auto vertex=first;vertex<end;vertex+=3) {
            const auto& a=native_bank[vertex];const auto& b=native_bank[vertex+1];const auto& c=native_bank[vertex+2];
            if(a.material!=b.material || a.material!=c.material)
                throw FormatError("Path Tracing body triangle has mixed material owners");
            if(!traces_material(materials[a.material]) || (scene.materials[a.material].texture[3]&16u) ||
               a.source_stipple_level==0)continue;
            const Vec3 ab{b.position.x-a.position.x,b.position.y-a.position.y,b.position.z-a.position.z};
            const Vec3 ac{c.position.x-a.position.x,c.position.y-a.position.y,c.position.z-a.position.z};
            const Vec3 normal{ab.y*ac.z-ab.z*ac.y,ab.z*ac.x-ab.x*ac.z,ab.x*ac.y-ab.y*ac.x};
            const auto area=normal.x*normal.x+normal.y*normal.y+normal.z*normal.z;
            if(!std::isfinite(area))throw FormatError("Path Tracing body bank has nonfinite source geometry");
            if(area<1.e-18f)continue; // Same final Scene degeneracy policy.
            ++remaining[body_key(a.material,a.position,b.position,c.position)];++eligible;
        }
        SecondaryBodyCount count{body.root,body.definition,body.render_model,body.source_tick,body.lod,0,eligible};
        for(const auto& triangle:scene.triangles)if(contains(body.materials,triangle.metadata[0])) {
            const auto key=body_key(triangle.metadata[0],{triangle.p0[0],triangle.p0[1],triangle.p0[2]},
                {triangle.p1[0],triangle.p1[1],triangle.p1[2]},{triangle.p2[0],triangle.p2[1],triangle.p2[2]});
            const auto found=remaining.find(key);
            if(found!=remaining.end() && found->second) {--found->second;++count.triangles;}
        }
        result.push_back(count);
    }
    return result;
}
void append_secondary_geometry(std::vector<Vertex>& output,std::span<const Vertex> input,
    std::span<const Material> materials,std::span<const MaterialRange> replaced) {
    if(input.size()%3)throw FormatError("Path Tracing requires complete source triangles");
    for(const auto& range:replaced)if(!range.count || range.first>=materials.size() ||
        range.count>materials.size()-range.first)
        throw FormatError("Path Tracing replacement material range escapes its resident owner");
    const auto admitted=[&](std::uint32_t material) {
        if(std::any_of(replaced.begin(),replaced.end(),[&](const auto& range) {
            return material>=range.first && material-range.first<range.count;
        }))return false;
        return traces_material(materials[material]);
    };
    std::size_t count{};
    for(std::size_t first=0;first<input.size();first+=3) {
        const auto material=input[first].material;
        if(material>=materials.size() || input[first+1].material!=material || input[first+2].material!=material)
            throw FormatError("Path Tracing triangle lost its single resident material owner");
        if(admitted(material))count+=3;
    }
    if(output.size()>std::numeric_limits<std::uint32_t>::max() ||
        count>std::numeric_limits<std::uint32_t>::max()-output.size())
        throw FormatError("Path Tracing geometry exceeds its vertex index domain");
    output.reserve(output.size()+count);
    for(std::size_t first=0;first<input.size();first+=3)if(admitted(input[first].material))
        output.insert(output.end(),input.begin()+first,input.begin()+first+3);
}
std::vector<Vertex> posed_vertices(std::span<const Vertex> input,
    std::span<const ModelSkinSubmission> submissions) {
    if(input.size()>std::numeric_limits<std::uint32_t>::max())
        throw FormatError("Path Tracing geometry exceeds its vertex index domain");
    // Validate complete declared ranges before materializing any one owner.
    std::vector<std::pair<std::size_t,std::size_t>> ranges;
    ranges.reserve(submissions.size());
    std::size_t records_count{},palette_count{};
    for(const auto& submission:submissions) {
        if(!submission.geometry)throw FormatError("Path Tracing skin has no geometry owner");
        const auto& geometry=*submission.geometry;
        const auto count=geometry.vertices.size();
        if(submission.first>input.size() || count>input.size()-submission.first ||
           geometry.bindings.size()!=count || geometry.source_vertices.size()!=count)
            throw FormatError("Path Tracing skin lost its complete source range");
        if(geometry.palette.size()>std::numeric_limits<std::uint32_t>::max()-palette_count)
            throw FormatError("Path Tracing skin palette exceeds its index domain");
        if(count)ranges.emplace_back(submission.first,submission.first+count);
        records_count+=count;palette_count+=geometry.palette.size();
    }
    std::sort(ranges.begin(),ranges.end());
    for(std::size_t i=1;i<ranges.size();++i)if(ranges[i].first<ranges[i-1].second)
        throw FormatError("Path Tracing skin submissions overlap");
    std::vector<Vertex> output(input.begin(),input.end());
    std::vector<Record> records;records.reserve(records_count);
    std::vector<std::array<float,12>> palette;palette.reserve(palette_count);
    for(const auto& submission:submissions) {
        const auto& geometry=*submission.geometry;
        const auto first=std::uint32_t(palette.size());
        palette.insert(palette.end(),geometry.palette.begin(),geometry.palette.end());
        for(std::size_t i=0;i<geometry.vertices.size();++i) {
            const auto& binding=geometry.bindings[i];
            Record record;
            record.destination=std::uint32_t(submission.first+i);
            record.palette=first;record.palette_count=std::uint32_t(geometry.palette.size());
            record.post_identity_placement=geometry.post_identity_placement;
            std::copy(binding.bones.begin(),binding.bones.end(),record.bones.begin());
            record.weights=binding.weights;records.push_back(record);
        }
    }
    model_skin_cpu_materialize(std::span{output},std::span{records},
        std::span<const std::array<float,12>>{palette});
    return output;
}
}
