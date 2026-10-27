#define SDL_MAIN_HANDLED
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/navigation.hpp"
#include "helpers/gameplay_driver_navigation.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value, const char* message) {
    if (!value) throw FormatError(message);
}
struct Controller {
    int device = -1;
    SDL_Joystick* joystick{};
    Controller() {
        device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
            SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        check(device >= 0 && SDL_IsGameController(device), "SDL controller unavailable");
        joystick = SDL_JoystickOpen(device);
        check(joystick, "SDL controller did not open");
        axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT, -1);
        axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -1);
    }
    ~Controller() {
        if (joystick) SDL_JoystickClose(joystick);
        if (device >= 0) SDL_JoystickDetachVirtual(device);
    }
    void axis(SDL_GameControllerAxis id, float value) {
        check(SDL_JoystickSetVirtualAxis(joystick, id,
            Sint16(std::clamp(value, -1.f, 1.f)*32767.f)) == 0, "SDL axis rejected");
    }
    void button(SDL_GameControllerButton id, bool pressed) {
        check(SDL_JoystickSetVirtualButton(joystick, id, pressed) == 0, "SDL button rejected");
    }
    void stop() {
        for (auto id : {SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY,
                        SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY}) axis(id, 0);
        axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -1);
    }
};
float look_axis(float error) {
    return std::abs(error) < .012f ? 0.f :
        std::copysign(std::clamp(std::abs(error)*4.f, .18f, 1.f), error);
}
}

