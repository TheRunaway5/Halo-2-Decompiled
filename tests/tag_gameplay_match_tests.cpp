#define SDL_MAIN_HANDLED
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/navigation.hpp"
#include "helpers/gameplay_driver_navigation.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value, const char* message) {
    if (!value) throw FormatError(message);
}
struct Pad {
    int device = -1;
    SDL_JoystickID instance = -1;
    SDL_Joystick* joystick{};
    Pad() {
        device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
            SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        require(device >= 0 && SDL_IsGameController(device), "Virtual SDL gamepad unavailable");
        joystick = SDL_JoystickOpen(device);
        require(joystick, "Virtual SDL gamepad did not open");
        instance=SDL_JoystickInstanceID(joystick);
        axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT, -1);
        axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -1);
    }
    ~Pad() {
        if (joystick) SDL_JoystickClose(joystick);
        // Device indices shift when another virtual controller is detached.
        for(int index=SDL_NumJoysticks()-1;index>=0;--index)
            if(SDL_JoystickGetDeviceInstanceID(index)==instance) {
                SDL_JoystickDetachVirtual(index);break;
            }
    }
    Pad(const Pad&)=delete;
    Pad&operator=(const Pad&)=delete;
    void axis(SDL_GameControllerAxis axis, float value) {
        require(SDL_JoystickSetVirtualAxis(joystick, axis,
            Sint16(std::clamp(value, -1.f, 1.f) * 32767.f)) == 0, "Virtual SDL axis rejected");
    }
    void button(SDL_GameControllerButton button, bool down) {
        require(SDL_JoystickSetVirtualButton(joystick, button, down) == 0, "Virtual SDL button rejected");
    }
    void stop() {
        for (auto axis : {SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY,
                          SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY}) this->axis(axis, 0);
        axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -1);
    }
};
float axis(float error) {
    if (std::abs(error) < .012f) return 0;
    return std::copysign(std::clamp(std::abs(error) * 4.f, .18f, 1.f), error);
}
}

