#include "halo2/pstencil/point_caster.hpp"
#include "halo2/pstencil/caster.hpp"
#include "halo2/source_draw_unit.hpp"
#include "halo2/source_effect_light_opaque.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/effect_light_admission.hpp"
#include "halo2/source_bsp_admission.hpp"
#include <algorithm>
#include <map>
#include <tuple>

namespace halo2::pstencil {
struct PointCasterBank::Impl {
    const Cache& cache;unsigned bsp;
    SourceBspAdmissionBsp geometry;
    std::uint32_t position_shader{},position_classes{};
    struct Section {
        std::shared_ptr<const MenuWorldSourceSection> raw;
        SourceOpaqueGeometryMetadata metadata;
    };
    using Key=std::tuple<MenuWorldSourceSectionKind,std::uint32_t,unsigned>;
    std::map<Key,Section> sections;
    Impl(const Cache& source,unsigned index):cache(source),bsp(index),geometry(cache.load_source_bsp_admission(bsp)) {
        const auto shader=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto& tag){
            return tag.group=="vrtx" && tag.name.ends_with("\\lightaccum_sphere_diffuse");
        });
        if(shader==cache.tags().end())throw FormatError("pStencil source position program is absent");
        position_shader=shader->datum;
        const auto owned=cache.load_source_tag_metadata(position_shader);
        position_classes=Bytes(owned.bytes_at(owned.source_address(),12)).u32(4);
    }
    const Section& section(MenuWorldSourceSectionKind kind,std::uint32_t model,unsigned ordinal) {
        const Key key{kind,model,ordinal};auto found=sections.find(key);
        if(found==sections.end()) {
            Section value;
            if(kind==MenuWorldSourceSectionKind::render_model) {
                value.raw=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_model_section(cache,model,ordinal));
                value.metadata=load_source_opaque_model_geometry_metadata(cache,model,ordinal);
            } else {
                value.raw=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,bsp,kind,ordinal));
                value.metadata=load_source_opaque_bsp_geometry_metadata(cache,bsp,kind,ordinal);
            }
            found=sections.emplace(key,std::move(value)).first;
        }
        return found->second;
    }
};
PointCasterBank::PointCasterBank(const Cache& cache,unsigned bsp):impl_(std::make_unique<Impl>(cache,bsp)){}
PointCasterBank::~PointCasterBank()=default;
void PointCasterBank::collect(const SourceDrawUnitState& units,PhysicsLiveWorldTu15* world,
    const SourceEffectLightRuntime& light,const MenuWorldSourceConstants& constants,
    std::uint32_t domain,const std::function<bool(const SourceCullDrawUnit&)>& color_available,
    std::vector<PointCaster>& output) {
    auto& i=*impl_;
    for(unsigned n=units.main_prefix;n<units.draw_count;++n) {
        const auto& unit=units.units.at(n);
        const auto opacity=std::uint8_t(unit.words[6]>>16);
        if(!opacity)continue;
        const Impl::Section* section;
        MenuWorldSourceOwner owner;
        std::vector<std::array<float,12>> palette;
        bool available=true;
        if(unit.words[1]!=0xffffffffu) {
            if(!world)throw FormatError("pStencil point caster has no actual model world");
            section=&i.section(MenuWorldSourceSectionKind::render_model,unit.words[1],unit.words[3]>>9&511u);
            const auto geometry=source_live_model_command_geometry_tu15(*world,unit);
            palette=source_live_model_palette_rows_tu15(*world,unit,geometry);
            const auto object=Bytes(world->memory().view(unit.words[4],4)).u32(0);
            owner={MenuWorldSourceOwnerKind::dynamic_object,section->raw->owner_datum(),object&65535u,object};
            if(color_available)available=color_available(unit);
        } else {
            const bool instance=unit.words[0]&0x1000u;
            const auto ordinal=instance?(unit.words[3]>>18&2047u):(unit.words[3]>>9&511u);
            const auto definition=instance?unsigned(i.geometry.instances.at(ordinal).definition):ordinal;
            const auto kind=instance?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster;
            section=&i.section(kind,0xffffffffu,definition);
            owner={instance?MenuWorldSourceOwnerKind::bsp_instance:MenuWorldSourceOwnerKind::bsp_cluster,
                section->raw->owner_datum(),ordinal,{}};
            if(instance)palette.push_back(load_menu_world_source_bsp_owner(i.cache,i.bsp,ordinal).rows);
        }
        const auto& raw=section->raw;
        const auto& metadata=section->metadata;
        const auto vertex_class=source_opaque_vertex_class(metadata.geometry_class,metadata.selectors[0],
            [&]{return i.position_classes;});
        for(unsigned p=0;p<raw->parts().size();++p) {
            const auto& part=raw->parts()[p];
            if(!shadow_casting_part(part.type) || part.index_count<3)continue;
            const auto topology=(unit.words[0]&0x200u) && !(part.flags&8u)?
                MenuWorldSourceTopology::triangle_strip:MenuWorldSourceTopology::triangle_list;
            auto draw=raw->kind()==MenuWorldSourceSectionKind::bsp_cluster?
                menu_world_source_cluster_draw(raw,p,topology):menu_world_source_draw(raw,p,topology,palette);
            auto binding=menu_world_source_binding_class(i.cache,owner,std::move(draw),i.position_shader,vertex_class,constants);
            output.push_back({std::move(binding),light.identity,domain,light.position,opacity,available});
        }
    }
}
}
