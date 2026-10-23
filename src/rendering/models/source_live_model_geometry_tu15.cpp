#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_prt.hpp"
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/model.hpp"
#include "halo2/model_skin_submission.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <cstdlib>

namespace halo2 {
std::vector<Vertex> source_live_model_selected_vertices_tu15(std::uint32_t owner,unsigned lod,
    const RenderModel& model,std::span<const Transform> nodes,std::span<const unsigned> sections,
    const SourceLiveModelPoseProviderTu15& provide_pose) {
    if(nodes.size()!=model.bones.size())throw FormatError("posed model world bank count changed");
    if(model.skin.size()!=model.mesh.vertices.size())throw FormatError("invalid model skin mapping");
    std::vector<Vertex> owned_pose;
    std::span<const Vertex> posed;
    const auto *scalar=std::getenv("HALO2_REFERENCE_SCALAR_SKINNING");
    const auto *verify=std::getenv("HALO2_VERIFY_SIMD_SKINNING");
    if(provide_pose && !(scalar && *scalar) && !(verify && *verify))
        posed=provide_pose(owner,lod,model,nodes);
    else {owned_pose=skin_model_world_nodes(model,nodes);posed=owned_pose;}
    if(posed.size()!=model.mesh.vertices.size())
        throw FormatError("Selected WORLD pose provider lost its complete model vertex bank");
    std::vector<Vertex> vertices;
    for(auto section:sections) {
        bool resident=false;
        for(const auto& vertex:posed)if(vertex.source_section==section) {
            resident=true;vertices.push_back(vertex);
        }
        if(!resident)throw FormatError("Third-person collector selected an unprepared original section");
    }
    return vertices;
}
std::optional<std::vector<Vertex>> source_live_model_base_vertices_tu15(
    std::uint32_t owner,std::uint32_t definition,std::uint32_t render_model,
    std::span<const SourceLiveModelBaseDrawTu15> draws,std::span<const RenderModel,6> lods,
    const SourceDynamicModelShaders& shaders,std::span<const Transform> nodes,
    const SourceModelShaderView& view,const SourceLiveModelPoseProviderTu15& provide_pose,
    std::shared_ptr<const ModelSkinGeometry>* deferred_skin,
    const SourceLiveModelDeferredPoseProviderTu15& provide_deferred) {
    if(deferred_skin)deferred_skin->reset();
    if(draws.empty() || nodes.empty())return std::nullopt;
    std::shared_ptr<ModelSkinGeometry> gpu;
    std::vector<Vertex> owned_pose;
    std::span<const Vertex> posed;
    bool pose_ready{};
    std::vector<Vertex> result;
    for(const auto& draw:draws) {
        if(draw.object!=owner || draw.definition!=definition)
            throw FormatError("source unit base lost its full owner/definition identity");
        if(draw.unit.words[1]!=render_model || draw.lighting_class!=2 ||
           draw.override_slot!=255 || !(draw.unit.words[0]&1u))return std::nullopt;
        if(draw.lod>=6)throw FormatError("source unit base selected a LOD outside its six banks");
        if(draw.lod!=draws.front().lod)
            throw FormatError("source unit base changed LOD within one completed owner");
        const auto& model=lods[draw.lod];
        if(nodes.size()!=model.bones.size())
            throw FormatError("source unit base lacks its completed render WORLD hierarchy");
        if(!pose_ready) {
            // The guard above admits exactly one LOD for this complete owner.
            if(deferred_skin) {
                ModelSkinWorldPose owned;
                ModelSkinWorldPoseView prepared;
                if(provide_deferred)prepared=provide_deferred(owner,draw.lod,model,nodes);
                else {
                    owned=model_skin_world_pose(model,nodes,true);
                    prepared={owned.vertices,owned.palette,owned.gpu};
                }
                if(prepared.gpu) {
                    gpu=std::make_shared<ModelSkinGeometry>();
                    gpu->owner=owner;gpu->definition=definition;gpu->render_model=render_model;
                    gpu->lod=draw.lod;gpu->palette.assign(prepared.palette.begin(),prepared.palette.end());
                }
                if(provide_deferred)posed=prepared.vertices;
                else {owned_pose=std::move(owned.vertices);posed=owned_pose;}
            } else if(provide_pose)posed=provide_pose(owner,draw.lod,model,nodes);
            else {owned_pose=skin_model_world_nodes(model,nodes);posed=owned_pose;}
            if(posed.size()!=model.mesh.vertices.size())
                throw FormatError("source unit pose provider lost its complete model vertex bank");
            pose_ready=true;
        }
        const auto section=draw.unit.words[3]>>9&511u;
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> ordinals;
        for(std::size_t n=0;n<posed.size();++n)if(posed[n].source_section==section) {
            vertices.push_back(posed[n]);
            if(gpu)ordinals.push_back(std::uint32_t(n));
        }
        if(vertices.empty())return std::nullopt;
        if(gpu)shaders.select_indexed(vertices,ordinals,SourceModelShaderCollector{0,draw.coverage},view);
        else shaders.select(vertices,SourceModelShaderCollector{0,draw.coverage},view);
        if(gpu)for(const auto ordinal:ordinals) {
            gpu->source_vertices.push_back(ordinal);gpu->bindings.push_back(model.skin.at(ordinal));
        }
        result.insert(result.end(),vertices.begin(),vertices.end());
    }
    if(result.empty())return std::nullopt;
    if(gpu) {gpu->vertices=result;*deferred_skin=std::move(gpu);}
    return result;
}
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
void SourceLiveModelBaseSelectionTu15::remember(const SourceObjectDrawCollector& collector) {
    const Bytes c(collector.bytes);
    const auto count=std::bit_cast<std::int16_t>(c.u16(0x166));
    if(count<=0)return;
    const auto lod=c.u16(0x168);
    if(count>5 || lod>5)throw FormatError("live base collector exceeds original model/LOD bank");
    for(unsigned n=0;n<unsigned(count);++n)
        lods_[{c.u32(0x5cu+n*4u),c.u32(0x48u+n*4u)}]=lod;
}
ObjectColors source_live_model_base_colors_tu15(const SourceLiveModelBaseDrawTu15& draw,ObjectColors retained) {
    if(draw.color_count>retained.size())throw FormatError("live base colors exceed their original destination bank");
    constexpr float scale=std::bit_cast<float>(0x3b808081u);
    for(unsigned n=0;n<draw.color_count;++n) {
        const auto rgb=draw.packed_colors[n];
        retained[n]={float(rgb>>16&255u)*scale,float(rgb>>8&255u)*scale,float(rgb&255u)*scale};
    }
    return retained;
}
bool source_live_model_base_matches_tu15(const SourceLiveModelBaseDrawTu15& base,
    PhysicsLiveWorldTu15& world,const SourceCullDrawUnit& unit) {
    const auto& a=base.unit.words;const auto& b=unit.words;
    if(a[1]!=b[1] || a[2]!=b[2] || a[4]!=b[4] ||
        (a[3]&0xe003fe0fu)!=(b[3]&0xe003fe0fu) || base.lighting_class!=(b[0]>>29))return false;
    const Reader r{world.memory()};if(base.object!=r.word(b[4]))return false;
    const auto geometry=source_live_model_command_geometry_tu15(world,unit);
    const auto rows=source_live_model_palette_rows_tu15(world,unit,geometry);
    return rows.size()==base.palette_rows.size() &&
        (rows.empty() || !std::memcmp(rows.data(),base.palette_rows.data(),rows.size()*sizeof(rows.front())));
}
std::vector<SourceLiveModelBaseDrawTu15> SourceLiveModelBaseSelectionTu15::copy(
    PhysicsLiveWorldTu15& world,const SourceDrawUnitState& state,
    const std::function<bool(const SourceCullDrawUnit&)>& available)const {
    if(state.main_prefix!=state.draw_count || state.draw_count>state.units.size())
        throw FormatError("live base handoff requires the completed main prefix before secondary queries");
    const Reader r{world.memory()};std::vector<SourceLiveModelBaseDrawTu15> result;
    for(unsigned n=0;n<state.main_prefix;++n) {
        const auto& unit=state.units[n];if(unit.words[1]==0xffffffffu)continue;
        if(available && !available(unit))continue;
        const auto selected=lods_.find({unit.words[1],unit.words[4]});
        if(selected==lods_.end())throw FormatError("live base unit has no completed builder selection");
        SourceLiveModelBaseDrawTu15 out;
        out.unit=unit;out.ordinal=n;out.lod=selected->second;out.lighting_class=unit.words[0]>>29;
        out.coverage=state.coverage[n];out.override_slot=state.override_slots[n];
        if(out.override_slot!=255) {
            if(out.override_slot>=state.override_count || out.override_slot>=state.overrides.size())
                throw FormatError("live base override escaped its completed bank");
            out.shader_override=state.overrides[out.override_slot];
        }
        const auto context=unit.words[4];out.object=r.word(context);
        (void)world.object_body(out.object); // Validate the complete source generation.
        const auto body=r.word(r.word(r.word(0x4ecbd0u)+0x44u)+(out.object&65535u)*12u+8u);
        out.definition=r.word(body);
        out.object_flags=r.word(body+4u);
        out.sphere=r.get<ShadowCasterSphere>(body+0x30u);
        out.lighting=r.get<ObjectLightingState>(context+0x10u);
        out.lighting_metadata=r.word(context+0x60u);
        out.color_count=r.get<std::uint8_t>(context+0x74u);
        if(out.color_count>4)throw FormatError("live base colors exceed the original four-channel bank");
        for(unsigned i=0;i<out.color_count;++i)out.packed_colors[i]=r.word(context+0x64u+i*4u);
        out.geometry=source_live_model_command_geometry_tu15(world,unit);
        out.palette_rows=source_live_model_palette_rows_tu15(world,unit,out.geometry);
        result.push_back(std::move(out));
    }
    return result;
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
std::shared_ptr<const VertexProgramBank> source_live_model_prt_bank_tu15(
    PhysicsLiveWorldTu15& world,const SourceCullDrawUnit& unit,const SourcePrtDefinition& definition) {
    const Reader r{world.memory()};const auto count=r.word(0x515ec8u);
    if(count>24)throw FormatError("live PRT owner cache exceeds original capacity");
    if(!count)return {};
    const auto owner=r.word(unit.words[4]);
    std::uint32_t record=0;
    for(unsigned n=0;n<count;++n)if(r.word(0x4c51d0u+n*104u)==owner){record=0x4c51d0u+n*104u;break;}
    if(!record)return {};
    const auto region=unit.words[3]&15u;
    const auto nodes=std::min(unsigned(r.get<std::uint8_t>(record+76u+region)),2u);
    if(!nodes)return {};
    if(definition.render_model!=unit.words[1] || definition.order!=3 || definition.clusters!=10 || definition.pca_vectors!=4)
        throw FormatError("live PRT draw requires its admitted original coefficient dimensions");
    const auto token=r.word(record+8u),offset=r.word(record+12u+region*4u);
    if(std::bit_cast<std::int32_t>(token)<0)throw FormatError("live PRT coefficient allocation is absent");
    const auto first=std::uint64_t(token&0x0fffffffu)+std::uint64_t(offset)*4u;
    const auto bytes=nodes*640u,cursor=r.word(0x494338u);
    if(first>cursor || bytes>cursor-first)throw FormatError("live PRT coefficients escape the current arena");
    auto bank=std::make_shared<VertexProgramBank>();bank->first_constant=68;bank->prt_inputs=true;
    const auto incident=r.get<Vec3>(record+92u);
    bank->constants.push_back({incident.x,incident.y,incident.z,0});bank->constants.push_back(source_prt_scale_constants());
    bank->constants.resize(2u+nodes*40u);
    std::memcpy(bank->constants.data()+2,r.memory.view(0x49433cu+std::uint32_t(first),bytes).data(),bytes);
    return bank;
}
SourceCullDrawBanks source_live_model_cull_geometry_tu15(
    PhysicsLiveWorldTu15& world,const SourceCullDrawUnit& unit,const SourceBspRangeGeometry& ranges) {
    const Reader r{world.memory()};SourceCullDrawBanks out;out.geometry_resolved=true;
    const auto model=r.model(unit),section=r.section(model,unit);
    if(!r.word(section+0x34u))return out;
    out.geometry=source_bsp_sphere_cull_geometry(ranges);
    if(unit.words[7])out.transform=r.get<Transform>(unit.words[7]);
    if(ranges.accelerator_count>0 || ranges.bounds.empty())return out;
    out.geometry->resolve_owner_pose=[&world,model,context=unit.words[4]] {
        const Reader r{world.memory()};
        SourceCullOwnerPose pose;pose.owner=r.word(context);
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
        return pose;
    };
    return out;
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
