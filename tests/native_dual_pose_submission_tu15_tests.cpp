#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/math.hpp"
#include "halo2/model_skin_submission.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <tuple>
#if defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#endif
#ifndef _WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool value,const char *message){if(!value)throw FormatError(message);}
float error(std::span<const Transform> a,std::span<const Transform> b) {
    need(a.size()==b.size(),"Submitted bank lost its actual model-order node identity");
    float result{};
    for(unsigned n=0;n<a.size();++n) {
        const auto x=std::bit_cast<std::array<float,13>>(a[n]),y=std::bit_cast<std::array<float,13>>(b[n]);
        for(unsigned k=0;k<x.size();++k) {
            need(std::isfinite(x[k]) && std::isfinite(y[k]),"Submitted bank contains a nonfinite native component");
            result=std::max(result,std::abs(x[k]-y[k]));
        }
    }
    return result;
}
std::uint64_t digest(std::span<const Transform> nodes) {
    std::uint64_t hash=1469598103934665603ull;
    for(const auto byte:std::as_bytes(nodes))hash=(hash^std::to_integer<unsigned char>(byte))*1099511628211ull;
    return hash;
}
unsigned unused_udp_port() {
#ifndef _WIN32
    const auto fd=::socket(AF_INET,SOCK_DGRAM,0);need(fd>=0,"Allocate private native-host UDP port");
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(::bind(fd,reinterpret_cast<sockaddr*>(&address),sizeof(address))) {::close(fd);throw FormatError("Bind private native-host UDP port");}
    socklen_t size=sizeof(address);const auto result=::getsockname(fd,reinterpret_cast<sockaddr*>(&address),&size);::close(fd);
    need(!result,"Read private native-host UDP port");return ntohs(address.sin_port);
#else
    throw FormatError("Native submission fixture requires its POSIX private UDP port owner");
#endif
}
void hold(Uint32 window,bool down) {
    SDL_Event event{};event.type=down?SDL_KEYDOWN:SDL_KEYUP;event.key.windowID=window;
    event.key.keysym.scancode=SDL_SCANCODE_C;event.key.state=down?SDL_PRESSED:SDL_RELEASED;
    need(SDL_PushEvent(&event)==1,"Queue actual native dual-pickup SDL input");
}
std::uint32_t fixture_weapon(const MultiplayerPreparedWorld &world) {
    // Source-owned rules intentionally leave rules.weapon as NONE: the
    // original spawn owner selects it. Resolve the fixture's admitted choice
    // through the same authored Cache table used by the native-class test.
    return world.cache->slayer_starting_weapon(5);
}
Json setup_state(const MultiplayerPreparedWorld &world) {
    const auto state=world.match.state();Json result{{"tick",state.tick},{"weapon",fixture_weapon(world)},
        {"rules_weapon",world.rules.weapon},{"players",Json::array()},{"loose_weapons",Json::array()}};
    for(unsigned id=0;id<state.players.size();++id) {
        const auto &p=state.players[id];if(!p.connected)continue;
        result["players"].push_back({{"slot",id},{"unit",p.source_unit},{"primary",p.source_weapon},
            {"offhand",p.secondary_weapon.datum},{"dead",p.dead},
            {"position",{p.position.x,p.position.y,p.position.z}}});
    }
    for(const auto &item:state.world_weapons)result["loose_weapons"].push_back(
        {{"object",item.handle},{"definition",item.definition},{"position",{item.position.x,item.position.y,item.position.z}}});
    return result;
}
void retain_setup_fade(MultiplayerPreparedWorld &world,SourceGameplayFadePresentationTu15 &fade) {
    const auto state=world.match.state();
    if(!state.source_gameplay_fade)return;
    std::vector<std::uint32_t> local_players;
    for(const auto slot:world.local_slots) {
        const auto player=state.players.at(slot).source_player;
        if(player==null_datum)return; // The genuine deferred player birth has not happened yet.
        local_players.push_back(player);
    }
    if(!local_players.empty())fade.consume(*state.source_gameplay_fade,local_players);
}
void acquire(MultiplayerPreparedWorld &world,std::span<const unsigned> players,Json *diagnostic=nullptr,
    SourceGameplayFadePresentationTu15 *fade=nullptr) {
    const auto weapon=fixture_weapon(world);
    unsigned respawn_seconds{};
    Json respawn_rows=Json::array();
    for(unsigned index=0;index<world.content.netgame_equipment.size();++index) {
        const auto &row=world.content.netgame_equipment[index];
        if(std::none_of(row.items.begin(),row.items.end(),
                [&](const auto &item){return item.definition==weapon;}))continue;
        respawn_seconds=std::max(respawn_seconds,unsigned(std::max(0,int(row.respawn_seconds))));
        respawn_rows.push_back({{"row",index},{"respawn_seconds",row.respawn_seconds},
            {"timer_start",row.timer_start},{"flags",row.flags}});
    }
    // Lockout has one loose SMG. Its original scenario timer must replenish
    // that actual generation before a second participant can acquire a pair.
    const unsigned tick_budget=std::max(200u,30u*respawn_seconds+200u);
    const auto setup=[&] {
        auto result=setup_state(world);result["pickup_tick_budget"]=tick_budget;
        result["authored_weapon_respawn_rows"]=respawn_rows;return result;
    };
    if(fade)retain_setup_fade(world,*fade);
    const auto step=[&](const std::array<Controls,multiplayer_capacity> &input) {
        world.match.step(input);
        if(fade)retain_setup_fade(world,*fade);
    };
    // Complete one actual source transfer before selecting another loose
    // generation. No two participants compete for an in-flight pickup.
    for(const auto id:players) {
        std::optional<Vec3> home;bool acquired{};
        for(unsigned tick=0;tick<tick_budget;++tick) {
            const auto state=world.match.state();const auto &p=state.players.at(id);
            if(p.source_unit!=null_datum && !home)home=p.position;
            if(p.source_unit!=null_datum && p.secondary_weapon.datum!=null_datum) {
                if(home)world.source_objects->tu15_world().set_object_position(p.source_unit,*home);
                acquired=true;break;
            }
            std::array<Controls,multiplayer_capacity> controls{};
            if(p.source_unit!=null_datum) {
                const auto item=std::find_if(state.world_weapons.begin(),state.world_weapons.end(),
                    [&](const auto &w){return w.definition==weapon;});
                if(item!=state.world_weapons.end()) {
                    world.source_objects->tu15_world().set_object_position(p.source_unit,item->position+Vec3{-.2f,0,0});
                    controls[id].dual_wield=true;controls[id].dual_wield_milliseconds=1000;
                }
            }
            step(controls);
        }
        if(!acquired) {
            const auto state=setup();if(diagnostic)*diagnostic=state;
            throw FormatError("Actual native pickups did not supply requested dual participant "+
                std::to_string(id)+": "+state.dump());
        }
        step({}); // Release the real pickup input before the next participant.
    }
    if(diagnostic)*diagnostic=setup();
}
Camera camera_at(MultiplayerPreparedWorld &world,unsigned slot,Json *provenance=nullptr) {
    const auto state=world.match.state();const auto &player=state.players.at(slot);
    const auto target=player.position;
    const bool close=std::getenv("HALO2_TEST_DUAL_SUBMISSION_CLOSE")!=nullptr;
    Camera result;result.oriented=true;
    if(!close) {
        result.position=target+Vec3{-3,-2,1};
        result.forward=normalized(target+Vec3{0,0,.5f}-result.position);result.up={0,0,1};
        if(provenance)*provenance={{"close",false},{"source","Existing standard fixed-world-offset diagnostic camera"}};
        return result;
    }
    need(world.source_objects && world.source_objects->owns_physics() && !player.dead &&
        player.source_unit!=null_datum && player.source_weapon!=null_datum && player.secondary_weapon.datum!=null_datum,
        "Close grip camera requires the actual native target and both held OBJECT owners");
    auto &native=*world.source_objects;
    const auto unit=native.object_transform(player.source_unit);
    const std::array roots{native.object_transform(player.source_weapon).position,
        native.object_transform(player.secondary_weapon.datum).position};
    need(length(unit.basis[0])>1e-6f && length(unit.basis[1])>1e-6f && length(unit.basis[2])>1e-6f,
        "Close grip camera lost the actual native unit orientation basis");
    const auto forward=normalized(unit.basis[0]),side=normalized(unit.basis[1]),up=normalized(unit.basis[2]);
    const auto focus=(roots[0]+roots[1])*.5f;
    // A declared camera input only: the actual native poses and attachment
    // roots are unchanged. Render node zero is not an OBJECT attachment root.
    result.position=focus+forward*.9f+side*.75f+up*.25f;
    result.forward=normalized(focus-result.position);result.up=up;
    if(provenance)*provenance={{"close",true},{"tick",state.tick},{"unit",player.source_unit},
        {"held_objects",{player.source_weapon,player.secondary_weapon.datum}},
        {"source","Actual native unit OBJECT basis and mean primary/offhand OBJECT root positions after authored pickup"},
        {"unit_root",{unit.position.x,unit.position.y,unit.position.z}},
        {"unit_forward",{forward.x,forward.y,forward.z}},{"unit_side",{side.x,side.y,side.z}},{"unit_up",{up.x,up.y,up.z}},
        {"held_object_roots",{{roots[0].x,roots[0].y,roots[0].z},{roots[1].x,roots[1].y,roots[1].z}}},
        {"look_at",{focus.x,focus.y,focus.z}},{"offset_in_unit_basis",{.9f,.75f,.25f}},
        {"camera_position",{result.position.x,result.position.y,result.position.z}},
        {"camera_forward",{result.forward.x,result.forward.y,result.forward.z}},
        {"caption","Front oblique view of native held OBJECT roots; no pose offset or render-node-zero substitution"}};
    return result;
}
struct GripReference {
    struct Weapon {std::uint32_t object{},definition{};Transform root;std::vector<std::uint8_t> permutations;};
    std::uint32_t unit{},definition{};std::vector<std::uint8_t> permutations;
    std::array<Weapon,2> weapons;std::array<SourceWorldPose,3> poses;
};
GripReference native_grip(MultiplayerPreparedWorld &world,const MultiplayerState &state,unsigned slot) {
    auto &native=*world.source_objects;const auto &p=state.players[slot];GripReference result;
    result.unit=p.source_unit;result.definition=native.object_definition(p.source_unit);
    result.permutations=native.object_model_regions(p.source_unit).selection.permutations;
    const std::array owners{p.source_unit,p.source_weapon,p.secondary_weapon.datum};
    for(unsigned n=0;n<owners.size();++n) {
        const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),
            [&](const auto &entry){return entry.object==owners[n];});
        need(pose!=state.source_body_poses.end(),"Authority native grip reference lost its actual published bank");result.poses[n]=*pose;
        if(n)result.weapons[n-1]={owners[n],native.object_definition(owners[n]),native.object_transform(owners[n]),
            native.object_model_regions(owners[n]).selection.permutations};
    }
    return result;
}
struct Server {
    struct Ready {std::string address;unsigned target;Camera camera;Json camera_provenance;};
    std::mutex mutex;std::exception_ptr failure;Ready ready;
    std::map<std::uint32_t,GripReference> references;std::jthread worker;
    explicit Server(MultiplayerOptions options) {
        std::promise<Ready> started;auto startup=started.get_future();options.host_port=1;
        worker=std::jthread([this,options,started=std::move(started)](std::stop_token stop)mutable {
            bool published{};
            try {
                MultiplayerPreparedWorld world(options);world.bind_source_local_views(world.local_slots);
                need(world.source_objects && world.source_objects->owns_physics(),"Joining fixture lacks a native authority");
                const std::array target{world.local_slots.at(0)};acquire(world,target);
                const auto fingerprint=multiplayer_fingerprint(options.map,world.cache->shared_directory(),0,
                    world.network_starting_weapon_identity());
                LanHost host(world.match,0,fingerprint,options.variant);
                Json camera_provenance;const auto camera=camera_at(world,target[0],&camera_provenance);
                started.set_value({host.join_address(),target[0],camera,std::move(camera_provenance)});
                published=true;std::array<Controls,multiplayer_capacity> controls{};
                auto next=std::chrono::steady_clock::now();std::optional<unsigned> dual_tick;
                while(!stop.stop_requested()) {
                    host.receive(controls);const auto state=world.match.state();
                    for(unsigned id=0;id<state.players.size();++id) {
                        const auto &p=state.players[id];
                        if(id==target[0] || !p.connected || p.source_unit==null_datum || p.secondary_weapon.datum!=null_datum)continue;
                        const auto item=std::find_if(state.world_weapons.begin(),state.world_weapons.end(),
                            [&](const auto &w){return w.definition==fixture_weapon(world);});
                        if(item!=state.world_weapons.end())
                            world.source_objects->tu15_world().set_object_position(p.source_unit,item->position+Vec3{-.2f,0,0});
                    }
                    const bool joined_dual=std::any_of(state.players.begin(),state.players.end(),[&](const auto &p){
                        return p.connected && p.source_unit!=state.players[target[0]].source_unit && p.secondary_weapon.datum!=null_datum;});
                    if(joined_dual && !dual_tick)dual_tick=state.tick;
                    // A declared endpoint capture: finish twenty real native
                    // dual ticks, then keep exchanging actual packets while
                    // playout reaches that immutable completed endpoint.
                    if(!dual_tick || state.tick-*dual_tick<20)world.match.step(controls);
                    const auto completed=world.match.state();
                    {
                        auto grip=native_grip(world,completed,target[0]);std::lock_guard lock(mutex);
                        references.insert_or_assign(completed.tick,std::move(grip));
                        while(references.size()>128)references.erase(references.begin());
                    }
                    host.broadcast();
                    next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
                }
            }catch(...) {
                if(!published)started.set_exception(std::current_exception());
                std::lock_guard lock(mutex);failure=std::current_exception();
            }
        });
        ready=startup.get();
    }
    GripReference reference(std::uint32_t tick) {
        std::lock_guard lock(mutex);if(failure)std::rethrow_exception(failure);
        const auto at=references.find(tick);need(at!=references.end(),"Accepted packet has no same-tick native authority marker reference");
        return at->second;
    }
    void finish(){worker.request_stop();if(worker.joinable())worker.join();std::lock_guard lock(mutex);if(failure)std::rethrow_exception(failure);}
};
struct Bank {std::uint32_t tick{},frame{},model{};float fraction{};std::vector<Transform> nodes;};
using Key=std::pair<std::uint32_t,std::uint32_t>;
struct Witness {
    std::array<std::map<Key,Bank>,7> banks;
    std::map<std::tuple<unsigned,std::uint32_t,std::uint32_t>,Bank> submitted;
    MultiplayerState state;unsigned target{};std::optional<unsigned> local;
    unsigned frames{},world_frames{},fp_frames{},gpu_frames{},fp_endpoint_samples{};
    std::uint64_t gpu_vertices{},gpu_verified{},gpu_deformed{};
    unsigned gpu_body_frames{},cpu_body_frames{},gpu_gun_submissions{};
    std::array<unsigned,2> gpu_gun_submissions_by_hand{};
    std::uint64_t gpu_body_vertices{},gpu_body_weighted{},gpu_body_verified{};
    std::vector<std::pair<std::uint32_t,ModelSkinGpuOwnerObservation>> gpu_owners;
    float endpoint_error{},submitted_error{},marker_error{},fp_endpoint_error{};
    Json samples=Json::array();
    const Bank &bank(unsigned stage,const Bank &actual,std::uint32_t owner)const {
        const auto at=banks.at(stage).find({owner,actual.model});
        need(at!=banks.at(stage).end() && at->second.tick==actual.tick,
            "Final draw has no same-tick actual native completion/interpolation bank");
        return at->second;
    }
    void observe(unsigned view,std::uint32_t tick,const MultiplayerNodeObservation &sample) {
        if(view)return;
        const auto stage=unsigned(sample.stage);need(stage>0 && stage<7,"Unknown native submission stage");
        Bank value{tick,sample.frame,sample.render_model,sample.fraction,{sample.nodes.begin(),sample.nodes.end()}};
        banks[stage].insert_or_assign({sample.owner,sample.render_model},value);
        if(stage>=5)submitted.insert_or_assign({stage,sample.owner,sample.render_model},std::move(value));
    }
};
// Two actual local cameras deliberately share the normal native cache/renderer
// traversal. Per-view owner keys catch a later view replacing an earlier
// group's FP cache. The single-view fixture below separately qualifies the
// third-person body/held markers and its actual GPU admission.
int split_submission(int argc,char **argv) {
    Json report{{"passed",false},{"scope","Two physical local views, exact native FP packets and final per-view submission"}};
    std::array<Witness,2> views;
    const auto save=[&]{if(argc>=4)std::ofstream(argv[3])<<report.dump(2)<<'\n';};
    try {
        need(argc==6,"Supply MAP SHARED REPORT REPRESENTATION original|pstencil");
        if(!std::getenv("DISPLAY"))return 77;
        const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Run split submission through tools/run_test.py");
        const unsigned representation=std::stoul(argv[4]);const std::string graphics=argv[5];
        need(representation==2 || representation==3,"Require human or Elite");
        need(graphics=="original" || graphics=="pstencil","Unknown split graphics mode");
        const bool gpu=graphics=="original";
        if(gpu)need(std::getenv("HALO2_VERIFY_GPU_SKINNING"),"Original split submission requires actual GPU readback verification");
        SDL_SetHintWithPriority(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0xffff/0xffff",SDL_HINT_OVERRIDE);SDL_SetMainReady();
        MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.starting_weapon=5;
        options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}};options.width=800;options.height=600;
        options.local_players=2;options.mute=true;options.source_object_world=true;
        options.source_object_savegame_directory=std::filesystem::path(scratch)/"split-world";
        auto profile=source_default_player_configuration();profile[0x44]=representation==3;
        options.selected_profiles={profile,profile};options.host_port=unused_udp_port();
        options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
        auto &world=*options.prepared_world;need(world.cache->is_mcc() && world.source_objects && world.source_objects->owns_physics(),
            "Split submission requires its actual native MCC authority");
        world.bind_source_local_views(world.local_slots);need(world.local_slots.size()==2,"Split fixture lost a physical local view");
        SourceGameplayFadePresentationTu15 setup_fade;
        acquire(world,world.local_slots,&report["setup"],&setup_fade);
        report["source_fade_setup_sequence"]=setup_fade.sequence();
        options.initial_gameplay_fade=std::move(setup_fade);
        const auto fp=world.cache->load_first_person_binding(fixture_weapon(world),representation);
        auto &native=world.source_objects->tu15_world();const auto &memory=native.memory();
        const auto caller=native.prepare_source_call();
        const std::shared_ptr<const SourceRuntimeResources> resources(options.prepared_world,&native.runtime_resources());
        std::array<std::unique_ptr<SourceFirstPersonPoseReceiverTu15>,2> receivers;
        for(auto &receiver:receivers)receiver=std::make_unique<SourceFirstPersonPoseReceiverTu15>(*world.cache,resources);
        std::array<Camera,2> cameras;std::array<std::uint32_t,2> camera_ticks{};
        std::array<bool,2> camera_seen{};
        std::array<std::set<Key>,2> completion_callbacks;
        std::array<std::uint64_t,2> native_instructions{},verified_fp_vertices{};
        std::array<std::set<std::uint32_t>,2> completed_ticks;
        std::array<std::vector<Transform>,2> initial_local_arms;
        std::array<float,2> local_arm_variation{};
        unsigned input_commands{};bool sent_fire{},released_fire{},fire_observed{},release_observed{},look_observed{};
        std::optional<float> initial_yaw;bool distinct_view_cameras_and_banks{};
        float native_packet_error{};MultiplayerState current;
        options.desktop=std::make_shared<DesktopSession>();options.desktop->display_settings.graphics_mode=
            gpu?pstencil::GraphicsMode::Original:pstencil::GraphicsMode::pStencil;
        options.desktop->display_settings.upscaler=Upscaler::Off;options.desktop->upscaling_preferences_initialized=true;
        auto *window=options.desktop->window(800,600,false);SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);
        const auto window_id=SDL_GetWindowID(window);
        SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
        options.frames=100'000;options.observe_submitted_nodes=true;
        report["draw_frame_cap"]=options.frames;report["wall_clock_deadline_seconds"]=180;
        report["render_tick_overlap_enabled"]=false;
        report["overlap_limit"]="Frame/node/input observers qualify the synchronous diagnostic traversal; ordinary overlapped production scheduling is a separate gate";
        report["completed_bank_binding"]="Fresh per-view completion and camera callbacks in this synchronous frame; native completion stage has no explicit renderer frame number";
        const auto mouse_button=[&](bool down) {
            SDL_Event event{};event.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
            event.button.windowID=window_id;event.button.button=SDL_BUTTON_LEFT;
            event.button.state=down?SDL_PRESSED:SDL_RELEASED;
            need(SDL_PushEvent(&event)==1,"Queue actual split keyboard-view fire input");
        };
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(180);
        options.input_observer=[&](unsigned,std::span<const Controls> commands,const MultiplayerState &) {
            need(std::chrono::steady_clock::now()<deadline,"Finite two-view native submission timed out");
            need(commands.size()==2,"Actual split command delivery lost a physical local player");
            if(!initial_yaw)initial_yaw=commands[0].yaw;
            look_observed|=std::abs(commands[0].yaw-*initial_yaw)>1e-4f;
            const bool firing=commands[0].fire || commands[0].fire_secondary;
            fire_observed|=firing;
            release_observed|=released_fire && fire_observed && !firing;
            need(!commands[1].fire && !commands[1].fire_secondary,
                "Split fixture's idle physical view received the other keyboard view's fire input");
            if(!sent_fire){mouse_button(true);sent_fire=true;}
            if(++input_commands<=8u) {
                SDL_Event motion{};motion.type=SDL_MOUSEMOTION;motion.motion.windowID=window_id;
                motion.motion.xrel=8;motion.motion.yrel=input_commands%2u?1:-1;
                need(SDL_PushEvent(&motion)==1,"Queue actual split keyboard-view look input");
            }else if(!released_fire){mouse_button(false);released_fire=true;}
        };
        options.source_scene_observer=[&](const auto &state,const auto &){current=state;};
        options.view_state_observer=[&](unsigned view,const auto &state,const Camera &camera) {
            need(view<2,"Split camera escaped its selected physical view");cameras[view]=camera;camera_ticks[view]=state.tick;camera_seen[view]=true;
        };
        options.node_observer=[&](unsigned view,auto tick,const auto &sample) {
            need(view<2,"Split node submission escaped its selected physical view");views[view].observe(0,tick,sample);
            if(sample.stage==MultiplayerNodeStage::first_person_completed)
                completion_callbacks[view].emplace(sample.owner,sample.render_model);
        };
        if(gpu)options.model_skin_gpu_owner_observer=[&](unsigned view,auto tick,const auto &sample) {
            need(view<2,"Split GPU owner escaped its selected physical view");views[view].gpu_owners.emplace_back(tick,sample);
        };
        options.frame_observer=[&](const MultiplayerFrameTiming &frame) {
            need(std::chrono::steady_clock::now()<deadline,"Finite two-view native render sequence timed out");
            need(frame.views==2,"Native split renderer did not retain both physical views");
            report["last_frame"]={{"frame",frame.frame},{"tick",current.tick},{"views",frame.views}};
            auto completed_in_frame=std::move(completion_callbacks);for(auto &callbacks:completion_callbacks)callbacks.clear();
            const auto cameras_in_frame=camera_seen;camera_seen={};
            std::array<std::optional<std::uint64_t>,2> frame_arms;
            bool all_complete=true;
            for(unsigned view=0;view<2;++view) {
                auto &witness=views[view];auto gpu_owners=std::move(witness.gpu_owners);witness.gpu_owners.clear();
                if(!frame.submitted){all_complete=false;continue;}
                const auto &player=current.players.at(world.local_slots[view]);
                need(!player.dead && player.first_person_pose && player.secondary_weapon.datum!=null_datum,
                    "Split native participant lost its genuine dual FP packet");
                need(cameras_in_frame[view] && camera_ticks[view]==current.tick,
                    "Split camera differs from its actual submitted frame/tick");
                const std::array owners{player.source_unit,player.source_weapon,player.secondary_weapon.datum};
                std::set<std::uint32_t> shown;
                for(const auto &[key,value]:witness.submitted) {
                    const auto [stage,owner,model]=key;
                    if(stage!=6 || value.frame!=frame.frame || value.tick!=current.tick ||
                        (model!=fp.arms_model && model!=fp.weapon_model))continue;
                    need(std::ranges::find(owners,owner)!=owners.end(),"A physical view consumed another player's native FP cache");
                    need(completed_in_frame[view].contains({owner,model}),
                        "Split final submission has no fresh completion callback for this physical view/frame");
                    const auto &completed=witness.bank(3,value,owner),&interpolated=witness.bank(4,value,owner);
                    const auto final=error(value.nodes,interpolated.nodes);
                    witness.submitted_error=std::max(witness.submitted_error,final);
                    need(final==0,"Split final FP bank differs from its own native interpolation owner");
                    need((owner==player.source_unit && model==fp.arms_model) ||
                        (owner!=player.source_unit && model==fp.weapon_model),"Split FP model differs from its own hand/arms binding");
                    if(value.fraction==1.f) {
                        const auto endpoint=error(completed.nodes,interpolated.nodes);
                        witness.fp_endpoint_error=std::max(witness.fp_endpoint_error,endpoint);++witness.fp_endpoint_samples;
                        need(endpoint<=1e-4f,"Split natural FP endpoint differs from its actual native completion");
                    }
                    shown.insert(owner);
                }
                if(shown.size()!=3){all_complete=false;continue;}
                const auto &camera=cameras[view];const auto forward=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
                const auto up=camera.oriented?camera.up:cross(forward,{-std::sin(camera.yaw),std::cos(camera.yaw),0});
                SourcePhysicsWorldRuntimeServices cpu;cpu.instruction=[&](auto){++native_instructions[view];};
#if defined(__i386__) || defined(__x86_64__)
                cpu.reciprocal_sqrt=[](auto bits){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(
                    _mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));};
                cpu.timestamp_counter=[](){return __rdtsc();};
#else
                throw FormatError("Split native packet comparison requires actual CPU instruction services");
#endif
                SourceRuntimeWriteSerial unchanged(memory);
                const auto sampled=receivers[view]->sample(memory,caller,*player.first_person_pose,
                    player.first_person_pose->temporal.identity,camera.position,forward,up,cpu);
                need(!unchanged.load(),"Split native packet sampler changed its borrowed authority");
                unsigned compared{};
                for(const auto &model:sampled)if(model.mode==1u) {
                    const auto actual=witness.banks[3].find({model.owner,model.render_model});
                    need(actual!=witness.banks[3].end() && actual->second.tick==current.tick,
                        "Split actual FP cache lacks its same-view native packet model");
                    const auto delta=error(model.nodes,actual->second.nodes);native_packet_error=std::max(native_packet_error,delta);
                    report["last_native_packet_comparison"]={{"view",view},{"tick",current.tick},
                        {"owner",model.owner},{"model",model.render_model},{"component_error",delta}};
                    need(delta<=1e-5f,"Split actual FP cache differs from that player's exact native packet composition");++compared;
                    if(model.owner==player.source_unit && model.render_model==fp.arms_model) {
                        frame_arms[view]=digest(actual->second.nodes);
                        const auto inverse=inverse_transform(camera_transform(camera.position,forward,up));
                        std::vector<Transform> local;local.reserve(actual->second.nodes.size());
                        for(const auto &node:actual->second.nodes)local.push_back(compose_transform(inverse,node));
                        if(initial_local_arms[view].empty())initial_local_arms[view]=std::move(local);
                        else local_arm_variation[view]=std::max(local_arm_variation[view],error(local,initial_local_arms[view]));
                    }
                }
                need(compared==3,"Split native packet composition omitted arms or either held weapon");
                if(gpu) {
                    std::set<std::uint32_t> verified;std::size_t weighted_arms{};
                    for(const auto &[tick,owner]:gpu_owners) {
                        if(!owner.post_identity_placement || std::ranges::find(owners,owner.owner)==owners.end())continue;
                        need(tick==current.tick && owner.vertices && owner.verified_vertices==owner.vertices,
                            "Split native FP owner missed its own completed GPU byte verification");
                        need((owner.owner==player.source_unit && owner.render_model==fp.arms_model) ||
                            (owner.owner!=player.source_unit && owner.render_model==fp.weapon_model),
                            "Split GPU owner differs from that view's native FP bank");
                        verified.insert(owner.owner);verified_fp_vertices[view]+=owner.verified_vertices;
                        if(owner.owner==player.source_unit)weighted_arms+=owner.weighted_vertices;
                    }
                    report["last_fp_gpu_comparison"]={{"view",view},{"verified_owners",verified},
                        {"weighted_arm_vertices",weighted_arms},{"records",gpu_owners.size()}};
                    need(verified.size()==3 && weighted_arms,"Split native arms/guns lack per-view GPU owner/readback witnesses");
                }
                if(witness.samples.size()<16) {
                    Json row{{"frame",frame.frame},{"tick",current.tick},{"view",view},{"owners",owners},
                        {"camera_position",{camera.position.x,camera.position.y,camera.position.z}},
                        {"camera_forward",{forward.x,forward.y,forward.z}},{"banks",Json::array()}};
                    for(const auto &[key,value]:witness.submitted)if(value.frame==frame.frame && std::get<0>(key)==6u) {
                        const auto [stage,owner,model]=key;
                        row["banks"].push_back({{"owner",owner},{"model",model},{"nodes",value.nodes.size()},{"hash",digest(value.nodes)}});
                    }
                    witness.samples.push_back(std::move(row));
                }
                completed_ticks[view].insert(current.tick);++witness.frames;
            }
            if(frame_arms[0] && frame_arms[1] && *frame_arms[0]!=*frame_arms[1]) {
                const auto forward=[](const Camera &camera){return camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);};
                distinct_view_cameras_and_banks|=length(cameras[0].position-cameras[1].position)>1e-4f ||
                    length(forward(cameras[0])-forward(cameras[1]))>1e-4f;
            }
            report["variation_witness"]={{"input_commands",input_commands},{"fire_observed",fire_observed},
                {"release_observed",release_observed},{"look_observed",look_observed},
                {"camera_local_arm_variation",local_arm_variation},
                {"distinct_view_cameras_and_banks",distinct_view_cameras_and_banks},
                {"source_tick_counts",{completed_ticks[0].size(),completed_ticks[1].size()}}};
            if(all_complete && views[0].frames>=16 && views[1].frames>=16 &&
                completed_ticks[0].size()>=2u && completed_ticks[1].size()>=2u &&
                distinct_view_cameras_and_banks && fire_observed && release_observed && look_observed &&
                local_arm_variation[0]>1e-4f) {
                SDL_Event event{};event.type=SDL_QUIT;need(SDL_PushEvent(&event)==1,"Finish bounded two-view native submission");
            }
        };
        const auto result=run_multiplayer(options);
        report.update({{"graphics",graphics},{"representation",representation},{"physical_local_views",2},
            {"diagnostic_camera",false},{"native_packet_component_error",native_packet_error},
            {"distinct_view_cameras_and_banks",distinct_view_cameras_and_banks},
            {"keyboard_view_fire_observed",fire_observed},{"keyboard_view_fire_release_observed",release_observed},
            {"keyboard_view_look_observed",look_observed},{"other_physical_view_idle",true},
            {"camera_local_arm_component_variation",local_arm_variation},
            {"native_instructions_by_view",native_instructions},{"verified_fp_gpu_vertices_by_view",verified_fp_vertices},
            {"views",Json::array()}});
        for(unsigned view=0;view<2;++view) {
            const auto &witness=views[view];
            report["views"].push_back({{"view",view},{"complete_frames",witness.frames},
                {"final_submission_component_error",witness.submitted_error},
                {"distinct_source_ticks",completed_ticks[view]},
                {"natural_endpoint_verified",witness.fp_endpoint_samples>0},
                {"natural_endpoint_component_error",witness.fp_endpoint_samples?Json(witness.fp_endpoint_error):Json(nullptr)},
                {"natural_endpoint_samples",witness.fp_endpoint_samples},{"samples",witness.samples}});
            need(witness.frames>=16 && completed_ticks[view].size()>=2u && native_instructions[view],
                "Finite split run did not qualify both native physical views across genuine source ticks");
        }
        need(distinct_view_cameras_and_banks && fire_observed && release_observed && look_observed && local_arm_variation[0]>1e-4f,
            "Split fixture did not exercise distinct changing keyboard and idle native poses");
        need(result==0,"Native two-view submission returned a failure");report["passed"]=true;save();
        std::cout<<"PASS two physical native FP views through final "<<graphics<<" submission\n";return 0;
    }catch(const std::exception &error){report["error"]=error.what();save();std::cerr<<error.what()<<'\n';return 1;}
}
}
int main(int argc,char **argv) {
#ifdef HALO2_NATIVE_FP_SPLIT_SUBMISSION
    return split_submission(argc,argv);
#else
    Json report{{"passed",false},{"scope","Actual native endpoint, authored markers and final submitted banks; no Xbox framebuffer comparison"}};
    Witness witness;
    const auto save=[&]{if(argc>=4)std::ofstream(argv[3])<<report.dump(2)<<'\n';};
    try {
        need(argc==7,"Supply MAP SHARED REPORT REPRESENTATION host|joined original|pstencil");
        if(!std::getenv("DISPLAY"))return 77;
        const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Run native submission acceptance through tools/run_test.py");
        const unsigned representation=std::stoul(argv[4]);need(representation==2 || representation==3,"Require human or Elite");
        const std::string route=argv[5],graphics=argv[6];
        need(route=="host" || route=="joined","Unknown native submission route");
        need(graphics=="original" || graphics=="pstencil","Unknown native submission graphics mode");
        const bool gpu=graphics=="original";
        if(gpu)need(std::getenv("HALO2_VERIFY_GPU_SKINNING"),"Original submission acceptance requires actual GPU readback verification");
        SDL_SetHintWithPriority(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0xffff/0xffff",SDL_HINT_OVERRIDE);SDL_SetMainReady();
        MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.starting_weapon=5;
        options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}};options.width=800;options.height=600;
        options.local_players=1;options.input_instances={-1};options.mute=true;options.source_object_world=true;
        options.source_object_savegame_directory=std::filesystem::path(scratch)/"render-world";
        // The runtime admits a diagnostic camera only for a finite screenshot
        // request. Its output remains private even if a failing run reaches
        // the final frame before all bank witnesses are complete.
        options.screenshot=std::filesystem::path(scratch)/"native-dual-submission.png";
        auto profile=source_default_player_configuration();profile[0x44]=representation==3;options.selected_profiles={profile};
        std::unique_ptr<Server> server;
        if(route=="joined") {
            auto authority=options;authority.source_object_savegame_directory=std::filesystem::path(scratch)/"authority-world";
            server=std::make_unique<Server>(authority);options.join=server->ready.address;
            options.starting_weapon=0;witness.target=server->ready.target;options.capture_camera=server->ready.camera;
            report["capture_camera_provenance"]=server->ready.camera_provenance;
        }else options.host_port=unused_udp_port();
        options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
        auto &world=*options.prepared_world;need(world.cache->is_mcc(),"Submission acceptance requires admitted MCC content");
        if(route=="joined") {
            need(!world.source_objects && !world.match.source_object_runtime() && world.rules.requires_source_engine,
                "Joined submission fixture must exercise the replica without a gameplay World");
            report["joined_prepared_world_has_no_gameplay_owner"]=true;
        }
        if(route=="host") {
            world.bind_source_local_views(world.local_slots);witness.local=world.local_slots.at(0);
            witness.target=world.match.join("Native dual third-person",-1,profile);
            SourceGameplayFadePresentationTu15 setup_fade;
            const std::array participants{*witness.local,witness.target};
            acquire(world,participants,&report["setup"],&setup_fade);
            report["source_fade_setup_sequence"]=setup_fade.sequence();
            options.initial_gameplay_fade=std::move(setup_fade);
            options.capture_camera=camera_at(world,witness.target,&report["capture_camera_provenance"]);
        }
        const auto biped=world.content.player_representations.at(representation).biped.datum;
        const auto fp=world.cache->load_first_person_binding(fixture_weapon(world),representation);
        options.desktop=std::make_shared<DesktopSession>();options.desktop->display_settings.graphics_mode=
            gpu?pstencil::GraphicsMode::Original:pstencil::GraphicsMode::pStencil;
        options.desktop->display_settings.upscaler=Upscaler::Off;options.desktop->upscaling_preferences_initialized=true;
        auto *window=options.desktop->window(800,600,false);SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);
        const auto window_id=SDL_GetWindowID(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
        // The joined authority runs genuine30Hz ticks: Lockout row12's
        //30-second SMG respawn can outlast420 draws on a warm renderer. Keep
        // a finite draw cap for diagnostic-camera admission; the180-second
        // wall-clock guard and16 validated-frame quit own the actual bound.
        options.frames=route=="joined"?1'000'000u:420u;
        report["draw_frame_cap"]=options.frames;
        report["wall_clock_deadline_seconds"]=180;
        options.observe_submitted_nodes=true;
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(180);
        options.input_observer=[&](unsigned,std::span<const Controls>,const MultiplayerState &state) {
            need(std::chrono::steady_clock::now()<deadline,"Finite native submission sequence timed out");
            if(!witness.local)for(unsigned id=0;id<state.players.size();++id)
                if(id!=witness.target && state.players[id].connected && state.players[id].source_unit!=null_datum){witness.local=id;break;}
            if(route=="joined" && witness.local)hold(window_id,state.players[*witness.local].secondary_weapon.datum==null_datum);
        };
        options.source_scene_observer=[&](const auto &state,const auto &){witness.state=state;};
        options.node_observer=[&](auto view,auto tick,const auto &sample){witness.observe(view,tick,sample);};
        if(gpu)options.model_skin_gpu_owner_observer=[&](unsigned view,std::uint32_t tick,const auto &sample) {
            if(!view)witness.gpu_owners.emplace_back(tick,sample);
        };
        options.frame_observer=[&](const MultiplayerFrameTiming &frame) {
            auto gpu_owners=std::move(witness.gpu_owners);witness.gpu_owners.clear();
            if(!frame.submitted || !witness.local)return;
            const auto &target=witness.state.players[witness.target],&local=witness.state.players[*witness.local];
            if(target.secondary_weapon.datum==null_datum || local.secondary_weapon.datum==null_datum)return;
            need(!target.dead && !local.dead,"Native submission participant died before its grip comparison");
            const std::array world_owners{target.source_unit,target.source_weapon,target.secondary_weapon.datum};
            const std::array fp_owners{local.source_unit,local.source_weapon,local.secondary_weapon.datum};
            std::set<std::uint32_t> shown_world,shown_fp;
            for(const auto &[key,value]:witness.submitted) {
                const auto [stage,owner,model]=key;if(value.frame!=frame.frame || value.tick!=witness.state.tick)continue;
                if(stage==5 && std::ranges::find(world_owners,owner)!=world_owners.end()) {
                    const auto publication=std::find_if(witness.state.source_body_poses.begin(),witness.state.source_body_poses.end(),
                        [&](const auto &pose){return pose.object==owner;});
                    need(publication!=witness.state.source_body_poses.end(),"Rendered dual WORLD owner has no actual authority publication");
                    float endpoint{},final{};
                    if(route=="host") {
                        const auto &completed=witness.bank(1,value,owner),&interpolated=witness.bank(2,value,owner);
                        endpoint=error(completed.nodes,publication->nodes);final=error(value.nodes,interpolated.nodes);
                    }else {
                        if(value.fraction!=1.f)continue;
                        // A pure packet renderer has no native BF640 collector
                        // stages. Its final endpoint must preserve the actual
                        // accepted completed body/held banks directly.
                        endpoint=error(value.nodes,publication->nodes);
                    }
                    witness.endpoint_error=std::max(witness.endpoint_error,endpoint);witness.submitted_error=std::max(witness.submitted_error,final);
                    need(endpoint<=1e-6f && final==0,"WORLD completion or final submission changed the actual authority bank");shown_world.insert(owner);
                }else if(stage==6 && std::ranges::find(fp_owners,owner)!=fp_owners.end() &&
                    ((owner==local.source_unit && model==fp.arms_model) || (owner!=local.source_unit && model==fp.weapon_model))) {
                    const auto &completed=witness.bank(3,value,owner),&interpolated=witness.bank(4,value,owner);
                    const auto final=error(value.nodes,interpolated.nodes);
                    witness.submitted_error=std::max(witness.submitted_error,final);
                    if(value.fraction==1.f) {
                        const auto endpoint=error(completed.nodes,interpolated.nodes);
                        ++witness.fp_endpoint_samples;witness.fp_endpoint_error=std::max(witness.fp_endpoint_error,endpoint);
                        need(endpoint<=1e-4f,"Natural FP endpoint differs from actual native completion");
                    }
                    need(final==0,"Final FP bank differs from the actual native interpolation owner");shown_fp.insert(owner);
                }
            }
            if(shown_world.size()==3)++witness.world_frames;
            if(shown_fp.size()==3)++witness.fp_frames;
            if(shown_world.size()!=3 || shown_fp.size()!=3)return;
            const auto reference=server?server->reference(witness.state.tick):native_grip(world,witness.state,witness.target);
            need(reference.unit==target.source_unit && reference.definition==biped,"Rendered species differs from its actual native biped owner");
            for(const auto &original:reference.poses) {
                const auto accepted=std::find_if(witness.state.source_body_poses.begin(),witness.state.source_body_poses.end(),
                    [&](const auto &pose){return pose.object==original.object;});
                need(accepted!=witness.state.source_body_poses.end(),"Accepted endpoint omitted its actual native authority owner");
                const auto delta=error(accepted->nodes,original.nodes);witness.endpoint_error=std::max(witness.endpoint_error,delta);
                need(delta<=1e-6f,"Accepted endpoint bank differs from the same-tick native authority publication");
            }
            const auto completed=std::find_if(witness.state.source_body_poses.begin(),witness.state.source_body_poses.end(),
                [&](const auto &entry){return entry.object==target.source_unit;});
            need(completed!=witness.state.source_body_poses.end(),"Native grip body has no completed authority bank");
            for(unsigned hand=0;hand<2;++hand) {
                const auto &weapon=reference.weapons[hand];need(weapon.object==world_owners[hand+1],"Native marker reference lost held generation identity");
                const auto attachment=select_weapon_attachment(
                    world.cache->load_weapon_attachment(biped,weapon.definition,hand),
                    reference.permutations,weapon.permutations);
                need(!attachment.named_child,"This dual-SMG acceptance requires its actual unnamed-child attachment branch");
                const auto marker=compose_transform(completed->nodes.at(attachment.parent.node),attachment.parent.local);
                const auto expected=source_object_attachment_frame({weapon.root.position,weapon.root.basis[0],weapon.root.basis[2]},
                    marker,attachment.child.local,false);
                const auto expected_root=camera_transform(expected.position,expected.forward,expected.up);
                const auto delta=error(std::span(&weapon.root,1),std::span(&expected_root,1));
                witness.marker_error=std::max(witness.marker_error,delta);need(delta<=2e-5f,"Actual native weapon OBJECT root misses its authored completed hand marker");
            }
            const auto &r=frame.renderer;witness.gpu_vertices+=r.model_skin_gpu_vertices;witness.gpu_verified+=r.model_skin_gpu_verified_vertices;
            witness.gpu_deformed+=r.model_skin_gpu_deformed_vertices;
            if(r.model_skin_gpu_jobs && r.model_skin_gpu_post_identity_vertices)++witness.gpu_frames;
            if(gpu)need(r.model_skin_gpu_jobs && r.model_skin_gpu_post_identity_vertices && r.model_skin_gpu_deformed_vertices &&
                r.model_skin_gpu_verified_vertices>=r.model_skin_gpu_vertices && !r.model_skin_cpu_fallback_jobs,
                "Passive native observation did not retain actual verified GPU skin admission");
            if(gpu) {
                std::size_t body_vertices{},body_weighted{},body_verified{};
                for(const auto &[tick,owner]:gpu_owners) {
                    if(owner.post_identity_placement || std::ranges::find(world_owners,owner.owner)==world_owners.end())continue;
                    need(tick==witness.state.tick,"GPU skin witness changed the submitting view's actual tick");
                    const auto bank=witness.banks[5].find({owner.owner,owner.render_model});
                    need(bank!=witness.banks[5].end() && bank->second.frame==frame.frame &&
                        bank->second.tick==tick && owner.vertices && owner.verified_vertices==owner.vertices,
                        "Actual WORLD GPU owner has no matching submitted model bank or completed byte verification");
                    if(owner.owner==target.source_unit) {
                        need(owner.definition==biped,"GPU body record changed its actual biped definition");
                        body_vertices+=owner.vertices;body_weighted+=owner.weighted_vertices;body_verified+=owner.verified_vertices;
                    } else {
                        const auto hand=owner.owner==target.source_weapon?0u:1u;
                        need(owner.definition==reference.weapons[hand].definition,
                            "GPU held weapon record changed its actual native definition");
                        ++witness.gpu_gun_submissions;
                        ++witness.gpu_gun_submissions_by_hand[hand];
                    }
                }
                if(route=="host" || body_vertices)
                    need(body_vertices && body_weighted && body_verified==body_vertices,
                        "Actual third-person body has no weighted GPU submission with completed owner-specific readback");
                if(body_vertices) {
                    ++witness.gpu_body_frames;witness.gpu_body_vertices+=body_vertices;
                    witness.gpu_body_weighted+=body_weighted;witness.gpu_body_verified+=body_verified;
                }else ++witness.cpu_body_frames;
            }else ++witness.cpu_body_frames;
            if(witness.samples.size()<16) {
                Json row{{"frame",frame.frame},{"tick",witness.state.tick},{"gpu_jobs",r.model_skin_gpu_jobs},
                    {"gpu_vertices",r.model_skin_gpu_vertices},{"native_world_draws",r.world_native_prepared_draws},
                    {"gpu_owners",Json::array()},{"banks",Json::array()}};
                for(const auto &[tick,owner]:gpu_owners)row["gpu_owners"].push_back({{"tick",tick},{"owner",owner.owner},
                    {"definition",owner.definition},{"model",owner.render_model},{"vertices",owner.vertices},
                    {"weighted_vertices",owner.weighted_vertices},{"verified_vertices",owner.verified_vertices},
                    {"post_identity",owner.post_identity_placement}});
                for(const auto &[key,value]:witness.submitted)if(value.frame==frame.frame) {
                    const auto [stage,owner,model]=key;
                    row["banks"].push_back({{"stage",stage},{"owner",owner},{"model",model},{"nodes",value.nodes.size()},{"hash",digest(value.nodes)}});
                }
                witness.samples.push_back(std::move(row));
            }
            if(++witness.frames==16){SDL_Event event{};event.type=SDL_QUIT;need(SDL_PushEvent(&event)==1,"Finish bounded native submission capture");}
        };
        bool captured{};std::optional<std::filesystem::path> capture_file;
        if(const auto *capture=std::getenv("HALO2_TEST_DUAL_SUBMISSION_CAPTURE")) {
            const auto path=std::filesystem::absolute(capture).lexically_normal(),owner=std::filesystem::absolute(scratch).lexically_normal();
            need(!path.lexically_relative(owner).empty() && *path.lexically_relative(owner).begin()!="..","Native capture must remain under its owned scratch");
            capture_file=path;
            options.frame_capture=[&,path](const MultiplayerState &state)->std::filesystem::path {
                if(captured || witness.frames<12)return {};
                report["capture_request_tick"]=state.tick;
                captured=true;return path;
            };
            report["capture_path"]=path.string();
        }
        const auto result=run_multiplayer(options);if(server)server->finish();
        if(capture_file)need(captured && std::filesystem::is_regular_file(*capture_file) &&
            std::filesystem::file_size(*capture_file)>0,"Requested native submission capture was not written");
        report.update({{"route",route},{"graphics",graphics},{"representation",representation},{"complete_frames",witness.frames},
            {"world_frames",witness.world_frames},{"fp_frames",witness.fp_frames},{"gpu_frames",witness.gpu_frames},
            {"gpu_vertices",witness.gpu_vertices},{"gpu_verified_vertices",witness.gpu_verified},{"gpu_deformed_vertices",witness.gpu_deformed},
            {"authority_endpoint_component_error",witness.endpoint_error},
            {"native_fp_endpoint_component_error",witness.fp_endpoint_samples?Json(witness.fp_endpoint_error):Json(nullptr)},
            {"natural_fp_endpoint_samples",witness.fp_endpoint_samples},
            {"natural_fp_endpoint_verified",witness.fp_endpoint_samples>0},
            {"gpu_body_frames",witness.gpu_body_frames},{"gpu_body_vertices",witness.gpu_body_vertices},
            {"cpu_body_frames",witness.cpu_body_frames},{"world_body_gpu_required",gpu && route=="host"},
            {"world_body_skin_backend",witness.gpu_body_frames?(witness.cpu_body_frames?"mixed":"GPU"):"CPU"},
            {"gpu_body_weighted_vertices",witness.gpu_body_weighted},{"gpu_body_verified_vertices",witness.gpu_body_verified},
            {"gpu_held_gun_submissions",witness.gpu_gun_submissions},
            {"gpu_held_gun_submissions_by_hand",witness.gpu_gun_submissions_by_hand},
            {"world_held_gun_gpu_observed_by_hand",{witness.gpu_gun_submissions_by_hand[0]>0,witness.gpu_gun_submissions_by_hand[1]>0}},
            {"world_held_gun_gpu_required",false},
            {"world_held_gun_gpu_limit","Every observed GPU gun owner is byte-verified; absent records do not prove admission or CPU rendering"},
            {"joined_world_gpu_limit","Current joined WORLD presentation uses the CPU skin path; native FP GPU and final WORLD banks are verified separately"},
            {"final_submission_component_error",witness.submitted_error},{"authored_marker_component_error",witness.marker_error},
            {"animation_observer",false},{"posed_vertex_observers",false},{"gpu_required",gpu},
            {"render_tick_overlap_enabled",false},
            {"overlap_limit","Frame/node/input observers qualify the synchronous diagnostic traversal; ordinary overlapped production scheduling is a separate gate"},
            {"joined_authority_endpoint_pause",route=="joined"},
            {"samples",witness.samples},{"capture_written",captured}});
        need(result==0 && witness.frames>=16,"Finite native submission run did not observe body, arms and both weapons together");
        report["passed"]=true;save();std::cout<<"PASS actual native dual body/arms/weapons through final "<<graphics<<" submission on "<<route<<'\n';return 0;
    }catch(const std::exception &exception){
        report.update({{"error",exception.what()},{"complete_frames",witness.frames},{"world_frames",witness.world_frames},
            {"gpu_body_frames",witness.gpu_body_frames},{"gpu_body_verified_vertices",witness.gpu_body_verified},
            {"fp_frames",witness.fp_frames},{"authority_endpoint_component_error",witness.endpoint_error},
            {"native_fp_endpoint_verified",witness.fp_endpoint_samples>0},
            {"native_fp_endpoint_component_error",witness.fp_endpoint_samples?Json(witness.fp_endpoint_error):Json(nullptr)},
            {"final_submission_component_error",witness.submitted_error},
            {"authored_marker_component_error",witness.marker_error},{"samples",witness.samples}});
        save();std::cerr<<exception.what()<<'\n';return 1;
    }
#endif
}
