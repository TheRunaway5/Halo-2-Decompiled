#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool ok,const std::string &why){if(!ok)throw FormatError(why);}
struct Files {
    std::filesystem::path path;
    Files(){const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
        need(scratch,"Unit input regression requires tools/run_test.py scratch ownership");
        path=std::filesystem::path(scratch)/"unit-equipment-profile-world";
        need(std::filesystem::create_directory(path),"Cannot create owned unit input World directory");}
    ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t at,unsigned bytes=4){
    const auto data=m.view(at,bytes);if(bytes==1)return std::to_integer<unsigned>(data[0]);
    const Bytes value(data);return bytes==2?value.u16(0):value.u32(0);
}
std::array<float,4> vitality(PhysicsLiveWorldTu15 &world,std::uint32_t unit){
    const auto body=world.source_object_body(unit);const auto &m=world.memory();
    return {std::bit_cast<float>(word(m,body+0xe4u)),std::bit_cast<float>(word(m,body+0xe8u)),
        std::bit_cast<float>(word(m,body+0xecu)),std::bit_cast<float>(word(m,body+0xf0u))};
}
SourceUnitEquipmentWeaponInputTu15 ammo(const WeaponDefinition &weapon){
    need(weapon.capacity>0 && weapon.initial_rounds>=0,"Unit profile needs authored magazine ammunition");
    const auto loaded=std::min(weapon.initial_rounds,weapon.capacity);
    const auto reserve=std::min(weapon.initial_rounds-loaded,std::max(0,weapon.reserve));
    need(loaded>=0 && reserve>0 && loaded+reserve<=std::numeric_limits<std::int16_t>::max(),
        "Unit profile needs a bounded authored loaded-plus-reserve budget");
    return {weapon.datum,std::int16_t(loaded),std::int16_t(loaded+reserve),0x77656170u};
}
Json inventory(PhysicsLiveWorldTu15 &world,std::uint32_t unit){
    const auto equipment=world.player_equipment(unit);Json weapons=Json::array();
    for(unsigned slot=0;slot<equipment.weapons.size();++slot)if(equipment.weapons[slot]!=null_datum){
        const auto body=world.source_object_body(equipment.weapons[slot]);
        weapons.push_back({{"slot",slot},{"object",equipment.weapons[slot]},
            {"definition",equipment.definitions[slot]},{"parent",word(world.memory(),body+0x14u)},
            {"held_by",word(world.memory(),body+0x154u)},{"item_flags",word(world.memory(),body+0x12cu)},
            {"loaded",std::bit_cast<std::int16_t>(std::uint16_t(word(world.memory(),body+0x22cu,2)))},
            {"reserve",std::bit_cast<std::int16_t>(std::uint16_t(word(world.memory(),body+0x22au,2)))}});
    }
    return {{"weapons",weapons},{"grenades",equipment.grenades},{"current",equipment.current},
        {"requested",equipment.requested},{"vitality",vitality(world,unit)}};
}
struct Trace {
    PhysicsLiveWorldTu15 &world;std::string phase;
    Json calls=Json::array();std::map<std::uint32_t,std::vector<Json>> pending;
    SourcePhysicsEntryObserverBinding binding;
    explicit Trace(PhysicsLiveWorldTu15 &owner):world(owner),binding(owner.memory(),
        [&](bool entering,std::uint32_t entry,const SourcePhysicsWorldRuntimeAbi &abi){
            if(phase.empty() || (entry!=0xcf4d0u && entry!=0xcf240u && entry!=0xcf5a0u && entry!=0xba570u))return;
            if(entering){
                const unsigned count=entry==0xcf5a0u?3u:entry==0xcf240u?2u:1u;Json arguments=Json::array();
                for(unsigned n=0;n<count;++n)arguments.push_back(word(world.memory(),abi.gp[4]+4u+n*4u));
                pending[entry].push_back({{"phase",phase},{"owner",entry},{"arguments",arguments},
                    {"weapon_input",entry==0xcf240u?abi.gp[3]:0u}});
            }else{
                need(!pending[entry].empty(),"Unit profile native owner returned without its actual entry");
                auto call=pending[entry].back();pending[entry].pop_back();call["result"]=abi.gp[0];calls.push_back(call);
            }
        }){}
    Json phase_calls(std::string_view selected)const{Json out=Json::array();for(const auto &call:calls)if(call["phase"]==selected)out.push_back(call);return out;}
};
void detached_live(MultiplayerObjectRuntime &objects,const SourcePlayerEquipmentTu15 &old,
    const SourcePlayerEquipmentTu15 &current){
    const auto live=objects.live_objects();
    for(const auto object:old.weapons)if(object!=null_datum){
        need(std::ranges::find(current.weapons,object)==current.weapons.end(),"Profile retained a replaced carried object");
        need(std::ranges::any_of(live,[&](const auto &item){return item.object==object;}),
            "Profile clear destroyed an old weapon instead of following original CF4D0 drop semantics");
        need(objects.object_parent(object)==null_datum,"Original CF4D0 did not detach its replaced weapon");
    }
}
template<class Action>void rejects_without_effect(PhysicsLiveWorldTu15 &world,std::uint32_t unit,
    const std::string &name,Action action,Json &report){
    const auto before=inventory(world,unit);SourceRuntimeWriteSerial writes(world.memory());unsigned topology{};
    struct Topology {
        const SourceRuntimeMemory &memory;const void *identity;
        ~Topology(){source_runtime_unregister_topology_observer(memory,identity);}
    } scope{world.memory(),&topology};
    source_runtime_register_topology_observer(world.memory(),&topology,[&](auto,auto){++topology;});
    bool rejected{};try{action();}catch(const FormatError&){rejected=true;}
    const auto count=writes.load();const auto after=inventory(world,unit);
    report.push_back({{"control",name},{"rejected",rejected},{"source_writes",count},{"topology_changes",topology},
        {"inventory_unchanged",before==after}});
    need(rejected && !count && !topology && before==after,
        "Unit profile admission control changed source bytes/topology/input or inventory: "+name);
}
void observer_case(PhysicsLiveWorldTu15 &world,std::uint32_t unit,Json &report){
    const auto body=world.source_object_body(unit);Json stores=Json::array();
    world.observe_physics_stores_read_only([&](auto pc,const auto &store){
        if(pc==0xd822fu || pc==0xd823au || pc==0xd824du || pc==0xd825au)
            stores.push_back({{"instruction",pc},{"address",store.address},{"bytes",store.written.size()}});
    });
    struct Cleanup {PhysicsLiveWorldTu15 &world;~Cleanup(){world.observe_physics_stores_read_only({});}} cleanup{world};
    SourceRuntimeWriteSerial writes(world.memory());world.set_unit_maximum_vitality(unit,std::nullopt,0.f);
    const auto value=vitality(world,unit);
    report={{"source_writes",writes.load()},{"actual_vitality",value},{"observed_stores",stores},
        {"expected_body",body},{"source_result_supplied",false}};
    need(value[0]>0 && value[1]==0 && value[2]==1 && value[3]==0 && writes.load()>=4,
        "Actual native D81E0 did not publish its four maximum/current vitality fields");
    need(stores.size()==4 && stores[0]["address"]==body+0xe4u && stores[1]["address"]==body+0xe8u &&
        stores[2]["address"]==body+0xecu && stores[3]["address"]==body+0xf0u,
        "Registered World observer missed the four completed native D81E0 stores");
}
}
int main(int argc,char **argv){
    std::cout<<std::unitbuf;
    if(argc!=3 && argc!=4)return 2;
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    const bool observer_only=argc==4 && std::string_view(argv[3])=="--vitality-observer-only";
    if(argc==4 && !observer_only)return 2;
    Json report{{"passed",false},{"original_results_supplied",false},{"map",argv[1]},
        {"vitality_observer_only",observer_only},{"controls",Json::array()}};
    const auto save=[&]{std::ofstream out(argv[2]);out<<report.dump(2)<<'\n';need(bool(out),"Cannot write unit input report");};
    try{
        Files files;MultiplayerOptions options;options.map=argv[1];options.shared_maps=options.map.parent_path();
        options.local_players=1;options.starting_weapon=5;options.mute=true;options.source_object_savegame_directory=files.path;
        report["stage"]="canonical-native-world";save();MultiplayerPreparedWorld prepared(options);
        need(prepared.cache->is_mcc() && prepared.source_objects && prepared.source_objects->owns_physics() &&
            prepared.rules.requires_source_engine,"Unit profile requires canonical MCC source World admission");
        for(unsigned tick=0;tick<2;++tick)prepared.match.step({});
        const auto slot=prepared.local_slots.front(),unit=prepared.match.player(slot).source_current_object;
        auto &objects=*prepared.source_objects;auto &world=objects.tu15_world();
        need(unit!=null_datum,"Unit input regression has no actual source player biped");
        const auto defaults=vitality(world,unit);report["source_unit"]=unit;report["authored_vitality"]=defaults;
        need(defaults[0]>0 && defaults[1]>0,"Unit fixture needs its positive authored health/shield defaults");
        report["stage"]="native-vitality-observer";save();observer_case(world,unit,report["vitality_observer"]);
        world.set_unit_maximum_vitality(unit,std::nullopt,std::nullopt);
        if(!observer_only){
            Trace trace(world);const auto choices=prepared.cache->multiplayer_weapon_choices();std::uint32_t selection_seed{};
            const auto rifle=source_starting_weapon(null_datum,3,null_datum,choices,selection_seed),
                pistol=source_starting_weapon(null_datum,4,null_datum,choices,selection_seed),
                sword=source_starting_weapon(null_datum,17,null_datum,choices,selection_seed);
            Json selected=Json::array();
            for(const auto datum:std::array{rifle,pistol,sword}){
                const auto tag=prepared.cache->find_tag(TagHandle{datum});
                need(datum!=null_datum && tag && tag->group=="weap" && prepared.content.weapons.contains(datum),
                    "Unit profile authored option did not resolve a resident weapon definition");
                selected.push_back({{"definition",datum},{"group",tag->group},{"name",tag->name}});
            }
            need(rifle!=pistol,"Unit profile needs two distinct authored magazine weapons");
            report["authored_option_resolution"]={{"options",std::array{3u,4u,17u}},{"source_reference_bank",choices},{"selected",selected}};
            SourceUnitEquipmentProfileInputTu15 two;two.weapons={ammo(prepared.content.weapons.at(rifle)),ammo(prepared.content.weapons.at(pistol))};two.grenades={1,2};
            const auto previous=world.player_equipment(unit);report["stage"]="two-real-magazine-weapons";save();trace.phase="two";
            const auto first=world.apply_equipment_profile(unit,two);trace.phase.clear();detached_live(objects,previous,first);
            const auto first_state=inventory(world,unit);report["two_weapons"]=first_state;
            const auto calls=trace.phase_calls("two");report["two_native_calls"]=calls;
            report["carry_ownership_scope"]="Original D0F60 retains full weapon generations in unit+218; FC040 sets item+154 owner and low12C flags5. Object+14 is a separate hand/hierarchy parent and may be NONE for a stowed inventory weapon.";
            need(first_state["weapons"].size()==2 && first.grenades==two.grenades,"Native profile did not carry two actual weapons/exact grenades");
            for(unsigned n=0;n<2;++n){
                const auto &weapon=first_state["weapons"][n];need(weapon["definition"]==two.weapons[n].definition && weapon["held_by"]==unit &&
                    (weapon["item_flags"].get<unsigned>()&5u)==5u && (weapon["parent"]==unit || weapon["parent"]==null_datum) &&
                    weapon["loaded"]==two.weapons[n].rounds_loaded && weapon["reserve"]==two.weapons[n].rounds_total-two.weapons[n].rounds_loaded,
                    "Native CF240/106A50/carry lost declared definition, loaded-inclusive TOTAL or inventory ownership");
            }
            unsigned clear{},created{},carried{};
            for(const auto &call:calls){const auto owner=call["owner"].get<unsigned>();clear+=owner==0xcf4d0u;
                if(owner==0xcf240u){need(call["result"]!=null_datum,"Actual CF240 factory failed");++created;}
                if(owner==0xcf5a0u){need((call["result"].get<unsigned>()&255u)!=0,"Actual CF5A0 carry refused the two-weapon profile");++carried;}}
            need(clear==1 && created==2 && carried==2,"Actual clear/create/carry entry-return coverage is incomplete");
            report["two_native_calls"]=calls;
            auto &controls=report["controls"];
            rejects_without_effect(world,unit,"stale-full-unit-generation",[&]{world.apply_equipment_profile(unit^0x10000u,two);},controls);
            rejects_without_effect(world,unit,"nonunit-weapon",[&]{world.apply_equipment_profile(first.weapons[0],two);},controls);
            auto invalid=two;invalid.weapons[1].definition=0xfffffffeu;
            rejects_without_effect(world,unit,"nonresident-second-definition",[&]{world.apply_equipment_profile(unit,invalid);},controls);
            for(unsigned n=0;n<2;++n){
                invalid=two;(n?invalid.shield_damage:invalid.health_damage)=std::numeric_limits<float>::quiet_NaN();
                rejects_without_effect(world,unit,n?"NaN-shield-input":"NaN-health-input",[&]{world.apply_equipment_profile(unit,invalid);},controls);
                invalid=two;invalid.grenades[n]=5;
                rejects_without_effect(world,unit,n?"plasma-count5":"frag-count5",[&]{world.apply_equipment_profile(unit,invalid);},controls);
            }
            rejects_without_effect(world,unit,"vitality-stale-generation",[&]{world.set_unit_maximum_vitality(unit^0x10000u,std::nullopt,0.f);},controls);
            rejects_without_effect(world,unit,"vitality-nonunit",[&]{world.set_unit_maximum_vitality(first.weapons[0],std::nullopt,0.f);},controls);
            rejects_without_effect(world,unit,"vitality-NaN",[&]{world.set_unit_maximum_vitality(unit,std::nullopt,std::numeric_limits<float>::quiet_NaN());},controls);
            SourceUnitEquipmentProfileInputTu15 replacement;replacement.weapons[0].definition=sword;replacement.health_damage=.25f;replacement.shield_damage=.5f;
            report["stage"]="replacement-sword-and-maximum-vitality";save();trace.phase="replacement";
            const auto second=world.apply_equipment_profile(unit,replacement);trace.phase.clear();detached_live(objects,first,second);
            need(std::ranges::count(second.definitions,sword)==1 && std::ranges::count(second.weapons,null_datum)==3 &&
                second.grenades==std::array<std::uint8_t,2>{0,0},"Replacement profile did not carry exactly one native sword and zero grenades");
            need(vitality(world,unit)==std::array<float,4>{defaults[0],defaults[1],.75f,.5f},"CF2E0 profile damage fractions differ from native source semantics");
            world.set_unit_maximum_vitality(unit,std::nullopt,0.f);
            need(vitality(world,unit)==std::array<float,4>{defaults[0],0.f,1.f,0.f},"Maximum vitality after profile did not preserve authored health and eliminate shield");
            report["replacement"]=inventory(world,unit);report["replacement_native_calls"]=trace.phase_calls("replacement");
            report["stage"]="empty-profile-drop";save();trace.phase="empty";
            const auto empty=world.apply_equipment_profile(unit,{});trace.phase.clear();detached_live(objects,second,empty);
            need(std::ranges::all_of(empty.weapons,[](auto d){return d==null_datum;}) && empty.grenades==std::array<std::uint8_t,2>{0,0},
                "Empty profile did not drop actual inventory and reset grenade budgets");
            need(vitality(world,unit)==std::array<float,4>{defaults[0],0.f,1.f,1.f},"Empty CF2E0 did not preserve maxima while restoring current fractions");
            world.set_unit_maximum_vitality(unit,std::nullopt,std::nullopt);
            need(vitality(world,unit)==std::array<float,4>{defaults[0],defaults[1],1.f,1.f},"Absent vitality scalars did not restore real authored defaults");
            report["empty_profile"]=inventory(world,unit);report["empty_native_calls"]=trace.phase_calls("empty");
            // An authored bit29 weapon is refused by original CFB80 when the
            // unit still has a player. It is a genuine input, never a flag edit.
            std::uint32_t refused_definition=null_datum;
            for(const auto &[datum,weapon]:prepared.content.weapons)if(weapon.inventory_flags&(1u<<29u))
                refused_definition=std::min(refused_definition,datum);
            report["carry_refusal"]={{"authored_definition",refused_definition},{"qualified",false}};
            if(refused_definition!=null_datum){
                SourceUnitEquipmentProfileInputTu15 refused;refused.weapons[0].definition=refused_definition;
                report["stage"]="authored-native-carry-refusal";save();trace.phase="refusal";
                const auto rejected=world.apply_equipment_profile(unit,refused);trace.phase.clear();const auto actual=trace.phase_calls("refusal");
                std::uint32_t born=null_datum;unsigned failed{},retired{};
                for(const auto &call:actual){if(call["owner"]==0xcf240u)born=call["result"];
                    if(call["owner"]==0xcf5a0u)failed+=(call["result"].get<unsigned>()&255u)==0;
                    if(call["owner"]==0xba570u){need(call["arguments"][0]==born,"Native failed-carry retirement targeted another object");++retired;}}
                need(born!=null_datum && failed==1 && retired==1 && std::ranges::all_of(rejected.weapons,[](auto d){return d==null_datum;}),
                    "Authored CFB80 refusal did not create, refuse carry and retire the actual new weapon");
                const auto pool=world.source_object_pool(),headers=word(world.memory(),pool+0x44u),header=headers+(born&65535u)*12u;
                need(word(world.memory(),header,2)!=(born>>16u) || (word(world.memory(),header+2u,1)&0x10u),
                    "Actual BA570 did not retire or mark its failed-carry source object for deletion");
                report["carry_refusal"]={{"authored_definition",refused_definition},{"qualified",true},{"born_object",born},{"native_calls",actual}};
            }else report["carry_refusal"]["limitation"]="No resident authored bit29 weapon; failure-retirement branch remains unqualified";
            // Death remains a genuine source request. A corpse may legally
            // accept equipment; that outcome does not establish refusal parity.
            report["stage"]="native-corpse-profile";save();objects.kill_biped(unit);
            const auto body=world.source_object_body(unit);need(word(world.memory(),body+0x10au,1)&4u,"Genuine kill_biped did not produce a dead source unit");
            trace.phase="corpse";const auto corpse=world.apply_equipment_profile(unit,two);trace.phase.clear();
            report["corpse_profile"]={{"native_dead",true},{"inventory",inventory(world,unit)},
                {"native_calls",trace.phase_calls("corpse")},{"carry_failure_qualified",false}};
            unsigned failed{},retired{};for(const auto &call:trace.phase_calls("corpse")){
                if(call["owner"]==0xcf5a0u)failed+=(call["result"].get<unsigned>()&255u)==0;
                retired+=call["owner"]==0xba570u;
            }
            need(failed==retired,"Corpse profile lost actual failed-carry retirement calls");
            report["corpse_profile"]["carry_failure_qualified"]=failed>0;
            report["corpse_profile"]["source_permitted_carry"]=std::ranges::any_of(corpse.weapons,[](auto d){return d!=null_datum;});
            for(const auto &[owner,pending]:trace.pending)need(pending.empty(),"A genuine unit input child did not return");
        }
        report["stage"]="complete";report["passed"]=true;
    }catch(const std::exception &error){report["error"]=error.what();}
    save();std::cout<<report.dump()<<'\n';return report["passed"].get<bool>()?0:1;
}
