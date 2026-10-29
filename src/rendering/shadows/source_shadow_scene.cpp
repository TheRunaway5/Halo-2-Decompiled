#include "halo2/source_shadow_scene.hpp"
#include "halo2/source_live_shadow_phase_inputs_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_effect_light_opaque.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_shadow_bitmap_frames.hpp"
#include "halo2/source_view_projection.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <tuple>

namespace halo2 {
// The cache library validates immutable pass capsules without a World. Their
// native allocation must be admitted here, in the World-owning systems layer.
SourceShadowRawPassOwnerTu15 source_shadow_raw_pass_owner_tu15(
    PhysicsLiveWorldTu15& world,const Cache& cache,const MenuWorldSourcePass& pass) {
    if(&pass.cache_owner()!=&cache)
        throw FormatError("raw shadow native pass has a foreign Cache owner");
    const auto& expected=pass.implementation_record();
    const auto backing=world.resident_cache_address(cache,pass.implementation_address(),expected.size());
    const auto& memory=world.memory();const auto address=memory.source_address(backing,expected.size());
    if(!std::ranges::equal(memory.view(backing,expected.size()),expected))
        throw FormatError("raw shadow native implementation differs from its complete Cache preimage");
    return SourceShadowRawPassOwnerTu15(pass,address);
}
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    return Bytes(memory.view(address,4)).u32(0);
}
const Tag& bsp_tag(const Cache& cache,std::uint32_t datum) {
    const auto* tag=cache.find_tag({datum},tag_groups::scenario_structure_bsp);
    // Original disc directory rows remain unloaded until the source BSP
    // publisher installs their scenario-mapped physical root. MCC instead
    // retains the complete converted tag allocation before native delivery.
    if(!tag || (cache.is_mcc() && (!tag->address || tag->size<0x160)))
        throw FormatError("shadow scene lacks its complete salted BSP tag owner");
    return *tag;
}
void same_parts(const SourceRuntimeMemory& memory,std::uint32_t resident,
    const SourceBspRangeGeometry& geometry) {
    if(!resident || word(memory,resident)!=geometry.parts.size())
        throw FormatError("shadow scene lost its actual resident part bank");
    if(!geometry.parts.empty() && std::memcmp(memory.view(word(memory,resident+4u),
            geometry.parts.size()*72u).data(),geometry.parts.data(),geometry.parts.size()*72u))
        throw FormatError("shadow scene native parts differ from immutable resources");
}
// Constant animation still needs a separate owner. Bitmap-frame animation is
// completed from this Scene's immutable pass and actual native model below.
void static_pass(const Material& material) {
    const auto& pass=material.passes.at(material.forward_pass_index);
    //17 is this root's completed opacity.22 is the authored constant-one
    //shadow-darkening scalar. All other consumed externs need a new owner.
    if(!pass.state_externs.empty() || !pass.vertex_externs.empty() ||
       std::any_of(pass.pixel_externs.begin(),pass.pixel_externs.end(),
           [](const auto& r){return (r.source!=17 && r.source!=22) || (r.component&7)>=4;}))
        throw FormatError("shadow scene selected an unresolved live material extern");
    const auto& a=material.animation;
    if(a.overlays.empty())return;
    if(pass.template_pass>=a.passes.size())
        throw FormatError("shadow scene animation pass escapes authored metadata");
    const auto range=a.passes[pass.template_pass];
    if(pass.implementation>=range.count() || range.first()+range.count()>a.implementations.size())
        throw FormatError("shadow scene animation implementation escapes authored metadata");
    const auto& impl=a.implementations[range.first()+pass.implementation];
    if(impl.pixel_constants.count() || impl.vertex_constants.count())
        throw FormatError("shadow scene selected animation without its live function owner: material="+
            material.name+" shader="+std::to_string(material.source_shader_datum)+
            " lod="+std::to_string(material.selected_shader_lod)+
            " template_pass="+std::to_string(pass.template_pass)+
            " implementation="+std::to_string(pass.implementation)+
            " bitmap_mappings="+std::to_string(impl.bitmap_transforms.count())+
            " pixel_mappings="+std::to_string(impl.pixel_constants.count())+
            " vertex_mappings="+std::to_string(impl.vertex_constants.count()));
}
void phase_input(const SourceShadowScenePhaseInput& input,unsigned phase) {
    if(!input.controls.depth_clamp || (input.controls.cull!=0 &&
        input.controls.cull!=0x900 && input.controls.cull!=0x901) ||
        input.raster.stencil.enabled || input.raster.source_stencil_mode)
        throw FormatError("shadow scene lacks the fixed phase raster controls");
    const auto &surface_clip=input.controls.surface_clip;
    if(!surface_clip[2] || !surface_clip[3] ||
       surface_clip[0]>std::uint32_t(std::numeric_limits<std::int32_t>::max()) ||
       surface_clip[1]>std::uint32_t(std::numeric_limits<std::int32_t>::max()))
        throw FormatError("shadow scene lacks actual representable SURFACE_CLIP state");
    const std::array<float,2> offset=phase?std::array<float,2>{0,0}:std::array<float,2>{2,550};
    if(input.raster.window_exclusive ||
       std::memcmp(input.raster.depth_factor_offset.data(),offset.data(),sizeof(offset)))
        throw FormatError("shadow scene offset/window differs from the original fixed phase");
    for(float size:input.surface_size)if(!std::isfinite(size) || size<=0)
        throw FormatError("shadow scene lacks actual bound surface/source-AA dimensions");
    for(auto scale:input.raster.surface_scale)if(scale<=0)
        throw FormatError("shadow scene lacks actual source-AA/renderer surface scale");
}
}
struct SourceShadowScene::Impl {
    const Cache& cache;
    std::size_t bsp_index;
    std::uint32_t bsp_datum,bsp_address;
    struct Section {
        std::shared_ptr<const MenuWorldSourceSection> raw;
        SourceBspRangeGeometry ranges;
        std::uint32_t source_section{};
    };
    using SectionKey=std::tuple<MenuWorldSourceSectionKind,std::uint32_t,unsigned>;
    std::map<SectionKey,Section> sections;
    struct Instance {MenuWorldSourceBspOwner owner;Transform transform;std::uint32_t address;};
    std::vector<Instance> instances;
    std::map<std::uint32_t,Material> shaders;
    using MaterialKey=std::tuple<SourceShadowPhase,std::uint32_t,unsigned,unsigned>;
    struct Slot {
        unsigned material_slot;
        std::unique_ptr<SourceShadowMaterial> authored;
        MenuWorldSourcePass pass;
    };
    std::map<MaterialKey,Slot> slots;
    std::vector<SourceShadowRawPlanMaterial> resident;