// An unattended player, not an alternate game implementation. The only writes
// after world construction go to SDL virtual controllers. Authored collision,
// spawns, vitality, ammo, damage, scores, respawns and the game clock stay live.
int main(int argc, char** argv) try {
    require(argc == 6 || argc == 7,
        "Supply MAP SHARED XBE ffa|team|ffa-lan|team-lan OUTPUT_DIRECTORY [2|4]");
    require(argc == 6 || std::string_view(argv[6]) == "2" || std::string_view(argv[6]) == "4",
        "Local match observer supports two or four participants");
    const unsigned participants = argc == 7 && std::string_view(argv[6]) == "4" ? 4 : 2;
    if (!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) ||
        !std::filesystem::exists(argv[3])) return 77;
    const std::string_view mode=argv[4];
    const bool lan=mode.ends_with("-lan");
    require(!lan || participants == 2, "The embedded peer observer supports two participants");
    const bool team=mode=="team" || mode=="team-lan";
    require(team || mode=="ffa" || mode=="ffa-lan", "Unknown match type");
    const std::filesystem::path output = argv[5];
    std::filesystem::create_directories(output);
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    require(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0, "SDL controller initialization failed");
    std::vector<Pad> pads(participants);
    MultiplayerOptions options;
    options.map = argv[1]; options.shared_maps = argv[2]; options.gameplay_xbe = argv[3];
    options.local_players = lan?1:participants; options.width = 1280; options.height = 720;
    if(lan) options.host_port=29177;
    options.mute = false;
    options.desktop = std::make_shared<DesktopSession>();
    options.desktop->controller_settings.sensitivity = 5;
    auto* window = options.desktop->window(options.width, options.height, false);
    SDL_RaiseWindow(window); SDL_SetWindowInputFocus(window);
    const Cache menu(options.shared_maps / "mainmenu.map");
    const auto frontend = menu.load_frontend();
    const auto variant = std::find_if(frontend.variants.begin(), frontend.variants.end(), [&](const auto& row) {
        return row.engine == 0 && row.name.starts_with(team ? "Team Slayer " : "Slayer ");
    });
    require(variant != frontend.variants.end(), "Original authored Slayer variant absent");
    options.variant = *variant;
    // Ordinary editable source fields; larger rosters allow more combat.
    options.variant.values.set(2, participants == 4 ? 10 : 3); options.variant.values.set(10, 90);
    options.variant.values.set(89, 5);
    if (team) for(unsigned i=0;i<options.local_players;++i)
        options.selected_teams.push_back(std::int8_t(i%2));
    for(unsigned i=0;i<options.local_players;++i)
        options.input_instances.push_back(SDL_JoystickInstanceID(pads[i].joystick));
    options.prepared_world = std::make_shared<MultiplayerPreparedWorld>(options);
    const auto& world = *options.prepared_world;
    const test::GameplayDriverNavigation navigation(world.content, world.match.simulation().collision());
    std::cout << "Controller fallback walking nodes=" << navigation.fallback_nodes() << std::endl;
    const unsigned jump_period = navigation.fallback_nodes() ? 30 : 120;
    const CollisionWorld bsp_collision(world.content.collision);
    std::array<std::vector<Vec3>, 4> routes;
    std::array<std::optional<unsigned>, 4> route_steps;
    std::array<std::uint32_t, 4> old_units; old_units.fill(null_datum);
    std::array<bool, 4> died{}, respawned{};
    std::array<unsigned,4> fired_by_player{},moved_by_player{};
    unsigned audio_packets{}, particle_draws{}, frames{}, final_scoreboards{}, fired{}, moved{};
    unsigned scene_occlusions{};
    std::array<int, 4> prior_ammo{};
    std::array<std::uint32_t,4> prior_weapons;prior_weapons.fill(null_datum);
    std::array<Vec3, 4> initial_positions{};
    std::array<Vec3,4> progress_positions{};
    std::array<unsigned,4> stagnant{},avoid_until{};
    std::array<bool,4> target_visible{};
    std::array<bool,4> initial{};
    unsigned final_view_mask{};
    bool quit{}, finalized{}, peer_look_initialized{};
    std::unique_ptr<LanClient> peer;
    SDL_GameController* peer_controller=lan?SDL_GameControllerOpen(pads[1].device):nullptr;
    require(!lan || peer_controller,"Remote SDL controller did not open");
    float peer_yaw{},peer_pitch{};
    std::array<std::int8_t,1> peer_team{1};
    std::string raw_join;options.session_lan_host_ready=[&](const std::string &value){raw_join=value;};
    if(lan) options.session_world_loaded=[&](const MultiplayerMatch&,std::span<const unsigned>) {
        const auto fingerprint=multiplayer_fingerprint(options.map,options.shared_maps,
            options.bsp,world.rules.weapon,options.gameplay_xbe);
        peer=std::make_unique<LanClient>(raw_join,fingerprint,1,
            team?std::span<const std::int8_t>(peer_team):std::span<const std::int8_t>{});
    };
    MultiplayerState current;
    std::ofstream trace(output / "ticks.tsv");
    trace << "tick\tplayer\tx\ty\tz\tunit\tweapon\tammo\treserve\tshield\thealth\tscore\tkills\tdeaths\tfinished"
          << "\tyaw\tpitch\tlocal_fire\tmagazine_mode\tweapon_state\ttrigger_flags\tweapon_flags\thand_timer\n";
    options.input_observer = [&](unsigned step, std::span<const Controls> controls, const MultiplayerState& state) {
        current = state;
        require(state.tick <= 3000, "Full match exceeded authored time limit and postgame deadline");
        if(peer) {peer->receive();require(peer->error().empty(),"Real UDP peer failed");}
        require(controls.size() == options.local_players,"Actual local input roster changed");
        for(unsigned i=0;i<participants;++i)
            require(state.players[i].connected || (lan && i==1 && state.tick<64),
                "Actual participant roster changed");
        finalized |= state.results_finalized;
        for (unsigned i = 0; i < participants; ++i) {
            const auto& p = state.players[i];
            if(!p.connected) continue;
            trace << state.tick << '\t' << i << '\t' << p.position.x << '\t' << p.position.y << '\t'
                  << p.position.z << '\t' << p.source_unit << '\t' << p.source_weapon << '\t' << p.loaded
                  << '\t' << p.reserve << '\t' << p.shield << '\t' << p.health << '\t' << p.score
                  << '\t' << p.kills << '\t' << p.deaths << '\t' << state.finished;
            const auto& smg=world.match.unit(i).weapon.smg;
            trace << '\t'<<p.yaw<<'\t'<<p.pitch<<'\t'<<(i<controls.size()?int(controls[i].fire):-1)
                  << '\t'<<smg.magazine_mode<<'\t'<<int(smg.weapon_state)<<'\t'<<smg.trigger_flags
                  << '\t'<<smg.weapon_flags<<'\t'<<int(smg.hand_timers[0])<<'\n';
            if (!initial[i]) { initial_positions[i] = p.position; prior_ammo[i] = p.loaded; initial[i]=true; }
            if(p.source_weapon!=null_datum && prior_weapons[i]==p.source_weapon) {
                const auto rounds=unsigned(std::max(0,prior_ammo[i]-p.loaded));
                fired+=rounds;fired_by_player[i]+=rounds;
            }
            prior_ammo[i] = p.loaded;prior_weapons[i]=p.source_weapon;
            const bool advanced=length(p.position-initial_positions[i])>1.f;
            moved+=advanced;moved_by_player[i]+=advanced;
            if (p.dead) { died[i] = true; pads[i].stop(); routes[i].clear(); route_steps[i].reset(); continue; }
            if (p.source_unit == null_datum) continue;
            if (old_units[i] == null_datum) old_units[i] = p.source_unit;
            respawned[i] |= died[i] && old_units[i] != p.source_unit;
            unsigned opponent_id=participants;float nearest=INFINITY;
            for(unsigned j=0;j<participants;++j)if(j!=i && state.players[j].connected && !state.players[j].dead &&
                state.players[j].source_unit!=null_datum &&
                (!team || state.players[j].source_membership!=p.source_membership)) {
                const auto distance=length(state.players[j].position-p.position);
                if(distance<nearest){nearest=distance;opponent_id=j;}
            }
            if(state.finished || opponent_id==participants){pads[i].stop();continue;}
            const auto& opponent=state.players[opponent_id];
            const auto origin = p.position + Vec3{0,0,world.match.unit(i).definition.camera_height};
            const auto exposed=test::visible_driver_target(world.match.simulation().collision(),origin,
                opponent.position,world.match.unit(opponent_id).body.height);
            const auto target = exposed.value_or(opponent.position + Vec3{0,0,world.match.unit(opponent_id).body.height * .55f});
            const auto delta = target-origin; const auto distance = length(delta);
            // Use the same current scenery/device/prop collision as gameplay.
            // The BSP alone can see through authored fences and machines.
            const auto wall = world.match.simulation().collision().raycast(origin, normalized(delta), distance);
            const auto bsp_wall = bsp_collision.raycast(origin, normalized(delta), distance);
            if(wall && wall->distance<distance-.15f && (!bsp_wall || bsp_wall->distance>=distance-.15f)) {
                if(!scene_occlusions)std::cout<<"Driver scene occlusion: tick="<<state.tick<<" player="<<i
                    <<" target_distance="<<distance<<" collision_distance="<<wall->distance<<std::endl;
                ++scene_occlusions;
            }
            const bool visible = !wall || wall->distance >= distance-.15f;
            target_visible[i]=visible;
            if (!route_steps[i] || step >= *route_steps[i] + 30) {
                routes[i] = navigation.route(p.position, opponent.position);
                route_steps[i] = step;
            }
            while (!routes[i].empty() && length(routes[i].front()-p.position) < .6f) routes[i].erase(routes[i].begin());
            const auto destination = visible || routes[i].empty() ? opponent.position : routes[i].front();
            auto toward = destination-p.position;
            bool walk = !visible || distance > 4.f;
            if(!walk || std::hypot(p.position.x-progress_positions[i].x,
                                  p.position.y-progress_positions[i].y)>.1f) {
                stagnant[i]=0;progress_positions[i]=p.position;
            } else if(++stagnant[i]>60) {
                // If an authored route is unavailable at a rock/ledge, try
                // walking around it. This changes only controller movement.
                avoid_until[i]=step+60;stagnant[i]=0;
            }
            if(step<avoid_until[i]) {toward={-toward.y,toward.x,0};walk=true;}
            const auto look = visible ? delta : toward;
            const float yaw = std::atan2(look.y, look.x);
            const float pitch = visible ? std::atan2(look.z, std::hypot(look.x,look.y)) : 0;
            pads[i].axis(SDL_CONTROLLER_AXIS_RIGHTX, -axis(std::remainder(yaw-p.yaw,6.283185307f)));
            pads[i].axis(SDL_CONTROLLER_AXIS_RIGHTY, -axis(pitch-p.pitch));
            const float planar = std::hypot(toward.x,toward.y);
            pads[i].axis(SDL_CONTROLLER_AXIS_LEFTY, walk && planar>.1f ?
                -(toward.x*std::cos(p.yaw)+toward.y*std::sin(p.yaw))/planar : 0);
            pads[i].axis(SDL_CONTROLLER_AXIS_LEFTX, walk && planar>.1f ?
                (toward.x*std::sin(p.yaw)-toward.y*std::cos(p.yaw))/planar : 0);
            pads[i].axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, visible && distance<45.f ? 1.f : -1.f);
            pads[i].button(SDL_CONTROLLER_BUTTON_X, p.loaded == 0 && step%90 < 3);
            pads[i].button(SDL_CONTROLLER_BUTTON_A, walk && step%jump_period < 2);
        }
        if(peer) {
            const auto input=controller_input(sample_controller(peer_controller),options.desktop->controller_settings);
            if(!peer_look_initialized && state.players[1].connected) {
                peer_yaw=state.players[1].yaw;peer_pitch=state.players[1].pitch;peer_look_initialized=true;
            }
            controller_look(input,options.desktop->controller_settings,tick_seconds,peer_yaw,peer_pitch);
            Controls command;command.forward=input.forward;command.right=input.right;
            command.yaw=peer_yaw;command.pitch=peer_pitch;command.fire=input.fire;
            command.reload=input.reload;command.jump=input.jump;
            const std::array commands{command};peer->send(commands);
        }
        if (state.tick%300 == 0) std::cout << "Live " << argv[4] << " tick=" << state.tick
            << " score=" << state.players[0].score << ',' << state.players[1].score
            << " kills=" << state.players[0].kills << ',' << state.players[1].kills
            << " visible="<<target_visible[0]<<','<<target_visible[1]
            << " local_fire="<<controls[0].fire<<std::endl;
        if (state.game_over_complete() && !quit) {
            SDL_Event e{}; e.type = SDL_QUIT;
            require(SDL_PushEvent(&e) == 1, "Could not close completed match window"); quit = true;
        }
    };
    options.audio_observer = [&](const MultiplayerAudioSubmission& packet) {
        require(packet.frames > 0, "Live gameplay submitted empty audio"); ++audio_packets;
    };
    options.effect_observer = [&](unsigned, std::uint32_t, std::span<const ParticleDraw> draws) {
        particle_draws += unsigned(draws.size());
    };
    options.frame_observer = [&](const MultiplayerFrameTiming& frame) { frames += frame.submitted; };
    options.scoreboard_observer = [&](unsigned view, bool normal, const FullScoreboardLayout& layout,
                                      const FullScoreboardPaintResult& paint) {
        if (!normal && current.finished && !paint.draw.vertices.empty()) {
            require(layout.rows.size() == participants, "Final rendered scoreboard lost a participant");
            require(view<options.local_players,"Final scoreboard has an invalid local view");
            final_view_mask|=1u<<view;++final_scoreboards;
        }
    };
    MultiplayerRuntimeOutcome outcome;
    require(run_multiplayer(options,outcome) == 0, "Actual full-match runtime failed");
    require(outcome.state && outcome.authoritative_banks && outcome.state->finished &&
            outcome.state->results_finalized && outcome.state->game_over_complete() && finalized,
            "Full match did not complete its original result/countdown owners");
    require(fired > 10 && moved > 100 && audio_packets > 10 && particle_draws && frames > 100 && final_scoreboards,
            "Full rendered match missed movement, weapons, audio, particles or automatic results");
    unsigned kills=0;bool saw_respawn=false;
    for(unsigned i=0;i<participants;++i){kills+=current.players[i].kills;saw_respawn|=respawned[i];}
    require(kills>0 && saw_respawn,
            "Full match missed actual fatal damage and salted respawn");
    require(final_view_mask==(1u<<options.local_players)-1,"A local view missed the completed scoreboard");
    if(participants==4)for(unsigned i=0;i<participants;++i)
        require(fired_by_player[i]>0 && moved_by_player[i]>20,
            "A local participant missed real firing or movement");
    if(peer) {
        peer->receive();const auto& remote=peer->state();
        require(peer->ready() && remote.finished && remote.results_finalized && remote.game_over_complete(),
            "Real UDP peer missed complete final results");
        for(unsigned i=0;i<2;++i) require(remote.players[i].score==current.players[i].score &&
            remote.players[i].kills==current.players[i].kills && remote.players[i].deaths==current.players[i].deaths,
            "Real UDP peer final combat records differ");
        SDL_GameControllerClose(peer_controller);
    }
    std::cout << "PASS complete live " << argv[4] << " match: ticks=" << current.tick
        << " rendered_frames=" << frames << " fired_rounds=" << fired << " audio_packets=" << audio_packets
        << " particle_draws=" << particle_draws << " final_scoreboard_draws=" << final_scoreboards
        << " score=";
    for(unsigned i=0;i<participants;++i)std::cout<<(i?",":"")<<current.players[i].score;
    std::cout<<" scene_occlusions="<<scene_occlusions<<" players="<<participants;
    for(unsigned i=0;i<participants;++i)std::cout<<" fired_player"<<i<<'='<<fired_by_player[i];
    std::cout<<'\n';
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