// Run one role per OS process. Both roles load their own tag world, render,
// mix audio and send ordinary SDL controller input through the real runtime.
// Observers read state; they never supply game state, damage or a game clock.
int main(int argc, char** argv) try {
    check(argc == 6 || argc == 7, "Supply MAP SHARED XBE host|client OUTPUT_DIRECTORY [HOST_JOIN_TOKEN]");
    const bool client = std::string_view(argv[4]) == "client";
    check(client || std::string_view(argv[4]) == "host", "Unknown network role");
    if (!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) ||
        !std::filesystem::exists(argv[3])) return 77;
    const std::filesystem::path output = argv[5];
    std::filesystem::create_directories(output);
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0, "SDL initialization failed");
    Controller pad;
    MultiplayerOptions options;
    options.map = argv[1]; options.shared_maps = argv[2]; options.gameplay_xbe = argv[3];
    options.local_players = 1; options.width = 1280; options.height = 720;
    if (client) {check(argc==7,"Client requires the independently exchanged host join token");options.join=argv[6];}
    else options.host_port = 29179;
    options.selected_teams = {std::int8_t(client ? 1 : 0)};
    options.input_instances = {SDL_JoystickInstanceID(pad.joystick)};
    options.desktop = std::make_shared<DesktopSession>();
    options.desktop->controller_settings.sensitivity = 5;
    auto* window = options.desktop->window(options.width, options.height, false);
    SDL_RaiseWindow(window); SDL_SetWindowInputFocus(window);
    const Cache menu(options.shared_maps / "mainmenu.map");
    const auto frontend = menu.load_frontend();
    const auto variant = std::find_if(frontend.variants.begin(), frontend.variants.end(), [](const auto& v) {
        return v.engine == 0 && v.name.starts_with("Team Slayer ");
    });
    check(variant != frontend.variants.end(), "Authored Team Slayer variant absent");
    options.variant = *variant;
    options.variant.values.set(2, 3);
    options.variant.values.set(10, 90);
    options.variant.values.set(89, 5);
    options.prepared_world = std::make_shared<MultiplayerPreparedWorld>(options);
    const auto& world = *options.prepared_world;
    const test::GameplayDriverNavigation navigation(world.content, world.match.simulation().collision());
    std::cout << "Controller fallback walking nodes=" << navigation.fallback_nodes() << std::endl;
    const unsigned jump_period = navigation.fallback_nodes() ? 30 : 120;
    std::optional<unsigned> local_slot;
    if (!client) {
        check(world.local_slots.size() == 1, "Host local identity absent");
        local_slot = world.local_slots.front();
    }
    // The client's actual scoreboard highlight supplies its player identity.
    // Do not infer an assigned player slot from names or join order.
    const auto scoreboard_binding = options.desktop->keyboard_settings.bindings[unsigned(PcAction::Scoreboard)];
    check(scoreboard_binding > 0 && scoreboard_binding < SDL_NUM_SCANCODES,
          "Network identity test needs its configured keyboard scoreboard binding");
    const auto scoreboard_key = [&](bool down) {
        SDL_Event event{}; event.type = down ? SDL_KEYDOWN : SDL_KEYUP;
        event.key.windowID = SDL_GetWindowID(window);
        event.key.state = down ? SDL_PRESSED : SDL_RELEASED;
        event.key.keysym.scancode = SDL_Scancode(scoreboard_binding);
        event.key.keysym.sym = SDL_GetKeyFromScancode(SDL_Scancode(scoreboard_binding));
        check(SDL_PushEvent(&event) == 1, "Scoreboard input event rejected");
    };
    MultiplayerState current;
    std::vector<Vec3> route;
    std::optional<unsigned> route_step;
    std::array<std::uint32_t, 2> initial_units{null_datum, null_datum};
    std::array<bool, 2> died{}, respawned{};
    unsigned frames{}, audio{}, particles{}, boards{}, fired{}, moved{}, fire_inputs{}, finished_steps{};
    unsigned stagnant{}, avoid_until{};
    Vec3 first_position{}, progress_position{};
    bool position_seen{}, quit{};
    int previous_ammo{};
    std::uint32_t previous_weapon = null_datum;
    std::ofstream trace(output / "ticks.tsv");
    trace << "tick\tlocal_slot\tplayer\tx\ty\tz\tunit\tweapon\thealth\tshield\tammo\treserve\tscore\tkills\tdeaths\tfinished\tcountdown\tlocal_fire\n";
    options.input_observer = [&](unsigned step, std::span<const Controls> input, const MultiplayerState& state) {
        current = state;
        check(state.tick <= 3100 && step <= 3600, "Network full match exceeded its deadline");
        check(input.size() == 1, "Network local input roster changed");
        fire_inputs += input[0].fire;
        if (client && !local_slot && step%15 == 0) scoreboard_key(true);
        for (unsigned i = 0; i < 2; ++i) {
            const auto& p = state.players[i];
            if (!p.connected) continue;
            trace << state.tick << '\t' << (local_slot ? int(*local_slot) : -1) << '\t' << i << '\t'
                  << p.position.x << '\t' << p.position.y << '\t' << p.position.z << '\t'
                  << p.source_unit << '\t' << p.source_weapon << '\t' << p.health << '\t' << p.shield
                  << '\t' << p.loaded << '\t' << p.reserve << '\t' << p.score << '\t' << p.kills
                  << '\t' << p.deaths << '\t' << state.finished << '\t' << state.game_over_countdown
                  << '\t' << input[0].fire << '\n';
            if (p.source_unit != null_datum && initial_units[i] == null_datum) initial_units[i] = p.source_unit;
            died[i] |= p.dead;
            respawned[i] |= died[i] && p.source_unit != null_datum && p.source_unit != initial_units[i];
        }
        if (state.game_over_complete()) {
            pad.stop();
            if (++finished_steps == 15 && !quit) {
                SDL_Event event{}; event.type = SDL_QUIT;
                check(SDL_PushEvent(&event) == 1, "Complete match quit failed"); quit = true;
            }
        }
        if (!local_slot || !state.players[*local_slot].connected) return;
        const auto& p = state.players[*local_slot];
        if (p.source_weapon != null_datum && p.source_weapon == previous_weapon)
            fired += unsigned(std::max(0, previous_ammo - p.loaded));
        previous_ammo = p.loaded; previous_weapon = p.source_weapon;
        if (!position_seen) { first_position = p.position; position_seen = true; }
        moved += length(p.position - first_position) > 1.f;
        const auto& opponent = state.players[1-*local_slot];
        if (p.dead || state.finished || !opponent.connected || opponent.dead) {
            pad.stop();if(p.dead){route.clear();route_step.reset();}return;
        }
        const auto& definition = multiplayer_biped(world.content, multiplayer_representation(p.source_configuration));
        const auto& other = multiplayer_biped(world.content, multiplayer_representation(opponent.source_configuration));
        const auto origin = p.position + Vec3{0,0,definition.camera_height};
        const auto exposed=test::visible_driver_target(world.match.simulation().collision(),origin,
            opponent.position,other.height);
        const auto delta = exposed.value_or(opponent.position + Vec3{0,0,other.height*.55f}) - origin;
        const auto distance = length(delta);
        const auto wall = world.match.simulation().collision().raycast(origin, normalized(delta), distance);
        const bool visible = !wall || wall->distance >= distance-.15f;
        if (!route_step || step >= *route_step + 30) {
            route = navigation.route(p.position, opponent.position);route_step = step;
        }
        while (!route.empty() && length(route.front()-p.position) < .6f) route.erase(route.begin());
        auto toward = (visible || route.empty() ? opponent.position : route.front()) - p.position;
        bool walk = !visible || distance > 4.f;
        if (!walk || std::hypot(p.position.x-progress_position.x,
                               p.position.y-progress_position.y) > .1f) {
            stagnant = 0; progress_position = p.position;
        } else if (++stagnant > 60) { avoid_until = step+60; stagnant = 0; }
        if (step < avoid_until) { toward = {-toward.y,toward.x,0}; walk = true; }
        const auto look = visible ? delta : toward;
        const float yaw = std::atan2(look.y,look.x);
        const float pitch = visible ? std::atan2(look.z,std::hypot(look.x,look.y)) : 0;
        pad.axis(SDL_CONTROLLER_AXIS_RIGHTX, -look_axis(std::remainder(yaw-p.yaw,6.283185307f)));
        pad.axis(SDL_CONTROLLER_AXIS_RIGHTY, -look_axis(pitch-p.pitch));
        const float planar = std::hypot(toward.x,toward.y);
        pad.axis(SDL_CONTROLLER_AXIS_LEFTY, walk && planar>.1f ?
            -(toward.x*std::cos(p.yaw)+toward.y*std::sin(p.yaw))/planar : 0);
        pad.axis(SDL_CONTROLLER_AXIS_LEFTX, walk && planar>.1f ?
            (toward.x*std::sin(p.yaw)-toward.y*std::cos(p.yaw))/planar : 0);
        pad.axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, visible && distance<45.f ? 1.f : -1.f);
        pad.button(SDL_CONTROLLER_BUTTON_X, p.loaded == 0 && step%90 < 3);
        pad.button(SDL_CONTROLLER_BUTTON_A, walk && step%jump_period < 2);
        if (state.tick%300 == 0) std::cout << argv[4] << " tick=" << state.tick
            << " local_slot=" << *local_slot << " score=" << state.players[0].score
            << ',' << state.players[1].score << std::endl;
    };
    options.scoreboard_observer = [&](unsigned view, bool normal, const FullScoreboardLayout& layout,
                                      const FullScoreboardPaintResult& paint) {
        check(view == 0, "Network local view changed");
        for (const auto& row : layout.rows) if (row.current_player)
            for (unsigned i = 0; i < 2; ++i)
                if (current.players[i].connected && row.datum == current.players[i].source_player) {
                    if (local_slot) check(*local_slot == i, "Network local identity changed");
                    if (!local_slot && client) scoreboard_key(false);
                    local_slot = i;
                }
        if (!normal && current.finished && !paint.draw.vertices.empty()) {
            check(layout.rows.size() == 2, "Final network scoreboard lost a player"); ++boards;
        }
    };
    options.audio_observer = [&](const MultiplayerAudioSubmission& packet) {
        check(packet.frames > 0, "Empty network audio submission"); ++audio;
    };
    options.effect_observer = [&](unsigned, std::uint32_t, std::span<const ParticleDraw> draws) { particles += unsigned(draws.size()); };
    options.frame_observer = [&](const MultiplayerFrameTiming& timing) { frames += timing.submitted; };
    MultiplayerRuntimeOutcome outcome;
    check(run_multiplayer(options,outcome) == 0, "Network rendered runtime failed");
    check(outcome.state && outcome.state->results_finalized && outcome.state->game_over_complete(), "Network results/countdown incomplete");
    check(outcome.authority == (client ? MultiplayerOutcomeAuthority::client_snapshot : MultiplayerOutcomeAuthority::host) &&
          bool(outcome.authoritative_banks) == !client, "Network result authority was fabricated");
    check(local_slot && frames > 100 && audio > 10 && particles && boards && fired > 10 && moved > 100 && fire_inputs,
          "Network match missed local movement/fire, rendering, audio, effects or results");
    check(current.players[0].kills+current.players[1].kills > 0 && (respawned[0] || respawned[1]),
          "Network match missed actual fatal damage and salted respawn");
    std::ofstream final(output / "final.tsv");
    final << "player\tplayer_datum\tunit\tweapon\tscore\tkills\tdeaths\n";
    for (unsigned i = 0; i < 2; ++i) {
        const auto& p = outcome.state->players[i];
        final << i << '\t' << p.source_player << '\t' << p.source_unit << '\t' << p.source_weapon
              << '\t' << p.score << '\t' << p.kills << '\t' << p.deaths << '\n';
    }
    std::cout << "PASS complete two-process " << argv[4] << " match: ticks=" << current.tick
        << " local_slot=" << *local_slot << " rendered_frames=" << frames << " fired_rounds=" << fired
        << " audio_packets=" << audio << " particle_draws=" << particles << " final_scoreboard_draws=" << boards
        << " score=" << current.players[0].score << ',' << current.players[1].score << '\n';
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