    Impl(const Cache& c,Mesh& mesh,std::size_t bsp,std::span<const std::uint32_t> models)
      :cache(c),bsp_index(bsp),bsp_datum(c.bsps().at(bsp).datum),
       bsp_address(c.is_mcc()?bsp_tag(c,bsp_datum).address:c.bsps().at(bsp).memory_address+16u) {
        const auto metadata=cache.load_source_bsp_metadata(bsp);
        // The raw resource APIs retain the legacy chunk-header view. MCC's
        // actual sbsp tag root is separately allocated and relocated; it need
        // not be chunk+16 even when both views describe the same resources.
        const Bytes root(metadata.bytes_at(c.bsps().at(bsp).memory_address+16u,0x160));
        std::set<std::uint32_t> shader_ids;
        const auto add=[&](MenuWorldSourceSectionKind kind,std::uint32_t owner,unsigned n,
            std::uint32_t source_section) {
            const SectionKey key{kind,owner,n};if(sections.contains(key))return;
            auto raw=std::make_shared<const MenuWorldSourceSection>(kind==MenuWorldSourceSectionKind::render_model?
                load_menu_world_source_model_section(cache,owner,n):
                load_menu_world_source_bsp_section(cache,bsp,kind,n));
            auto ranges=kind==MenuWorldSourceSectionKind::render_model?
                source_model_range_geometry(*raw):source_bsp_range_geometry(*raw);
            for(const auto& part:raw->parts())if(part.shader!=0xffffffffu)shader_ids.insert(part.shader);
            sections.emplace(key,Section{std::move(raw),std::move(ranges),source_section});
        };
        for(unsigned n=0;n<root.u32(0x9c);++n) {
            const auto address=root.u32(0xa0)+n*0xb0u;
            if(Bytes(metadata.bytes_at(address+0x24u,2)).u16(0))
                add(MenuWorldSourceSectionKind::bsp_cluster,bsp_datum,n,address);
        }
        for(unsigned n=0;n<root.u32(0x138);++n) {
            const auto address=root.u32(0x13c)+n*0xc8u;
            if(Bytes(metadata.bytes_at(address+0x24u,2)).u16(0))
                add(MenuWorldSourceSectionKind::bsp_instance_definition,bsp_datum,n,address);
        }
        for(unsigned n=0;n<root.u32(0x140);++n) {
            const auto address=root.u32(0x144)+n*88u;Transform transform;
            std::memcpy(&transform,metadata.bytes_at(address,sizeof(transform)).data(),sizeof(transform));
            instances.push_back({load_menu_world_source_bsp_owner(cache,bsp,n),transform,address});
        }
        // Preload every authored material row as well as referenced raw parts.
        // Unused materials may still have source-selected shadow passes.
        for(unsigned n=0;n<root.u32(0xa4);++n) {
            const auto shader=Bytes(metadata.bytes_at(root.u32(0xa8)+n*32u+12u,4)).u32(0);
            if(shader!=0xffffffffu)shader_ids.insert(shader);
        }
        for(auto model:models) {
            const auto metadata=cache.load_source_tag_metadata(model);
            if(metadata.group()!="mode")throw FormatError("shadow scene model preload is not mode metadata");
            const Bytes root(metadata.bytes_at(metadata.source_address(),0x68));
            for(unsigned n=0;n<root.u32(0x60);++n) {
                const auto shader=Bytes(metadata.bytes_at(root.u32(0x64)+n*32u+12u,4)).u32(0);
                if(shader!=0xffffffffu)shader_ids.insert(shader);
            }
            for(unsigned n=0;n<root.u32(0x24);++n) {
                const auto address=root.u32(0x28)+n*92u;
                if(Bytes(metadata.bytes_at(address+4u,2)).u16(0))
                    add(MenuWorldSourceSectionKind::render_model,model,n,address);
            }
        }
        for(auto shader:shader_ids) {
            const auto at=shaders.emplace(shader,cache.load_material(shader)).first;
            auto& authored=at->second;
            for(unsigned lod=0;lod<authored.lods.size();++lod) {
                authored.selected_shader_lod=lod;
                for(unsigned phase=0;phase<2;++phase) {
                    const auto kind=phase?SourceShadowPhase::Apply:SourceShadowPhase::Generate;
                    auto materials=prepare_shadow_material(cache,authored,kind,true,false,1.f);
                    for(unsigned n=0;n<materials.size();++n) {
                        const auto layer=phase?11u:10u;
                        const auto& lod_bank=authored.lods.at(lod);
                        if(layer>=lod_bank.layers.count() ||
                           lod_bank.layers.first()+layer>=authored.layers.size())
                            throw FormatError("shadow scene preload layer escapes selected LOD");
                        const auto range=authored.layers[lod_bank.layers.first()+layer];
                        const auto& selected=materials[n].material.passes.at(materials[n].material.forward_pass_index);
                        if(selected.template_pass<range.first() || selected.template_pass-range.first()>=range.count())
                            throw FormatError("shadow scene preload pass escapes selected layer");
                        const auto pass_ordinal=unsigned(selected.template_pass-range.first());
                        const auto slot=unsigned(mesh.materials.size());
                        // Mesh retains textures for renderer residency. The separate
                        // immutable authored value keeps a stable identity thereafter.
                        mesh.materials.push_back(materials[n].material);
                        auto material=std::make_unique<SourceShadowMaterial>(std::move(materials[n]));
                        auto pass=load_menu_world_source_pass(cache,shader,{lod,layer,pass_ordinal,phase?4u:0u});
                        const auto pointer=material.get();
                        if(!slots.emplace(MaterialKey{kind,shader,lod,pass_ordinal},
                                Slot{slot,std::move(material),std::move(pass)}).second)
                            throw FormatError("shadow scene preload duplicated an original pass ordinal");
                        resident.push_back({slot,pointer});
                    }
                }
            }
        }
    }
    const Section& section(const SourceCullDrawUnit& unit)const {
        const auto& u=unit.words;const bool model=u[1]!=0xffffffffu;
        const bool instance=!model && (u[0]&0x1000u);
        const auto kind=model?MenuWorldSourceSectionKind::render_model:instance?
            MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster;
        const auto n=instance?instances.at(u[3]>>18&2047u).owner.section:(u[3]>>9&511u);
        const auto at=sections.find({kind,model?u[1]:bsp_datum,n});
        if(at==sections.end())throw FormatError("shadow scene selected a section that was not preloaded");
        return at->second;
    }
};
SourceShadowRawPlanRoot SourceShadowSceneRoot::view() const {
    return {owner_,uniforms_,command_masks_,casters_,receivers_};
}
SourceShadowScene::SourceShadowScene(const Cache& cache,Mesh& mesh,std::size_t bsp,
    std::span<const std::uint32_t> models):impl_(std::make_unique<Impl>(cache,mesh,bsp,models)){}
