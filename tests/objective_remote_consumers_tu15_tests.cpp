#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>

using namespace halo2;
namespace {
void need(bool value,const char* message) {if(!value)throw FormatError(message);}
void choose(MenuState& menu,unsigned row,unsigned screen) {
    menu.select(row);menu.accept();need(menu.frame().screen==screen,"Remote objective test missed its actual menu screen");
}
unsigned entry(const MenuState& menu,int value) {
    const auto& rows=menu.frame().entries;const auto found=std::ranges::find(rows,value,&MenuEntry::value);
    need(found!=rows.end(),"Remote objective test lost an authored menu choice");return unsigned(found-rows.begin());
}
FrontendLaunch launch(const FrontendContent& content,const std::filesystem::path& map,
    const std::string& variant,const std::string& name,bool guest) {
    MenuState menu(content);need(menu.controller_slot(100,true)==0,"Remote objective host controller missing");
    menu.accept();choose(menu,0,167);menu.text(name);menu.commit_text();menu.back();
    need(menu.frame().screen==6,"Remote objective profile sign-in failed");choose(menu,2,30);
    if(guest) {
        need(menu.controller_slot(101,true)==1 && menu.frame().screen==10,"Remote objective guest controller missing");
        menu.accept();menu.text("Remote Guest");menu.commit_text();menu.back();
        need(menu.frame().screen==30,"Remote objective guest profile missing");
    }
    choose(menu,0,14);
    const auto m=std::ranges::find_if(content.multiplayer_maps,[&](const auto& item){return item.scenario_path.ends_with(map.stem().string());});
    const auto v=std::ranges::find_if(content.variants,[&](const auto& item){
        std::string_view title=item.name;if(title.ends_with(" \xee\x80\x88"))title.remove_suffix(4);return title==variant;
    });
    need(m!=content.multiplayer_maps.end() && v!=content.variants.end(),"Remote objective actual map/preset absent");
    choose(menu,1,23);choose(menu,0,206);choose(menu,entry(menu,int(m-content.multiplayer_maps.begin())),14);
    choose(menu,1,23);choose(menu,1,207);choose(menu,entry(menu,v->engine),208);
    choose(menu,entry(menu,int(v-content.variants.begin())),14);
    if(guest){menu.auxiliary();menu.navigate(1,0);menu.accept();}
    menu.select(0);menu.accept();need(bool(menu.launch),"Remote objective menu did not produce a launch");return *menu.launch;
}
MultiplayerOptions options(const FrontendLaunch& launch,const std::filesystem::path& map) {
    MultiplayerOptions result;result.map=map;result.shared_maps=map.parent_path();result.variant=launch.variant;
    result.local_players=launch.local_players;result.selected_profiles=launch.selected_profiles;
    result.selected_teams=launch.selected_teams;result.mute=true;return result;
}
class Host {
    std::promise<unsigned> admitted_;
    std::mutex mutex_;
    std::exception_ptr error_;
    std::jthread worker_;
public:
    unsigned port{};
    explicit Host(MultiplayerOptions selected):worker_([this,selected=std::move(selected)](std::stop_token stop)mutable {
        bool admitted=false;
        try {
            MultiplayerPreparedWorld prepared(selected);prepared.bind_source_local_views(prepared.local_slots);
            const auto fingerprint=custom_slayer_fingerprint(multiplayer_fingerprint(selected.map,prepared.cache->shared_directory(),
                std::uint32_t(selected.bsp),prepared.network_starting_weapon_identity(),selected.gameplay_xbe),selected.variant);
            LanHost host(prepared.match,0,fingerprint,selected.variant);admitted_.set_value(host.port());admitted=true;
            std::array<Controls,multiplayer_capacity> controls{};
            auto next=std::chrono::steady_clock::now();const auto end=next+std::chrono::seconds(180);
            while(!stop.stop_requested()) {
                need(std::chrono::steady_clock::now()<end,"Remote objective host exceeded its finite test lifetime");
                host.receive(controls);prepared.match.step(controls);host.broadcast();
                next=std::max(next+std::chrono::nanoseconds(1000000000/30),std::chrono::steady_clock::now());
                std::this_thread::sleep_until(next);
            }
        }catch(...) {
            const auto error=std::current_exception();{const std::lock_guard lock(mutex_);error_=error;}
            if(!admitted)admitted_.set_exception(error);
        }
    }) {
        auto ready=admitted_.get_future();need(ready.wait_for(std::chrono::seconds(60))==std::future_status::ready,
            "Remote objective source host did not initialize");port=ready.get();
    }
    void finish() {worker_.request_stop();worker_.join();const std::lock_guard lock(mutex_);if(error_)std::rethrow_exception(error_);}
};
}
int main(int argc,char** argv)try {
    std::cout<<std::unitbuf;need(argc==4,"Supply MCC map, original presets and remote-consumer report");
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Remote objective consumer test requires owned scratch");
    const auto map=std::filesystem::path(argv[1]);if(!std::filesystem::is_regular_file(map))return 77;
    const auto content=Cache(map.parent_path()/"mainmenu.map").load_frontend();
    nlohmann::json fixture;std::ifstream(argv[2])>>fixture;
    nlohmann::json report={{"passed",false},{"cases",nlohmann::json::array()},{"scope",
        "Actual source-owned LAN host and rendered native remote client, real menu profiles/presets, authenticated join, native objective waypoint/goal/audio consumers and retained client snapshot. No host collision result or local authority is fabricated."}};
    const auto save=[&]{std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';need(bool(out),"Cannot retain remote objective report");};
    unsigned cases{},failures{};
    for(const auto& preset:fixture.at("variants")) {
        const auto type=preset.at("source_type").get<unsigned>();if(type!=1 && type!=9)continue;
        const auto name=preset.at("name").get<std::string>();
        if(const auto* selected=std::getenv("HALO2_OBJECTIVE_VARIANT");selected && name!=selected)continue;
        nlohmann::json row={{"name",name},{"passed",false}};std::cout<<"Remote objective render: "<<name<<'\n';
        try {
            auto server=options(launch(content,map,name,"Remote Host",true),map);server.host_port=1;
            server.source_object_savegame_directory=std::filesystem::path(scratch)/("host-"+std::to_string(cases));
            Host host(std::move(server));
            auto client=options(launch(content,map,name,"Objective Peer",false),map);
            client.join="127.0.0.1:"+std::to_string(host.port);client.frames=120;client.width=640;client.height=480;
            client.validation=!std::getenv("HALO2_OBJECTIVE_NO_VALIDATION");
            client.screenshot=std::filesystem::path(scratch)/("peer-"+std::to_string(cases)+".ppm");
            unsigned vertices{},submitted{},ready_views{};
            client.objective_waypoint_draw_observer=[&](unsigned,std::uint32_t,std::span<const Vertex> value){vertices+=value.size();};
            client.frame_observer=[&](const auto& frame){
                submitted+=frame.submitted;
                if(frame.submitted) {
                    // Actual native mouse input turns the admitted remote
                    // view through objectives. Camera culling remains live.
                    SDL_Event motion{};motion.type=SDL_MOUSEMOTION;
                    if(auto* window=SDL_GetKeyboardFocus())motion.motion.windowID=SDL_GetWindowID(window);
                    motion.motion.xrel=64;need(SDL_PushEvent(&motion)==1,"Cannot deliver native remote look input");
                }
            };
            client.view_state_observer=[&](unsigned slot,const auto& state,const auto&){ready_views+=state.players[slot].connected && state.players[slot].source_unit!=null_datum;};
            MultiplayerRuntimeOutcome outcome;need(run_multiplayer(client,outcome)==0,"Actual remote objective runtime returned failure");
            host.finish();
            need(outcome.authority==MultiplayerOutcomeAuthority::client_snapshot && outcome.state && !outcome.authoritative_banks,
                "Remote objective rendering fabricated a local source authority/outcome");
            row["submitted_frames"]=submitted;row["ready_views"]=ready_views;row["waypoint_vertices"]=vertices;
            need(outcome.state->source_engine==int(type) && submitted==client.frames && ready_views && vertices &&
                std::filesystem::file_size(client.screenshot)>640*480*3,"Remote objective client did not render its genuine admitted objective waypoints");
            row["passed"]=true;row["submitted_frames"]=submitted;row["ready_views"]=ready_views;row["waypoint_vertices"]=vertices;
            row["tick"]=outcome.state->tick;
        }catch(const std::exception& e){row["error"]=e.what();++failures;std::cerr<<name<<": "<<e.what()<<'\n';}
        report["cases"].push_back(std::move(row));++cases;save();
    }
    need(cases==(std::getenv("HALO2_OBJECTIVE_VARIANT")?1u:9u),"Remote objective test lost a preset");
    report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
