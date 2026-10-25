#include "halo2/frontend.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/native_vista_flow.hpp"
#include "halo2/source_objective_native_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string_view>

using namespace halo2;
namespace {
void need(bool value,const char* why) {if(!value)throw FormatError(why);}
void choose(MenuState& menu,unsigned row,unsigned screen) {
    menu.select(row);menu.accept();need(menu.frame().screen==screen,"Objective runtime missed an authored menu screen");
}
unsigned entry(const MenuState& menu,int value) {
    const auto& rows=menu.frame().entries;const auto found=std::ranges::find(rows,value,&MenuEntry::value);
    need(found!=rows.end(),"Objective runtime lost an authored menu choice");return unsigned(found-rows.begin());
}
}
int main(int argc,char** argv)try {
    std::cout<<std::unitbuf;
    need(argc==4,"Usage: ctf_assault_runtime_start_tu15_tests MCC_MAP ORIGINAL_PRESETS REPORT");
    const auto map=std::filesystem::path(argv[1]),maps=map.parent_path();
    if(!std::filesystem::is_regular_file(map) || !std::filesystem::is_regular_file(maps/"mainmenu.map"))return 77;
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Objective runtime requires owned scratch");
    const Cache cache(maps/"mainmenu.map");const auto content=cache.load_frontend();
    const auto selected_map=std::ranges::find_if(content.multiplayer_maps,[&](const auto& item) {
        return item.scenario_path.ends_with(map.stem().string());
    });
    need(selected_map!=content.multiplayer_maps.end(),"Supplied MCC map is absent from the real menu");
    nlohmann::json fixture;std::ifstream(argv[2])>>fixture;
    nlohmann::json report={{"passed",false},{"cases",nlohmann::json::array()},
        {"scope","Actual profile sign-in, map and stock variant selection, local Start, native runtime source ownership, three scheduled ticks, and retained outcome after world destruction. No GPU, audio or session-return acceptance."}};
    const bool rendered=std::getenv("HALO2_OBJECTIVE_RENDER");
    if(rendered)report["scope"]="All nine original presets: actual profile/map/variant menu selection, genuine local players, native runtime, three submitted Vulkan frames, genuine objective waypoint CPU/material consumers, source goal submission, and retained outcome after world destruction. Distant spawn cameras can cull goal rings; visible goal geometry has a separate GPU test. Screenshots are temporary. No physical Xbox framebuffer or installed launcher qualification.";
    const unsigned rendered_players=std::getenv("HALO2_OBJECTIVE_RENDER_PLAYERS")?unsigned(std::stoul(std::getenv("HALO2_OBJECTIVE_RENDER_PLAYERS"))):2u;
    need(rendered_players>=2 && rendered_players<=4,"Objective rendered cohort needs two to four actual local players");
    unsigned cases{};
    for(const auto& preset:fixture.at("variants")) {
        const auto type=preset.at("source_type").get<unsigned>();if(type!=1 && type!=9)continue;
        const auto name=preset.at("name").get<std::string>();
        if(const auto* selected=std::getenv("HALO2_OBJECTIVE_VARIANT");selected && name!=selected)continue;
        std::cout<<"Runtime Start: "<<name<<'\n';
        const auto selected_variant=std::ranges::find_if(content.variants,[&](const auto& item) {
            std::string_view title=item.name;if(title.ends_with(" \xee\x80\x88"))title.remove_suffix(4);return title==name;
        });
        need(selected_variant!=content.variants.end(),"Objective runtime lost an authored preset");
        MenuState menu(content);if(rendered)need(menu.controller_slot(100,true)==0,"Objective render host controller missing");menu.accept();choose(menu,0,167);
        menu.text("Objective Run");menu.commit_text();menu.back();need(menu.frame().screen==6,"Objective runtime profile sign-in failed");
        choose(menu,2,30);
        if(rendered)for(unsigned guest=1;guest<rendered_players;++guest) {
            need(menu.controller_slot(100+guest,true)==guest && menu.frame().screen==10,"Objective render guest picker missing");
            menu.accept();menu.text("Objective View "+std::to_string(guest));menu.commit_text();menu.back();
            need(menu.frame().screen==30,"Objective render guest profile missing");
        }
        choose(menu,0,14);
        choose(menu,1,23);choose(menu,0,206);choose(menu,entry(menu,int(selected_map-content.multiplayer_maps.begin())),14);
        choose(menu,1,23);choose(menu,1,207);choose(menu,entry(menu,selected_variant->engine),208);
        choose(menu,entry(menu,int(selected_variant-content.variants.begin())),14);
        if(rendered){menu.auxiliary();menu.navigate(1,0);menu.accept();}
        menu.select(0);menu.accept();need(menu.launch && menu.launch->multiplayer,"Objective menu did not issue a real local Start");
        const auto& launch=*menu.launch;const auto variant=source_game_variant(launch.variant);
        if(std::getenv("HALO2_OBJECTIVE_REQUIRE_RETURN"))
            need(NativeVistaOfflineFlow::supports(launch),"Actual CTF/Assault Start has no native postgame/lobby-return owner");
        const auto raw=preset.at("raw").get<std::string>();need(raw.size()==variant.bytes.size()*2,"Objective runtime fixture is incomplete");
        for(unsigned n=0;n<variant.bytes.size();++n)
            need(variant.bytes[n]==std::stoul(raw.substr(n*2,2),nullptr,16),"Objective runtime handoff changed an original option byte");
        MultiplayerOptions options;options.map=maps/launch.map;
        need(std::filesystem::equivalent(options.map,map),"Objective runtime changed the selected map");
        options.shared_maps=launch.shared_maps;options.variant=launch.variant;options.local_players=launch.local_players;
        options.selected_profiles=launch.selected_profiles;options.selected_teams=launch.selected_teams;
        options.input_instances=launch.input_instances;options.source_object_world=true;options.mute=true;options.ticks=3;
        options.source_object_savegame_directory=std::filesystem::path(scratch)/std::to_string(cases);
        bool loaded{};unsigned initial_tick{},waypoint_draws{},goal_draws{},submitted{};
        if(rendered) {
            options.ticks=0;options.frames=3;options.width=640;options.height=480;options.validation=!std::getenv("HALO2_OBJECTIVE_NO_VALIDATION");
            options.screenshot=std::filesystem::path(scratch)/(std::to_string(cases)+".ppm");
            options.objective_waypoint_draw_observer=[&](unsigned,std::uint32_t,std::span<const Vertex> v){waypoint_draws+=v.size();};
            options.objective_goal_draw_observer=[&](unsigned,std::uint32_t,std::span<const Vertex> v){goal_draws+=v.size();};
            options.frame_observer=[&](const auto& frame){submitted+=frame.submitted;};
        }
        options.session_world_loaded=[&](const MultiplayerMatch& match,std::span<const unsigned> slots) {
            need(match.source_engine_owned() && match.source_engine()==int(type) && slots.size()==launch.local_players,
                "Objective runtime selected a bounded adapter or lost its local player");
            need(match.session_configuration() && source_game_variant(*match.session_configuration()).bytes==variant.bytes,
                "Objective runtime changed the actual menu's source option buffer");
            initial_tick=match.state().tick;loaded=true;
        };
        static_assert(source_objective_native_owner_entries[31]==0x2496b0u);
        const auto before=source_objective_native_tu15_stats;
        MultiplayerRuntimeOutcome outcome;
        need(run_multiplayer(options,outcome)==0 && loaded && outcome.state && outcome.authoritative_banks,
            "Objective runtime lost its real world/outcome handoff");
        const auto& state=*outcome.state;
        need(outcome.reason==(rendered?MultiplayerExitReason::frame_limit:MultiplayerExitReason::headless_limit) && outcome.authority==MultiplayerOutcomeAuthority::local &&
            state.source_engine==int(type) && state.source_engine_entity && (rendered?state.tick>=initial_tick:state.tick==initial_tick+options.ticks) && state.team_mode,
            "Objective runtime lost the selected engine, source entity or actual scheduler ticks");
        if(rendered)need(submitted==3 && std::filesystem::file_size(options.screenshot)>640*480*3,"Objective rendering did not submit/capture actual frames");
        unsigned players{};
        for(const auto& player:state.players)if(player.connected) {
            const auto local=players++;need(player.source_player!=null_datum && player.source_unit!=null_datum && !player.dead &&
                player.source_membership==launch.selected_teams.at(local),"Objective runtime lost the selected profile/team/current unit");
        }
        need(players==launch.local_players && source_objective_native_tu15_stats.owners[31]>before.owners[31],
            "Objective runtime did not execute its native player tick");
        unsigned friendly_waypoints{};
        for(const auto& player:state.players)if(player.connected)
            for(const auto& packet:player.source_objective_presentation.waypoints) {
                const Bytes data(packet.bytes);
                friendly_waypoints+=data.u32(0x44)>0 && data.u32(0x48)==3;
            }
        if(rendered && rendered_players>2)need(friendly_waypoints>0,
            "Three-player native rendering did not reach original friendly-player waypoint production");
        report["cases"].push_back({{"name",name},{"passed",true},{"friendly_waypoints",friendly_waypoints},{"source_engine",type},{"tick",state.tick},
            {"players",players},{"submitted_frames",submitted},{"vulkan_validation",options.validation},{"waypoint_vertices",waypoint_draws},{"goal_vertices",goal_draws},{"source_player",state.players[0].source_player},{"source_unit",state.players[0].source_unit}});
        ++cases;
    }
    need(cases==(std::getenv("HALO2_OBJECTIVE_VARIANT")?1u:9u),"Objective runtime lost a stock preset");report["passed"]=true;
    std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';need(bool(out),"Cannot retain objective runtime evidence");
    std::cout<<"PASS all nine real menu Starts reach native source runtime ticks and retain actual outcomes after world destruction\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
