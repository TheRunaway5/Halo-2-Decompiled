#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/desktop_session.hpp"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
void reference(bool enabled) {
#ifdef _WIN32
    _putenv_s("HALO2_REFERENCE_REPEAT_UNIT_SKINS",enabled?"1":"");
#else
    if(enabled)setenv("HALO2_REFERENCE_REPEAT_UNIT_SKINS","1",1);
    else unsetenv("HALO2_REFERENCE_REPEAT_UNIT_SKINS");
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
    const auto initial=world.match.state();Vec3 center{};
    for(auto slot:world.local_slots) {
        const auto &p=initial.players[slot].position;
        center.x+=p.x/options.local_players;center.y+=p.y/options.local_players;
    }
    std::array<Controls,multiplayer_capacity> controls{};
    for(auto slot:world.local_slots) {
        const auto &p=initial.players[slot].position;
        controls[slot].yaw=std::atan2(center.y-p.y,center.x-p.x);
    }
    // Ordinary retained source players and actual input, with no injected
    // birth, pose, visibility, camera or cache backing.
    for(unsigned tick=0;tick<60;++tick)world.match.step(controls);
}
}
int main(int argc,char **argv)try {
    require(argc==3 || argc==4,"Supply MCC_MAP SHARED_MAPS [force-reference]");
    if(!std::filesystem::is_regular_file(argv[1])) {
        std::cout<<"SKIP: supply prepared MCC Coagulation map\n";return 77;
    }
    SDL_SetMainReady();MultiplayerOptions options;
    options.map=argv[1];options.shared_maps=argv[2];options.local_players=4;
    options.frames=3;options.width=640;options.height=480;options.source_object_world=true;
    options.starting_weapon=0;options.variant.values.set(76,1);options.mute=true;
    options.session_random_seed=0x1376c0;options.reference_tick_cadence=true;
    options.reference_render_fraction=.5f;options.input_observer=[](auto,auto,const auto &){};
    std::vector<Draw> expected;std::size_t cursor{};unsigned submitted{},builds{},reuses{};bool checking{};
    options.unit_source_draw_observer=[&](unsigned view,std::uint32_t owner,std::uint32_t definition,
        const ObjectLightingState &,bool light_valid,unsigned lod,std::span<const Vertex> vertices) {
        if(!checking){expected.push_back({view,lod,owner,definition,light_valid,{vertices.begin(),vertices.end()}});return;}
        require(cursor<expected.size(),"Cached unit path added a draw");const auto &draw=expected[cursor++];
        require(draw.view==view && draw.owner==owner && draw.definition==definition && draw.lod==lod &&
            draw.light_valid==light_valid && draw.vertices.size()==vertices.size(),
            "Cached unit path changed source draw identity/order/selection");
        require(vertices.empty() || !std::memcmp(draw.vertices.data(),vertices.data(),vertices.size_bytes()),
            "Cached unit path changed complete submitted vertex bytes");
    };
    options.frame_observer=[&](const MultiplayerFrameTiming &frame) {
        submitted+=frame.submitted;builds+=frame.unit_skin_builds;reuses+=frame.unit_skin_reuses;
    };
    reference(true);prepare(options);
    require(run_multiplayer(options)==0 && submitted==3 && !expected.empty(),"Reference unit geometry was not submitted");
    require(builds==0 && reuses==0,"Reference control unexpectedly reused unit skins");
    checking=true;submitted=0;reference(argc==4);prepare(options);
    require(run_multiplayer(options)==0 && submitted==3,"Cached unit frames were not submitted");
    require(cursor==expected.size(),"Cached unit path omitted a draw");
    require(builds>0 && reuses>0,"Four-view unit path did not eliminate repeated skins");
    std::cout<<"PASS four split views, "<<cursor<<" identical ordered unit draws, full vertex bytes exact, "
        <<builds<<" skins built, "<<reuses<<" repeated skins reused\n";
    reference(false);return 0;
}catch(const std::exception &error){reference(false);std::cerr<<error.what()<<'\n';return 1;}
