#include "halo2/source_campaign_weapons.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_campaign_view_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_weapon_material_input.hpp"
#include "halo2/source_weapon_firing_material_owner.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/simulation.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <set>

namespace halo2 {
SourceCampaignWeapons::SourceCampaignWeapons(const Cache &cache,const LevelContent &content,Mesh &mesh)
    :cache_(&cache),names_(SourceWeaponMaterialNames::from_cache(cache)),tables_(cache.shader_function_tables()) {
    for(const auto &tag:cache.select_tags({.group=tag_groups::weapon,.require_address=true})) {
        auto definition=cache.load_source_world_weapon_definition(tag.datum);
        if(definition.render_model==null_datum)continue;
        Resource resource;resource.definition=std::move(definition);
        if(const auto weapon=content.weapons.find(tag.datum);
           weapon!=content.weapons.end() && !weapon->second.triggers.empty())
            resource.primary_charge_seconds=weapon->second.triggers.front().charging_seconds;
        try {
            auto model=std::make_unique<SourceCampaignModel>(cache,tag.datum);
            Mesh candidate;model->prepare_materials(cache,candidate);
            model->rebase_materials(0,unsigned(mesh.materials.size()));
            mesh.materials.insert(mesh.materials.end(),std::make_move_iterator(candidate.materials.begin()),
                std::make_move_iterator(candidate.materials.end()));
            resource.model=std::move(model);
        } catch(const FormatError &error) {
            // An unused authored definition cannot partially publish materials
            // or prevent unrelated owners from loading. Reaching it as an
            // actual original main unit reports its unresolved resource owner.
            resource.failure=error.what();
        }
        resources_.emplace(tag.datum,std::move(resource));
    }
}
void SourceCampaignWeapons::rebase_materials(unsigned before,unsigned after) {
    for(auto &[definition,resource]:resources_)if(resource.model)resource.model->rebase_materials(before,after);
    poses_.clear();
}
SourceCampaignWeaponFrame SourceCampaignWeapons::draw(MultiplayerObjectRuntime &world,
    const SourceCampaignViewFrameTu15 &frame,const SourceModelShaderView &view,
    std::uint64_t tick,float fraction,float seconds) {
    validate_render_fraction(fraction);
    SourceCampaignWeaponFrame result;std::set<std::uint32_t> seen;
    std::map<std::uint32_t,std::vector<Transform>> rendered;
    struct Inputs {
        SourceWeaponShaderState shader;
        std::uint16_t damage_flags{};
        std::vector<SourceWeaponMagazineState> magazines;
        std::optional<FunctionValue> primary;
        std::optional<SourceWeaponFiringMaterialState> firing;
    };
    std::map<std::uint32_t,Inputs> inputs;
    const auto &memory=world.memory();
    const auto read=[&]<class T>(NativeRuntimeAddress at) {
        T value;const auto bytes=memory.view(at,sizeof(T));std::memcpy(&value,bytes.data(),sizeof(T));return value;
    };
    for(const auto &unit:frame.base_draws) {
        const auto tag=cache_->query_tag({.handle=TagHandle{unit.definition},.require_address=true});
        if(!tag || tag->group!="weap")continue;
        if(!frame.drawn.contains(unit.object) || world.object_definition(unit.object)!=unit.definition)
            throw FormatError("Campaign weapon main unit lost its full source owner/definition");
        if(unit.override_slot!=255) {
            result.pending_overrides.push_back({unit.object,unit.shader_override,unit.ordinal,unit.lighting_class});
            continue;
        }
        if(unit.lighting_class!=2 || !(unit.unit.words[0]&1u))
            throw FormatError("Campaign weapon ordinary main unit requires an unresolved lighting class");
        if(!unit.geometry.geometry.resident)continue;
        const auto found=resources_.find(unit.definition);
        if(found==resources_.end())throw FormatError("Campaign weapon source unit has no authored world render model");
        auto &resource=found->second;
        if(!resource.model)throw FormatError("Campaign weapon source resource unresolved: "+resource.failure);
        if(seen.insert(unit.object).second) {
            const auto built=frame.built.find(unit.object);
            if(built==frame.built.end() || Bytes(built->second.bytes).u32(0)!=unit.object)
                throw FormatError("Campaign weapon has no completed original model collector");
            const auto nodes=world.rendered_world_nodes(unit.object);
            if(!nodes || nodes->empty())throw FormatError("Campaign weapon lost its original rendered WORLD nodes");
            const auto body=world.object_body(unit.object);
            const auto parent=read.operator()<std::uint32_t>(body+0x14);
            auto [at,inserted]=poses_.try_emplace(unit.object);
            auto &pose=at->second;
            if(!inserted && (pose.definition!=unit.definition || pose.parent!=parent || tick<pose.tick))pose.history={};
            pose.definition=unit.definition;pose.parent=parent;pose.tick=tick;
            pose.history.capture_render(tick,*nodes);
            rendered.emplace(unit.object,pose.history.sample_render(resource.model->hierarchy(),fraction,{},resource.model->scale_resets()));
            Inputs actual;
            actual.damage_flags=read.operator()<std::uint16_t>(body+0x10au);
            actual.firing=source_weapon_firing_material_owner(world,unit.object,resource.definition);
            actual.shader={read.operator()<std::uint32_t>(body+0x12c),read.operator()<std::uint32_t>(body+0x16c),
                read.operator()<float>(body+0x184),read.operator()<float>(body+0x180)};
            if(resource.definition.magazines.size()>2)
                throw FormatError("Campaign weapon authored magazines exceed the original fixed bank");
            for(unsigned n=0;n<resource.definition.magazines.size();++n)
                actual.magazines.push_back({read.operator()<std::int16_t>(body+0x22c+n*16),
                    read.operator()<std::int16_t>(body+0x22a+n*16)});
            if(resource.primary_charge_seconds)
                actual.primary=source_weapon_primary_charge(read.operator()<std::uint8_t>(body+0x20c),
                    read.operator()<std::int16_t>(body+0x20e),*resource.primary_charge_seconds,tick_seconds);
            inputs.emplace(unit.object,std::move(actual));
        }
        auto vertices=resource.model->draw(unit.object,std::span{&unit,1},rendered.at(unit.object),view);
        if(vertices.empty())continue;
        auto input=[actual=inputs.at(unit.object),object=unit.object,definition=resource.definition,tables=tables_,names=names_,seconds](std::uint32_t name) {
            if(!name)return 0.f;
            const auto value=source_weapon_material_input(name,actual.shader,definition.magazines,
                actual.magazines,definition.functions,*tables,seconds,actual.primary,names,object,actual.damage_flags,actual.firing);
            if(!value)throw FormatError("Campaign weapon material requires an unresolved original producer: "+std::to_string(name));
            return value->value;
        };
        result.draws.push_back({unit.object,unit.definition,unit.ordinal,std::move(vertices),unit.lighting,
            source_live_model_base_colors_tu15(unit,{{{1,1,1},{1,1,1},{1,1,1},{1,1,1}}}),std::move(input)});
    }
    std::erase_if(poses_,[&](const auto &entry){return !seen.contains(entry.first);});
    return result;
}
}
