#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/animation_presentation.hpp"
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
void reference_selected(bool enabled) {
#ifdef _WIN32
    _putenv_s("HALO2_REFERENCE_REPEAT_SELECTED_SKINS",enabled?"1":"");
#else
    if(enabled)setenv("HALO2_REFERENCE_REPEAT_SELECTED_SKINS","1",1);
    else unsetenv("HALO2_REFERENCE_REPEAT_SELECTED_SKINS");
#endif
}
struct Draw {
    unsigned view{},lod{};std::uint32_t owner{},definition{};bool light_valid{};
    std::vector<Vertex> vertices;
};
template<class T> void append(std::vector<std::byte> &out,std::span<const T> values) {
    const auto bytes=std::as_bytes(values);out.insert(out.end(),bytes.begin(),bytes.end());
}
template<class T> void append(std::vector<std::byte> &out,const T &value) {
    append(out,std::span<const T>(&value,1));
}
std::vector<std::byte> selected_bytes(const AnimationRenderObservation &frame) {
    std::vector<std::byte> out;
    for(auto value:{frame.tick,frame.unit,frame.weapon,frame.graph,frame.animation,
        frame.ammunition_animation,frame.body_activity})append(out,value);
    for(auto value:{frame.fraction,frame.frame,frame.ammunition_frame,frame.body_facing})append(out,value);
    append(out,frame.first_person);append(out,frame.interpolated);
    append(out,frame.completed_locomotion_frame.has_value());
    if(frame.completed_locomotion_frame)append(out,*frame.completed_locomotion_frame);
    for(auto vertices:{frame.vertices,frame.weapon_vertices}) {
        append(out,vertices.size());append(out,vertices);
    }
    for(auto nodes:{frame.weapon_nodes,frame.body_nodes,frame.completed_weapon_nodes}) {
        append(out,nodes.size());append(out,nodes);
    }
    return out;
}
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
    std::vector<std::vector<std::byte>> selected;std::size_t selected_cursor{};
    unsigned selected_requests{},selected_reuses{},bodies{},weapons{};
    options.animation_observer=[&](unsigned view,unsigned player,const AnimationRenderObservation &frame) {
        if(frame.first_person)return;
        auto bytes=selected_bytes(frame);append(bytes,view);append(bytes,player);
        if(!checking) {
            selected.push_back(std::move(bytes));
            bodies+=!frame.vertices.empty();weapons+=!frame.weapon_vertices.empty();return;
        }
        require(selected_cursor<selected.size(),"Selected skin path added an animation submission");
        require(selected[selected_cursor++]==bytes,
            "Selected skin path changed body/weapon identities, nodes or complete vertex bytes");
    };
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
        selected_requests+=frame.selected_skin_requests;selected_reuses+=frame.selected_skin_reuses;
    };
    reference_selected(false);reference(true);prepare(options);
    require(run_multiplayer(options)==0 && submitted==3 && !expected.empty(),"Reference unit geometry was not submitted");
    require(builds==0 && reuses==0,"Reference control unexpectedly reused unit skins");
    require(selected_requests>0 && selected_reuses==0 && bodies>0 && weapons>0,
        "Reference path did not exercise fresh selected bodies and primary weapons");
    const auto reference_requests=selected_requests;
    checking=true;submitted=0;selected_requests=selected_reuses=0;
    reference(false);reference_selected(argc==4);prepare(options);
    require(run_multiplayer(options)==0 && submitted==3,"Cached unit frames were not submitted");
    require(cursor==expected.size(),"Cached unit path omitted a draw");
    require(selected_cursor==selected.size() && selected_requests==reference_requests,
        "Selected skin path omitted a submission or changed request count");
    require(selected_reuses>0,"Four-view selected path did not eliminate repeated skins");
    require(builds>0 && reuses>0,"Four-view unit path did not eliminate repeated skins");
    std::cout<<"PASS four split views, "<<cursor<<" identical ordered unit draws, full vertex bytes exact, "
        <<builds<<" skins built, "<<reuses<<" repeated skins reused; "<<selected_cursor
        <<" exact selected body/weapon submissions, "<<selected_reuses<<"/"<<selected_requests
        <<" selected requests reused\n";
    reference(false);reference_selected(false);return 0;
}catch(const std::exception &error){reference(false);reference_selected(false);std::cerr<<error.what()<<'\n';return 1;}
