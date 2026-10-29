#include "halo2/path_tracing/emitter_sampling.hpp"
#include "halo2/path_tracing/material_emission.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <map>

namespace halo2::path_tracing {
void append_emitter_groups(Scene& scene) {
    if(!scene.emissive_count)return;
    struct Group {
        std::uint32_t triangles{};
        float area{};
        std::array<Vec2,4> sample_uv{};
        unsigned samples{};
        std::array<float,3> minimum{std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max()};
        std::array<float,3> maximum{std::numeric_limits<float>::lowest(),std::numeric_limits<float>::lowest(),std::numeric_limits<float>::lowest()};
    };
    std::map<std::uint32_t,Group> groups;
    for(std::uint32_t emitter=0;emitter<scene.emissive_count;++emitter) {
        const auto& triangle=scene.triangles.at(scene.pixels.at(scene.texture_texel_count+emitter));
        auto& group=groups[triangle.metadata[0]];++group.triangles;
        if(group.samples<group.sample_uv.size())group.sample_uv[group.samples++]={
            (triangle.uv01[0]+triangle.uv01[2]+triangle.uv2[0])/3.f,
            (triangle.uv01[1]+triangle.uv01[3]+triangle.uv2[1])/3.f};
        const auto& a=triangle.p0;const auto& b=triangle.p1;const auto& c=triangle.p2;
        const float x=(b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]);
        const float y=(b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]);
        const float z=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
        group.area+=.5f*std::sqrt(x*x+y*y+z*z);
        for(unsigned lane=0;lane<3;++lane)for(const auto* position:{&a,&b,&c}) {
            group.minimum[lane]=std::min(group.minimum[lane],(*position)[lane]);
            group.maximum[lane]=std::max(group.maximum[lane],(*position)[lane]);
        }
    }
    const auto header=scene.pixels.size();scene.pixels.resize(header+emitter_group_header_words);
    scene.pixels[header]=emitter_group_magic;float cumulative{};std::uint32_t count{};
    for(const auto& [material,group]:groups) {
        // This is a proposal estimate, independent of transport radiance.
        // A missed small mask remains reachable through the BSDF half of the
        // distribution. Black textures must not add needless sampling noise.
        Vec3 emission{};
        for(unsigned sample_index=0;sample_index<group.samples;++sample_index) {
            const auto sample=material_emission(scene,material,group.sample_uv[sample_index]);
            const float weight=1.f/float(group.samples);
            emission.x+=sample.x*weight;emission.y+=sample.y*weight;emission.z+=sample.z*weight;
        }
        const float power=group.area*(.2126f*emission.x+.7152f*emission.y+.0722f*emission.z);
        if(!(power>0) || !std::isfinite(power))continue;
        const Vec3 center{(group.minimum[0]+group.maximum[0])*.5f,(group.minimum[1]+group.maximum[1])*.5f,
                          (group.minimum[2]+group.maximum[2])*.5f};
        const Vec3 half{group.maximum[0]-center.x,group.maximum[1]-center.y,group.maximum[2]-center.z};
        const float radius=std::sqrt(half.x*half.x+half.y*half.y+half.z*half.z)*1.000001f;
        cumulative+=power;++count;
        const std::array<std::uint32_t,emitter_group_words> record{
            material,group.triangles,0u,std::bit_cast<std::uint32_t>(group.area),
            std::bit_cast<std::uint32_t>(center.x),std::bit_cast<std::uint32_t>(center.y),
            std::bit_cast<std::uint32_t>(center.z),std::bit_cast<std::uint32_t>(radius),
            std::bit_cast<std::uint32_t>(emission.x*group.area),std::bit_cast<std::uint32_t>(emission.y*group.area),
            std::bit_cast<std::uint32_t>(emission.z*group.area),std::bit_cast<std::uint32_t>(cumulative)};
        scene.pixels.insert(scene.pixels.end(),record.begin(),record.end());
    }
    scene.pixels[header+1]=count;scene.pixels[header+2]=std::bit_cast<std::uint32_t>(cumulative);
}
}
