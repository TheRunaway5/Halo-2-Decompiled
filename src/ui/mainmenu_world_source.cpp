#include "halo2/mainmenu_world_source.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>

namespace halo2 {
namespace {
const Tag &tag(const Cache &cache,std::uint32_t datum,const char *group) {
    const auto found=cache.query_tag({.handle=TagHandle{datum}, .group=TagGroup::from_string(group)});
    if(!found)throw FormatError("mainmenu raw pass dependency identity is missing");
    return *found;
}
std::optional<ShaderRange> layer(const Material &material,unsigned index) {
    if(material.selected_shader_lod>=material.lods.size())
        throw FormatError("mainmenu raw material LOD escapes its template");
    const auto &lod=material.lods[material.selected_shader_lod];
    if(!(lod.available_layers&(1u<<index)))return {};
    if(index>=lod.layers.count() || lod.layers.first()+index>=material.layers.size())
        throw FormatError("mainmenu raw material layer escapes its LOD");
    return material.layers[lod.layers.first()+index];
}
MenuWorldSourceConstants constants(const MenuWorldSourceConstants &view,const Material &material) {
    auto result=view;
    std::copy(material.program_constants[1].begin(),material.program_constants[1].end(),result.begin()+18);
    return result;
}
std::optional<MainmenuWorldSourceReject> binding_rejection(const MenuWorldSourceBinding &binding) {
    if(!binding.external_attribute_registers().empty())return MainmenuWorldSourceReject::external_attribute;
    for(const auto index:binding.consumed_constants())
        for(const auto value:binding.constants()[index])
            if(!std::isfinite(value))return MainmenuWorldSourceReject::missing_constant;
    return {};
}
}

struct MainmenuWorldSourcePlan::Impl {
    const Cache &cache;
    const SourceBspResidentPlan &resident;
    std::size_t bsp;
    std::map<unsigned,MenuWorldSourceBspOwner> owners;
    std::map<unsigned,std::shared_ptr<const MenuWorldSourceSection>> sections;
    using PassKey=std::tuple<std::uint32_t,unsigned,unsigned,unsigned,unsigned>;
    std::map<PassKey,MenuWorldSourcePass> passes;
    std::map<std::uint32_t,bool> class1_supported;
    Impl(const Cache &cache,const SourceBspResidentPlan &resident,std::size_t bsp)
        :cache(cache),resident(resident),bsp(bsp) {
        if(bsp>=cache.bsps().size() || cache.load_source_geometry_scenario_type()!=2)
            throw FormatError("mainmenu raw producer requires an actual source type2 BSP");
    }
    const MenuWorldSourceBspOwner &owner(unsigned instance) {
        const auto found=owners.find(instance);
        if(found!=owners.end())return found->second;
        return owners.emplace(instance,load_menu_world_source_bsp_owner(cache,bsp,instance)).first->second;
    }
    std::shared_ptr<const MenuWorldSourceSection> section(unsigned definition) {
        const auto found=sections.find(definition);
        if(found!=sections.end())return found->second;
        return sections.emplace(definition,std::make_shared<const MenuWorldSourceSection>(
            load_menu_world_source_bsp_section(cache,bsp,
                MenuWorldSourceSectionKind::bsp_instance_definition,definition))).first->second;
    }
    const MenuWorldSourcePass &pass(std::uint32_t shader,const MenuWorldSourcePassChoice &choice) {
        const PassKey key{shader,choice.shader_lod,choice.layer,choice.pass_in_layer,choice.implementation};
        const auto found=passes.find(key);
        if(found!=passes.end())return found->second;
        return passes.emplace(key,load_menu_world_source_pass(cache,shader,choice)).first->second;
    }
    bool class1(std::uint32_t vertex) {
        const auto found=class1_supported.find(vertex);
        if(found!=class1_supported.end())return found->second;
        const auto payload=cache.load_source_tag_metadata(vertex);
        const Bytes header(payload.bytes_at(payload.source_address(),12));
        if(header.u16(0)!=1 || header.u32(4)<=1)
            return class1_supported.emplace(vertex,false).first->second;
        const auto declaration=cache.load_source_vertex_declaration(vertex,1);
        bool supported=!declaration.semantics.empty() && declaration.semantics.size()<=16;
        for(const auto semantic:declaration.semantics) {
            switch(SourceVertexSemantic(semantic)) {
            case SourceVertexSemantic::Position:case SourceVertexSemantic::UV:
            case SourceVertexSemantic::Normal:case SourceVertexSemantic::Tangent:
            case SourceVertexSemantic::Binormal:break;
            default:supported=false;break;
            }
            if(std::count(declaration.semantics.begin(),declaration.semantics.end(),semantic)!=1)supported=false;
        }
        const auto bytes=cache.load_vertex_shader_code(vertex,1);
        if(bytes.empty())supported=false;
        else try {supported&=!decode_vertex_program(bytes).relative_constants;}
        catch(const UnsupportedVertexProgram &) {supported=false;}
        return class1_supported.emplace(vertex,supported).first->second;
    }
    void matching_pass(const Material &material,const MaterialPass &selected,
                       const MenuWorldSourcePass &resolved) const {
        if(selected.template_pass!=resolved.template_pass() ||
           selected.implementation!=resolved.choice().implementation ||
           selected.name!=tag(cache,resolved.shader_pass(),"spas").name ||
           selected.vertex_shader!=tag(cache,resolved.vertex_shader(),"vrtx").name)
            throw FormatError("mainmenu raw selected pass differs from original16B90 resolution");
        // Material imports class0. The raw binding imports class1 independently
        // from this same resolved vrtx; comparing the two classes is invalid.
        const auto code=cache.load_vertex_shader_code(resolved.vertex_shader(),0);
        if(material.vertex_program!=code || selected.vertex_code!=code ||
           material.vertex_declaration!=selected.vertex_declaration)
            throw FormatError("mainmenu raw selected native program differs from its resolved vrtx");
        if(selected.vertex_declaration && *selected.vertex_declaration!=
            cache.load_source_vertex_declaration(resolved.vertex_shader(),0))
            throw FormatError("mainmenu raw selected native declaration differs from its resolved vrtx");
    }
    const MenuWorldSourcePass *base_pass(const Material &material) {
        const auto range=layer(material,3);
        if(!range || range->count()!=1 || !material.world_pass || !material.forward_pass ||
           material.forward_vertex!=6 || material.accumulation_texture || material.environment_pass ||
           material.static_lightmap_pass || material.baked_lightmap_texture)
            return nullptr;
        if(material.forward_pass_index>=material.passes.size())
            throw FormatError("mainmenu raw selected base pass index escapes material");
        const auto &selected=material.passes[material.forward_pass_index];
        if(selected.implementation!=3)return nullptr;
        if(selected.template_pass!=range->first())
            throw FormatError("mainmenu raw selected classification3 pass is outside layer3");
        const auto &resolved=pass(material.source_shader_datum,{material.selected_shader_lod,3,0,3});
        if(resolved.lod_layers().packed!=material.lods[material.selected_shader_lod].layers.packed ||
           resolved.layer_passes().packed!=range->packed ||
           material.template_name!=tag(cache,resolved.shader_template(),"stem").name)
            throw FormatError("mainmenu raw base shader template differs from resolved source metadata");
        matching_pass(material,selected,resolved);
        return &resolved;
    }
    const MenuWorldSourcePass *fog_pass(const Material &parent,const Material &child,unsigned implementation) {
        const auto range=layer(parent,16);
        if(!range || range->count()!=1 || !child.forward_pass || child.forward_vertex!=5 || child.forward_blend!=5)
            return nullptr;
        if(child.forward_pass_index>=child.passes.size())
            throw FormatError("mainmenu raw selected fog pass index escapes material");
        const auto &selected=child.passes[child.forward_pass_index];
        const auto &resolved=pass(parent.source_shader_datum,{parent.selected_shader_lod,16,0,implementation});
        if(resolved.layer_passes().packed!=range->packed)
            throw FormatError("mainmenu raw fog layer differs from resolved source metadata");
        // Existing planar children retain their pass but do not all populate
        // root shader/LOD fields. A populated identity must still agree.
        if(child.source_shader_datum!=0xffffffffu &&
           (child.source_shader_datum!=parent.source_shader_datum ||
            child.selected_shader_lod!=parent.selected_shader_lod))
            throw FormatError("mainmenu raw fog child differs from its source shader owner");
        matching_pass(child,selected,resolved);
        return &resolved;
    }
    MainmenuWorldSourceFrame prepare(const MainmenuBspAdmissionFrame &frame,
        std::span<const Material> materials,const MenuWorldSourceConstants &view_constants,
        const MainmenuBspRangeFrame *source_ranges=nullptr) {
        const auto selected=resident.select(frame.shader_view(),frame.cinematic);
        const auto &catalogue=resident.parts();
        if(selected.bindings.size()!=catalogue.size() || selected.decisions.size()!=catalogue.size())
            throw FormatError("mainmenu raw resident selection differs from catalogue extent");
        MainmenuWorldSourceFrame result;
        const auto reject=[&](MainmenuWorldSourceReject reason){++result.rejected[reason];};
        for(std::size_t index=0;index<catalogue.size();++index) {
            const auto &entry=catalogue[index];const auto &key=entry.original;
            const auto &decision=selected.decisions[index];const auto &binding=selected.bindings[index];
            if(decision.state!=SourceBspPartDecision::accepted) {reject(MainmenuWorldSourceReject::not_selected);continue;}
            if(!decision.selection || !binding.material)
                throw FormatError("mainmenu raw accepted resident part lacks its selection/binding");
            const auto &choice=*decision.selection;
            if(choice.geometry!=key.geometry || choice.section!=key.section || choice.part!=key.part ||
               choice.material!=*binding.material || choice.material>=materials.size())
                throw FormatError("mainmenu raw accepted part identity differs from its resident key");
            if(key.geometry>-2) {reject(MainmenuWorldSourceReject::non_instance);continue;}
            const auto instance=unsigned(-std::int64_t(key.geometry)-2);
            const auto *record=frame.admission.instance(instance);
            if(!record)throw FormatError("mainmenu raw selected instance is absent from the actual queue");
            // The live route still requires0800. The explicit CPU-prefix
            // route consumes44720's completed masks through ordered4DAA0
            // ranges; incomplete cannot silently become a whole source part.
            const MainmenuBspPartRanges *part_ranges=nullptr;
            if(source_ranges) {
                if(!source_ranges->complete) {reject(MainmenuWorldSourceReject::incomplete_bsp_ranges);continue;}
                const auto *unit=source_ranges->unit_for_geometry(key.geometry);
                if(!unit || unit->section!=key.section || key.part>=unit->parts.size() ||
                   unit->parts[key.part].part!=key.part || (unit->flags>>29)!=3u ||
                   unit->alpha!=choice.instance_alpha)
                    throw FormatError("mainmenu raw range unit differs from accepted source owner/section/alpha");
                part_ranges=&unit->parts[key.part];
                if(part_ranges->topology!=MenuWorldSourceTopology::triangle_list)
                    throw FormatError("mainmenu normal BSP range producer supplied a non-list part");
            } else if(!(record->flags&0x0800)) {reject(MainmenuWorldSourceReject::partial_instance);continue;}
            if(choice.instance_alpha!=255 || !binding.stipple || *binding.stipple!=16) {
                reject(MainmenuWorldSourceReject::instance_fade);continue;
            }
            const auto &material=materials[choice.material];
            if(material.selected_shader_lod!=choice.shader_lod)
                throw FormatError("mainmenu raw material differs from accepted source shader LOD");
            const auto &authored_owner=owner(instance);
            if(authored_owner.section!=key.section || authored_owner.bsp_datum!=cache.bsps()[bsp].datum)
                throw FormatError("mainmenu raw resident section differs from the actual BSP owner");
            const auto geometry=section(authored_owner.section);
            if(key.part>=geometry->parts().size())throw FormatError("mainmenu raw part escapes authored section");
            if(geometry->parts()[key.part].shader!=material.source_shader_datum)
                throw FormatError("mainmenu raw selected material differs from authored source part shader");
            const auto *base=base_pass(material);
            if(!base) {reject(MainmenuWorldSourceReject::unsupported_base_pass);continue;}
            if(!class1(base->vertex_shader())) {reject(MainmenuWorldSourceReject::unsupported_class1);continue;}
            const MenuWorldSourceOwner identity{MenuWorldSourceOwnerKind::bsp_instance,
                authored_owner.bsp_datum,instance,{}};
            std::shared_ptr<const Material> fog_child;
            const MenuWorldSourcePass *fog_resolved=nullptr;
            const auto implementation=bsp_planar_fog_implementation(frame.fog,
                frame.admission.geometry_contains_fog(key.geometry));
            if(implementation && layer(material,16)) {
                auto child=material.world_planar_passes.at(*implementation);
                if(!child && *implementation==0)child=material.planar_fog_pass;
                if(!child) {reject(MainmenuWorldSourceReject::missing_fog_material);continue;}
                const auto *resolved=fog_pass(material,*child,*implementation);
                if(!resolved) {reject(MainmenuWorldSourceReject::unsupported_fog_pass);continue;}
                if(!class1(resolved->vertex_shader())) {reject(MainmenuWorldSourceReject::unsupported_class1);continue;}
                fog_child=std::move(child);fog_resolved=resolved;
            }
            std::vector<MainmenuWorldSourceRange> ranges;
            std::optional<MainmenuWorldSourceReject> failure;
            const auto append=[&](std::optional<MenuWorldSourceIndexRange> indices) {
                // Actual normal BSP collector cannot emit unit0200.4DAA0
                // consumes literal triangle-list indices and degenerates.
                const auto draw=menu_world_source_draw(geometry,key.part,MenuWorldSourceTopology::triangle_list,
                    std::span(&authored_owner.rows,1),indices);
                auto raw_base=menu_world_source_binding(cache,identity,draw,base->vertex_shader(),constants(view_constants,material));
                if(const auto reason=binding_rejection(raw_base)) {failure=reason;return;}
                std::optional<MainmenuWorldSourceFog> fog;
                if(fog_child) {
                    auto raw_fog=menu_world_source_binding(cache,identity,draw,fog_resolved->vertex_shader(),constants(view_constants,*fog_child));
                    if(const auto reason=binding_rejection(raw_fog)) {failure=reason;return;}
                    fog.emplace(MainmenuWorldSourceFog{*implementation,fog_child,std::move(raw_fog),*fog_resolved});
                }
                ranges.push_back({std::move(raw_base),*base,std::move(fog)});
            };
            if(part_ranges) {
                for(const auto indices:part_ranges->ranges) {append(indices);if(failure)break;}
            } else append({});
            if(failure) {reject(*failure);continue;}
            result.candidates.push_back({index,key,choice.material,std::move(ranges)});
        }
        return result;
    }
};
MainmenuWorldSourcePlan::MainmenuWorldSourcePlan(const Cache &cache,const SourceBspResidentPlan &resident,std::size_t bsp)
    :impl_(std::make_unique<Impl>(cache,resident,bsp)) {}
MainmenuWorldSourcePlan::~MainmenuWorldSourcePlan()=default;
MainmenuWorldSourceFrame MainmenuWorldSourcePlan::prepare(const MainmenuBspAdmissionFrame &frame,
    std::span<const Material> materials,const MenuWorldSourceConstants &constants) {
    return impl_->prepare(frame,materials,constants);
}
MainmenuWorldSourceFrame MainmenuWorldSourcePlan::prepare_bsp_prefix(const MainmenuBspAdmissionFrame &frame,
    const MainmenuBspRangeFrame &ranges,std::span<const Material> materials,const MenuWorldSourceConstants &constants) {
    return impl_->prepare(frame,materials,constants,&ranges);
}
std::string_view mainmenu_world_source_reject_name(MainmenuWorldSourceReject reason) {
    switch(reason) {
    case MainmenuWorldSourceReject::not_selected:return "not_selected";
    case MainmenuWorldSourceReject::non_instance:return "non_instance";
    case MainmenuWorldSourceReject::partial_instance:return "partial_instance";
    case MainmenuWorldSourceReject::incomplete_bsp_ranges:return "incomplete_bsp_ranges";
    case MainmenuWorldSourceReject::instance_fade:return "instance_fade";
    case MainmenuWorldSourceReject::unsupported_base_pass:return "unsupported_base_pass";
    case MainmenuWorldSourceReject::unsupported_fog_pass:return "unsupported_fog_pass";
    case MainmenuWorldSourceReject::missing_fog_material:return "missing_fog_material";
    case MainmenuWorldSourceReject::unsupported_class1:return "unsupported_class1";
    case MainmenuWorldSourceReject::external_attribute:return "external_attribute";
    case MainmenuWorldSourceReject::missing_constant:return "missing_constant";
    }
    throw FormatError("unknown mainmenu raw producer rejection reason");
}
} // namespace halo2
