#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/unit_animation_metadata_store.hpp"
#include "halo2/source_weapon_animation_query_symbols_tu15.hpp"
#include "halo2/physics_contact_record.hpp"
#include "helpers/first_person_dual_variants.hpp"
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
    for(const auto [caller,key]:std::array{
        std::pair{0x112406u,0x0400054bu},std::pair{0x1124e8u,0x07000101u},
        std::pair{0x1125e0u,0x0400054bu}}) {
        SourceWeaponAnimationQueryScopeTu15 equip_lookup(query_bank,caller,key);
        check(source_weapon_animation_scoped_class_symbol_tu15(query_bank,0x0400054bu)==0x04000557u,
            "Copied dual body class lost its actual MCC name");
        SourceWeaponAnimationQueryScopeTu15 wrong_caller(query_bank,caller+1u,key);
        check(source_weapon_animation_scoped_class_symbol_tu15(query_bank,0x0400054bu)==0x0400054bu,
            "Copied dual body class escaped its original lookup boundary");
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
    unsigned bindings{},clips{},sword_actions{},foreign_controls{},body_contexts{},body_actions{},source_query_keys{},fp_query_keys{},reload_query_keys{},reload_timing_query_keys{},lunge_timing_query_keys{};
    unsigned body_grip_queries{},body_arm_offset_queries{},raw_body_controls{};
    unsigned body_action_queries{},raw_body_action_controls{};
    std::map<std::string,unsigned> body_action_matches;
    std::map<std::string,unsigned> body_action_overlay_matches,body_action_fallback_matches;
    nlohmann::json body_pose_rows=nlohmann::json::array();
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
                if(original==0x0b0005b2u || original==0x140005b3u) {
                    // Independently named authored metadata supplies the
                    // expected impact event; the canonical action is only a
                    // copied EC22B timing-query operand at this boundary.
                    const auto key=source_weapon_animation_query_symbol_tu15(query_bank,0xec230u,original);
                    check(cache.string_id(key)==name,"Lunge impact timing query lost its authored MCC name");
                    const auto timing=cache.load_unit_weapon_action(definition.graph,0x06000086u,
                        0x07000083u,0x030000d9u,key);
                    check(timing.source_identifier==expected.source_identifier && timing.source_graph==expected.source_graph &&
                        timing.frame_count==expected.frame_count && timing.frame_events.size()==expected.frame_events.size() &&
                        std::equal(timing.frame_events.begin(),timing.frame_events.end(),expected.frame_events.begin(),
                            [](const auto &a,const auto &b){return a.type==b.type && a.frame==b.frame;}),
                        "Lunge impact timing selected different authored clip/events");
                    check(std::any_of(expected.frame_events.begin(),expected.frame_events.end(),
                        [](const auto &event){return event.type==0;}),"Authored Sword lunge lacks its impact event");
                    ++lunge_timing_query_keys;
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
    unsigned dual_actions{},dual_smg_actions{},dual_timing_actions{},raw_dual_controls{},dual_variant_controls{};
    nlohmann::json dual_rows=nlohmann::json::array();
    for(const auto &[datum,weapon]:content.weapons)if(weapon.dual_wieldable)
        for(const unsigned representation:{2,3}) {
            const auto binding=cache.load_first_person_binding(datum,representation);
            if(!binding.record_present || binding.graph==0xffffffffu)continue;
            if(weapon.name.ends_with("\\smg")) {
                const test::AuthoredFirstPersonDualVariants variants(cache,binding);
                const auto action=[](const auto &clips,unsigned activity)->const FirstPersonClockClip& {
                    const auto at=std::find_if(clips.begin(),clips.end(),[&](const auto &clip){return clip.activity==activity;});
                    if(at==clips.end())throw FormatError("SMG variant controls lost an actual authored action");
                    return *at;
                };
                const auto &idle=action(binding.dual_clock_clips,0x0400000cu);
                const auto &ready=action(binding.dual_clock_clips,0x05000024u);
                const auto &single=action(binding.clock_clips,0x0400000cu);
                check(idle.inheritance==0xffffu,"SMG variant controls require the actual local graph owner");
                const auto metadata=cache.load_source_unit_animation_graph_metadata(binding.graph);
                const Bytes header(metadata.header());
                const auto next=Bytes(metadata.bytes_at(header.u32(0x30)+idle.index*0x6cu,0x6c)).u16(0x32);
                check(next!=0xffffu,"Actual SMG dual idle lost its authored variant successor");
                const auto child=(std::uint32_t(next)<<16)|idle.inheritance;
                check(variants.contains(binding.graph,idle.activity,first_person_clip_identifier(idle)),
                    "Authored dual action root was rejected");
                check(variants.contains(binding.graph,idle.activity,child),"Native legal dual idle variant was rejected");
                check(!variants.contains(binding.graph,idle.activity,first_person_clip_identifier(single)),
                    "Ordinary single-weapon idle entered the dual variant witness");
                check(!variants.contains(binding.graph,idle.activity,first_person_clip_identifier(ready)),
                    "Foreign dual action chain entered the idle witness");
                check(!variants.contains(binding.graph^0x10000u,idle.activity,child),
                    "Foreign manager graph entered the authored dual witness");
                check(!variants.contains(binding.graph,idle.activity,child^1u),
                    "Foreign packed inheritance entered the authored dual witness");
                check(!variants.contains(binding.graph,ready.activity,child),
                    "Dual idle variant entered a different activity witness");
                dual_variant_controls+=7;
            }
            for(const auto activity:{0x0400000cu,0x06000006u,0x0b000065u,0x05000024u}) {
                // The named selector is independent of the compiled query
                // boundary and uses the actual authored dual class record.
                std::optional<AnimationClip> expected;
                try {expected=cache.load_action(binding.graph,"any","dual","any",cache.string_id(activity));}
                catch(const FormatError &e) {
                    if(!std::string(e.what()).starts_with("animation action category missing:"))throw;
                    continue;
                }
                if(weapon.name.ends_with("\\smg")) {
                    const auto raw=cache.load_unit_weapon_action(binding.graph,0x06000086u,0x0400054bu,
                        0x030000d9u,activity);
                    check(raw.source_identifier!=expected->source_identifier,
                        "Raw authored class lookup silently translated the compiled dual key");
                    ++raw_dual_controls;
                }
                for(const auto caller:{0x169a56u,0x16b402u}) {
                    SourceWeaponAnimationQueryScopeTu15 request(query_bank,caller,activity);
                    const auto key=source_weapon_animation_scoped_class_symbol_tu15(query_bank,0x0400054bu);
                    const auto selected=cache.load_unit_weapon_action(binding.graph,0x06000086u,key,
                        0x030000d9u,activity);
                    check(selected.source_identifier==expected->source_identifier &&
                        selected.source_graph==expected->source_graph,
                        "Native dual-class lookup selected a single-weapon pose instead of the authored dual pose");
                    if(caller==0x169a56u) {
                        ++dual_actions;if(weapon.name.ends_with("\\smg"))++dual_smg_actions;
                    }else ++dual_timing_actions;
                }
                dual_rows.push_back({{"weapon",weapon.name},{"representation",representation},
                    {"canonical_activity",activity},{"authored_class","dual"},
                    {"source_graph",expected->source_graph},{"packed_identifier",expected->source_identifier}});
            }
        }
    check(dual_actions>=50 && dual_smg_actions==8 && dual_timing_actions==dual_actions && raw_dual_controls==8,
        "Actual MCC dual-class playback/timing/species coverage is incomplete");
    check(dual_variant_controls==14,"Actual human/elite SMG variant membership controls are incomplete");
    UnitAnimationMetadataStore metadata(std::shared_ptr<const Cache>(&cache,[](const Cache *){}));
    const auto memory=metadata.graph_memory();
    check(memory.dual_weapon_class==0x04000557u,"MCC retained graph owner lost its verified dual-class namespace");
    for(const unsigned representation:{2,3}) {
        const auto graph=cache.load_model_animation_graph(content.player_representations.at(representation).biped.datum);
        for(const auto &weapon:cache.select_tags({.group=tag_groups::weapon,.require_address=true})) {
            const auto ids=cache.load_weapon_animation_identifiers(weapon.datum);
            if(ids[0]==0xffffffffu)continue;
            struct BodyQuery {unsigned activity_site,class_site,original,authored;bool dual;};
            constexpr std::array body_queries{
                BodyQuery{0x115ccf,0x115cd4,0x0b00054a,0x0b000556,true},
                BodyQuery{0x115ce8,0x115ced,0x0b00054a,0x0b000556,true},
                BodyQuery{0x115d0c,0x115d11,0x0400060b,0x04000617,true},
                BodyQuery{0x115d2a,0x115d2f,0x0400060b,0x04000617,true},
                BodyQuery{0x115d68,0,0x0400060b,0x04000617,false},
                BodyQuery{0x115d83,0,0x0400060b,0x04000617,false}};
            for(const auto &query:body_queries) {
                const auto definition=content.weapons.find(weapon.datum);
                if(query.dual && (definition==content.weapons.end() || !definition->second.dual_wieldable))continue;
                const auto expected_class=query.dual?0x04000557u:ids[0];
                //EADB5 passes weap+28C into112420; extension+98 retains
                //that actual secondary type for115AF0's copied query.
                const auto query_type=ids[1];
                std::optional<AnimationClip> expected;
                try {expected=cache.load_unit_weapon_action(graph,0x06000086u,expected_class,query_type,query.authored,true);}
                catch(const FormatError &e) {
                    if(!std::string(e.what()).starts_with("authored unit weapon action missing:"))throw;
                    continue;
                }
                const auto activity=source_weapon_animation_body_query_symbol_tu15(query_bank,query.activity_site,query.original);
                const auto weapon_class=query.dual?
                    source_weapon_animation_body_query_symbol_tu15(query_bank,query.class_site,0x0400054bu):ids[0];
                const auto selected=cache.load_unit_weapon_action(graph,0x06000086u,weapon_class,query_type,activity,true);
                check(selected.source_identifier==expected->source_identifier && selected.source_graph==expected->source_graph,
                    "Native body grip/arm-offset query lost its actual authored overlay");
                bool raw_differs{};
                try {raw_differs=cache.load_unit_weapon_action(graph,0x06000086u,expected_class,query_type,query.original,true).
                    source_identifier!=expected->source_identifier;}
                catch(const FormatError &e) {
                    if(!std::string(e.what()).starts_with("authored unit weapon action missing:"))throw;
                    raw_differs=true;
                }
                check(raw_differs,"Raw authored body lookup silently translated a compiled pose key");++raw_body_controls;
                if(query.original==0x0400060bu)++body_grip_queries;else ++body_arm_offset_queries;
                body_pose_rows.push_back({{"weapon",weapon.name},{"representation",representation},
                    {"instruction",query.activity_site},{"dual",query.dual},{"authored_name",cache.string_id(activity)},
                    {"source_graph",selected.source_graph},{"packed_identifier",selected.source_identifier}});
            }
            const auto definition=content.weapons.find(weapon.datum);
            if(definition!=content.weapons.end()) {
                struct BodyAction {unsigned original,mcc,caller;const char *name;};
                constexpr std::array actions{
                    BodyAction{0x0b00053e,0x0b00054a,0xe9a83,"fire_1_left"},
                    BodyAction{0x0b00053f,0x0b00054b,0xe9a9d,"fire_2_left"},
                    BodyAction{0x0e00053c,0x0e000548,0xe9ab7,"chamber_1_left"},
                    BodyAction{0x0e00053d,0x0e000549,0xe9ad1,"chamber_2_left"},
                    BodyAction{0x0e000540,0x0e00054c,0xe9aeb,"charged_1_left"},
                    BodyAction{0x0e000541,0x0e00054d,0xe9b05,"charged_2_left"},
                    BodyAction{0x0d00053a,0x0d000546,0xe97a9,"reload_1_left"},
                    BodyAction{0x0d00053b,0x0d000547,0xe97a9,"reload_2_left"},
                    BodyAction{0x1000067e,0x1000068a,0xe9b6d,"overheating_left"},
                    BodyAction{0x0f00067f,0x0f00068b,0xe9b87,"overheated_left"},
                    BodyAction{0x0d000680,0x0d00068c,0xe9ba1,"o_h_exit_left"},
                    BodyAction{0x0a000537,0x0a000543,0xeaef3,"ready_left"},
                    BodyAction{0x15000538,0x15000544,0xeaec2,"ready_left_overheated"},
                    BodyAction{0x0d000539,0x0d000545,0xeab2e,"put_away_left"},
                    BodyAction{0x0a00022d,0x0a00023b,0xee9ae,"smash_left"},
                    BodyAction{0x0b00022e,0x0b00023c,0xee9ae,"smash_right"}};
                for(const auto &action:actions)for(const bool dual_context:{false,true})for(const bool overlay:{true,false}) {
                    if(dual_context && !definition->second.dual_wieldable)continue;
                    if(!dual_context && action.caller!=0xee9aeu)continue;
                    body_action_matches.try_emplace(action.name,0);
                    std::optional<AnimationClip> expected;
                    // Left overlays resolve the actual second weapon type
                    // from+98(EADB5 ->112420); fallback/smash use current+78.
                    const auto query_type=ids[1],expected_class=dual_context?0x04000557u:ids[0];
                    try {expected=cache.load_unit_weapon_action(graph,0x06000086u,expected_class,query_type,action.mcc,overlay);}
                    catch(const FormatError &e) {
                        if(!std::string(e.what()).starts_with("authored unit weapon action missing:"))throw;
                        continue;
                    }
                    const auto select=[&] {
                        const auto key=overlay?source_weapon_animation_scoped_overlay_symbol_tu15(query_bank,action.original):
                            source_weapon_animation_scoped_query_symbol_tu15(query_bank,action.original);
                        if(cache.string_id(key)!=action.name)
                            throw FormatError("Native body request "+std::string(action.name)+" selected foreign authored name "+cache.string_id(key));
                        const auto weapon_class=source_weapon_animation_scoped_class_symbol_tu15(query_bank,
                            dual_context?0x0400054bu:ids[0]);
                        return cache.load_unit_weapon_action(graph,0x06000086u,weapon_class,query_type,key,overlay);
                    };
                    SourceWeaponAnimationQueryScopeTu15 origin(query_bank,action.caller,action.original);
                    AnimationClip selected;
                    if(overlay) {
                        SourceWeaponAnimationQueryScopeTu15 lookup(query_bank,0x116654u,action.original);selected=select();
                    }else {
                        SourceWeaponAnimationQueryScopeTu15 fallback(query_bank,0x1166e4u,action.original);
                        SourceWeaponAnimationQueryScopeTu15 playback(query_bank,0x111bceu,action.original);selected=select();
                    }
                    check(selected.source_identifier==expected->source_identifier && selected.source_graph==expected->source_graph,
                        "Native retained body action selected a different authored clip");
                    ++body_action_queries;++body_action_matches[action.name];
                    ++(overlay?body_action_overlay_matches:body_action_fallback_matches)[action.name];
                    bool raw_differs{};
                    try {raw_differs=cache.load_unit_weapon_action(graph,0x06000086u,expected_class,query_type,action.original,overlay).
                        source_identifier!=expected->source_identifier;}
                    catch(const FormatError &e) {
                        if(!std::string(e.what()).starts_with("authored unit weapon action missing:"))throw;
                        raw_differs=true;
                    }
                    check(raw_differs,"Raw authored body action silently translated a canonical weapon request");++raw_body_action_controls;
                }
            }
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
    check(body_grip_queries>=60 && body_arm_offset_queries>=30 &&
        raw_body_controls==body_grip_queries+body_arm_offset_queries,
        "MCC native body grip/arm-offset/source-site coverage is incomplete");
    std::cout<<"Native body action lookup counts: "<<nlohmann::json(body_action_matches).dump()<<'\n';
    check(body_action_queries>=30 && raw_body_action_controls==body_action_queries && body_action_matches["fire_1_left"]>=16 &&
        body_action_matches["reload_1_left"]>=8,"MCC retained body weapon-action coverage is incomplete");
    check(fp_query_keys==12,"FP query scope missed an authored Sword strike/dash/lunge/representation");
    check(reload_query_keys>=8,"FP query scope missed authored staged reload activities");
    check(reload_timing_query_keys==reload_query_keys,"Magazine timing lookup missed staged reload activities");
    check(lunge_timing_query_keys==4,"Lunge impact timing missed a ground/air/species metadata lookup");
    for(const auto &[graph,hash]:before)
        check(hash==graph_digest(graph),"First-person query changed authored sword graph metadata");
    const nlohmann::json report{{"passed",true},{"runtime_cache_source","MCC"},{"map",argv[1]},
        {"bindings",bindings},{"canonical_clock_clips",clips},{"sword_named_action_matches",sword_actions},
        {"foreign_raw_lookup_controls",foreign_controls},{"authored_sword_metadata_unchanged",true},
        {"MCC_body_weapon_contexts",body_contexts},{"MCC_body_melee_actions",body_actions},
        {"MCC_dual_class_retained",true},
        {"qualified_native_body_grip_queries",body_grip_queries},
        {"qualified_native_body_arm_offset_queries",body_arm_offset_queries},
        {"unchanged_raw_body_pose_controls",raw_body_controls},{"body_pose_rows",body_pose_rows},
        {"qualified_native_body_action_queries",body_action_queries},
        {"unchanged_raw_body_action_controls",raw_body_action_controls},{"body_action_matches",body_action_matches},
        {"body_action_overlay_matches",body_action_overlay_matches},{"body_action_fallback_matches",body_action_fallback_matches},
        {"qualified_dual_equip_class_literals",3},
        {"qualified_dual_FP_actions",dual_actions},{"qualified_dual_SMG_FP_actions",dual_smg_actions},
        {"qualified_dual_timing_actions",dual_timing_actions},
        {"unchanged_raw_dual_class_controls",raw_dual_controls},{"dual_rows",dual_rows},
        {"authored_dual_variant_membership_controls",dual_variant_controls},
        {"qualified_source_query_key_matches",source_query_keys},
        {"qualified_FP_source_query_key_matches",fp_query_keys},
        {"qualified_lunge_timing_query_key_matches",lunge_timing_query_keys},
        {"qualified_FP_staged_reload_query_key_matches",reload_query_keys},
        {"qualified_magazine_timing_query_key_matches",reload_timing_query_keys},
        {"source_clock_keeps_original_activity",true},{"rows",rows},
        {"natural_melee_or_rendering_verified",false},{"full_weapon_parity",false}};
    std::ofstream output(argv[2]);output<<report.dump(2)<<'\n';check(bool(output),"Cannot write first-person binding report");
    std::cout<<bindings<<" MCC bindings; "<<sword_actions<<" independently named sword actions matched\n";return 0;
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
