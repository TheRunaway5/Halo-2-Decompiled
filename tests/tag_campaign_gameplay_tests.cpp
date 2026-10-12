#define SDL_MAIN_HANDLED
#include "halo2/simulation.hpp"
#include "halo2/navigation.hpp"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

// The Linux test executable links the real application entry point and wraps
// its step call only to observe state. Inputs still enter through SDL. No
// production source or simulation state is changed by this observer.
int tag_campaign_runtime_entry(int, char**);
extern "C" void actual_step(halo2::Simulation*, const halo2::Controls&)
    asm("__real__ZN5halo210Simulation4stepERKNS_8ControlsE");
extern "C" void observed_step(halo2::Simulation*, const halo2::Controls&)
    asm("__wrap__ZN5halo210Simulation4stepERKNS_8ControlsE");

namespace {
using namespace halo2;
SDL_Joystick* pad{};
SDL_Window* application_window{};
std::unique_ptr<NavigationWorld> navigation;
std::unique_ptr<Cache> campaign_cache;
std::vector<Vec3> route;
std::ofstream trace;
unsigned fired{}, hits{}, dead{}, moved{}, observed{}, fire_inputs{};
unsigned faulted_ticks{}, skip_inputs{}, peak_actors{};
std::uint64_t restores{};
Vec3 initial_position;
bool done{};
bool normal_campaign{}, skip_sent{};
std::size_t observed_bsp = std::size_t(-1);
void check(bool value, const char* message) { if (!value) throw FormatError(message); }
void axis(SDL_GameControllerAxis axis, float value) {
    check(SDL_JoystickSetVirtualAxis(pad,axis,Sint16(std::clamp(value,-1.f,1.f)*32767.f))==0,
          "Campaign SDL axis rejected");
}
void button(SDL_GameControllerButton button, bool value) {
    check(SDL_JoystickSetVirtualButton(pad,button,value)==0,"Campaign SDL button rejected");
}
float look_axis(float error) {
    if(std::abs(error)<.012f) return 0;
    return std::copysign(std::clamp(std::abs(error)*4.f,.18f,1.f),error);
}
}

extern "C" SDL_Window* __real_SDL_CreateWindow(const char*,int,int,int,int,Uint32);
extern "C" SDL_Window* __wrap_SDL_CreateWindow(const char* title,int x,int y,int w,int h,Uint32 flags) {
    application_window=__real_SDL_CreateWindow(title,x,y,w,h,flags);
    return application_window;
}

