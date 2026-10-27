#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include <SDL.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>

namespace {
using namespace halo2;
using Json=nlohmann::json;
void require(bool yes,const char *why){if(!yes)throw FormatError(why);}
Json xyz(Vec3 v){return Json::array({v.x,v.y,v.z});}
Vec3 vec(const Json &j){return {j.at(0).get<float>(),j.at(1).get<float>(),j.at(2).get<float>()};}
struct Clock final:PhysicsLiveClockSource {
    unsigned tick{};mutable std::atomic<std::uint64_t> counter{};
    std::uint32_t milliseconds()const noexcept override{return 1000u+tick*1000u/30u;}
    std::uint64_t cycles()const noexcept override{return counter.fetch_add(1000);}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
}
int main(int argc,char **argv){
    Json report={{"passed",false},{"rendered",true},{"runtime_cache_source","MCC"},
        {"supplied_contacts_or_velocity",false},{"original_superbounce_parity",false},
        {"full_native_physics_closure",false},{"ticks",Json::array()}};
    const auto save=[&]{if(argc==4){std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';}};
    try{
        require(argc==4,"Supply MCC MAP, input JSON, and bounded report path");
        if(!std::getenv("DISPLAY")&&!std::getenv("WAYLAND_DISPLAY"))return 77;
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch,"Use tools/run_test.py");
        const auto directory=std::filesystem::path(scratch)/"rendered-superbounce";
        require(std::filesystem::create_directory(directory),"Rendered superbounce storage already exists");
        std::ifstream input(argv[2]);Json spec;input>>spec;
        require(!spec.value("diagnostic_position_inputs",false),"Rendered replay requires ordinary commands");
        SDL_SetMainReady();MultiplayerOptions options;options.map=argv[1];
        options.shared_maps=options.map.parent_path();options.local_players=1;options.starting_weapon=5;
        options.width=800;options.height=450;options.mute=true;options.source_object_world=true;
        options.source_object_savegame_directory=directory/"source";options.session_random_seed=0x12345678u;
        options.variant.values.set(2,0);options.variant.values.set(10,0);
        const auto facing=vec(spec.at("facing"));Spawn start{};start.position=vec(spec.at("spawn"));
        start.facing=std::atan2(facing.y,facing.x);options.native_spawn_inputs={start};
        std::vector<Controls> expected;
        for(const auto &segment:spec.at("segments")){
            require(!segment.contains("position"),"Rendered replay cannot reposition a live player");
            const auto count=segment.at("ticks").get<unsigned>();require(count<=600,"Unbounded rendered segment");
            for(unsigned n=0;n<count;++n){
                require(expected.size()<3600,"Unbounded rendered replay");Controls c{};
                c.yaw=segment.value("yaw",start.facing);c.pitch=segment.value("pitch",0.f);
                c.forward=segment.value("forward",0.f);c.right=segment.value("right",0.f);
                c.jump=segment.value("jump",false);c.crouch=segment.value("crouch",false);expected.push_back(c);
            }
        }
        options.input_script=directory/"commands.txt";
        {std::ofstream script(options.input_script);script<<std::setprecision(9);
            for(unsigned tick=0;tick<expected.size();++tick){const auto &c=expected[tick];
                script<<tick<<" 0 0 0 0 "<<c.yaw<<' '<<c.pitch<<" move "<<c.forward<<' '<<c.right;
                if(c.jump)script<<" jump";if(c.crouch)script<<" crouch";script<<'\n';
            }
            require(bool(script),"Cannot write owned ordinary input script");
        }
        auto clock=std::make_shared<Clock>();
        options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options,std::nullopt,clock);
        auto &prepared=*options.prepared_world;require(prepared.cache->is_mcc(),"Rendered acceptance requires MCC content");
        require(prepared.source_objects&&prepared.local_slots.size()==1,"Rendered replay lost native source ownership");
        const auto slot=prepared.local_slots.front();auto &world=prepared.source_objects->tu15_world();
        std::array<Controls,multiplayer_capacity> warmup{};warmup[slot].yaw=start.facing;
        ++clock->tick;prepared.match.step(warmup);
        const auto unit=prepared.match.player(slot).source_current_object;
        require(unit!=null_datum,"Rendered replay lost its original player birth");
        const auto initial_tick=prepared.match.state().tick;
        options.capture_tick=initial_tick+unsigned(expected.size());options.reference_tick_cadence=true;
        options.reference_render_fraction=1.f;options.screenshot=directory/"final.ppm";
        const auto ordinary=prepared.content.player.jump_velocity;
        const auto threshold=spec.value("minimum_launch_speed",2.f*ordinary);
        require(std::isfinite(threshold)&&ordinary>0&&threshold>1.5f*ordinary,"Invalid rendered launch threshold");
        float previous=0,maximum=0,flight_height=0;std::optional<Vec3> launch_position;
        unsigned seen=0,submitted=0;std::set<unsigned> captures;
        ++clock->tick;
        options.input_observer=[&](unsigned step,std::span<const Controls> delivered,const MultiplayerState &state){
            require(step<expected.size()&&delivered.size()==1,"Rendered replay delivered an unexpected command");
            const auto &want=expected[step],&actual=delivered.front();
            require(actual.jump==want.jump&&actual.crouch==want.crouch&&actual.forward==want.forward&&
                actual.right==want.right&&actual.yaw==want.yaw&&actual.pitch==want.pitch,
                "Rendered runtime changed a declared player command");
            require(state.players[slot].source_unit==unit&&!state.players[slot].dead,"Rendered replay lost its living original unit");
            const auto body=world.source_object_body(unit);
            const auto scalar=[&](unsigned at){return Bytes(world.memory().view(at,4)).f32(0);};
            const Vec3 p{scalar(body+0x64),scalar(body+0x68),scalar(body+0x6c)},
                v{scalar(body+0x88),scalar(body+0x8c),scalar(body+0x90)};
            require(std::isfinite(p.z)&&std::isfinite(v.z),"Nonfinite rendered movement");
            if(step+1>=spec.value("observe_after_tick",0u)){
                maximum=std::max(maximum,v.z);
                if(!launch_position&&!actual.jump&&previous<-.5f&&v.z>=threshold){
                    launch_position=p;flight_height=p.z;report["launch_tick"]=step+1;
                    report["launch_position"]=xyz(p);
                }
            }
            if(launch_position)flight_height=std::max(flight_height,p.z);
            report["ticks"].push_back({{"tick",step+1},{"position",xyz(p)},{"velocity",xyz(v)},
                {"crouch",scalar(body+0x2c4)},{"jump_input",actual.jump},{"crouch_input",actual.crouch}});
            previous=v.z;++seen;++clock->tick;
        };
        options.frame_observer=[&](const auto &frame){submitted+=frame.submitted;};
        options.frame_capture=[&](const MultiplayerState &state)->std::filesystem::path{
            const auto elapsed=state.tick-initial_tick;
            if((elapsed==20||elapsed==86||elapsed==100||elapsed==140||elapsed==expected.size())&&
                captures.insert(elapsed).second)return directory/("tick-"+std::to_string(elapsed)+".ppm");
            return {};
        };
        const auto result=run_multiplayer(options);
        report.update({{"runtime_status",result},{"stimulus",spec},{"input_ticks",seen},{"submitted_frames",submitted},
            {"tick_rate",30},{"ordinary_jump_speed",ordinary},{"minimum_launch_speed",threshold},
            {"maximum_upward_speed",maximum},{"uncommanded_downward_to_upward_launch",launch_position.has_value()},
            {"maximum_height_after_launch",flight_height},{"neutral_setup_ticks",1},
            {"canonical_source_engine",prepared.match.source_engine_owned()},
            {"owned_captures",directory.string()},{"large_artifacts_retained",Json::array()}});
        if(launch_position)report["launch_height_gain"]=flight_height-launch_position->z;
        require(result==0&&seen==expected.size()&&submitted>20,"Rendered runtime did not complete its finite command sequence");
        require(launch_position&&flight_height-launch_position->z>=spec.value("minimum_launch_height_gain",10.f),
            "Rendered native match did not complete a super-bounce flight");
        report["passed"]=true;save();std::cout<<"PASS rendered native super bounce: "<<maximum<<" upward speed\n";return 0;
    }catch(const std::exception &error){report["error"]=error.what();save();std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}
}
