#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/desktop_session.hpp"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
void reference(bool enabled) {
#ifdef _WIN32
    _putenv_s("HALO2_REFERENCE_REPEAT_VEHICLE_SKINS",enabled?"1":"");
    _putenv_s("HALO2_REFERENCE_INSTRUCTION_CALLBACKS",enabled?"1":"");
#else
    for(const auto *name:{"HALO2_REFERENCE_REPEAT_VEHICLE_SKINS","HALO2_REFERENCE_INSTRUCTION_CALLBACKS"}) {
        if(enabled)setenv(name,"1",1);else unsetenv(name);
    }
#endif
}
struct Draw {
    unsigned view{},lod{};std::uint32_t owner{},definition{};bool light_valid{};
    std::vector<Vertex> vertices;
};
void prepare(MultiplayerOptions &options) {
    options.prepared_world.reset();
    options.desktop=std::make_shared<DesktopSession>();options.desktop->auxiliary_random()=1;
    options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
    auto &world=*options.prepared_world;world.bind_source_local_views(world.local_slots);
    std::array<Controls,multiplayer_capacity> controls{};
    const auto initial=world.match.state();
    for(auto slot:world.local_slots)controls[slot].yaw=initial.players[slot].yaw;
    for(unsigned tick=0;tick<60;++tick)world.match.step(controls);
    require(!world.match.state().world_vehicles.empty(),"Original constructor did not publish vehicle snapshot");
}
}
int main(int argc,char **argv)try {
    require(argc==4 || argc==5,"Supply MCC_MAP SHARED_MAPS PLAYERS [force-reference]");
    if(!std::filesystem::is_regular_file(argv[1])) {
        std::cout<<"SKIP: supply prepared MCC Coagulation map\n";return 77;
    }
    SDL_SetMainReady();
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
    options.local_players=std::stoul(argv[3]);options.frames=3;options.width=640;options.height=480;
    options.source_object_world=true;options.starting_weapon=0;options.variant.values.set(76,1);
    options.mute=true;options.session_random_seed=0x1376c0;
    options.reference_tick_cadence=true;
    options.reference_render_fraction=.5f;
    options.input_observer=[](auto,auto,const auto &){};
    require(options.local_players==2 || options.local_players==4,"Use two or four actual split views");
    std::vector<Draw> expected;std::size_t cursor{};unsigned submitted{},builds{},reuses{};
    bool checking=false;
    options.vehicle_draw_observer=[&](unsigned view,std::uint32_t owner,std::uint32_t definition,
        const ObjectLightingState &,bool light_valid,unsigned lod,std::span<const Vertex> vertices) {
        if(!checking){expected.push_back({view,lod,owner,definition,light_valid,{vertices.begin(),vertices.end()}});return;}
        require(cursor<expected.size(),"Cached vehicle path added a draw");
        const auto &draw=expected[cursor++];
        require(draw.view==view && draw.lod==lod && draw.owner==owner && draw.definition==definition &&
            draw.light_valid==light_valid && draw.vertices.size()==vertices.size(),
            "Cached vehicle path changed source draw identity/order/selection");
        require(vertices.empty() || !std::memcmp(draw.vertices.data(),vertices.data(),vertices.size_bytes()),
            "Cached vehicle path changed complete submitted vertex bytes");
    };
    options.frame_observer=[&](const MultiplayerFrameTiming &frame) {
        submitted+=frame.submitted;builds+=frame.vehicle_skin_builds;reuses+=frame.vehicle_skin_reuses;
    };
    reference(true);
    prepare(options);
    require(run_multiplayer(options)==0 && submitted==3 && !expected.empty(),"Reference vehicle frames were not observed");
    require(builds==0 && reuses==0,"Reference control unexpectedly used cached vehicle skins");
    checking=true;submitted=0;reference(argc==5);
    prepare(options);
    require(run_multiplayer(options)==0 && submitted==3,"Cached vehicle frames were not submitted");
    require(cursor==expected.size(),"Cached vehicle path omitted a draw");
    require(builds>0 && reuses>0,"Split vehicle path did not eliminate repeated section/view skins");
    std::cout<<"PASS "<<options.local_players<<" split views, "<<cursor
        <<" identical ordered vehicle draws, complete submitted vertex bytes exact, "
        <<builds<<" skins built, "<<reuses<<" repeated skins reused\n";
    reference(false);return 0;
}catch(const std::exception &error){reference(false);std::cerr<<error.what()<<'\n';return 1;}