extern "C" void observed_step(halo2::Simulation* simulation, const halo2::Controls& input) {
    actual_step(simulation,input);
    using namespace halo2;
    const auto& p=simulation->player();
    if(!observed++) {
        initial_position=p.body.position;
        // The application's ordinary click-to-capture path also admits its
        // controller fire input. Exercise that event path before shooting.
        auto* window=application_window;check(window,"Campaign application window was not created");
        SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);
        for(bool down:{true,false}) {
            SDL_Event event{};event.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
            event.button.windowID=SDL_GetWindowID(window);event.button.button=SDL_BUTTON_LEFT;
            event.button.state=down?SDL_PRESSED:SDL_RELEASED;
            check(SDL_PushEvent(&event)==1,"Campaign capture click rejected");
        }
    }
    moved+=length(p.body.position-initial_position)>1.f;
    fire_inputs+=input.fire;
    faulted_ticks+=!simulation->scripts().diagnostics().empty();
    skip_inputs+=input.skip_cinematic;
    restores=simulation->checkpoint_restores();
    peak_actors=std::max(peak_actors,unsigned(simulation->actors().size()));
    fired=unsigned(simulation->shots()); hits=unsigned(simulation->hits());
    dead=unsigned(std::count_if(simulation->actors().begin(),simulation->actors().end(),
        [](const auto& actor){return actor.state==Unit::Dead;}));
    trace<<simulation->ticks()<<'\t'<<p.body.position.x<<'\t'<<p.body.position.y<<'\t'
         <<p.body.position.z<<'\t'<<p.yaw<<'\t'<<p.pitch<<'\t'<<p.weapon.loaded<<'\t'
         <<p.weapon.reserve<<'\t'<<p.vitality.body<<'\t'<<p.vitality.shield<<'\t'
         <<fired<<'\t'<<hits<<'\t'<<dead<<'\t'<<input.fire<<'\t'<<simulation->bsp()<<'\t'
         <<simulation->scripts().diagnostics().size()<<'\t'<<simulation->has_checkpoint()<<'\t'
         <<simulation->checkpoint_restores()<<'\t'<<input.skip_cinematic<<'\n';
    if(normal_campaign && !skip_sent && simulation->cinematic_skippable()) {
        for(bool down:{true,false}) {
            SDL_Event event{}; event.type=down?SDL_KEYDOWN:SDL_KEYUP;
            event.key.windowID=SDL_GetWindowID(application_window);
            event.key.keysym.sym=SDLK_ESCAPE;event.key.keysym.scancode=SDL_SCANCODE_ESCAPE;
            event.key.state=down?SDL_PRESSED:SDL_RELEASED;
            check(SDL_PushEvent(&event)==1,"Campaign cinematic skip key rejected");
        }
        skip_sent=true;
        std::cout<<"Ordinary cinematic skip requested at tick="<<simulation->ticks()<<std::endl;
    }
    if(normal_campaign && observed_bsp!=simulation->bsp()) {
        const auto content=campaign_cache->load_bsp_content(simulation->bsp());
        navigation=std::make_unique<NavigationWorld>(content.navigation);
        route.clear();observed_bsp=simulation->bsp();
    }
    const auto origin=p.body.position+Vec3{0,0,p.definition.camera_height};
    const Unit* target{};
    for(const auto& actor:simulation->actors())
        if(actor.exists && actor.state!=Unit::Dead && actor.team!=p.team &&
           (!target || length(actor.body.position-origin)<length(target->body.position-origin))) target=&actor;
    if(target) {
        const auto goal=target->body.position+Vec3{0,0,target->definition.height*.55f};
        const auto delta=goal-origin; const float distance=length(delta);
        const auto wall=simulation->collision().raycast(origin,normalized(delta),distance);
        const bool visible=!wall || wall->distance>=distance-.15f;
        if(simulation->ticks()%30==0 || route.empty()) route=navigation->route(p.body.position,target->body.position);
        while(!route.empty() && length(route.front()-p.body.position)<.6f) route.erase(route.begin());
        const auto toward=(visible || route.empty()?target->body.position:route.front())-p.body.position;
        const auto look=visible?delta:toward;
        const float yaw=std::atan2(look.y,look.x),pitch=visible?std::atan2(look.z,std::hypot(look.x,look.y)):0;
        axis(SDL_CONTROLLER_AXIS_RIGHTX,-look_axis(std::remainder(yaw-input.yaw,6.283185307f)));
        axis(SDL_CONTROLLER_AXIS_RIGHTY,-look_axis(pitch-input.pitch));
        const float planar=std::hypot(toward.x,toward.y);
        const bool walk=!visible || distance>5.f;
        axis(SDL_CONTROLLER_AXIS_LEFTY,walk && planar>.1f?
            -(toward.x*std::cos(input.yaw)+toward.y*std::sin(input.yaw))/planar:0);
        axis(SDL_CONTROLLER_AXIS_LEFTX,walk && planar>.1f?
            (toward.x*std::sin(input.yaw)-toward.y*std::cos(input.yaw))/planar:0);
        axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,visible && distance<40.f?1.f:-1.f);
        button(SDL_CONTROLLER_BUTTON_X,p.weapon.loaded==0 && simulation->ticks()%90<3);
        button(SDL_CONTROLLER_BUTTON_A,walk && simulation->ticks()%120<2);
        if(simulation->ticks()%300==0) std::cout<<"Campaign target distance="<<distance
            <<" visible="<<visible<<" route="<<route.size()<<' '<<simulation->status()<<std::endl;
    } else if(dead || normal_campaign) {
        // Continue walking after clearing the encounter, through the same
        // controller path, so a short fight still verifies movement updates.
        axis(SDL_CONTROLLER_AXIS_LEFTY,-.8f);axis(SDL_CONTROLLER_AXIS_LEFTX,0);
        axis(SDL_CONTROLLER_AXIS_RIGHTX,0);axis(SDL_CONTROLLER_AXIS_RIGHTY,0);
        axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-1);
        button(SDL_CONTROLLER_BUTTON_X,false);button(SDL_CONTROLLER_BUTTON_A,false);
    }
    if(!done && ((dead && fired>=12 && hits && moved>100) || simulation->ticks()>=1800)) {
        for(const auto& fault:simulation->scripts().diagnostics())
            std::cerr<<"Observed campaign script fault: "<<fault<<'\n';
        SDL_Event event{}; event.type=SDL_QUIT;
        check(SDL_PushEvent(&event)==1,"Could not close campaign verification window"); done=true;
    }
}

