#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cstring>

namespace halo2 {
namespace {
struct Reader {
    SourceRuntimeMemory& memory;
    template<class T>T get(std::uint32_t at)const {
        T value;const auto bytes=memory.view(at,sizeof(T));
        std::memcpy(&value,bytes.data(),sizeof(T));return value;
    }
    std::uint32_t word(std::uint32_t p)const{return get<std::uint32_t>(p);}
    std::uint32_t model(const SourceCullDrawUnit& unit)const {
        const auto id=unit.words[1];
        if(id==0xffffffffu)throw FormatError("live model geometry requires a model draw unit");
        const auto row=word(0x4f0414u)+(id&65535u)*16u;
        if(word(row)!=0x6d6f6465u || word(row+4u)!=id)
            throw FormatError("live model geometry lost its full render-model identity");
        return word(row+8u);
    }
    std::uint32_t section(std::uint32_t model,const SourceCullDrawUnit& unit)const {
        const auto index=(unit.words[3]>>9)&511u;
        if(index>=word(model+0x24u))throw FormatError("live model section escapes its actual owner");
        return word(model+0x28u)+index*92u;
    }
};
}
SourceModelCommandGeometry source_live_model_command_geometry_tu15(
    PhysicsLiveWorldTu15& world,const SourceCullDrawUnit& unit) {
    const Reader r{world.memory()};SourceModelCommandGeometry out;
    const auto model=r.model(unit),section=r.section(model,unit),resident=r.word(section+0x34u);
    out.geometry.resident=resident!=0;
    if(!resident)return out; // Original449D0's unresident branch consumes no palette.
    const auto parts=r.word(resident);
    if(parts>256u)throw FormatError("live model part bank exceeds source capacity");
    out.geometry.parts.resize(parts);
    if(parts)std::memcpy(out.geometry.parts.data(),r.memory.view(r.word(resident+4u),parts*72u).data(),parts*72u);
    const auto count=r.word(model+0x60u),materials=r.word(model+0x64u);
    if(count>65535u)throw FormatError("live model material bank exceeds source capacity");
    for(unsigned n=0;n<count;++n)out.materials.push_back({r.word(materials+n*32u+12u),255});
    out.model_flags=r.get<std::uint16_t>(model+4u);out.rigid_node=r.get<std::int16_t>(section+0x2cu);
    // Whole1D690 resolves only a nonnegative token into the current49433C arena.
    const auto token=unit.words[2];
    if(std::bit_cast<std::int32_t>(token)<0)throw FormatError("live model current palette is absent");
    const auto offset=token&0x0fffffffu,cursor=r.word(0x494338u);
    if(offset>cursor || cursor-offset<0x44u)throw FormatError("live model palette is outside the current geometry arena");
    const auto palette=0x49433cu+offset;
    out.palette.base_count=r.get<std::uint16_t>(palette);
    const auto regions=r.get<std::uint16_t>(palette+2u);
    if(regions>16u)throw FormatError("live model palette region bank exceeds source capacity");
    unsigned matrices=out.palette.base_count;
    for(unsigned n=0;n<regions;++n) {
        const auto range=r.get<SourceModelPaletteRange>(palette+4u+n*4u);
        out.palette.regions.push_back(range);matrices=std::max(matrices,unsigned(range.first)+range.count);
    }
    if(matrices>256u || matrices*48u>cursor-offset-0x44u)
        throw FormatError("live model palette matrices escape the current arena");
    out.palette.matrices.resize(matrices);
    if(matrices)std::memcpy(out.palette.matrices.data(),r.memory.view(palette+0x44u,matrices*48u).data(),matrices*48u);
    return out;
}
SourceCullDrawBanks source_live_model_cull_geometry_tu15(
    PhysicsLiveWorldTu15& world,const SourceCullDrawUnit& unit,const SourceBspRangeGeometry& ranges) {
    const Reader r{world.memory()};SourceCullDrawBanks out;out.geometry_resolved=true;
    const auto model=r.model(unit),section=r.section(model,unit);
    if(!r.word(section+0x34u))return out;
    out.geometry=source_bsp_sphere_cull_geometry(ranges);
    if(unit.words[7])out.transform=r.get<Transform>(unit.words[7]);
    if(ranges.accelerator_count>0 || ranges.bounds.empty())return out;
    SourceCullOwnerPose pose;pose.owner=r.word(unit.words[4]);
    if(pose.owner!=0xffffffffu) {
        (void)world.object_body(pose.owner);
        const auto body=r.word(r.word(r.word(0x4ecbd0u)+0x44u)+(pose.owner&65535u)*12u+8u);
        const auto bytes=r.get<std::int16_t>(body+0x114u),offset=r.get<std::int16_t>(body+0x116u);
        if(bytes<0 || bytes%52 || offset<0)throw FormatError("live model WORLD node bank is malformed");
        const unsigned count=unsigned(bytes)/52u;
        pose.nodes.resize(count);
        if(count)std::memcpy(pose.nodes.data(),r.memory.view(body+unsigned(offset),count*52u).data(),count*52u);
        const auto inverse_count=r.word(model+0x48u),inverse=r.word(model+0x4cu);
        if(inverse_count>256u)throw FormatError("live model inverse-bind bank exceeds source capacity");
        for(unsigned n=0;n<inverse_count;++n)pose.inverse_bind.push_back(r.get<Transform>(inverse+n*96u+0x28u));
    }
    out.geometry->owner_pose=std::move(pose);return out;
}
std::vector<std::array<float,12>> source_live_model_palette_rows_tu15(
    PhysicsLiveWorldTu15& world,const SourceCullDrawUnit& unit,const SourceModelCommandGeometry& geometry) {
    if(unit.words[7]) {
        const Reader r{world.memory()};const auto t=r.get<Transform>(unit.words[7]);
        return {{{t.basis[0].x,t.basis[1].x,t.basis[2].x,t.position.x,
                  t.basis[0].y,t.basis[1].y,t.basis[2].y,t.position.y,
                  t.basis[0].z,t.basis[1].z,t.basis[2].z,t.position.z}}};
    }
    if(unit.words[2]==0xffffffffu)throw FormatError("live model c50 upload has no actual palette");
    const auto region=unit.words[3]&15u;
    if(region>=geometry.palette.regions.size())throw FormatError("live model c50 region is absent");
    const auto selected=geometry.palette.regions[region];
    auto first=std::int32_t(std::bit_cast<std::int16_t>(selected.first));
    unsigned count=selected.count;
    if((unit.words[3]&0xe0000000u)==0x20000000u && geometry.rigid_node!=-1) {
        if(count!=1u)first=geometry.rigid_node;
        count=1;
    }
    if(first<0 || !count || count>40u || std::size_t(first)+count>geometry.palette.matrices.size())
        throw FormatError("live model c50 upload escapes its produced palette");
    return {geometry.palette.matrices.begin()+first,geometry.palette.matrices.begin()+first+count};
}
}
