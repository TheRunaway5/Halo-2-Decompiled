#include "halo2/multiplayer_network.hpp"
#include "halo2/original_data.hpp"
#include "helpers/gameplay_driver_navigation.hpp"
#include "helpers/multiplayer_snapshot_checks.hpp"
#include "helpers/loopback_packet_proxy.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <deque>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string_view>
#include <thread>

using namespace halo2;
namespace {
void require(bool value, const char *message) {
    if (!value) throw FormatError(message);
}

// Read actual gameplay state and submit ordinary Controls. No rendering,
// audio device, state injection, collision edits or alternate game rules.
struct Driver {
    Vec3 initial{}, progress{};
    std::uint32_t first_unit = null_datum, prior_weapon = null_datum;
    int prior_ammo{};
    unsigned fired{}, moved{}, stagnant{}, avoid_until{};
    bool initialized{}, died{}, respawned{};
    std::vector<Vec3> route;
    std::optional<unsigned> route_tick;

    Controls input(unsigned id, const MultiplayerMatch &match,
                   const MultiplayerState &state, const test::GameplayDriverNavigation &navigation) {
        const auto &player = state.players[id];
        Controls result;
        result.yaw = player.yaw;
        result.pitch = player.pitch;
        if (!initialized) { initial = progress = player.position; initialized = true; }
        moved += std::hypot(player.position.x - initial.x, player.position.y - initial.y) > .5f;
        if (player.source_weapon == prior_weapon && player.loaded < prior_ammo) ++fired;
        prior_weapon = player.source_weapon;
        prior_ammo = player.loaded;
        if (player.dead) { died = true; route.clear(); route_tick.reset(); return result; }
        if (player.source_unit == null_datum) return result;
        if (first_unit == null_datum) first_unit = player.source_unit;
        respawned |= died && first_unit != player.source_unit;
        if (state.finished) return result;
        const auto &collision = match.simulation().collision();
        const auto origin = player.position + Vec3{0, 0, match.unit(id).definition.camera_height};
        unsigned target_id = multiplayer_capacity;
        std::optional<Vec3> visible;
        std::array<std::pair<float, unsigned>, multiplayer_capacity> candidates;
        std::size_t count = 0;
        for (unsigned other = 0; other < multiplayer_capacity; ++other) {
            const auto &target = state.players[other];
            if (other == id || !target.connected || target.dead || target.source_unit == null_datum ||
                (state.team_mode && target.source_membership == player.source_membership)) continue;
            const auto distance = length(target.position - player.position);
            require(std::isfinite(distance), "Controller target distance is not finite");
            candidates[count++] = {distance, other};
        }
        // The first visible candidate in distance/slot order is the same
        // opponent as the exhaustive search, with fewer collision queries.
        std::sort(candidates.begin(), candidates.begin() + count);
        if (count) target_id = candidates[0].second;
        for (std::size_t index = 0; index < count; ++index) {
            const auto other = candidates[index].second;
            visible = test::visible_driver_target(collision, origin, state.players[other].position,
                                                  match.unit(other).body.height);
            if (visible) { target_id = other; break; }
        }
        if (target_id == multiplayer_capacity) return result;
        const auto &target = state.players[target_id];
        const auto delta = visible.value_or(target.position + Vec3{0, 0,
                                      match.unit(target_id).body.height * .55f}) - origin;
        result.yaw = std::atan2(delta.y, delta.x);
        result.pitch = std::clamp(std::atan2(delta.z, std::hypot(delta.x, delta.y)), -1.5f, 1.5f);
        result.fire = visible.has_value();
        result.reload = player.loaded == 0 && player.reserve > 0;
        if (!visible && (!route_tick || state.tick >= *route_tick + 30)) {
            route = navigation.route(player.position, target.position);
            route_tick = state.tick;
        }
        while (!route.empty() && length(route.front() - player.position) < .6f)
            route.erase(route.begin());
        auto toward = (visible || route.empty() ? target.position : route.front()) - player.position;
        const bool walk = !visible || length(delta) > 4.f;
        if (!walk || std::hypot(player.position.x - progress.x, player.position.y - progress.y) > .1f) {
            progress = player.position; stagnant = 0;
        } else if (++stagnant > 60) {
            stagnant = 0; avoid_until = state.tick + 60;
        }
        if (state.tick < avoid_until) toward = {-toward.y, toward.x, 0};
        if (walk) {
            const auto direction = normalized(Vec3{toward.x, toward.y, 0});
            result.forward = std::clamp(dot(direction, Vec3{std::cos(result.yaw), std::sin(result.yaw), 0}), -1.f, 1.f);
            // step_players supplies -Controls.right to the unit's left axis.
            result.right = std::clamp(dot(direction, Vec3{std::sin(result.yaw), -std::cos(result.yaw), 0}), -1.f, 1.f);
            result.jump = state.tick % (navigation.fallback_nodes() ? 30 : 120) == 0;
        }
        return result;
    }
};


}