int main(int argc,char** argv) try {
    check(argc==5,"Supply MAP SHARED SQUAD|--normal OUTPUT_DIRECTORY");
    if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1])) return 77;
    const std::filesystem::path output=argv[4];std::filesystem::create_directories(output);
    Cache cache(argv[1],argv[2]);const auto content=cache.load_content(0);
    normal_campaign=std::string_view(argv[3])=="--normal";
    if(normal_campaign)campaign_cache=std::make_unique<Cache>(argv[1],argv[2]);
    if(std::string_view(argv[3])=="--list-encounters") {
        check(!cache.spawns().empty(),"Campaign map has no authored player start");
        const auto spawn=cache.spawns().front();
        std::cout<<"Player team="<<content.player.team<<" start="<<spawn.position.x<<','
            <<spawn.position.y<<','<<spawn.position.z<<" bsp="<<spawn.bsp<<'\n';
        std::vector<std::pair<float,const SquadDefinition*>> nearby;
        for(const auto& squad:content.squads) if(squad.count && (squad.bsp==0 || squad.bsp==-1)) {
            float distance=INFINITY;
            for(const auto& start:squad.starts) distance=std::min(distance,length(start.position-spawn.position));
            nearby.emplace_back(distance,&squad);
        }
        std::sort(nearby.begin(),nearby.end(),[](const auto& a,const auto& b){return a.first<b.first;});
        for(unsigned i=0;i<std::min<std::size_t>(nearby.size(),20);++i)
            std::cout<<nearby[i].first<<" team="<<nearby[i].second->team<<" count="
                <<nearby[i].second->count<<" squad="<<nearby[i].second->name<<'\n';
        return 0;
    }
    navigation=std::make_unique<NavigationWorld>(content.navigation);
    const auto weapon=std::find_if(content.weapons.begin(),content.weapons.end(),
        [](const auto& item){return item.second.name.ends_with("\\smg\\smg");});
    check(normal_campaign || weapon!=content.weapons.end(),"Campaign map has no authored SMG");
    std::cout<<"Campaign authored start="<<cache.spawns().front().position.x<<','
             <<cache.spawns().front().position.y<<','<<cache.spawns().front().position.z<<'\n';
    for(const auto& squad:content.squads) if(squad.name==argv[3])
        for(const auto& start:squad.starts) std::cout<<"Authored actor start="<<start.position.x<<','
            <<start.position.y<<','<<start.position.z<<'\n';
    SDL_SetMainReady();SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
    check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)==0,"Campaign SDL initialization failed");
    const int device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
        SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
    check(device>=0 && SDL_IsGameController(device),"Campaign virtual controller unavailable");
    pad=SDL_JoystickOpen(device);check(pad,"Campaign virtual controller did not open");
    axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT,-1);axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-1);
    trace.open(output/"ticks.tsv");
    trace<<"tick\tx\ty\tz\tyaw\tpitch\tammo\treserve\thealth\tshield\tshots\thits\tdead_actors\tfire_input\tbsp\tscript_faults\tcheckpoint\trestores\tskip_input\n";
    std::vector<std::string> arguments{"tag_campaign_gameplay_tests","--map",argv[1],
        "--shared-maps",argv[2],"--viewport","1280","720"};
    if(normal_campaign)arguments.emplace_back("--campaign");
    else arguments.insert(arguments.end(),{"--play","--place-squad",argv[3],"--weapon",weapon->second.name});
    std::vector<char*> pointers;for(auto& argument:arguments)pointers.push_back(argument.data());
    const int result=tag_campaign_runtime_entry(int(pointers.size()),pointers.data());
    SDL_JoystickClose(pad);SDL_JoystickDetachVirtual(device);
    std::cout<<"Campaign observation: normal_startup="<<normal_campaign<<" ticks="<<observed
        <<" shots="<<fired<<" hits="<<hits<<" dead_actors="<<dead<<" peak_actors="<<peak_actors
        <<" moved_ticks="<<moved<<" faulted_ticks="<<faulted_ticks<<" skip_inputs="<<skip_inputs
        <<" checkpoint_restores="<<restores<<'\n';
    check(result==0,"Actual campaign runtime failed");
    check(!normal_campaign || !faulted_ticks,"Normal campaign startup encountered script faults");
    check(observed>100 && fired>=12 && hits && dead && moved>100 && fire_inputs,
        "Campaign encounter did not exercise actual movement, shots, hits and fatal damage");
    std::cout<<"PASS visible authored campaign encounter: ticks="<<observed<<" shots="<<fired
        <<" hits="<<hits<<" dead_actors="<<dead<<" moved_ticks="<<moved<<'\n';
    return 0;
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