SourceShadowScene::~SourceShadowScene()=default;
std::span<const SourceShadowRawPlanMaterial> SourceShadowScene::resident()const{return impl_->resident;}
SourceShadowBitmapFrameOwnerTu15 SourceShadowScene::complete_bitmap_frames(
    PhysicsLiveWorldTu15& world,const MenuWorldSourcePass& pass,
    const MenuWorldSourceOwner& owner,SourceShadowMaterial& completed)const {
    const auto& i=*impl_;const auto& choice=pass.choice();
    if(&pass.cache_owner()!=&i.cache || (choice.layer!=10 && choice.layer!=11))
        throw FormatError("shadow bitmap completion has a foreign Cache or phase owner");
    const auto phase=choice.layer==10?SourceShadowPhase::Generate:SourceShadowPhase::Apply;
    const auto slot=i.slots.find({phase,pass.shader(),choice.shader_lod,choice.pass_in_layer});
    if(slot==i.slots.end())throw FormatError("shadow bitmap completion has no immutable resident pass");
    const auto& authored=*slot->second.authored;const auto& original=authored.material;
    const auto& selected=original.passes.at(original.forward_pass_index);
    const auto& actual=completed.material;
    if(!source_shadow_raw_pass_owner_tu15(world,i.cache,pass).matches(slot->second.pass,
            world.memory().source_address(world.resident_cache_address(i.cache,
                pass.implementation_address(),pass.implementation_record().size()),pass.implementation_record().size())) ||
       actual.source_shader_datum!=original.source_shader_datum ||
       actual.selected_shader_lod!=original.selected_shader_lod ||
       actual.forward_pass_index!=original.forward_pass_index || selected.template_pass!=pass.template_pass() ||
       selected.implementation!=choice.implementation || completed.external_targets!=authored.external_targets)
        throw FormatError("shadow bitmap completion changed its immutable selected pass");
    if(owner.kind!=MenuWorldSourceOwnerKind::dynamic_object || !owner.generation ||
       *owner.generation==0xffffffffu || owner.ordinal!=(*owner.generation&65535u))
        throw FormatError("shadow bitmap completion requires its full salted native model owner");
    (void)world.object_body(*owner.generation);
    std::array<std::size_t,4> counts{};
    for(unsigned stage=0;stage<counts.size();++stage) {
        counts[stage]=original.animated_textures[stage].size();
        if(actual.animated_textures[stage].size()!=counts[stage])
            throw FormatError("shadow bitmap completion changed its immutable frame bank");
    }
    const ShaderInput input=[&](std::uint32_t function) {
        // Same original scalar owner as model_build_world and native loop
        // inputs: BCB70 initializes the result, honors the32 override slots,
        // then executes the actual object's authored functions/handlers.
        const auto output=world.prepare_source_call().gp[4]+0x100u;
        auto call=world.prepare_source_call(std::array{*owner.generation,function,output});
        world.run_actual(0xbcb70u,call);
        return Bytes(world.memory().view(output,4)).f32(0);
    };
    completed.material.texture_frames=source_shadow_bitmap_frames(original,input);
    return SourceShadowBitmapFrameOwnerTu15(completed,pass,owner,counts);
}
SourceShadowSceneRoot SourceShadowScene::draw(PhysicsLiveWorldTu15& world,
    const SourceLiveShadowRootTu15& root,const SourceShadowUniforms& uniforms,
    const std::array<SourceShadowScenePhaseInput,2>& inputs)const {
    return draw_with_phase(world,root,uniforms,[&](SourceShadowPhase phase,
        const MenuWorldSourcePass&,unsigned,const SourceShadowMaterial&)->const SourceShadowScenePhaseInput& {
        return inputs[phase==SourceShadowPhase::Apply?1u:0u];
    },[&] {
        std::array<ShaderVector,12> camera;std::copy_n(inputs[0].constants.begin(),camera.size(),camera.begin());return camera;
    },&inputs);
}
SourceShadowSceneRoot SourceShadowScene::draw(PhysicsLiveWorldTu15& world,
    const SourceLiveShadowRootTu15& root,const SourceShadowUniforms& uniforms,
    const SourceLiveShadowPhaseInputsTu15& owner,const RendererTargets::Use& use)const {
    return draw_with_phase(world,root,uniforms,[&](SourceShadowPhase phase,
        const MenuWorldSourcePass& pass,unsigned vertex_class,
        const SourceShadowMaterial& material)->const SourceShadowScenePhaseInput& {
        return owner.phase(use,impl_->cache,phase,pass,vertex_class,material);
    },[&] {return owner.camera(use);},nullptr);
}
SourceShadowSceneRoot SourceShadowScene::draw_with_phase(PhysicsLiveWorldTu15& world,
    const SourceLiveShadowRootTu15& root,const SourceShadowUniforms& uniforms,
    const PhaseProvider& provide,const CameraProvider& provide_camera,
    const std::array<SourceShadowScenePhaseInput,2>* diagnostic_inputs)const {
    const auto& i=*impl_;SourceShadowSceneRoot output;
    if(root.owner==0xffffffffu || root.route!=SourceLiveShadowRootRouteTu15::projected ||
       !root.frame || !root.uniforms || !root.commands)
        throw FormatError("shadow scene requires a completed original projected root");
    const auto& retained=*root.uniforms;
    if(std::memcmp(uniforms.projection.data(),retained.projection.data(),sizeof(uniforms.projection)) ||
       std::memcmp(uniforms.constants.data(),retained.constants.data(),sizeof(uniforms.constants)) ||
       uniforms.texture_width!=retained.texture_width || uniforms.texture_height!=retained.texture_height ||
       std::bit_cast<std::uint32_t>(uniforms.opacity)!=std::bit_cast<std::uint32_t>(retained.opacity) ||
       std::bit_cast<std::uint32_t>(uniforms.opacity)!=std::bit_cast<std::uint32_t>(root.frame->opacity))
        throw FormatError("shadow scene uniforms changed the completed original root bank");
    output.owner_=root.owner;output.uniforms_=uniforms;
    output.command_masks_={(*root.commands)[1].header[2],(*root.commands)[0].header[2]};
    if(!source_shadow_raw_phases_present(output.command_masks_)) {
        if(root.ordered_draws)for(const auto& phase:*root.ordered_draws)if(!phase.records.empty())
            throw FormatError("shadow scene ordered records precede original command guard");
        return output;
    }
    if(!root.collected || !root.ordered_draws)
        throw FormatError("shadow scene lacks completed original units or ordered descriptors");
    const auto& state=root.collected->state;
    if(state.draw_count>state.units.size() || state.main_prefix>state.draw_count ||
       state.override_count>state.overrides.size() || state.arena.used>512 ||
       root.collected->main_arena.used>512 || root.collected->main_arena_address!=0x557074u ||
       root.collected->secondary_arena_address!=0x559b54u)
        throw FormatError("shadow scene collected bank exceeds original capacities");
    auto& memory=world.memory();
    const auto* scenario=i.cache.find_tag(i.cache.scenario_tag(),tag_groups::scenario);
    if(!scenario || !scenario->address || !scenario->size)
        throw FormatError("shadow scene lacks its actual Cache scenario owner");
    (void)world.resident_cache_address(i.cache,scenario->address,scenario->size);
    // Converted MCC metadata has its own admitted runtime allocation. Resolve
    // immutable cache addresses through that same owner before comparing or
    // reading the live bank, as the ordinary model/resource consumers do.
    const auto resident=[&](std::uint32_t address,std::size_t bytes) {
        if(!i.cache.is_mcc()){(void)memory.view(address,bytes);return address;}
        return memory.source_address(world.resident_cache_address(i.cache,address,bytes),bytes);
    };
    const auto expected_bsp=resident(i.bsp_address,i.cache.is_mcc()?bsp_tag(i.cache,i.bsp_datum).size:0x160u);
    const auto table=word(memory,0x4f0414u);
    const auto tag=std::uint64_t(table)+(i.bsp_datum&65535u)*16u;
    if(!table || tag>0xfffffff0u || world.source_bsp_index()!=i.bsp_index ||
       word(memory,0x4ecc18u)!=expected_bsp || word(memory,std::uint32_t(tag))!=tag_groups::scenario_structure_bsp.value ||
       word(memory,std::uint32_t(tag)+4u)!=i.bsp_datum || word(memory,std::uint32_t(tag)+8u)!=expected_bsp)
        throw FormatError("shadow scene current BSP differs from preloaded immutable owner");
    if(diagnostic_inputs) {
        for(unsigned phase=0;phase<diagnostic_inputs->size();++phase)phase_input((*diagnostic_inputs)[phase],phase);
        if(std::memcmp((*diagnostic_inputs)[0].constants.data(),(*diagnostic_inputs)[1].constants.data(),12*sizeof(ShaderVector)))
            throw FormatError("shadow scene phases do not retain the same actual main camera bank");
    }
    //Active empty roots still execute Generate-clear/filter/Apply. Validate
    //their actual native lease/camera without inventing a selected pass or
    //exposing a phase bank. Inactive command guards already returned above.
    const auto camera=provide_camera();
    struct Completed {
        const SourceShadowMaterial* material;
        std::optional<SourceShadowBitmapFrameOwnerTu15> frames;
    };
    using CompletedKey=std::tuple<Impl::MaterialKey,unsigned,std::uint32_t,std::uint32_t,std::uint32_t>;
    std::map<CompletedKey,Completed> completed;
    for(unsigned phase=0;phase<2;++phase) {
        const auto kind=phase?SourceShadowPhase::Apply:SourceShadowPhase::Generate;
        const auto& ordered=(*root.ordered_draws)[phase];
        const auto& bank=(*root.commands)[phase?0:1];
        const auto address=phase?0x4ce280u:0x4ce298u,capacity=phase?1024u:512u;
        if(bank.address!=address || bank.header[0]!=capacity || bank.header[1]!=bank.commands.size() ||
           bank.header[1]>capacity || bank.header[3]>bank.header[1] || ordered.owner!=root.owner ||
           ordered.phase!=kind || ordered.bank!=address || ordered.category!=(phase?1u:2u) ||
           ordered.layer!=(phase?11u:10u) || ordered.capacity!=2560 || ordered.records.size()>ordered.capacity)
            throw FormatError("shadow scene phase bank differs from its original ordered owner");
        auto& draws=phase?output.receivers_:output.casters_;
        for(unsigned ordinal=0;ordinal<ordered.records.size();++ordinal) {
            const auto& descriptor=ordered.records[ordinal];const auto command_index=descriptor[0]&65535u;
            if(command_index<bank.header[3] || command_index>=bank.commands.size())
                throw FormatError("shadow scene descriptor escapes its actual command interval");
            const auto& command=bank.commands[command_index];const auto unit_index=command[3]>>8&65535u;
            if(unit_index>=state.draw_count || state.override_slots[unit_index]!=255)
                throw FormatError("shadow scene unit or shader override has no admitted owner");
            const auto& unit=state.units[unit_index];const auto& u=unit.words;
            if(!(u[0]&1u))throw FormatError("shadow scene command consumes an unadmitted original unit");
            const auto& section=i.section(unit);const bool model=u[1]!=0xffffffffu;
            const bool instance=!model && (u[0]&0x1000u);
            const auto part=command[3]&255u;
            if(part>=section.raw->parts().size() || section.raw->parts()[part].shader!=command[2])
                throw FormatError("shadow scene selected a missing part or unowned shader override");
            MenuWorldSourceOwner owner;std::vector<std::array<float,12>> palette;
            if(model) {
                const auto salted_owner=word(memory,u[4]);(void)world.object_body(salted_owner);
                owner={MenuWorldSourceOwnerKind::dynamic_object,u[1],salted_owner&65535u,salted_owner};
                const auto live=source_live_model_command_geometry_tu15(world,unit);
                if(!live.geometry.resident || live.geometry.parts!=section.ranges.parts)
                    throw FormatError("shadow scene live model parts differ from preloaded resources");
                palette=source_live_model_palette_rows_tu15(world,unit,live);
            } else {
                const auto native_section=resident(section.source_section,instance?0xc8u:0xb0u);
                same_parts(memory,word(memory,native_section+0x50u),section.ranges);
                if(instance) {
                    const auto n=u[3]>>18&2047u;const auto& actual=i.instances.at(n);
                    if(u[7]!=resident(actual.address,88) || std::memcmp(memory.view(u[7],sizeof(Transform)).data(),
                            &actual.transform,sizeof(Transform)))
                        throw FormatError("shadow scene instance lost its actual source transform");
                    owner={MenuWorldSourceOwnerKind::bsp_instance,i.bsp_datum,n,{}};
                    palette.push_back(actual.owner.rows);
                } else owner={MenuWorldSourceOwnerKind::bsp_cluster,i.bsp_datum,u[3]>>9&511u,{}};
            }
            const Impl::MaterialKey key{kind,command[2],command[5]>>8&255u,command[5]&255u};
            const auto slot=i.slots.find(key);
            if(slot==i.slots.end())throw FormatError("shadow scene selected a pass that was not preloaded");
            // Empty filtered descriptors still own a source binding. Validate
            // their command identity even though no nonempty GPU range follows.
            const auto pass_owner=source_shadow_raw_pass_owner_tu15(world,i.cache,slot->second.pass);
            if(command[0]!=0x4cbe0u || command[1]!=0x4d0a0u ||
               !(command[4]&(1u<<(phase?11u:10u))) ||
               (descriptor[0]>>16)!=(command[2]&65535u) ||
               (descriptor[1]&255u)!=(phase?4u:0u) ||
               (descriptor[1]>>8&255u)!=((u[3]>>29)<=1?1u:2u) ||
               ((u[3]>>29)!=0 && descriptor[3]!=((u[3]>>9&511u)|((u[1]&65535u)<<11))) ||
               !pass_owner.matches(slot->second.pass,descriptor[4]) || descriptor[5]!=u[4])
                throw FormatError("shadow scene original descriptor/command identity changed");
            const bool animated=std::any_of(slot->second.authored->material.animated_textures.begin(),
                slot->second.authored->material.animated_textures.end(),[](const auto& bank){return !bank.empty();});
            const CompletedKey completed_key{key,animated?unsigned(owner.kind):0u,
                animated?owner.scope_datum:0xffffffffu,animated?owner.ordinal:0xffffffffu,
                animated?owner.generation.value_or(0xffffffffu):0xffffffffu};
            const Completed* bound;
            if(const auto at=completed.find(completed_key);at!=completed.end())bound=&at->second;
            else {
                auto authored=i.shaders.at(command[2]);authored.selected_shader_lod=command[5]>>8&255u;
                auto values=prepare_shadow_material(i.cache,authored,kind,true,false,uniforms.opacity);
                const auto& expected=slot->second.authored->material;
                const auto& expected_pass=expected.passes.at(expected.forward_pass_index);
                const auto selected=std::find_if(values.begin(),values.end(),[&](const auto& value) {
                    const auto& m=value.material;const auto& pass=m.passes.at(m.forward_pass_index);
                    return pass.template_pass==expected_pass.template_pass && pass.implementation==expected_pass.implementation;
                });
                if(selected==values.end())throw FormatError("shadow scene selected pass disappeared during completion");
                output.materials_.push_back(std::make_unique<SourceShadowMaterial>(std::move(*selected)));
                auto& material=*output.materials_.back();static_pass(material.material);
                std::optional<SourceShadowBitmapFrameOwnerTu15> frames;
                if(animated)frames=complete_bitmap_frames(world,slot->second.pass,owner,material);
                else (void)source_shadow_bitmap_frames(material.material,[](std::uint32_t)->float {
                    throw FormatError("shadow bitmap function lost its actual native model owner");
                });
                bound=&completed.emplace(completed_key,Completed{&material,std::move(frames)}).first->second;
            }
            const auto* material=bound->material;
            static_pass(material->material);
            auto input=provide(kind,slot->second.pass,descriptor[2]&65535u,*material);
            phase_input(input,phase);
            if(std::memcmp(camera.data(),input.constants.data(),sizeof(camera)))
                throw FormatError("shadow scene selected phases changed the actual main camera bank");
            std::optional<SourceShadowRawEntryOwnerTu15> entry;
            if(u[0]&0x800u) {
                //4D630's EAX0/BL1 selects the same completed submitting
                // projection as12D30's normal entry, then stores all16
                // ordered products before multiplying each stored float.
                // Reuse that authenticated arithmetic, not a second matrix
                // construction or a scale of an already transformed vertex.
                SourceViewProjectionBank projection;
                std::memcpy(&projection,memory.view(0x4c6634u,sizeof(projection)).data(),sizeof(projection));
                const auto distance=std::bit_cast<float>(word(memory,0x491e7cu));
                const auto ordinary=source_view_shader_constants(projection,distance,false);
                if(!memory.is_read_only(0x44a8a0u,4) || word(memory,0x44a8a0u)!=0x3e19999au ||
                   std::memcmp(ordinary.data(),input.constants.data(),sizeof(ordinary)))
                    throw FormatError("Shadow per-entry projection lost its actual main bank or authenticated44A8A0 operand");
                const auto foreground=source_view_shader_constants(projection,distance,true);
                std::copy_n(foreground.begin(),4,input.constants.begin());
                input.clip_range[2]=source_shadow_raw_entry_minimum_w_tu15(input.clip_range[2],
                    std::bit_cast<float>(word(memory,0x44a8a0u)));
                std::array<ShaderVector,12> current;std::copy_n(input.constants.begin(),12,current.begin());
                entry=SourceShadowRawEntryOwnerTu15(kind,u,camera,current,input.clip_range);
            }
            const auto constants=source_shadow_raw_constants(input.constants,*material,uniforms);
            // Construct the owned raw resource/class/palette binding first.
            // This is native raw admission, never an invented1CAA0 return.
            const auto topology=(u[0]&0x200u) && !(section.raw->parts()[part].flags&8u)?
                MenuWorldSourceTopology::triangle_strip:MenuWorldSourceTopology::triangle_list;
            auto whole=model || instance?menu_world_source_draw(section.raw,part,topology,palette):
                menu_world_source_cluster_draw(section.raw,part,topology);
            (void)menu_world_source_binding_class(i.cache,owner,std::move(whole),
                slot->second.pass.vertex_shader(),descriptor[2]&65535u,constants);
            const SourceBspRangeArena* arena=&root.collected->main_arena;
            auto arena_address=root.collected->main_arena_address;
            if((u[0]&0x40u) && u[5] && section.ranges.subpart_count) {
                const auto contains=[&](std::uint32_t base) {
                    return base && u[5]>=base && std::uint64_t(u[5])<std::uint64_t(base)+2048u && !((u[5]-base)&3u);
                };
                const bool main=contains(root.collected->main_arena_address),
                    secondary=contains(root.collected->secondary_arena_address);
                if(main==secondary)throw FormatError("shadow scene mask has no unique main/secondary arena owner");
                if(secondary){arena=&state.arena;arena_address=root.collected->secondary_arena_address;}
            }
            const auto ranges=source_effect_light_part_ranges(unit,section.ranges,part,*arena,arena_address);
            bool submitted=false;
            SourceShadowRawCommandTu15 proof{kind,root.owner,address,phase?1u:2u,phase?11u:10u,
                ordinal,unit_index,descriptor,command,u,owner};
            proof.native_pass_owner=pass_owner;
            for(auto range:ranges.ranges) {
                if(!range.count)continue;
                auto raw=model || instance?menu_world_source_draw(section.raw,part,ranges.topology,palette,range):
                    menu_world_source_cluster_draw(section.raw,part,ranges.topology,range);
                if(!raw.triangle_count())continue;
                auto binding=menu_world_source_binding_class(i.cache,owner,std::move(raw),
                    slot->second.pass.vertex_shader(),descriptor[2]&65535u,constants);
                draws.push_back(source_shadow_raw_draw(slot->second.material_slot,*material,slot->second.pass,
                    std::move(binding),proof,uniforms,input.controls,input.raster,
                    input.clip_range,input.fog_parameters,input.surface_size,bound->frames,entry));
                submitted=true;
            }
            if(!submitted)++output.empty_descriptors_;
        }
    }
    const auto view=output.view();SourceShadowRawPlan plan;
    plan.camera=camera;plan.roots=std::span(&view,1);
    (void)validate_source_shadow_raw_plan(plan,i.resident);
    return output;
}
} // namespace halo2