int main(int argc, char **argv) try {
    require(argc >= 7 && argc <= 9, "Supply MAP SHARED XBE MODE OUTPUT PLAYERS [PEER_SEATS [loss-reorder]]");
    const std::string mode = argv[4];
    const bool team = mode == "team" || mode == "team-udp";
    const bool network = mode == "ffa-udp" || mode == "team-udp";
    require(team || mode == "ffa" || mode == "ffa-udp", "Unknown simulation mode");
    const unsigned participants = std::stoul(argv[6]);
    require(participants == 4 || participants == 16, "Use four or sixteen simulated participants");
    const unsigned peer_seats = argc >= 8 ? std::stoul(argv[7]) : 1;
    const bool faults = argc == 9;
    require(!faults || (network && std::string_view(argv[8]) == "loss-reorder"), "Unknown network fault profile");
    require(peer_seats == 1 || peer_seats == 4, "Use one or four local players per peer");
    require(!network || participants > peer_seats, "UDP simulation needs at least two peer groups");
    const std::filesystem::path output = argv[5];
    std::filesystem::create_directories(output);
    auto rules = slayer_rules({"Tag simulation", 0, {{2, 25}, {10, 180}, {89, 5}, {91, team ? 1 : 0}}}, team);
    LevelContent content;
    std::vector<Spawn> spawns;
    {
        // Decode through the real shared tag API. Release the file buffer
        // before simulating; the imported content owns its gameplay data.
        Cache cache(argv[1], argv[2], OriginalData::load_xbe(argv[3]));
        content = cache.load_content(0, true);
        spawns.assign(cache.spawns().begin(), cache.spawns().end());
        rules.weapon = cache.slayer_starting_weapon(5);
        require(cache.find_tag({rules.weapon}, tag_groups::weapon), "Starting weapon lost its tag identity");
    }
    MultiplayerMatch match(content, 0, spawns, rules);
    match.configure_weapon_pickups();
    const unsigned authority_seats = network ? peer_seats : participants;
    for (unsigned i = 0; i < authority_seats; ++i)
        require(match.join("Authority", team ? int(i % 2) : -1) == i, "Authority roster is incomplete");
    std::unique_ptr<LanHost> host;
    std::vector<std::unique_ptr<LanClient>> peers;
    std::vector<std::unique_ptr<test::LoopbackPacketProxy>> proxies;
    const auto pump = [&](std::uint32_t tick) { for (auto &proxy : proxies) proxy->pump(tick); };
    std::array<Controls, multiplayer_capacity> controls{};
    if (network) {
        const auto fingerprint = multiplayer_fingerprint(argv[1], argv[2], 0, rules.weapon, argv[3]);
        host = std::make_unique<LanHost>(match, 0, fingerprint);
        const auto address = "127.0.0.1:" + std::to_string(host->port());
        for (unsigned i = authority_seats; i < participants; i += peer_seats) {
            auto peer_address = address;
            if (faults) {
                proxies.push_back(std::make_unique<test::LoopbackPacketProxy>(host->port()));
                peer_address = "127.0.0.1:" + std::to_string(proxies.back()->port());
            }
            std::vector<std::int8_t> membership(peer_seats);
            for (unsigned seat = 0; seat < peer_seats; ++seat) membership[seat] = std::int8_t((i + seat) % 2);
            auto peer = std::make_unique<LanClient>(peer_address, fingerprint, peer_seats,
                team ? std::span<const std::int8_t>(membership) : std::span<const std::int8_t>{});
            for (unsigned attempt = 0; attempt < 64 && !peer->ready(); ++attempt) {
                peer->send(std::vector<Controls>(peer_seats)); pump(0); host->receive(controls);
                host->broadcast(); pump(0); peer->receive();
                for (auto &joined : peers) joined->receive();
            }
            require(peer->ready() && peer->slots().size() == peer_seats, "Lightweight UDP peer did not join");
            peers.push_back(std::move(peer));
        }
        require(host->peers() == participants / peer_seats - 1, "UDP peer roster is incomplete");
        if (faults) {
            host->broadcast(); pump(0);
            for (auto &peer : peers) peer->receive();
        }
        for (auto &proxy : proxies) proxy->enable();
    }
    const test::GameplayDriverNavigation navigation(content, match.simulation().collision());
    std::cout << "Headless " << mode << " players=" << participants
              << " navigation_nodes=" << navigation.fallback_nodes() << std::endl;
    std::array<Driver, multiplayer_capacity> drivers;
    unsigned deliveries = 0;
    unsigned maximum_peer_lag = 0;
    std::deque<MultiplayerState> history;
    if (faults) history.push_back(match.state());
    const auto receive_faulted = [&](LanClient &peer) {
        peer.receive();
        require(peer.error().empty(), "UDP peer rejected the faulted session");
        for (const auto &snapshot : peer.received_states()) {
            const auto original = std::find_if(history.begin(), history.end(), [&](const auto &state) {
                return state.tick == snapshot.tick;
            });
            require(original != history.end(), "UDP snapshot escaped the bounded authority history");
            test::verify_multiplayer_snapshot(*original, snapshot);
            ++deliveries;
        }
    };
    std::ofstream trace(output / "ticks.tsv");
    trace << "tick\tplayer\tx\ty\tz\tunit\tweapon\tammo\tscore\tkills\tdeaths\tfinished\n";
    while (!match.state().game_over_complete()) {
        const auto state = match.state();
        require(state.tick < rules.time_ticks + 600, "Simulation failed to complete its ordinary clock/countdown");
        controls = {};
        for (unsigned i = 0; i < participants; ++i) {
            require(state.players[i].connected, "A simulated participant disappeared");
            controls[i] = drivers[i].input(i, match, state, navigation);
            if (std::abs(controls[i].forward) > 1 || std::abs(controls[i].right) > 1 ||
                std::abs(controls[i].pitch) > 1.5f) {
                std::ostringstream error;
                error << std::setprecision(9) << "Simulated input exceeds normal PC bounds: player=" << i
                      << " forward=" << controls[i].forward << " right=" << controls[i].right
                      << " pitch=" << controls[i].pitch;
                throw FormatError(error.str());
            }
        }
        if (network) {
            for (auto &peer : peers) {
                std::vector<Controls> local;
                for (auto id : peer->slots()) local.push_back(controls[id]);
                peer->send(local);
            }
            pump(state.tick); host->receive(controls);
        }
        match.step(controls);
        const auto current = match.state();
        if (network) {
            if (faults) {
                history.push_back(current);
                if (history.size() > 32) history.pop_front();
            }
            host->broadcast();
            pump(current.tick);
            if (faults) {
                for (unsigned poll = 0; poll < 3; ++poll) {
                    pump(current.tick);
                    for (auto &peer : peers) receive_faulted(*peer);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                for (auto &peer : peers) {
                    require(peer->state().tick <= current.tick, "UDP peer is ahead of authority");
                    const auto lag = current.tick - peer->state().tick;
                    maximum_peer_lag = std::max(maximum_peer_lag, lag);
                    require(lag <= 16, "UDP peer did not recover within the fault profile lag bound");
                }
            } else for (auto &peer : peers) {
                // UDP delivery is asynchronous even on loopback. Poll the
                // actual socket without advancing or altering the game state.
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
                do {
                    peer->receive();
                    if (peer->state().tick == current.tick) break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } while (std::chrono::steady_clock::now() < deadline);
                test::verify_multiplayer_snapshot(current, peer->state()); ++deliveries;
            }
        }
        if (current.tick % 30 == 0 || current.game_over_complete())
            for (unsigned i = 0; i < participants; ++i) {
                const auto &p = current.players[i];
                trace << current.tick << '\t' << i << '\t' << p.position.x << '\t' << p.position.y << '\t'
                      << p.position.z << '\t' << p.source_unit << '\t' << p.source_weapon << '\t'
                      << p.loaded << '\t' << p.score << '\t' << p.kills << '\t' << p.deaths << '\t'
                      << current.finished << '\n';
            }
    }
    const auto final = match.state();
    require(final.finished && final.results_finalized && final.game_over_complete(), "Final results did not complete");
    test::PacketFaultStatistics fault_statistics;
    if (faults) {
        require(deliveries >= final.tick * peers.size() / 4, "Too few real snapshots survived the fault profile");
        bool synchronized = false;
        // Drain actual delayed final packets without stepping the game or
        // changing its completed results/countdown. Only proxy time advances.
        for (unsigned attempt = 0; attempt < 128 && !synchronized; ++attempt) {
            host->broadcast(); pump(final.tick + attempt);
            synchronized = true;
            for (auto &peer : peers) {
                receive_faulted(*peer);
                synchronized &= peer->state().tick == final.tick;
            }
            if (!synchronized) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(synchronized, "Final results did not recover through the faulted UDP path");
        for (auto &peer : peers) test::verify_multiplayer_snapshot(final, peer->state());
        for (const auto &proxy : proxies) {
            const auto &statistics = proxy->statistics();
            fault_statistics.dropped += statistics.dropped;
            fault_statistics.delayed += statistics.delayed;
            fault_statistics.duplicated += statistics.duplicated;
            fault_statistics.delivered += statistics.delivered;
            fault_statistics.peak_queued_bytes = std::max(fault_statistics.peak_queued_bytes, statistics.peak_queued_bytes);
        }
        require(fault_statistics.dropped && fault_statistics.delayed && fault_statistics.duplicated,
                "Network fault profile did not exercise loss, delay and duplication");
    }
    unsigned kills = 0, respawns = 0, fired_players = 0, moved_players = 0;
    std::ofstream records(output / "final.tsv");
    records << "player\tplayer_datum\tunit\tweapon\tscore\tkills\tdeaths\tfired\tmoved\tsalted_respawn\n";
    for (unsigned i = 0; i < participants; ++i) {
        const auto &p = final.players[i]; const auto &d = drivers[i];
        kills += p.kills; respawns += d.respawned;
        fired_players += d.fired > 0; moved_players += d.moved > 0;
        records << i << '\t' << p.source_player << '\t' << p.source_unit << '\t' << p.source_weapon << '\t'
                << p.score << '\t' << p.kills << '\t' << p.deaths << '\t' << d.fired << '\t' << d.moved
                << '\t' << d.respawned << '\n';
    }
    require(kills && respawns && match.simulation().shots() > 10, "Simulation missed real combat/death/salted respawn");
    require(fired_players == participants && moved_players == participants,
            "A participant missed real weapon consumption or movement");
    std::cout << "PASS complete headless " << mode << " match: players=" << participants
              << " ticks=" << final.tick << " shots=" << match.simulation().shots()
              << " hits=" << match.simulation().hits() << " kills=" << kills << " respawns=" << respawns
              << " fired_players=" << fired_players << " moved_players=" << moved_players
              << " udp_peers=" << peers.size() << " peer_seats=" << peer_seats
              << " verified_udp_deliveries=" << deliveries;
    if (faults) std::cout << " dropped_datagrams=" << fault_statistics.dropped
                         << " delayed_datagrams=" << fault_statistics.delayed
                         << " duplicated_datagrams=" << fault_statistics.duplicated
                         << " forwarded_datagrams=" << fault_statistics.delivered
                         << " maximum_peer_lag=" << maximum_peer_lag
                         << " maximum_proxy_queue_bytes=" << fault_statistics.peak_queued_bytes;
    std::cout << '\n';
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n'; return 1;
}
