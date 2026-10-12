#include "halo2/source_draw_unit_cull.hpp"
#include "halo2/source_bsp_accelerator.hpp"
#include "halo2/source_bsp_ranges.hpp"
#include <cmath>
#include <functional>

namespace halo2 {
namespace {
void finite(Vec3 p) {
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))
        throw FormatError("source draw-unit sphere requires finite point");
}
void finite(const Transform& t) {
    if(!std::isfinite(t.scale))throw FormatError("source draw-unit transform scale is nonfinite");
    for(const auto& axis:t.basis)finite(axis);
    finite(t.position);
}
Vec3 inverse_point(const Transform& t,Vec3 p) {
    if(t.scale==0.f)return {};
    p={p.x-t.position.x,p.y-t.position.y,p.z-t.position.z};
    if(t.scale!=1.f){const float r=1.f/t.scale;p={r*p.x,r*p.y,r*p.z};}
    return {(t.basis[0].z*p.z+t.basis[0].y*p.y)+t.basis[0].x*p.x,
        (t.basis[1].z*p.z+t.basis[1].y*p.y)+t.basis[1].x*p.x,
        (t.basis[2].z*p.z+t.basis[2].y*p.y)+t.basis[2].x*p.x};
}
using MaskLookup=std::function<std::span<std::uint32_t>(std::uint32_t,std::size_t)>;
void linear(SourceCullDrawUnit& u,SourceCullLinearGeometry& g,
    const MaskLookup& lookup,Vec3 center,float radius) {
    if(g.accelerator_count>0) {
        if(!g.accelerator || g.accelerator->accelerator_count!=g.accelerator_count ||
            g.accelerator->subpart_count!=g.subpart_count)
            throw FormatError("source draw-unit sphere accelerator geometry is incomplete");
        std::vector<std::uint32_t> direct,ranges;
        const auto result=source_bsp_accelerator_sphere(g.accelerator->accelerator,center,radius,direct,ranges);
        if(!result.complete)throw FormatError("source draw-unit sphere accelerator: "+result.diagnostic);
        if(direct.empty()&&ranges.empty())return;
        const auto n=g.subpart_count>0?std::size_t(g.subpart_count):0;
        const auto mask=lookup(u.words[5],(n+31)/32);
        source_bsp_expand_accelerator_lists(*g.accelerator,false,direct,ranges,mask,u.words[0]);
        return;
    }
    const auto n=g.subpart_count>0?std::size_t(g.subpart_count):0;
    if(n>g.subpart_bounds.size())throw FormatError("source draw-unit part prefix is incomplete");
    if(!n)return;
    const auto selected=lookup(u.words[5],(n+31)/32);
    std::uint32_t last_node=0xffffffffu;
    const auto original_center=center;
    for(std::size_t i=0;i<n;++i) {
        const auto bit=1u<<(i&31);auto& mask=selected[i>>5];
        if(mask&bit)continue;
        const auto index=g.subpart_bounds[i];
        if(index!=0xffff) {
            if(index>=g.bounds.size())throw FormatError("source draw-unit part bound is unavailable");
            const auto& b=g.bounds[index];finite(b.center);
            if(!std::isfinite(b.radius))throw FormatError("source draw-unit bound radius is nonfinite");
            if(u.words[1]!=0xffffffffu) {
                if(!g.owner_pose)throw FormatError("source draw-unit context owner bank is unavailable");
                const auto& pose=*g.owner_pose;
                // The source's MOVZX byte then compare-FFFFFFFF never treatsFF
                // as a sentinel. It requires literal node255 when consumed.
                if(pose.owner!=0xffffffffu && b.node!=last_node && !pose.nodes.empty()) {
                    if(b.node>=pose.nodes.size() || b.node>=pose.inverse_bind.size())
                        throw FormatError("source draw-unit posed/inverse-bind node bank is incomplete");
                    finite(pose.nodes[b.node]);finite(pose.inverse_bind[b.node]);
                    const auto matrix=compose_transform(pose.nodes[b.node],pose.inverse_bind[b.node]);
                    center=inverse_point(matrix,original_center);last_node=b.node;
                }
            }
            const float z=center.z-b.center.z,y=center.y-b.center.y,x=center.x-b.center.x;
            const float r=b.radius+radius;
            // COMISS/JB also rejects unordered derived values. Finite input
            // transforms can overflow; do not turn NaN into admission.
            if(!(r*r>=(z*z+y*y)+x*x))continue;
        }
        u.words[0]|=1;mask|=bit;
    }
}
std::vector<SourceDrawUnitSphereCall> cull(
    std::span<SourceCullDrawUnit> units,std::span<SourceCullDrawBanks> banks,
    const MaskLookup& lookup,const SourceDrawUnitCullContext& c) {
    //44720 truncates endpoints to unsigned16 and signs the actual unit index.
    const unsigned first=c.mode?c.main_prefix&65535u:0u;
    const unsigned end=(c.mode?c.draw_count:c.main_prefix)&65535u;
    std::vector<SourceDrawUnitSphereCall> calls;
    for(unsigned i=first;i<end;++i) {
        if(i>=32768 || i>=units.size())throw FormatError("source draw-unit prefix is outside actual storage");
        auto& unit=units[i];
        if((unit.words[0]&1) || !(unit.words[0]&0x40))continue;
        if(c.query_mode!=1)throw FormatError("source draw-unit frustum44E90 service is not supplied");
        if(i>=banks.size())throw FormatError("source draw-unit geometry bank is unavailable");
        finite(c.center);if(!std::isfinite(c.radius))throw FormatError("source draw-unit query radius is nonfinite");
        auto& b=banks[i];Vec3 center=c.center;
        if(!b.geometry_resolved)throw FormatError("source draw-unit geometry lookup is unresolved");
        if(unit.words[7]) {
            if(!b.transform)throw FormatError("source draw-unit transform bank is unavailable");
            finite(*b.transform);
            center=inverse_point(*b.transform,center);
        }
        calls.push_back({std::uint16_t(i),center,c.radius});
        if(b.geometry)linear(unit,*b.geometry,lookup,center,c.radius);
    }
    return calls;
}
}
std::vector<SourceDrawUnitSphereCall> source_draw_unit_sphere_cull(
    std::span<SourceCullDrawUnit> units,std::span<SourceCullDrawBanks> banks,
    std::span<SourceCullPartMask> masks,const SourceDrawUnitCullContext& c) {
    return cull(units,banks,[&](std::uint32_t address,std::size_t count) {
        SourceCullPartMask* selected=nullptr;
        for(auto& mask:masks)if(mask.source_address==address) {
            if(selected)throw FormatError("source draw-unit mask backing identity is duplicated");
            selected=&mask;
        }
        if(!selected || selected->words.size()<count)
            throw FormatError("source draw-unit part mask is incomplete");
        return std::span<std::uint32_t>(selected->words);
    },c);
}
std::vector<SourceDrawUnitSphereCall> source_draw_unit_sphere_cull(
    std::span<SourceCullDrawUnit> units,std::span<SourceCullDrawBanks> banks,
    SourceBspRangeArena& arena,std::uint32_t address,const SourceDrawUnitCullContext& c) {
    return cull(units,banks,[&](std::uint32_t pointer,std::size_t count) {
        if(!address || pointer<address || ((pointer-address)&3))
            throw FormatError("source draw-unit mask arena address is invalid");
        const std::size_t offset=(pointer-address)/4;
        if(offset>arena.words.size() || count>arena.words.size()-offset)
            throw FormatError("source draw-unit mask arena extent is incomplete");
        return std::span<std::uint32_t>(arena.words).subspan(offset,count);
    },c);
}
}
