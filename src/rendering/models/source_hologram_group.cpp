#include "halo2/source_hologram_group.hpp"
#include "halo2/source_effect_light_opaque.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>

namespace halo2 {
SourceHologramCameraOwner::SourceHologramCameraOwner(SourceHologramCameraPose pose,
    const MenuWorldSourceConstants& constants):pose_(pose) {
    for(const auto value:{pose.position.x,pose.position.y,pose.position.z,
                         pose.forward.x,pose.forward.y,pose.forward.z})
        if(!std::isfinite(value))throw FormatError("hologram submitting camera pose is not finite");
    const std::array eye{pose.position.x,pose.position.y,pose.position.z};
    if(std::memcmp(eye.data(),constants[7].data(),sizeof(eye)))
        throw FormatError("hologram submitting raw eye differs from its actual c7 input");
    std::copy_n(constants.begin(),camera_.size(),camera_.begin());
}
bool SourceHologramCameraOwner::matches(std::span<const ShaderVector,12> camera)const {
    return std::memcmp(camera.data(),camera_.data(),sizeof(camera_))==0;
}
SourceHologramGroupBindings::SourceHologramGroupBindings(const Cache &cache,Mesh &mesh,
    std::span<const std::uint32_t> definitions):cache_(cache) {
    std::set<std::uint32_t> shaders;
    for(const auto definition:definitions) {
        const auto model=cache.load_initial_model_definition(definition).model;
        if(model==0xffffffffu)continue;
        const auto metadata=cache.load_source_tag_metadata(model);
        if(metadata.group()!="hlmt")throw FormatError("hologram definition lost its authored HLMT owner");
        const auto shader=Bytes(metadata.bytes_at(metadata.source_address()+0xf4,4)).u32(0);
        if(shader!=0xffffffffu)shaders.insert(shader);
    }
    for(const auto shader:shaders) {
        auto authored=cache.load_material(shader);
        for(unsigned lod=0;lod<authored.lods.size();++lod) {
            if(!(authored.lods[lod].available_layers&(1u<<21)))continue;
            authored.selected_shader_lod=lod;
            auto material=prepare_hologram_material(cache,authored,{lod,21,0,0});
            for(const auto &overlay:material.material.animation.overlays)
                if(overlay.input || overlay.range)
                    throw FormatError("hologram requires an unresolved object-function owner");
            materials_.emplace(std::pair{shader,lod},unsigned(mesh.materials.size()));
            mesh.materials.push_back(std::move(material.material));
        }
    }
}
void SourceHologramGroupBindings::rebase_materials(unsigned previous,unsigned next) {
    for(auto &[key,slot]:materials_)slot=slot-previous+next;
}
std::vector<SourceHologramGroupDraw> SourceHologramGroupBindings::draw(
    std::span<const SourceLiveTransparentGroupTu15> groups,std::span<const SourceTransparentGroupParts> bindings,
    std::span<const SourceLiveModelBaseDrawTu15> units,const MenuWorldSourceConstants &constants,
    MenuWorldRasterPacket raster,std::array<float,4> clip,std::array<float,2> fog,std::array<float,2> surface,
    std::optional<SourceHologramCameraPose> submitting_camera) {
    if(groups.size()!=bindings.size())throw FormatError("hologram group lost its completed descriptor binding");
    std::shared_ptr<const SourceHologramCameraOwner> camera;
    if(submitting_camera)camera.reset(new SourceHologramCameraOwner(*submitting_camera,constants));
    std::vector<SourceHologramGroupDraw> result;
    for(unsigned g=0;g<groups.size();++g) {
        const auto &group=groups[g];const auto &bound=bindings[g];
        const bool hologram=std::ranges::any_of(bound.ordered_layers,[](const auto &layer) {
            return layer.layer==21 && !layer.parts.empty();
        });
        if(!hologram)continue;
        if(!group.geometry)throw FormatError("hologram group has no owned post-cull geometry/range bank");
        SourceHologramGroupDraw output{&cache_,camera,group.queue_command,group.shader_clock,bound.owners,{},
            constants,raster,clip,fog,surface};
        for(const auto &layer:bound.ordered_layers)for(const auto &part:layer.parts) {
            if(part.base_unit>=units.size())throw FormatError("hologram descriptor escaped its retained main unit bank");
            const auto &base=units[part.base_unit];
            const auto &command=group.commands.at(part.command_ordinal-group.first_command).words;
            const auto &descriptor=group.ordered_layers.at(unsigned(&layer-bound.ordered_layers.data())).descriptors.at(part.sorted_ordinal);
            SourceHologramGroupPart selected{layer.layer,part.command_ordinal,part.sorted_ordinal,base.ordinal,
                unsigned(base.unit.words[3]>>9&511u),command[3]&255u,descriptor[2]&65535u,base.object,part.pass};
            const auto key=std::pair{base.unit.words[1],selected.section};
            auto geometry=geometry_.find(key);
            if(geometry==geometry_.end())geometry=geometry_.emplace(key,std::make_shared<const MenuWorldSourceSection>(
                load_menu_world_source_model_section(cache_,key.first,key.second))).first;
            const auto range_geometry=source_model_range_geometry(*geometry->second);
            if(base.geometry.geometry.parts!=range_geometry.parts)
                throw FormatError("hologram raw geometry changed its retained native part bank");
            const auto &native=group.geometry->units.at(base.ordinal);
            const auto ranges=source_effect_light_part_ranges(native,range_geometry,
                selected.part,group.geometry->arena,group.geometry->arena_address);
            if(layer.layer==21) {
                const auto slot=materials_.find({part.pass->shader(),part.pass->choice().shader_lod});
                if(slot==materials_.end() || part.pass->choice().pass_in_layer || part.pass->choice().implementation)
                    throw FormatError("hologram selected pass was not admitted during authored preload");
                selected.hologram_material=slot->second;
                for(const auto range:ranges.ranges)if(range.count) {
                    auto draw=menu_world_source_draw(geometry->second,selected.part,ranges.topology,base.palette_rows,range);
                    if(draw.triangle_count())selected.hologram_ranges.push_back(std::move(draw));
                }
            } else {
                if(base.override_slot!=255)throw FormatError("non-hologram override requires its own group material owner");
                const auto whole=menu_world_source_draw(geometry->second,selected.part,ranges.topology,base.palette_rows);
                if(ranges.ranges!=std::vector{whole.index_range()})
                    throw FormatError("filtered hologram body part requires its native CPU range consumer");
                for(std::size_t triangle=0;triangle<whole.triangle_count();++triangle) {
                    const auto vertices=whole.triangle(triangle);
                    if(vertices[0]!=vertices[1] && vertices[0]!=vertices[2] && vertices[1]!=vertices[2])
                        selected.body_vertices+=3;
                }
                selected.geometry_lod=base.lod;selected.body_source=whole;
                const auto origin_key=std::tuple{key.first,base.lod,selected.section};
                auto model=model_origins_.find(origin_key);
                if(model==model_origins_.end()) {
                    const std::array sections{selected.section};
                    auto geometry=std::make_shared<const RenderModel>(
                        cache_.load_model_sections(key.first,false,0,base.lod,sections));
                    model=model_origins_.emplace(origin_key,std::shared_ptr<const SourceHologramBodyGeometry>(
                        new SourceHologramBodyGeometry(cache_,key.first,base.lod,selected.section,std::move(geometry)))).first;
                }
                selected.body_geometry=model->second;
            }
            output.parts.push_back(std::move(selected));
        }
        result.push_back(std::move(output));
    }
    return result;
}
void validate_source_hologram_group_submissions(std::span<const SourceHologramGroupDraw> groups,
    std::span<const SourceHologramGroupSubmission> events) {
    std::size_t event=0;
    for(unsigned g=0;g<groups.size();++g) {
        // Sort order between groups is owned by1E370, so group IDs need not be
        // increasing. Locate this group's contiguous ledger before comparing.
        const auto begin=std::ranges::find_if(events,[&](const auto &e){return e.group==g;});
        if(begin==events.end() || begin->kind!=SourceHologramGroupSubmission::Kind::scene_copy)
            throw FormatError("hologram group did not copy target19 before its body");
        auto at=begin+1;++event;
        for(unsigned d=0;d<groups[g].parts.size();++d) {
            const auto &part=groups[g].parts[d];const unsigned count=part.layer==21?
                unsigned(std::max<std::size_t>(part.hologram_ranges.size(),1)):1u;
            for(unsigned r=0;r<count;++r) {
                if(at==events.end() || at->kind!=SourceHologramGroupSubmission::Kind::part || at->group!=g ||
                   at->descriptor!=d || at->layer!=part.layer || at->object!=part.object ||
                   at->shader!=part.pass->shader() || at->range!=r)
                    throw FormatError("hologram actual submission dropped/reordered a source descriptor");
                if(part.layer==21) {
                    // MenuWorldSourceStorage expands every source triangle,
                    // including degenerates, into its ordered ushort list.
                    const auto indices=part.hologram_ranges.empty()?0:part.hologram_ranges[r].triangle_count()*3u;
                    if(at->index_count!=indices || (indices && (!at->pipeline || !at->descriptor_set ||
                        !at->indexed || !at->index_buffer || at->first_index || at->vertex_offset || (at->index_offset&1u))))
                        throw FormatError("hologram raw submission lost its actual range/pipeline/descriptor");
                } else if(at->index_count!=part.body_vertices || !at->index_count || !at->pipeline || !at->descriptor_set)
                    throw FormatError("hologram body descriptor did not produce an actual draw");
                ++at;++event;
            }
        }
        if(at!=events.end() && at->group==g)throw FormatError("hologram group submitted an extra descriptor or scene copy");
    }
    if(event!=events.size())throw FormatError("hologram submission ledger contains an unowned group");
}
}
