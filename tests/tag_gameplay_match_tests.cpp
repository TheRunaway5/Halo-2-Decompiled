#define SDL_MAIN_HANDLED
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/navigation.hpp"
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
    SDL_Joystick* joystick{};
    Pad() {
        device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
            SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        require(device >= 0 && SDL_IsGameController(device), "Virtual SDL gamepad unavailable");
        joystick = SDL_JoystickOpen(device);
        require(joystick, "Virtual SDL gamepad did not open");
        axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT, -1);
        axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -1);
    }
    ~Pad() {
        if (joystick) SDL_JoystickClose(joystick);
        if (device >= 0) SDL_JoystickDetachVirtual(device);
    }
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
    require(argc == 6, "Supply MAP SHARED XBE ffa|team|ffa-lan|team-lan OUTPUT_DIRECTORY");
    if (!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) ||
        !std::filesystem::exists(argv[3])) return 77;
    const std::string_view mode=argv[4];
    const bool lan=mode.ends_with("-lan");
    const bool team=mode=="team" || mode=="team-lan";
    require(team || mode=="ffa" || mode=="ffa-lan", "Unknown match type");
    const std::filesystem::path output = argv[5];
    std::filesystem::create_directories(output);
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    require(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0, "SDL controller initialization failed");
    Pad pads[2];
    MultiplayerOptions options;
    options.map = argv[1]; options.shared_maps = argv[2]; options.gameplay_xbe = argv[3];
    options.local_players = lan?1:2; options.width = 1280; options.height = 720;
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
    // Ordinary editable source fields: three kills, 90 seconds, authored SMG.
    options.variant.values.set(2, 3); options.variant.values.set(10, 90);
    options.variant.values.set(89, 5);
    if (team) options.selected_teams = lan?std::vector<std::int8_t>{0}:std::vector<std::int8_t>{0,1};
    for(unsigned i=0;i<options.local_players;++i)
        options.input_instances.push_back(SDL_JoystickInstanceID(pads[i].joystick));
    options.prepared_world = std::make_shared<MultiplayerPreparedWorld>(options);
    const auto& world = *options.prepared_world;
    const NavigationWorld navigation(world.content.navigation);
    const CollisionWorld bsp_collision(world.content.collision);
    std::array<std::vector<Vec3>, 2> routes;
    std::array<std::uint32_t, 2> old_units{null_datum, null_datum};
    std::array<bool, 2> died{}, respawned{};
    unsigned audio_packets{}, particle_draws{}, frames{}, final_scoreboards{}, fired{}, moved{};
    unsigned scene_occlusions{};
    std::array<int, 2> prior_ammo{};
    std::array<std::uint32_t,2> prior_weapons{null_datum,null_datum};
    std::array<Vec3, 2> initial_positions{};
    std::array<Vec3,2> progress_positions{};
    std::array<unsigned,2> stagnant{},avoid_until{};
    std::array<bool,2> target_visible{};
    std::array<bool,2> initial{};
    bool quit{}, finalized{}, peer_look_initialized{};
    std::unique_ptr<LanClient> peer;
    SDL_GameController* peer_controller=lan?SDL_GameControllerOpen(pads[1].device):nullptr;
    require(!lan || peer_controller,"Remote SDL controller did not open");
    float peer_yaw{},peer_pitch{};
    std::array<std::int8_t,1> peer_team{1};
    if(lan) options.session_world_loaded=[&](const MultiplayerMatch&,std::span<const unsigned>) {
        const auto fingerprint=multiplayer_fingerprint(options.map,options.shared_maps,
            options.bsp,world.rules.weapon,options.gameplay_xbe);
        peer=std::make_unique<LanClient>("127.0.0.1:"+std::to_string(options.host_port),fingerprint,1,
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
        require(controls.size() == options.local_players && state.players[0].connected &&
                (state.players[1].connected || (lan && state.tick<64)),
                "Actual two-player roster/input changed");
        finalized |= state.results_finalized;
        for (unsigned i = 0; i < 2; ++i) {
            const auto& p = state.players[i]; const auto& opponent = state.players[1-i];
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
            if(p.source_weapon!=null_datum && prior_weapons[i]==p.source_weapon)
                fired += unsigned(std::max(0, prior_ammo[i] - p.loaded));
            prior_ammo[i] = p.loaded;prior_weapons[i]=p.source_weapon;
            moved += length(p.position - initial_positions[i]) > 1.f;
            if (p.dead) { died[i] = true; pads[i].stop(); continue; }
            if (p.source_unit == null_datum) continue;
            if (old_units[i] == null_datum) old_units[i] = p.source_unit;
            respawned[i] |= died[i] && old_units[i] != p.source_unit;
            if (state.finished || !opponent.connected || opponent.dead) { pads[i].stop(); continue; }
            const auto origin = p.position + Vec3{0,0,world.match.unit(i).definition.camera_height};
            const auto target = opponent.position + Vec3{0,0,world.match.unit(1-i).body.height * .55f};
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
            if (step % 30 == 0 || routes[i].empty()) routes[i] = navigation.route(p.position, opponent.position);
            while (!routes[i].empty() && length(routes[i].front()-p.position) < .6f) routes[i].erase(routes[i].begin());
            const auto destination = visible || routes[i].empty() ? opponent.position : routes[i].front();
            auto toward = destination-p.position;
            bool walk = !visible || distance > 4.f;
            if(!walk || length(p.position-progress_positions[i])>.1f) {
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
            pads[i].button(SDL_CONTROLLER_BUTTON_A, walk && step%120 < 2);
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
    options.scoreboard_observer = [&](unsigned, bool normal, const FullScoreboardLayout& layout,
                                      const FullScoreboardPaintResult& paint) {
        if (!normal && current.finished && !paint.draw.vertices.empty()) {
            require(layout.rows.size() == 2, "Final rendered scoreboard lost a participant"); ++final_scoreboards;
        }
    };
    MultiplayerRuntimeOutcome outcome;
    require(run_multiplayer(options,outcome) == 0, "Actual full-match runtime failed");
    require(outcome.state && outcome.authoritative_banks && outcome.state->finished &&
            outcome.state->results_finalized && outcome.state->game_over_complete() && finalized,
            "Full match did not complete its original result/countdown owners");
    require(fired > 10 && moved > 100 && audio_packets > 10 && particle_draws && frames > 100 && final_scoreboards,
            "Full rendered match missed movement, weapons, audio, particles or automatic results");
    require(current.players[0].kills+current.players[1].kills > 0 && (respawned[0] || respawned[1]),
            "Full match missed actual fatal damage and salted respawn");
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
        << " score=" << current.players[0].score << ',' << current.players[1].score
        << " scene_occlusions="<<scene_occlusions<<'\n';
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
