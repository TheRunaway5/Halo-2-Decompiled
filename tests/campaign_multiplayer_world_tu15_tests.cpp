#include "halo2/world_material.hpp"
#include "halo2/appearance.hpp"
#include "halo2/frontend_model_ik.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include <thread>
#include <chrono>
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include "helpers/campaign_vehicle_observer_inputs_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cstdlib>
using namespace halo2;
using Json=nlohmann::json;
int main(int argc,char **argv) {
    Json report{{"passed",false},{"reference","English TU1.5 only"},{"full_physics_parity",false}};
    std::filesystem::path output;
    try {
        if(argc!=4)throw FormatError("Supply original TU1.5 XBE, campaign map and output directory");
        output=argv[3];std::filesystem::create_directories(output);
        const auto original=OriginalData::load_xbe(argv[1]);
        if(original->image!=OriginalData::Image::TitleUpdate15)throw FormatError("Campaign physical roster requires TU1.5");
        Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),original);
        const auto bsp=std::getenv("HALO2_TU15_CAMPAIGN_BSP")?std::stoul(std::getenv("HALO2_TU15_CAMPAIGN_BSP")):0u;
        const auto content=cache.load_content(bsp,false);
        const auto selected=std::getenv("HALO2_TU15_CAMPAIGN_PLACEMENT")?std::stol(std::getenv("HALO2_TU15_CAMPAIGN_PLACEMENT")):68;
        const auto at=std::ranges::find_if(content.objects,[&](const auto &p){return p.type==1 && p.placement==selected && !p.model_attachment;});
        if(at==content.objects.end())throw FormatError("Declared authored campaign vehicle placement absent");
        const auto metadata=cache.load_source_tag_metadata(at->definition);
        if(Bytes(metadata.bytes_at(metadata.source_address(),metadata.size())).u16(0x1f0u)!=3)
            throw FormatError("Declared authored vehicle is not class3");
        const auto starts=test::campaign_vehicle_observer_starts<3>(cache,content,bsp,*at);
        MultiplayerOptions options;options.map=argv[2];options.shared_maps=std::filesystem::path(argv[2]).parent_path();
        options.gameplay_xbe=argv[1];options.bsp=bsp;options.local_players=2;
        options.campaign_difficulty=1;options.source_object_world=true;
        options.source_object_savegame_directory=output/"source-storage";
        options.native_spawn_inputs.assign(starts.begin(),starts.end());
        options.campaign_spawn_input=[starts](unsigned slot,bool initial)->std::optional<Spawn> {
            if(!initial || slot>=starts.size())return {};
            return starts[slot]; // Declared cached-BSP input before source birth.
        };
        test::LivePhysicsEntryCaptureTu15 cloth(output/"original-cloth-construction-capture"),
            rotation(output/"original-campaign-rotation-codec-capture"),
            translation(output/"original-campaign-translation-codec-capture"),
            uncompressed(output/"original-campaign-uncompressed-codec-capture"),
            constraint(output/"original-constraint-construction-capture"),
            constraint_callback(output/"original-constraint-callback-capture"),
            constraint_release(output/"original-constraint-release-capture");
        std::optional<SourcePhysicsEntryObserverBinding> observation;
        options.source_world_observer=[&](const SourceRuntimeMemory &memory) {
            observation.emplace(memory,[&](bool entering,std::uint32_t entry,const auto &state) {
                if(entry==0x2fbb70u)constraint.observe(entering,state,memory,entry);
                else if(entry==0x2fc570u)constraint_callback.observe(entering,state,memory,entry);
                else if(entry==0x2fbb10u)constraint_release.observe(entering,state,memory,entry);
                else if(entry==0x119060u)cloth.observe(entering,state,memory,entry);
                else if(entry==0x297270u)rotation.observe(entering,state,memory,entry);
                else if(entry==0x297400u)translation.observe(entering,state,memory,entry);
                else if(entry==0x296180u)uncompressed.observe(entering,state,memory,entry);
            });
        };
        report["stage"]="construct shared physical player world and authored scenario";
        // This is a declared finite physics stimulus. Campaign AI/story startup
        // is a separate integration gate; no guessed script result is supplied.
        MultiplayerPreparedWorld world(options,ScriptProgram{});
        auto &bank=world.source_objects->memory();
        const auto game=Bytes(bank.view(0x4f3218u,4)).u32(0);
        if(Bytes(bank.view(game+8u,4)).u32(0)!=1 || Bytes(bank.view(game+0x132u,2)).u16(0)!=1)
            throw FormatError("Shared campaign world lost original kind/difficulty");
        const auto engine_globals=Bytes(bank.view(0x4f63b8u,4)).u32(0);
        const auto engine_index=Bytes(bank.view(engine_globals+0xc14u,4)).u32(0);
        const auto engine_callback=Bytes(bank.view(0x56dfa0u+engine_index*4u,4)).u32(0);
        report["original_game_engine_index"]=engine_index;report["original_game_engine_callback"]=engine_callback;
        if(engine_index!=0 || engine_callback!=0)throw FormatError("Campaign source world unexpectedly admitted a multiplayer engine");
        if(!world.matches(options) || world.local_slots.size()!=2)throw FormatError("Prepared campaign roster binding differs");
        auto wrong=options;wrong.campaign_difficulty=2;
        if(world.matches(wrong))throw FormatError("Prepared world reused a different original difficulty");
        report["stage"]="create authored runtime class3 through shared scene owner";
        const auto placement=std::ranges::find_if(world.content.objects,[&](const auto &p){return p.type==1 && p.placement==selected && p.definition==at->definition;});
        if(placement==world.content.objects.end())throw FormatError("Shared content lost authored vehicle placement");
        const auto index=std::size_t(placement-world.content.objects.begin());
        const auto object=world.match.create_world_object(index);
        if(object==null_datum)throw FormatError("Shared source scene returned no runtime vehicle");
        world.source_objects->set_unit_controller_active(object,true);
        report["object"]=object;report["kind"]=1;report["difficulty"]=1;
        report["players"]=Json::array();
        int expected_representation=0;
        for(const auto &start:cache.spawns())if(start.player_type!=-1){expected_representation=start.player_type;break;}
        if(expected_representation<0 || unsigned(expected_representation)>=world.content.player_representations.size())
            throw FormatError("Authored representation requires an additional original clamp case");
        report["expected_representation"]=expected_representation;

        for(auto slot:world.local_slots) {
            const auto &p=world.match.player(slot);
            if(p.source_datum==null_datum || p.source_current_object==null_datum)throw FormatError("Shared campaign player source admission failed");
            if(!p.source_configuration || int((*p.source_configuration)[0x44])!=expected_representation || p.source_membership!=1 || (*p.source_configuration)[0x7c]!=1)
                throw FormatError("Campaign player profile was remapped to multiplayer appearance/team");
            const auto definition=world.source_objects->object_definition(p.source_current_object);
            if(definition!=world.content.player_representations[unsigned(expected_representation)].biped.datum)
                throw FormatError("Campaign source unit uses a different authored biped");
            report["players"].push_back({{"slot",slot},{"player",p.source_datum},{"unit",p.source_current_object},
                {"representation",(*p.source_configuration)[0x44]},{"membership",p.source_membership},{"definition",definition}});
        }
        report["stage"]="bind original campaign local first-person views";
        std::vector<FirstPersonBindingDefinition> definitions;
        for(const auto &[datum,weapon]:world.content.weapons)
            if(!weapon.first_person_models.empty())
                for(auto representation:source_session_player_representations(world.content,1))
                    {auto binding=cache.load_first_person_binding(datum,int(representation));if(binding.record_present && binding.weapon_model!=null_datum && binding.graph!=null_datum)definitions.push_back(std::move(binding));}
        report["first_person_definitions"]=Json::array();
        for(const auto &definition:definitions)report["first_person_definitions"].push_back({{"representation",definition.representation},{"weapon",definition.weapon_definition},{"graph",definition.graph}});
        report["current_player_weapons"]=Json::array();
        for(auto slot:world.local_slots)report["current_player_weapons"].push_back(world.match.state().players[slot].weapon);
        world.match.configure_local_views(world.local_slots,definitions);
        if(world.match.state().source_game_type!=1)throw FormatError("Source snapshot lost campaign world kind");
        report["stage"]="admit original NONE world-model weapon IK";
        unsigned none_world_models{};
        for(const auto &[datum,weapon]:world.content.weapons) {
            const auto metadata=cache.load_source_tag_metadata(datum);
            if(Bytes(metadata.bytes_at(metadata.source_address(),metadata.size())).u32(0x38u)!=null_datum)continue;
            const auto ik=cache.load_unit_weapon_ik(world.content.player.datum,datum,0x06000086u);
            if(!ik.definition.constraints.empty())throw FormatError("NONE world-model weapon acquired an invented IK marker");
            ++none_world_models;
        }
        if(!none_world_models)throw FormatError("Campaign NONE model IK regression did not reach its real source case");
        report["none_world_model_weapons_without_ik"]=none_world_models;
        report["shield_materials"]=Json::array();
        for(auto shader:{world.content.player.vitality.shield_damaged_shader,world.content.player.vitality.shield_damaged_first_person_shader}) {
            if(shader==null_datum)continue;
            const auto material=cache.load_material(shader);Json row{{"shader",shader},{"name",material.name},{"template",material.template_name},{"passes",Json::array()}};
            for(const auto &pass:material.passes) {
                Json remaps=Json::array();
                for(const auto &remap:pass.pixel_externs)remaps.push_back({{"source",remap.source},{"component",remap.component},{"destination",remap.destination}});
                row["passes"].push_back({{"name",pass.name},{"vertex",pass.vertex_shader},{"pixel_externs",remaps}});
            }
            auto prepared=material;
            if(!prepare_shield_overlay_material(cache,prepared))throw FormatError("Authored campaign change-color shield pass is not connected");
            if(prepared.vertex_program!=material.passes.front().vertex_code)throw FormatError("Campaign shield binding replaced its original vertex program");
            const auto white=prepared.forward_program.words;
            const ObjectColors colors{{{.25f,.5f,.75f},{1,0,0},{0,1,0},{0,0,1}}};
            apply_object_render_colors(prepared,colors);
            if(prepared.forward_program.words==white)throw FormatError("Campaign shield ignored its actual object primary color binding");
            auto invalid=material;invalid.passes.front().pixel_externs.front().source=37;
            if(prepare_shield_overlay_material(cache,invalid))throw FormatError("Campaign shield silently admitted an unowned extern");
            row["authored_program_retained"]=true;row["actual_primary_color_binding_responds"]=true;
            report["shield_materials"].push_back(std::move(row));
        }
        report["stage"]="advance the actual unarmed source campaign player world";
        std::array<Controls,multiplayer_capacity> controls{};
        const auto initial_tick=world.match.simulation().ticks();
        world.match.step(controls);
        if(world.match.simulation().ticks()!=initial_tick+1)
            throw FormatError("Shared campaign source tick did not advance exactly once");
        report["source_ticks_after_unarmed_step"]=world.match.simulation().ticks();
        if(world.match.state().finished)throw FormatError("Campaign inherited multiplayer round end without a registered original engine");
        report["stage"]="admit the complete actual campaign state over loopback UDP";
        const auto fingerprint=multiplayer_fingerprint(options.map,world.cache->shared_directory(),options.bsp,world.rules.weapon,options.gameplay_xbe);
        LanHost host(world.match,0,fingerprint,options.variant);
        LanClient peer("127.0.0.1:"+std::to_string(host.port()),fingerprint,1);
        for(unsigned attempt=0;attempt<64 && !peer.ready();++attempt) {
            peer.send(std::span(controls).first(1));host.receive(controls);host.broadcast();peer.receive();
            if(!peer.ready())std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        report["udp_host_peers"]=host.peers();report["udp_peer_error"]=peer.error();
        if(!peer.ready())throw FormatError("Complete shared campaign UDP snapshot did not admit its actual client: "+peer.error());
        const auto expected=world.match.state();const auto &received=peer.state();
        if(received.source_game_type!=1 || received.source_scene_roots.size()!=expected.source_scene_roots.size() ||
            received.source_body_poses.size()!=expected.source_body_poses.size() || received.world_vehicles.size()!=expected.world_vehicles.size())
            throw FormatError("Campaign UDP admission omitted an original scene/body/vehicle bank");
        unsigned connected{};
        for(unsigned slot=0;slot<multiplayer_capacity;++slot)if(expected.players[slot].connected) {
            ++connected;const auto &actual=received.players[slot];const auto &wanted=expected.players[slot];
            if(!actual.connected || actual.source_player!=wanted.source_player || actual.source_unit!=wanted.source_unit || actual.source_configuration!=wanted.source_configuration)
                throw FormatError("Campaign UDP admission changed actual source player identity/configuration");
        }
        if(connected!=3)throw FormatError("Campaign actual UDP peer did not own its third admitted player");
        report["udp_source_scene_roots"]=received.source_scene_roots.size();
        report["udp_source_body_poses"]=received.source_body_poses.size();
        report["udp_world_vehicles"]=received.world_vehicles.size();
        report["udp_actual_players"]=connected;
        report["stage"]="adopt actual campaign scene/device/prop banks in the joined client";
        auto client_options=options;client_options.source_object_world=false;
        client_options.source_object_savegame_directory.clear();client_options.native_spawn_inputs.clear();
        client_options.campaign_spawn_input={};client_options.local_players=1;
        client_options.join="127.0.0.1:"+std::to_string(host.port());
        MultiplayerPreparedWorld client(client_options);
        client.match.replicate_devices(received.devices);
        client.match.replicate_source_scene(received.source_scene_roots,received.source_body_poses);
        client.match.replicate_prop_removals(received.prop_removals);
        client.match.replicate_prop_regions(received.prop_regions);
        for(const auto &owner:received.source_scene_roots)
            if(client.match.simulation().scene().get(owner.object).placement!=owner.placement)
                throw FormatError("Campaign client adopted an authority handle into a different authored placement");
        report["client_source_scene_roots_adopted"]=received.source_scene_roots.size();
        report["stage"]="shared source player roster and vehicle admitted";report["passed"]=true;
        report["boundary"]="Actual shared prepared-world, original campaign context, two source player records and authored class3 runtime birth. No supplied physical output. Presentation, networking and continued controller frames remain separate gates.";
    } catch(const std::exception &e) {report["error"]=e.what();std::cerr<<e.what()<<'\n';}
    if(!output.empty())std::ofstream(output/"report.json")<<report.dump(2)<<'\n';
    std::cout<<report.dump(2)<<'\n';return report["passed"].get<bool>()?0:1;
}
