#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/unit_animation_metadata_store.hpp"
#include "halo2/source_weapon_animation_query_symbols_tu15.hpp"
#include "halo2/physics_contact_record.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>

using namespace halo2;
namespace {
struct QueryBank final:SourcePhysicsContactMemory {
    void read(std::uint32_t,std::span<std::byte>)const override {throw FormatError("Namespace component must not read a source bank");}
    void write(std::uint32_t,std::span<const std::byte>)override {throw FormatError("Namespace component must not change a source bank");}
};
void check(bool value,const char *message) {if(!value)throw FormatError(message);}
auto digest(std::span<const std::byte> bytes) {
    std::array<unsigned char,SHA256_DIGEST_LENGTH> result{};
    SHA256(reinterpret_cast<const unsigned char *>(bytes.data()),bytes.size(),result.data());
    return result;
}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply prepared MCC map and bounded report path");
    Cache cache(argv[1]);check(cache.is_mcc(),"First-person acceptance requires MCC runtime content");
    QueryBank query_bank;
    SourceWeaponAnimationQuerySymbolsTu15 query_symbols(query_bank,true,[&](auto id){return cache.string_id(id);});
    for(auto pc:{0xead6au,0x1124b7u,0x1125d6u}) {
        check(source_weapon_animation_equip_class_symbol_tu15(query_bank,pc,0x0400054bu)==0x04000557u,
            "Compiled dual equip class lost its actual MCC name");
        check(source_weapon_animation_equip_class_symbol_tu15(query_bank,pc+1u,0x0400054bu)==0x0400054bu,
            "Dual equip class escaped its original operand boundary");
    }
    const auto graph_digest=[&](std::uint32_t datum) {
        const auto metadata=cache.load_source_tag_metadata(datum);
        return digest(metadata.bytes_at(metadata.source_address(),metadata.size()));
    };
    std::map<std::uint32_t,std::array<unsigned char,SHA256_DIGEST_LENGTH>> before;
    for(const auto &graph:cache.select_tags({.group=tag_groups::model_animation_graph,.require_address=true}))
        if(graph.name.find("\\fp_energy_blade\\")!=std::string::npos)
            before.emplace(graph.datum,graph_digest(graph.datum));
    check(before.size()==2,"MCC sword graph metadata coverage changed");
    unsigned bindings{},clips{},sword_actions{},foreign_controls{},body_contexts{},body_actions{},source_query_keys{},fp_query_keys{},reload_query_keys{},reload_timing_query_keys{};
    nlohmann::json rows=nlohmann::json::array();
    for(const auto &weapon:cache.select_tags({.group=tag_groups::weapon,.require_address=true})) {
        for(const int representation:{2,3}) {
            const auto definition=cache.load_first_person_binding(weapon.datum,representation);
            if(!definition.record_present || definition.graph==0xffffffffu)continue;
            ++bindings;clips+=definition.clock_clips.size()+definition.dual_clock_clips.size();
            for(const auto &[original,name]:std::array<std::pair<std::uint32_t,const char *>,4>{{
                {0x0c0005aeu,"reload_enter"},{0x150005afu,"reload_continue_empty"},
                {0x140005b0u,"reload_continue_full"},{0x0b0005b1u,"reload_exit"}}}) {
                const auto clip=std::find_if(definition.clock_clips.begin(),definition.clock_clips.end(),
                    [&](const auto &candidate){return candidate.activity==original;});
                if(clip==definition.clock_clips.end())continue;
                const auto expected=cache.load_action(definition.graph,"any","any","any",name);
                SourceWeaponAnimationQueryScopeTu15 request(query_bank,0x169a56u,original);
                const auto key=source_weapon_animation_scoped_query_symbol_tu15(query_bank,original);
                check(cache.string_id(key)==name,"FP staged reload lookup lost its MCC authored name");
                const auto selected=cache.load_unit_weapon_action(definition.graph,0x06000086u,
                    0x07000083u,0x030000d9u,key);
                check(selected.source_identifier==expected.source_identifier &&
                    first_person_clip_identifier(*clip)==expected.source_identifier,
                    "FP staged reload lookup selected a different authored clip");
                const auto timing_key=source_weapon_animation_query_symbol_tu15(query_bank,0x16b45du,original);
                check(cache.string_id(timing_key)==name,"Magazine timing lookup lost its MCC authored name");
                const auto timing=cache.load_unit_weapon_action(definition.graph,0x06000086u,
                    0x07000083u,0x030000d9u,timing_key);
                check(timing.source_identifier==expected.source_identifier,
                    "Magazine timing lookup selected a different authored clip");
                ++reload_timing_query_keys;
                ++reload_query_keys;
            }
            if(!weapon.name.ends_with("\\energy_blade"))continue;
            // Independent name lookup reads the authored rows. The source clock
            // must retain its original activity while selecting those exact clips.
            for(const auto &[original,name]:std::array<std::pair<std::uint32_t,const char *>,6>{{
                {0x0e000607u,"melee_strike_1"},{0x0e000608u,"melee_strike_2"},
                {0x0a0005bbu,"melee_dash"},{0x0b0005b2u,"melee_lunge"},
                {0x130005bcu,"melee_dash_airborne"},{0x140005b3u,"melee_lunge_airborne"}}}) {
                const auto expected=cache.load_action(definition.graph,"any","any","any",name);
                const auto selected=std::find_if(definition.clock_clips.begin(),definition.clock_clips.end(),
                    [&](const auto &clip){return clip.activity==original;});
                check(selected!=definition.clock_clips.end(),"MCC sword lost an authored source-clock activity");
                check(selected->index==expected.source_index &&
                    first_person_clip_identifier(*selected)==expected.source_identifier,
                    "Canonical sword activity selected a different authored packed clip");
                ++sword_actions;
                if(original==0x0e000607u || original==0x0e000608u) {
                    for(const auto caller:{0xebf41u,0xec154u,0xec230u}) {
                        const auto key=source_weapon_animation_query_symbol_tu15(query_bank,caller,original);
                        check(cache.string_id(key)==name,"Compiled live query lost its actual MCC authored name");
                        const auto through_defaults=cache.load_unit_weapon_action(definition.graph,0x06000086u,
                            0x07000083u,0x030000d9u,key);
                        check(through_defaults.source_identifier==expected.source_identifier,
                            "Live manager defaults/fallback select a different actual MCC strike");
                        ++source_query_keys;
                    }
                    check(source_weapon_animation_query_symbol_tu15(query_bank,0x12345678u,original)==original,
                        "Unqualified caller changed an authored query");
                }
                {
                    SourceWeaponAnimationQueryScopeTu15 request(query_bank,0x169a56u,original);
                    const auto key=source_weapon_animation_scoped_query_symbol_tu15(query_bank,original);
                    check(cache.string_id(key)==name,"FP record search lost its actual MCC name");
                    const auto through_defaults=cache.load_unit_weapon_action(definition.graph,0x06000086u,
                        0x07000083u,0x030000d9u,key);
                    check(through_defaults.source_identifier==expected.source_identifier,
                        "FP query scope selected a different authored Sword clip");
                    ++fp_query_keys;
                }
                check(source_weapon_animation_scoped_query_symbol_tu15(query_bank,original)==original,
                    "Completed FP query scope leaked into a raw authored request");
                rows.push_back({{"weapon",weapon.name},{"representation",representation},
                    {"canonical_activity",original},{"authored_name",name},
                    {"source_graph",expected.source_graph},{"packed_identifier",expected.source_identifier}});
            }
            bool rejected=false;
            try {(void)cache.load_unit_weapon_action(definition.graph,0x030000d9u,0x030000d9u,
                0x030000d9u,0x0e000607u);}
            catch(const FormatError &e) {rejected=std::string(e.what()).starts_with("authored unit weapon action missing:");}
            check(rejected,"Raw authored lookup silently translated a foreign Xbox identifier");++foreign_controls;
            const auto authored=cache.load_unit_weapon_action(definition.graph,0x030000d9u,0x030000d9u,
                0x030000d9u,0x0e000613u);
            check(authored.source_identifier==cache.load_action(definition.graph,"any","any","any","melee_strike_1").source_identifier,
                "Raw MCC sword selector changed its authored meaning");
        }
    }
    check(bindings>=4 && sword_actions==12 && foreign_controls==2,"MCC binding coverage is incomplete");
    const auto content=cache.load_content(0,true);
    UnitAnimationMetadataStore metadata(std::shared_ptr<const Cache>(&cache,[](const Cache *){}));
    const auto memory=metadata.graph_memory();
    check(memory.dual_weapon_class==0x04000557u,"MCC retained graph owner lost its verified dual-class namespace");
    for(const unsigned representation:{2,3}) {
        const auto graph=cache.load_model_animation_graph(content.player_representations.at(representation).biped.datum);
        for(const auto &weapon:cache.select_tags({.group=tag_groups::weapon,.require_address=true})) {
            const auto ids=cache.load_weapon_animation_identifiers(weapon.datum);
            if(ids[0]==0xffffffffu)continue;
            check(source_animation_weapon_class(memory.graph_address(graph),0x04000557u,ids[1],memory)==0x04000557u,
                "MCC graph class map replaced an actual dual-hand request");++body_contexts;
            try {
                const auto selected=cache.load_unit_weapon_action(graph,0x06000086u,ids[0],ids[1],0x0e000613u,true);
                check(selected.source_identifier!=0xffffffffu,"MCC body melee lookup lost its packed identifier");++body_actions;
            }catch(const FormatError &e) {
                if(!std::string(e.what()).starts_with("authored unit weapon action missing:"))throw;
                continue;
            }
            bool rejected=false;
            try {(void)cache.load_unit_weapon_action(graph,0x06000086u,ids[0],ids[1],0x0e000607u,true);}
            catch(const FormatError &e) {rejected=std::string(e.what()).starts_with("authored unit weapon action missing:");}
            check(rejected,"Raw body lookup silently remapped a foreign original action");
        }
    }
    check(body_contexts>=40 && body_actions>=20,"MCC body graph/class coverage is incomplete");
    check(fp_query_keys==12,"FP query scope missed an authored Sword strike/dash/lunge/representation");
    check(reload_query_keys>=8,"FP query scope missed authored staged reload activities");
    check(reload_timing_query_keys==reload_query_keys,"Magazine timing lookup missed staged reload activities");
    for(const auto &[graph,hash]:before)
        check(hash==graph_digest(graph),"First-person query changed authored sword graph metadata");
    const nlohmann::json report{{"passed",true},{"runtime_cache_source","MCC"},{"map",argv[1]},
        {"bindings",bindings},{"canonical_clock_clips",clips},{"sword_named_action_matches",sword_actions},
        {"foreign_raw_lookup_controls",foreign_controls},{"authored_sword_metadata_unchanged",true},
        {"MCC_body_weapon_contexts",body_contexts},{"MCC_body_melee_actions",body_actions},
        {"MCC_dual_class_retained",true},
        {"qualified_dual_equip_class_literals",3},
        {"qualified_source_query_key_matches",source_query_keys},
        {"qualified_FP_source_query_key_matches",fp_query_keys},
        {"qualified_FP_staged_reload_query_key_matches",reload_query_keys},
        {"qualified_magazine_timing_query_key_matches",reload_timing_query_keys},
        {"source_clock_keeps_original_activity",true},{"rows",rows},
        {"natural_melee_or_rendering_verified",false},{"full_weapon_parity",false}};
    std::ofstream output(argv[2]);output<<report.dump(2)<<'\n';check(bool(output),"Cannot write first-person binding report");
    std::cout<<bindings<<" MCC bindings; "<<sword_actions<<" independently named sword actions matched\n";return 0;
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
