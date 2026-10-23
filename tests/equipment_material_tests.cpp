#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/world_material.hpp"
#include "halo2/source_camouflage_material.hpp"
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void check(bool ok,const std::string &why){if(!ok)throw FormatError(why);}
}
int main(int argc,char **argv)try {
    check(argc==3 || argc==4,"Supply disc/map-pack directories, optionally preceded by the prepared MCC maps directory");
    const bool mcc=argc==4;
    const std::array names{"ascension","beavercreek","burial_mounds","coagulation","colossus",
        "cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar",
        "containment","warlock","deltatap","turf","backwash","dune","elongation","gemini","triplicate"};
    unsigned definitions{},models{},lods{},overshields{},camouflage_passes{},inert_ammo{};
    std::set<unsigned> powerup_types;
    for(unsigned map=0;map<names.size();++map) {
        const auto filename=std::string(names[map])+".map";
        const auto path=std::filesystem::path(argv[mcc?1:map<12?1:2])/filename;
        if(!std::filesystem::is_regular_file(path))return 77;
        Cache cache(path);std::set<std::uint32_t> shaders;
        std::set<std::string> authored,matched;
        if(mcc) {
            const auto original=std::filesystem::path(argv[map<12?2:3])/filename;
            if(!std::filesystem::is_regular_file(original))return 77;
            Cache reference(original);
            for(const auto &tag:reference.select_tags())if(tag.group=="eqip")authored.insert(tag.name);
        }
        for(const auto &tag:cache.select_tags())if(tag.group=="eqip" && (!mcc || authored.contains(tag.name))) {
            matched.insert(tag.name);
            const auto metadata=cache.load_source_tag_metadata(tag.datum);
            const Bytes definition(metadata.bytes_at(metadata.source_address(),metadata.size()));
            check(definition.u16(0)==3,"Equipment tag has a different object type");
            powerup_types.insert(definition.u16(0x12c));++definitions;
            if(definition.u16(0x12c)==0) {
                check(definition.f32(0x130)==0.f,"Original ammo box acquired an unproved powerup duration");++inert_ammo;
            }
            if(definition.u32(0x38)==0xffffffffu)continue;
            const auto model=cache.load_initial_model_definition(tag.datum);
            if(model.render_model==0xffffffffu)continue;
            const auto geometry=cache.load_model(model.render_model,true);++models;
            for(const auto &material:geometry.mesh.materials)
                if(material.source_shader_datum!=0xffffffffu)shaders.insert(material.source_shader_datum);
        }
        if(mcc)check(matched==authored,"Prepared MCC cache is missing original equipment definitions");
        for(const auto shader:shaders) {
            const auto authored=cache.load_material(shader);
            for(unsigned lod=0;lod<authored.lods.size();++lod) {
                auto material=authored;material.selected_shader_lod=lod;
                check(prepare_dynamic_material(cache,material,{},true,0.f),
                    std::string(names[map])+": equipment shader binding failed: "+authored.name+
                    " LOD "+std::to_string(lod));
                check(material.forward_pass,"Equipment material has no submitted base pass");++lods;
            }
        }
        const auto content=cache.load_content(0,true);
        for(unsigned species:{2u,3u})for(bool fp:{false,true}) {
            const auto &v=content.player_representations.at(species).biped.vitality;
            auto material=cache.load_material(fp?v.overshield_first_person_shader:v.overshield_shader);
            check(prepare_shield_overlay_material(cache,material),
                std::string(names[map])+": authored overshield overlay was rejected");
            bind_object_camouflage(material,.75f);bind_object_camouflage(material,0.f);++overshields;
            const auto &representation=content.player_representations.at(species);
            const auto model=cache.load_model(fp?representation.hands:representation.biped.model,true);
            for(const auto &original:model.mesh.materials)if(original.source_shader_datum!=0xffffffffu) {
                for(unsigned implementation=0;implementation<4;++implementation) {
                    auto bound=prepare_camouflage_material(cache,original,18,implementation,.75f);
                    check(bound.forward_pass && bound.camouflage_group_pass &&
                        bound.camouflage_scene_texture==(implementation!=3),"Camouflage texture dependency changed");
                    bind_object_camouflage(bound,0.f);++camouflage_passes;
                }
                for(unsigned layer:{1u,19u}) {
                    const auto bound=prepare_camouflage_material(cache,original,layer,0,.75f);
                    check(bound.forward_pass && bound.camouflage_group_pass && !bound.camouflage_scene_texture,
                        "Camouflage depth/fog pass changed its authored scene dependency");++camouflage_passes;
                }
            }
        }
    }
    std::cout<<definitions<<" equipment models across "<<names.size()<<" maps; "<<lods
        <<" authored pickup shader LODs, "<<overshields<<" overshield bindings, "<<camouflage_passes
        <<" camouflage pass bindings and "<<inert_ammo<<" zero-duration ammo definitions passed\n";
    check(definitions==231 && models==231 && overshields==84 && lods>0 &&
        inert_ammo==147 && camouflage_passes>0 && powerup_types==std::set<unsigned>{0,2,3,6},"Incomplete all-map equipment material coverage");
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
