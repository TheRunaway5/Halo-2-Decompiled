#define SDL_MAIN_HANDLED
#include "halo2/desktop_session.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/posed_collision.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "helpers/destruction_firing_lane_inputs_tu15.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace halo2;
using J = nlohmann::json;
constexpr unsigned width = 640, height = 480;
constexpr float tau = 6.283185307179586f;
constexpr std::string_view warthog_tag = "objects\\vehicles\\warthog\\warthog";
void check(bool value, const std::string &why) { if (!value) throw FormatError(why); }
J xyz(Vec3 v) { return J::array({v.x, v.y, v.z}); }
bool finite(Vec3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
std::uint32_t word(const SourceRuntimeMemory &memory, std::uint32_t address, unsigned size = 4) {
    std::array<std::byte, 4> bytes{}; memory.read(address, std::span(bytes).first(size));
    std::uint32_t value{};
    for (unsigned n = 0; n < size; ++n) value |= std::uint32_t(std::to_integer<unsigned>(bytes[n])) << (n * 8u);
    return value;
}
bool inside(const std::filesystem::path &path, const std::filesystem::path &root) {
    const auto relative = std::filesystem::weakly_canonical(path).lexically_relative(root);
    return !relative.empty() && !relative.is_absolute() && *relative.begin() != "..";
}
struct Arguments {
    std::filesystem::path map, shared, shaders, report, screenshot, scratch;
    unsigned capture_tick = 1800;
};
Arguments arguments(int argc, char **argv) {
    Arguments a; std::set<std::string> seen;
    for (int n = 1; n < argc; ++n) {
        const std::string key = argv[n];
        check(n + 1 < argc && seen.insert(key).second, "Missing or duplicate SMG regression argument");
        const std::string value = argv[++n];
        if (key == "--map") a.map = value;
        else if (key == "--shared-maps") a.shared = value;
        else if (key == "--shader-directory") a.shaders = value;
        else if (key == "--report-output") a.report = value;
        else if (key == "--screenshot") a.screenshot = value;
        else if (key == "--capture-tick") {
            std::size_t parsed{}; const auto ticks = std::stoul(value, &parsed);
            check(parsed == value.size() && ticks >= 500 && ticks <= 1800, "SMG capture tick must be in 500..1800");
            a.capture_tick = static_cast<unsigned>(ticks);
        } else throw FormatError("Unknown SMG regression argument: " + key);
    }
    check(!a.map.empty() && !a.shared.empty() && !a.shaders.empty() && !a.report.empty() && !a.screenshot.empty(),
          "Supply --map --shared-maps --shader-directory --report-output --screenshot [--capture-tick N]");
    const auto *scratch = std::getenv("HALO2_TEST_SCRATCH");
    check(scratch && std::filesystem::path(scratch).is_absolute(), "Run this regression through tools/run_test.py");
    a.scratch = std::filesystem::canonical(scratch);
    a.map = std::filesystem::canonical(a.map); a.shared = std::filesystem::canonical(a.shared);
    a.shaders = std::filesystem::canonical(a.shaders); a.report = std::filesystem::absolute(a.report);
    a.screenshot = std::filesystem::absolute(a.screenshot);
    check(a.map.parent_path() == a.shared && std::filesystem::is_directory(a.shaders), "MCC map/shared or shader directory differs");
    check(inside(a.screenshot, a.scratch) && !std::filesystem::exists(a.screenshot), "Use a fresh screenshot inside HALO2_TEST_SCRATCH");
    check(a.report != a.screenshot && a.report.extension() == ".json", "SMG report must be a separate JSON file");
    std::ifstream map(a.map, std::ios::binary); std::array<std::uint32_t, 2> header{};
    map.read(reinterpret_cast<char *>(header.data()), sizeof(header));
    check(bool(map) && header[0] == 0x68656164u && header[1] == 13, "SMG regression requires an imported MCC version13 cache");
    return a;
}
void retain_report(const std::filesystem::path &path, const J &report) {
    if (path.empty()) return;
    const auto text = report.dump(2) + "\n"; check(text.size() < 524288, "SMG regression report exceeds its bound");
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc); out << text;
    check(bool(out), "Cannot retain SMG regression report");
}
void environment(const Arguments &a) {
    SDL_SetMainReady();
    check(SDL_setenv("SDL_AUDIODRIVER", "dummy", 1) == 0, "Could not select real dummy SDL audio device");
    check(SDL_setenv("H2_NET_MODE", "original", 1) == 0, "Could not select original netcode");
    for (const auto &[key, folder] : std::array<std::pair<const char *, const char *>, 6>{{
             {"XDG_CONFIG_HOME", "config"}, {"XDG_DATA_HOME", "data"}, {"XDG_CACHE_HOME", "cache"},
             {"HALO2_PIPELINE_CACHE_DIRECTORY", "pipelines"}, {"MESA_SHADER_CACHE_DIR", "mesa"},
             {"__GL_SHADER_DISK_CACHE_PATH", "nvidia"}}})
        check(SDL_setenv(key, (a.scratch / folder).string().c_str(), 1) == 0, "Could not isolate native SMG runtime storage");
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT, "0x0000/0x0000");
}
void push(SDL_Event event) { check(SDL_PushEvent(&event) == 1, "SDL rejected ordinary SMG regression input"); }
void key(Uint32 window, SDL_Scancode code, bool down) {
    SDL_Event event{}; event.type = down ? SDL_KEYDOWN : SDL_KEYUP; event.key.windowID = window;
    event.key.keysym.scancode = code; event.key.keysym.sym = SDL_GetKeyFromScancode(code);
    event.key.state = down ? SDL_PRESSED : SDL_RELEASED; push(event);
}
void mouse(Uint32 window, bool down) {
    SDL_Event event{}; event.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    event.button.windowID = window; event.button.button = SDL_BUTTON_LEFT;
    event.button.state = down ? SDL_PRESSED : SDL_RELEASED; push(event);
}

struct Pool {
    const SourceRuntimeMemory *memory{};
    std::uint32_t rows{}, bits{}, high{}, capacity{}, stride{};
    bool valid{};
    static Pool read(const SourceRuntimeMemory &m, std::uint32_t global, std::uint32_t size) {
        Pool p; p.memory = &m; const auto header = word(m, global); if (!header) return p;
        p.capacity = word(m, header + 0x20u); p.stride = word(m, header + 0x24u);
        p.high = word(m, header + 0x38u); p.rows = word(m, header + 0x44u); p.bits = word(m, header + 0x48u);
        p.valid = p.capacity > 0 && p.capacity <= 65536u && p.stride == size && p.high <= p.capacity && p.rows && p.bits &&
            std::uint64_t(p.rows) + std::uint64_t(p.capacity) * p.stride <= 0x100000000ull;
        return p;
    }
    std::uint32_t identity(unsigned index) const {
        if (!valid || index >= high || !(word(*memory, bits + (index / 32u) * 4u) & (1u << (index % 32u)))) return null_datum;
        const auto salt = word(*memory, rows + index * stride, 2);
        return salt ? (salt << 16u) | index : null_datum;
    }
    std::uint32_t lookup(std::uint32_t datum) const {
        const auto index = datum & 65535u;
        return datum != null_datum && identity(index) == datum ? rows + index * stride : 0;
    }
};

// These callbacks run inside the retained source floating-point scope. They
// copy/validate integer representations only, and never throw across the
// source return observer, call a source owner, or replace a source result.
struct Mode3 {
    struct Pending {
        std::uint32_t sp{}, effect{}, loop{}, definition{}, loop_definition{}, owner{}, owner_definition{}, tick{}, translation{}, forward{}, control{};
        bool exact_caller{}, valid{}, post_fire{}, new_generation{}, target_owned{};
    };
    struct Sample { Pending call; std::array<std::uint32_t, 7> outputs{}; std::uint32_t result{}, precondition_first_seen_source_tick{}; bool precondition_generation_seen{}; };
    const SourceRuntimeMemory *memory{};
    std::optional<SourcePhysicsEntryObserverBinding> binding;
    std::array<Pending, 4> pending{}; unsigned depth{};
    std::vector<std::uint32_t> baseline_effects, baseline_loops;
    bool baseline{}, shot_seen{}; std::uint32_t first_shot_tick{}, first_shot_weapon_tick{};
    std::uint64_t entries{}, returns{}, validated{}, successful{}, after_fire{}, new_generation{}, ticks{}, invalid{}, read_failures{};
    std::uint64_t target_owned_after_fire{}, preexisting_target_owned{}, precondition_target_returns{};
    std::uint32_t target_owner = null_datum, target_definition = null_datum;
    bool precondition_observed{};
    std::array<std::array<std::uint32_t, 3>, 64> precondition_generations{}; unsigned precondition_generation_count{};
    std::uint32_t last_success_tick{}; bool have_success_tick{};
    std::array<Sample, 24> samples{}; unsigned sample_count{};
    std::vector<std::uint32_t> census_identities;
    std::array<std::uint64_t, 5> loop_births_by_mode{};
    std::uint64_t other_loop_births{}, loop_census_samples{}, loop_census_errors{};
    std::uint32_t last_loop_census_tick{}; J pool_history = J::array();
    void bind(const SourceRuntimeMemory &m) {
        check(!memory && !binding, "SMG regression received a second source world"); memory = &m;
        binding.emplace(m, [this](bool entering, std::uint32_t entry, const auto &abi) noexcept {
            if (entry != 0x17e160u) return;
            try { observe(entering, abi); } catch (...) { ++read_failures; }
        });
    }
    void baseline_generations() {
        check(memory && !baseline, "SMG pool baseline has no actual source memory");
        const auto effects = Pool::read(*memory, 0x4f720cu, 400);
        const auto loops = Pool::read(*memory, 0x4f9b5cu, 24);
        check(effects.valid && loops.valid, "Actual effect/loop pool topology differs");
        baseline_effects.assign(effects.capacity, null_datum); baseline_loops.assign(loops.capacity, null_datum);
        for (unsigned n = 0; n < effects.high; ++n) baseline_effects[n] = effects.identity(n);
        for (unsigned n = 0; n < loops.high; ++n) baseline_loops[n] = loops.identity(n);
        baseline = true;
    }
    // Resident full generations at completed ordinary input boundaries. This
    // census observes births between samples; it never allocates a source row.
    void census(std::uint32_t tick) {
        try {
            const auto pool = Pool::read(*memory, 0x4f9b5cu, 24);
            if (!pool.valid) { ++loop_census_errors; return; }
            if (census_identities.empty()) census_identities.assign(pool.capacity, null_datum);
            if (census_identities.size() != pool.capacity) { ++loop_census_errors; return; }
            std::array<unsigned, 5> counts{}; unsigned other{}; J births = J::array();
            for (unsigned index = 0; index < pool.capacity; ++index) {
                const auto identity = pool.identity(index);
                if (identity != null_datum) {
                    const auto row = pool.rows + index * pool.stride, mode = word(*memory, row + 3u, 1);
                    if (mode < counts.size()) ++counts[mode]; else ++other;
                    if (identity != census_identities[index]) {
                        if (mode < loop_births_by_mode.size()) ++loop_births_by_mode[mode]; else ++other_loop_births;
                        if (births.size() < 8) births.push_back({{"loop", identity}, {"mode", mode},
                            {"flags", word(*memory, row + 4u, 2)}, {"definition", word(*memory, row + 0xcu)},
                            {"source", word(*memory, row + 0x10u)}});
                    }
                }
                census_identities[index] = identity;
            }
            ++loop_census_samples;
            if (pool_history.size() < 32 && (pool_history.empty() || tick >= last_loop_census_tick + 60u || !births.empty())) {
                pool_history.push_back({{"tick", tick}, {"high_water", pool.high}, {"mode_counts", counts},
                    {"other_modes", other}, {"new_full_generations", births}}); last_loop_census_tick = tick;
            }
        } catch (...) { ++loop_census_errors; }
    }
    bool precondition_generation(std::uint32_t effect, std::uint32_t loop) const noexcept {
        for (unsigned n = 0; n < precondition_generation_count; ++n)
            if (precondition_generations[n][0] == effect && precondition_generations[n][1] == loop) return true;
        return false;
    }
    std::uint32_t precondition_first_seen(std::uint32_t effect, std::uint32_t loop) const noexcept {
        for (unsigned n = 0; n < precondition_generation_count; ++n)
            if (precondition_generations[n][0] == effect && precondition_generations[n][1] == loop)
                return precondition_generations[n][2];
        return 0;
    }
    void observe(bool entering, const SourcePhysicsWorldRuntimeAbi &a) {
        if (entering) {
            ++entries;
            if (depth >= pending.size()) { ++invalid; return; }
            auto &p = pending[depth++]; p = {}; p.sp = a.gp[4]; p.effect = a.gp[0];
            p.exact_caller = word(*memory, p.sp) == 0x18fa4au;
            if (!p.exact_caller) return;
            p.translation = a.gp[3]; p.forward = word(*memory, p.sp + 4u); p.control = word(*memory, p.sp + 8u);
            const auto clock = word(*memory, 0x51df54u); if (!clock) { ++invalid; return; }
            p.tick = word(*memory, clock + 8u);
            const auto effects = Pool::read(*memory, 0x4f720cu, 400);
            const auto loops = Pool::read(*memory, 0x4f9b5cu, 24);
            const auto effect = effects.lookup(p.effect), loop_row = a.gp[6];
            if (!effect || !loops.valid || loop_row < loops.rows || (loop_row - loops.rows) % 24u ||
                (loop_row - loops.rows) / 24u >= loops.high) { ++invalid; return; }
            p.loop = loops.identity((loop_row - loops.rows) / 24u);
            if (!loops.lookup(p.loop) || word(*memory, loop_row + 3u, 1) != 3u ||
                (word(*memory, loop_row + 4u, 2) & 2u) || word(*memory, loop_row + 0x10u) != p.effect ||
                p.translation != a.gp[7] + 0xcu || p.control != a.gp[7] + 4u) { ++invalid; return; }
            p.definition = word(*memory, effect + 4u); p.owner = word(*memory, effect + 0x48u);
            p.loop_definition = word(*memory, loop_row + 0xcu);
            if (p.definition == null_datum || p.loop_definition == null_datum) { ++invalid; return; }
            if (target_owner != null_datum && p.owner == target_owner) {
                const auto header = Pool::read(*memory, 0x4ecbd0u, 12u).lookup(p.owner);
                const auto body = header ? word(*memory, header + 8u) : 0u;
                if (!body || word(*memory, body) != target_definition) { ++invalid; return; }
                p.target_owned = true; p.owner_definition = target_definition;
            }
            p.post_fire = shot_seen && p.tick >= first_shot_tick;
            if (baseline && (effects.capacity != baseline_effects.size() || loops.capacity != baseline_loops.size())) {
                ++invalid; return;
            }
            p.new_generation = baseline && baseline_effects[p.effect & 65535u] != p.effect &&
                baseline_loops[p.loop & 65535u] != p.loop;
            p.valid = true; ++validated;
        } else {
            ++returns;
            if (!depth) { ++invalid; return; }
            const auto p = pending[--depth]; if (!p.exact_caller || !p.valid) return;
            if (a.gp[4] != p.sp + 12u) { ++invalid; return; }
            if (!(a.gp[0] & 255u)) return;
            Sample sample; sample.call = p; sample.result = a.gp[0];
            for (unsigned n = 0; n < 3; ++n) {
                sample.outputs[n] = word(*memory, p.translation + n * 4u);
                sample.outputs[n + 3] = word(*memory, p.forward + n * 4u);
            }
            sample.outputs[6] = word(*memory, p.control);
            for (const auto value : sample.outputs) if ((value & 0x7f800000u) == 0x7f800000u) { ++invalid; return; }
            const auto effects = Pool::read(*memory, 0x4f720cu, 400);
            const auto loops = Pool::read(*memory, 0x4f9b5cu, 24);
            if (!effects.lookup(p.effect) || !loops.lookup(p.loop)) { ++invalid; return; }
            ++successful;
            if (p.post_fire) ++after_fire;
            if (!shot_seen && p.target_owned && precondition_observed) {
                ++precondition_target_returns;
                if (!precondition_generation(p.effect, p.loop)) {
                    if (precondition_generation_count == precondition_generations.size()) { ++invalid; return; }
                    precondition_generations[precondition_generation_count++] = {p.effect, p.loop, p.tick};
                }
            }
            sample.precondition_first_seen_source_tick = precondition_first_seen(p.effect, p.loop);
            sample.precondition_generation_seen = sample.precondition_first_seen_source_tick &&
                shot_seen && sample.precondition_first_seen_source_tick < first_shot_tick;
            if (p.post_fire && p.target_owned && (p.new_generation || sample.precondition_generation_seen)) {
                const auto header = Pool::read(*memory, 0x4ecbd0u, 12u).lookup(p.owner);
                const auto body = header ? word(*memory, header + 8u) : 0u;
                if (!body || word(*memory, body) != p.owner_definition) { ++invalid; return; }
                ++target_owned_after_fire;
                if (p.new_generation) ++new_generation; else ++preexisting_target_owned;
                if (!have_success_tick || p.tick != last_success_tick) {
                    if (have_success_tick && p.tick < last_success_tick) ++invalid;
                    last_success_tick = p.tick; have_success_tick = true; ++ticks;
                }
                if (sample_count < samples.size()) samples[sample_count++] = sample;
            }
        }
    }
    J report() const {
        J out{{"entries", entries}, {"returns", returns}, {"validated_entries", validated},
              {"successful_returns", successful}, {"post_fire_successful_returns", after_fire},
              {"new_generation_successful_returns", new_generation}, {"post_fire_successful_ticks", ticks},
              {"target_owned_post_fire_successful_returns", target_owned_after_fire},
              {"preexisting_target_owned_successful_returns", preexisting_target_owned},
              {"precondition_target_owned_successful_returns", precondition_target_returns},
              {"precondition_generation_seen", precondition_generation_count != 0u},
              {"precondition_generation_count", precondition_generation_count},
              {"first_shot_tick", first_shot_tick}, {"first_shot_weapon_tick", first_shot_weapon_tick},
              {"invalid_calls", invalid}, {"read_failures", read_failures}, {"unfinished_calls", depth},
              {"baseline_recorded", baseline}, {"effect_pool_capacity", baseline_effects.size()},
              {"loop_pool_capacity", baseline_loops.size()}, {"samples", J::array()},
              {"loop_births_by_mode", loop_births_by_mode}, {"other_loop_births", other_loop_births},
              {"loop_census_samples", loop_census_samples}, {"loop_census_errors", loop_census_errors},
              {"loop_pool_history", pool_history},
              {"loop_census_scope", "Full occupied generations resident at completed input samples; does not infer transient births between samples"}};
        for (unsigned n = 0; n < sample_count; ++n) {
            const auto &s = samples[n];
            out["samples"].push_back({{"tick", s.call.tick}, {"caller_return", 0x18fa4au},
                {"effect", s.call.effect}, {"loop", s.call.loop}, {"effect_definition", s.call.definition},
                {"loop_definition", s.call.loop_definition}, {"effect_object_owner", s.call.owner},
                {"effect_object_definition", s.call.owner_definition}, {"new_generation", s.call.new_generation},
                {"precondition_generation_seen", s.precondition_generation_seen},
                {"precondition_first_seen_source_tick", s.precondition_first_seen_source_tick},
                {"new_generation", s.call.new_generation}, {"eax", s.result}, {"output_bits", s.outputs}});
        }
        return out;
    }
};

struct Probe {
    const Arguments &args; MultiplayerOptions &options; Mode3 mode3;
    MultiplayerPreparedWorld *world{}; SDL_Window *window{}; Uint32 window_id{}; unsigned slot{};
    std::uint32_t weapon_definition = null_datum, projectile_definition = null_datum, firing_sound = null_datum;
    std::set<std::uint32_t> reload_sounds;
    std::map<std::uint32_t, ModelCollision> models;
    std::map<std::uint32_t, int> hull_sections;
    std::map<std::uint32_t, unsigned> hull_regions;
    std::map<std::uint32_t, unsigned> authored_damage_sections;
    std::unique_ptr<test::DestructionFiringLaneInputs> lane;
    std::uint32_t player = null_datum, unit = null_datum, weapon = null_datum, target = null_datum;
    std::uint32_t target_definition = null_datum, target_placement{}, target_netgame_row{};
    unsigned capacity{}, identity_changes{}, fire_commands{}, fire_streak{}, longest_fire_streak{}, reload_commands{};
    unsigned shot_transitions{}, rounds_consumed{}, reload_transfers{}, reload_rounds{}, walk_commands{}, aim_inputs{};
    unsigned vitality_samples{}, target_missing{}, native_detonations{}, owned_detonations{}, legacy_impacts{};
    unsigned valid_packets{}, invalid_packets{}, smg_packets{}, reload_packets{}, ambient_packets{};
    unsigned submitted_frames{}, capture_frame_count{}, last_input_tick{}, last_state_tick{}, last_view_tick{};
    std::uint64_t last_detonation_serial{}, last_impact_serial{}, generated_draws{}, applied_draws{};
    bool identities{}, have_sample{}, fire_held{}, walking{}, requested_fire{}, waiting_reload{};
    bool body_damage{}, have_vitality{}, capture_submitted{};
    float initial_body{}, minimum_body{}, initial_shield{}, minimum_shield{}, pre_fire_body{}, minimum_post_fire_body{};
    int last_loaded{}, last_reserve{}; std::uint32_t last_shot{}, last_reload{};
    std::uint32_t first_fire_tick{}, first_damage_tick{};
    bool precondition_requested{}, precondition_released{}, precondition_detonated{}, precondition_damage{}, precondition_complete{};
    unsigned precondition_commands{}, precondition_projectile_samples{};
    std::uint32_t precondition_request_tick{}, precondition_release_tick{}, precondition_damage_tick{}, precondition_complete_tick{};
    std::uint32_t precondition_projectile=null_datum, precondition_definition=null_datum;
    std::uint64_t precondition_detonation_serial{}, precondition_detonation_tick{};
    float precondition_before_body{}, precondition_after_body{};
    J precondition_projectile_history = J::array(), precondition_detonation = J::object();
    std::optional<Vec3> walking_target; std::uint32_t last_plan_tick{};
    Vec3 initial_player_position{}, initial_target_position{}, latest_target_position{};
    J history = J::array();
    J planning = J::array(), latest_planning = J::object();
    struct Candidate {
        std::uint32_t definition{}, placement{}, first_tick{}, last_tick{}, pose_tick{}, regions_tick{};
        int netgame_row{}; Vec3 initial_position{}, latest_position{}; bool generation_verified{};
    };
    std::map<std::uint32_t, Candidate> candidates;
    unsigned diagnostic_errors{}, camera_absent_returns{}, camera_mode_returns{}, no_surface_returns{}, unaligned_returns{};
    std::array<unsigned, 4> camera_mode_return_counts{};
    std::uint32_t first_camera_block_tick{}, last_camera_block_tick{};
    unsigned current_ray_misses{}, blocked_current_rays{}, non_hull_current_rays{}, replan_attempts{}, no_walking_vantage{}, kept_walking_target{};
    unsigned aim_calls{}, aim_zero_inputs{};
    std::uint32_t last_planning_tick{}, last_aim_tick{}; std::string last_planning_phase;
    Vec3 last_aim_point{}; float last_desired_yaw{}, last_desired_pitch{}; int last_mouse_x{}, last_mouse_y{};
    Vec3 submitted_camera_position{}, submitted_camera_forward{}; bool have_submitted_camera{}, submitted_camera_oriented{};
    J birth_inventory = J::object(), hull_history = J::array(), hull_effect_history = J::array();
    std::uint32_t last_hull_effect_sample_tick{}, last_effect_link = null_datum, last_effect_loop_flags = null_datum;
    struct EffectStore {
        std::uint32_t pc{}, cursor{}, tick{}, address{}, bytes{}, effect{}, definition{}, flags{}, loop{}, owner{},
            primary{}, secondary{}, control18{}, control6c{}, control70{}, header_flags{}, body_flags{}, parent{},
            elapsed{}, duration{}, remainder{};
        std::array<unsigned,16> before{}, written{};
    };
    std::array<EffectStore,128> effect_stores{};
    unsigned effect_store_count{}, effect_store_errors{}, effect_store_overflow{};
    bool effect_store_bound{};
    J hull_record_latest = J::object(), hull_record_history = J::array();
    std::uint32_t last_hull_record_tick{}; unsigned last_hull_response_bits{};
    unsigned maximum_hull_damage{}; int last_hull_damage = -1, last_hull_permutation = -1;
    std::uint32_t last_hull_effect = null_datum;
    std::filesystem::path delivered_input_path;
    std::ofstream delivered_input_stream;
    unsigned delivered_input_rows{}, delivered_roundtrip_rows{}; bool delivered_input_closed{}, delivered_input_io_error{};
    Probe(const Arguments &a, MultiplayerOptions &o) : args(a), options(o), delivered_input_path(a.scratch / "delivered-inputs.tsv") {
        check(!std::filesystem::exists(delivered_input_path), "Delivered input schedule already exists");
        delivered_input_stream.open(delivered_input_path, std::ios::binary | std::ios::trunc);
        check(bool(delivered_input_stream), "Cannot create the owned delivered input schedule");
        delivered_input_stream.imbue(std::locale::classic());
        delivered_input_stream << std::setprecision(std::numeric_limits<float>::max_digits10);
    }
    ~Probe() { close_delivered_inputs(); }
    void observe_effect_store(std::uint32_t cursor,const SourcePhysicsContactRecordStore &store) noexcept {
        // This callback is inside the original FPU scope: fixed storage and
        // integer reads only. It never calls an owner or supplies a result.
        const auto pc=store.instruction;
        if(!((pc>=0x17a0f0u && pc<0x17a193u)||(pc>=0x17a380u && pc<0x17a560u)) || target==null_datum)return;
        try {
            const auto &m=world->source_objects->memory();
            const auto effects=Pool::read(m,0x4f720cu,400), loops=Pool::read(m,0x4f9b5cu,24);
            std::uint32_t effect=0,identity=null_datum;
            if(effects.valid && store.address>=effects.rows && std::uint64_t(store.address)-effects.rows<std::uint64_t(effects.high)*400u) {
                const auto index=(store.address-effects.rows)/400u;identity=effects.identity(index);effect=effects.lookup(identity);
            } else if(loops.valid && store.address>=loops.rows && std::uint64_t(store.address)-loops.rows<std::uint64_t(loops.high)*24u) {
                const auto index=(store.address-loops.rows)/24u;
                if(loops.identity(index)!=null_datum) {identity=word(m,loops.rows+index*24u+0x10u);effect=effects.lookup(identity);}
            }
            if(!effect || word(m,effect+0x48u)!=target)return;
            const auto definition=word(m,effect+4u),table=word(m,0x4f0414u);
            if(definition==null_datum || !table)return;
            const auto root=word(m,table+(definition&65535u)*16u+8u);
            if(!root || word(m,root+0x20u)==null_datum)return;
            if(effect_store_count==effect_stores.size()){++effect_store_overflow;return;}
            if(store.written.size()>16 || store.before.size()!=store.written.size()){++effect_store_errors;return;}
            auto &s=effect_stores[effect_store_count++];s.pc=store.instruction;s.cursor=cursor;s.address=store.address;
            s.bytes=std::uint32_t(store.written.size());s.effect=identity;s.definition=word(m,effect+4u);
            const auto clock=word(m,0x51df54u);if(clock)s.tick=word(m,clock+8u);
            s.flags=word(m,effect+2u,2);s.loop=word(m,effect+8u);s.owner=word(m,effect+0x48u);
            s.primary=word(m,effect+0x10u);s.secondary=word(m,effect+0x14u);s.control18=word(m,effect+0x18u,2);
            s.control6c=word(m,effect+0x6cu);s.control70=word(m,effect+0x70u);
            s.elapsed=word(m,effect+0x60u);s.duration=word(m,effect+0x64u);s.remainder=word(m,effect+0x68u);
            const auto object=Pool::read(m,0x4ecbd0u,12).lookup(s.owner);
            if(object){s.header_flags=word(m,object+2u,1);const auto body=word(m,object+8u);
                if(body){s.body_flags=word(m,body+4u);s.parent=word(m,body+0x14u);}}
            for(unsigned n=0;n<s.bytes;++n){s.before[n]=std::to_integer<unsigned>(store.before[n]);s.written[n]=std::to_integer<unsigned>(store.written[n]);}
        }catch(...){++effect_store_errors;}
    }
    void unbind_effect_store() {
        if(effect_store_bound){world->source_objects->tu15_world().observe_physics_stores_read_only({});effect_store_bound=false;}
    }
    void close_delivered_inputs() noexcept {
        if (delivered_input_closed) return;
        try {
            delivered_input_stream.flush(); delivered_input_io_error |= !bool(delivered_input_stream);
            delivered_input_stream.close(); delivered_input_io_error |= !bool(delivered_input_stream);
        } catch (...) { delivered_input_io_error = true; }
        delivered_input_closed = true;
    }
    void retain_delivered_input(unsigned tick, const Controls &c) {
        check(tick == delivered_input_rows && tick < args.capture_tick && !delivered_input_closed,
              "Delivered input schedule lost its real steps key or finite extent");
        check(!c.skip_cinematic && !c.pickup && !c.dual_wield && !c.switch_weapon && !c.fire_secondary &&
            !c.switch_grenade && !c.cancel && !c.melee && !c.flashlight && !c.start && !c.back &&
            c.zoom == -1 && c.look_stick == 0 && c.pickup_milliseconds == 0 && c.dual_wield_milliseconds == 0,
            "Delivered controls contain an action the native CLI schedule cannot represent");
        // MultiplayerKbm::snapshot also sets accept for the held left mouse
        // button. Preserve that actual bit through this explicit invariant;
        // the legacy CLI projection records its value in the fire column.
        check(c.accept == c.fire, "Actual accept input differs from the recorded left-mouse fire value");
        check(std::isfinite(c.yaw) && std::isfinite(c.pitch) && c.pitch >= -1.5f && c.pitch <= 1.5f &&
            std::isfinite(c.forward) && std::isfinite(c.right) && std::abs(c.forward) <= 1.f && std::abs(c.right) <= 1.f,
            "Delivered controls exceed the native CLI script parser bounds");
        std::ostringstream encoded; encoded.imbue(std::locale::classic());
        encoded << std::setprecision(std::numeric_limits<float>::max_digits10);
        encoded << tick << " 0 " << unsigned(c.fire) << ' ' << unsigned(c.reload) << ' ' << unsigned(c.grenade)
            << ' ' << c.yaw << ' ' << c.pitch << " move " << c.forward << ' ' << c.right;
        if (c.jump) encoded << " jump";
        if (c.crouch) encoded << " crouch";
        encoded << '\n';
        std::istringstream decoded(encoded.str()); decoded.imbue(std::locale::classic());
        unsigned decoded_tick{}, view{}, firing{}, reloading{}, grenade{};
        float yaw{}, pitch{}, forward{}, right{}; std::string move, extra; bool jump{}, crouch{};
        check(bool(decoded >> decoded_tick >> view >> firing >> reloading >> grenade >> yaw >> pitch >> move >> forward >> right) &&
            decoded_tick == tick && view == 0 && firing == unsigned(c.fire) && reloading == unsigned(c.reload) &&
            grenade == unsigned(c.grenade) && move == "move" &&
            std::bit_cast<std::uint32_t>(yaw) == std::bit_cast<std::uint32_t>(c.yaw) &&
            std::bit_cast<std::uint32_t>(pitch) == std::bit_cast<std::uint32_t>(c.pitch) &&
            std::bit_cast<std::uint32_t>(forward) == std::bit_cast<std::uint32_t>(c.forward) &&
            std::bit_cast<std::uint32_t>(right) == std::bit_cast<std::uint32_t>(c.right),
            "Delivered input schedule changed a native control representation on parser roundtrip");
        while (decoded >> extra) {
            if (extra == "jump" && !jump) jump = true;
            else if (extra == "crouch" && !crouch) crouch = true;
            else throw FormatError("Delivered input schedule contains an unrepresented action");
        }
        check(jump == c.jump && crouch == c.crouch, "Delivered input schedule changed jump/crouch on parser roundtrip");
        ++delivered_roundtrip_rows; delivered_input_stream << encoded.str();
        check(bool(delivered_input_stream), "Could not retain actual delivered controls");
        ++delivered_input_rows;
    }
    // All of this history is passive. A planned/query hit is recorded only as
    // input-planning evidence; the actual source vitality gate remains separate.
    void census(const MultiplayerState &state) {
        for (const auto &vehicle : state.world_vehicles) {
            if (vehicle.handle == null_datum || vehicle.placement >= world->content.objects.size()) continue;
            const auto &object = world->content.objects[vehicle.placement];
            if (!models.contains(object.definition) || object.netgame_row < 0) continue;
            if (!candidates.contains(vehicle.handle)) {
                if (candidates.size() >= 8) { ++diagnostic_errors; continue; }
                Candidate row{object.definition, vehicle.placement, state.tick, state.tick, 0, 0,
                    object.netgame_row, vehicle.transform.position, vehicle.transform.position, false};
                try { row.generation_verified = world->source_objects->object_definition(vehicle.handle) == object.definition; }
                catch (...) { ++diagnostic_errors; }
                candidates.emplace(vehicle.handle, row);
            }
            auto &row = candidates.at(vehicle.handle); row.last_tick = state.tick; row.latest_position = vehicle.transform.position;
            const auto pose = std::ranges::find_if(state.source_body_poses, [&](const auto &v) { return v.object == vehicle.handle; });
            if (!row.pose_tick && pose != state.source_body_poses.end() && !pose->nodes.empty()) row.pose_tick = state.tick;
            if (!row.regions_tick && vehicle.regions) row.regions_tick = state.tick;
        }
    }
    struct PlanningSample {
        Probe &probe; const MultiplayerState &state; const Controls &input;
        std::string phase = "begin"; J fields = J::object(); bool important{};
        ~PlanningSample() noexcept {
            try {
                const auto &p = state.players[probe.slot];
                J row{{"tick", state.tick}, {"phase", phase}, {"position", xyz(p.position)},
                    {"velocity", xyz(p.velocity)}, {"yaw", p.yaw}, {"pitch", p.pitch},
                    {"player_angle_forward", xyz(direction(p.yaw, p.pitch))}, {"source_parent", p.source_parent}, {"source_seat", p.source_seat},
                    {"grounded", p.native_grounded}, {"physical_state", p.native_physical_state},
                    {"camera_mode", p.source_player_camera ? J(p.source_player_camera->camera_mode) : J(nullptr)},
                    {"eye", p.source_player_camera ? xyz(p.source_player_camera->position) : J(nullptr)},
                    {"camera_forward", p.source_player_camera ? xyz(p.source_player_camera->forward) : J(nullptr)},
                    {"input", {{"forward", input.forward}, {"right", input.right}, {"yaw", input.yaw}, {"pitch", input.pitch},
                        {"fire", input.fire}, {"reload", input.reload}}},
                    {"target", probe.target}, {"queued_walk", probe.walking}, {"queued_fire", probe.fire_held},
                    {"walking_target", probe.walking_target ? xyz(*probe.walking_target) : J(nullptr)},
                    {"last_aim_tick", probe.last_aim_tick}, {"last_aim_point", xyz(probe.last_aim_point)},
                    {"desired_yaw", probe.last_desired_yaw}, {"desired_pitch", probe.last_desired_pitch},
                    {"queued_mouse", {probe.last_mouse_x, probe.last_mouse_y}}, {"last_plan_tick", probe.last_plan_tick},
                    {"submitted_camera_tick", probe.have_submitted_camera ? J(probe.last_view_tick) : J(nullptr)},
                    {"submitted_camera_oriented", probe.submitted_camera_oriented},
                    {"submitted_camera_eye", probe.have_submitted_camera ? xyz(probe.submitted_camera_position) : J(nullptr)},
                    {"submitted_camera_forward", probe.have_submitted_camera ? xyz(probe.submitted_camera_forward) : J(nullptr)}};
                const auto angle_forward = direction(p.yaw, p.pitch);
                const auto angle_gap = [&](Vec3 forward) { return std::acos(std::clamp(dot(normalized(forward), angle_forward), -1.f, 1.f)); };
                if (p.source_player_camera && length(p.source_player_camera->forward) > 0)
                    row["source_camera_vs_player_angle_gap"] = angle_gap(p.source_player_camera->forward);
                if (probe.have_submitted_camera && length(probe.submitted_camera_forward) > 0)
                    row["submitted_camera_vs_player_angle_gap"] = angle_gap(probe.submitted_camera_forward);
                row.update(fields); probe.latest_planning = row;
                if (probe.planning.size() >= 32 || (!important && !probe.planning.empty() && phase == probe.last_planning_phase &&
                    state.tick < probe.last_planning_tick + 60u)) return;
                const auto vehicle = std::ranges::find_if(state.world_vehicles, [&](const auto &v) { return v.handle == probe.target; });
                const auto pose = std::ranges::find_if(state.source_body_poses, [&](const auto &v) { return v.object == probe.target; });
                if (vehicle != state.world_vehicles.end()) {
                    row["target_position"] = xyz(vehicle->transform.position);
                    row["target_range"] = length(vehicle->transform.position - p.position);
                }
                if (p.source_player_camera && vehicle != state.world_vehicles.end() && vehicle->regions &&
                    pose != state.source_body_poses.end() && !pose->nodes.empty()) {
                    const auto eye = p.source_player_camera->position;
                    const auto ray = direction(p.yaw, p.pitch) * (length(pose->sphere.center - eye) * 2.f);
                    const auto hit = posed_collision(probe.models.at(probe.target_definition), pose->nodes,
                        vehicle->regions->selection.permutations, *probe.world->content.original_data, eye, ray, 2u);
                    row["diagnostic_current_view_hit"] = bool(hit);
                    row["diagnostic_current_view_clear"] = hit && probe.lane->clear(eye, eye + ray * hit->fraction);
                    if (hit) {
                        row["diagnostic_current_view_surface"] = xyz(eye + ray * hit->fraction);
                        row["diagnostic_current_view_material"] = hit->model_material;
                        const auto &materials = probe.models.at(probe.target_definition).materials;
                        if (hit->model_material >= 0 && std::size_t(hit->model_material) < materials.size())
                            row["diagnostic_current_view_damage_section"] = materials[unsigned(hit->model_material)].damage_section;
                    }
                    const auto source_ray = normalized(p.source_player_camera->forward) * (length(pose->sphere.center - eye) * 2.f);
                    const auto source_hit = posed_collision(probe.models.at(probe.target_definition), pose->nodes,
                        vehicle->regions->selection.permutations, *probe.world->content.original_data, eye, source_ray, 2u);
                    row["diagnostic_source_camera_hit"] = bool(source_hit);
                    row["diagnostic_source_camera_clear"] = source_hit && probe.lane->clear(eye, eye + source_ray * source_hit->fraction);
                }
                probe.planning.push_back(std::move(row)); probe.last_planning_tick = state.tick; probe.last_planning_phase = phase;
            } catch (...) { ++probe.diagnostic_errors; }
        }
    };
    void focus() {
        if (SDL_GetKeyboardFocus() != window) {
            SDL_RaiseWindow(window); check(SDL_SetWindowInputFocus(window) == 0, "Could not focus the actual SMG test window");
            const auto deadline = SDL_GetTicks64() + 1000;
            do { SDL_PumpEvents(); if (SDL_GetKeyboardFocus() == window) break; SDL_Delay(1); }
            while (SDL_GetTicks64() < deadline);
        }
        check(SDL_GetKeyboardFocus() == window, "SMG regression lost OS keyboard focus");
        check(SDL_SetRelativeMouseMode(SDL_TRUE) == 0, "Could not admit ordinary relative mouse input");
    }
    void fire(bool down) { if (fire_held != down) { mouse(window_id, down); fire_held = down; } }
    void walk(bool down) { if (walking != down) { key(window_id, SDL_SCANCODE_W, down); walking = down; } }
    void reload() { key(window_id, SDL_SCANCODE_R, true); key(window_id, SDL_SCANCODE_R, false); }
    void grenade() {
        const auto binding = options.desktop->keyboard_settings.bindings[unsigned(PcAction::Grenade)];
        check(binding > SDL_SCANCODE_UNKNOWN && binding < SDL_NUM_SCANCODES,
              "Ordinary grenade precondition requires its actual keyboard binding");
        key(window_id, SDL_Scancode(binding), true); key(window_id, SDL_Scancode(binding), false);
    }
    void setup(MultiplayerPreparedWorld &w) {
        world = &w; check(w.cache->is_mcc() && w.source_objects && w.source_objects->owns_physics(), "SMG test lacks the real MCC TU1.5 object world");
        check(w.local_slots.size() == 1, "SMG test needs one genuinely allocated local player"); slot = w.local_slots.front();
        check(mode3.memory == &w.source_objects->memory(), "SMG observer is bound to a different source bank");
        weapon_definition = w.cache->slayer_starting_weapon(5);
        const auto &definition = w.content.weapons.at(weapon_definition);
        check(definition.smg && definition.magazine && definition.magazine->capacity > 0 && definition.sound != null_datum,
              "Starting selector5 lacks an authored SMG magazine and firing sound");
        projectile_definition = definition.projectile; firing_sound = definition.sound;
        capacity = unsigned(definition.magazine->capacity);
        check(projectile_definition != null_datum, "Authored SMG projectile definition is absent");
        const auto initial = w.match.state(); const auto &born = initial.players.at(slot);
        birth_inventory = {{"tick", initial.tick}, {"starting_weapon_selector", options.starting_weapon},
            {"connected", born.connected}, {"dead", born.dead}, {"source_player", born.source_player}, {"source_unit", born.source_unit},
            {"source_weapon", born.source_weapon}, {"weapon_definition", born.weapon}, {"loaded", born.loaded}, {"reserve", born.reserve},
            {"position", xyz(born.position)}, {"scope", "Actual ordinary source-engine birth, before any regression input"}};
        check(options.starting_weapon == 0 && born.connected && !born.dead && born.source_player != null_datum &&
            born.source_unit != null_datum && born.source_weapon != null_datum && born.weapon == weapon_definition,
            "Ordinary Coagulation default source birth did not produce its authored SMG; no pickup or inventory is fabricated");
        player = born.source_player; unit = born.source_unit; weapon = born.source_weapon; identities = true;
        initial_player_position = born.position;
        const auto fp = w.cache->load_first_person_binding(weapon_definition, 2);
        for (const auto &clip : fp.clock_clips) if (clip.activity == 0x0b000065u || clip.activity == 0x0c000064u)
            for (const auto &event : clip.events) if (event.type == 1 && event.tag != null_datum) reload_sounds.insert(event.tag);
        check(!reload_sounds.empty(), "Authored SMG reload audio events are absent");
        for (const auto &object : w.content.objects) {
            if (object.type != 1 || object.netgame_row < 0 || object.model_attachment) continue;
            const auto tag = w.cache->query_tag({.handle = TagHandle{object.definition}});
            if (!tag || tag->name != warthog_tag || models.contains(object.definition)) continue;
            const auto metadata = w.cache->load_source_tag_metadata(object.definition);
            const Bytes data(metadata.bytes_at(metadata.source_address(), metadata.size()));
            const auto model = w.cache->load_source_tag_metadata(data.u32(0x38u));
            check(model.group() == "hlmt", "Authored Warthog lost its model owner");
            models.emplace(object.definition, w.cache->load_model_collision(model.datum()));
            const Bytes head(model.bytes_at(model.source_address(), model.size()));
            check(head.u32(0x60u) == 1, "Warthog hull input plan requires its actual single damage-info block");
            const Bytes info(w.cache->read_tag_bytes(head.u32(0x64u), 0xf8u));
            check(info.u32(0xbcu) <= 64, "Warthog damage-section input bank exceeds bound");
            int hull = -1;
            for (unsigned index = 0; index < info.u32(0xbcu); ++index) {
                const Bytes section(w.cache->read_tag_bytes(info.u32(0xc0u) + index * 0x38u, 0x38u));
                if (w.cache->string_id(section.u32(0)) == "hull") { check(hull == -1, "Warthog has ambiguous hull damage sections"); hull = int(index); }
            }
            check(hull >= 0 && std::ranges::any_of(models.at(object.definition).materials,
                [&](const auto &material) { return material.damage_section == hull; }), "Warthog hull has no actual collision material");
            hull_sections.emplace(object.definition, hull);
            authored_damage_sections.emplace(object.definition, info.u32(0xbcu));
            const auto initial_model = w.cache->load_initial_model_definition(object.definition);
            const auto region = std::ranges::find_if(initial_model.regions,
                [&](const auto &value) { return w.cache->string_id(value.name) == "hull"; });
            check(region != initial_model.regions.end(), "Warthog model has no authored hull region");
            hull_regions.emplace(object.definition, unsigned(region - initial_model.regions.begin()));
        }
        check(!models.empty(), "Map has no authored Warthog collision model");
        lane = std::make_unique<test::DestructionFiringLaneInputs>(*w.cache, w.content, options.bsp);
        window = options.desktop->window(width, height, false); window_id = SDL_GetWindowID(window);
        for (int n = 0; n < SDL_NumJoysticks(); ++n) check(!SDL_IsGameController(n), "A physical controller contaminated the isolated input test");
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT); focus();
        w.source_objects->tu15_world().observe_physics_stores_read_only(
            [this](auto cursor,const auto &store) noexcept {observe_effect_store(cursor,store);});
        effect_store_bound=true;
    }
    void choose_target(const MultiplayerState &state, const MultiplayerState::Player &p) {
        if (target != null_datum) return;
        const WorldVehiclePresentation *nearest{}; float distance{};
        for (const auto &vehicle : state.world_vehicles) {
            if (vehicle.handle == null_datum || vehicle.placement >= world->content.objects.size()) continue;
            const auto &object = world->content.objects[vehicle.placement];
            if (!models.contains(object.definition) || object.netgame_row < 0) continue;
            const auto pose = std::ranges::find_if(state.source_body_poses, [&](const auto &v) { return v.object == vehicle.handle; });
            if (pose == state.source_body_poses.end() || pose->nodes.empty() || !vehicle.regions) continue;
            const auto range = length(vehicle.transform.position - p.position);
            if (!nearest || range < distance) { nearest = &vehicle; distance = range; }
        }
        if (!nearest) return;
        target = nearest->handle; target_placement = nearest->placement;
        const auto &object = world->content.objects[target_placement]; target_definition = object.definition;
        target_netgame_row = unsigned(object.netgame_row); initial_target_position = nearest->transform.position;
        check(world->source_objects->object_definition(target) == target_definition, "Live Warthog generation differs from its authored definition");
    }
    std::optional<Vec3> visible(const WorldVehiclePresentation &vehicle, const SourceWorldPose &pose, Vec3 eye,
                              PosedCollisionHit *selected_hit = nullptr) const {
        const auto definition = world->content.objects[vehicle.placement].definition;
        const auto &model = models.at(definition);
        const auto admit = [&](Vec3 point) -> std::optional<Vec3> {
            const auto ray = (point - eye) * 2.f;
            const auto hit = posed_collision(model, pose.nodes, vehicle.regions->selection.permutations,
                *world->content.original_data, eye, ray, 2u);
            if (hit && hit->model_material >= 0 && std::size_t(hit->model_material) < model.materials.size() &&
                model.materials[unsigned(hit->model_material)].damage_section == hull_sections.at(definition)) {
                const auto surface = eye + ray * hit->fraction;
                if (lane->clear(eye, surface)) { if (selected_hit) *selected_hit = *hit; return surface; }
            }
            return {};
        };
        if (auto hit = admit(pose.sphere.center)) return hit;
        for (float z : {-.5f, 0.f, .5f}) for (float x : {0.f, -.5f, .5f}) for (float y : {0.f, -.5f, .5f}) {
            const auto &basis = pose.nodes.front().basis;
            if (auto hit = admit(pose.sphere.center + (basis[0] * x + basis[1] * y + basis[2] * z) * pose.sphere.radius)) return hit;
        }
        return {};
    }
    void aim(const MultiplayerState::Player &p, Vec3 eye, Vec3 point) {
        const auto delta = point - eye; if (!(length(delta) > .01f)) return;
        ++aim_calls; last_aim_tick = last_state_tick; last_aim_point = point;
        last_desired_yaw = std::atan2(delta.y, delta.x); last_desired_pitch = std::atan2(delta.z, std::hypot(delta.x, delta.y));
        const auto &settings = options.desktop->keyboard_settings;
        const auto gain = .0025f * float(settings.sensitivity) / 3.f; check(gain > 0, "SMG mouse sensitivity is unavailable");
        SDL_Event event{}; event.type = SDL_MOUSEMOTION; event.motion.windowID = window_id;
        event.motion.xrel = -int(std::lround(std::remainder(last_desired_yaw - p.yaw, tau) / gain));
        event.motion.yrel = -int(std::lround((last_desired_pitch - p.pitch) / gain)) * (settings.invert ? -1 : 1);
        last_mouse_x = event.motion.xrel; last_mouse_y = event.motion.yrel;
        if (event.motion.xrel || event.motion.yrel) { push(event); ++aim_inputs; }
        else ++aim_zero_inputs;
    }
    void observe_hull_record(std::uint32_t tick) {
        auto &source = *world->source_objects; const auto &memory = source.memory();
        const auto body = source.source_object_body(target);
        const SourceRawDatumPoolServices readback{.read = [&](auto address, auto size) { return memory.view(address, size); }};
        const auto header = source_raw_datum_lookup(source.source_object_pool(), target, readback);
        check(header && word(memory, header + 8u) == body && word(memory, body) == target_definition,
              "Native hull damage record lost its full Warthog object generation");
        const auto extent = word(memory, header + 6u, 2);
        check(extent >= 0x124u, "Native Warthog body does not contain its section descriptors");
        const auto size = std::bit_cast<std::int16_t>(std::uint16_t(word(memory, body + 0x120u, 2)));
        const auto offset = std::bit_cast<std::int16_t>(std::uint16_t(word(memory, body + 0x122u, 2)));
        const auto section = hull_sections.at(target_definition);
        check(size >= 0 && size % 8 == 0 && offset >= 0 && section >= 0 && section < size / 8 &&
            unsigned(section) < authored_damage_sections.at(target_definition) &&
            std::uint64_t(unsigned(offset)) + unsigned(size) <= extent &&
            std::uint64_t(body) + unsigned(offset) + unsigned(size) <= 0x100000000ull,
            "Native hull damage section escapes its actual body/descriptor bank");
        memory.view(body + unsigned(offset), unsigned(size));
        const auto address = body + unsigned(offset) + unsigned(section) * 8u;
        std::array<std::byte, 8> raw{}; memory.read(address, raw); std::array<unsigned, 8> values{};
        for (unsigned n = 0; n < raw.size(); ++n) values[n] = std::to_integer<unsigned>(raw[n]);
        const auto bits = word(memory, address, 2), pending = word(memory, address + 4u, 2);
        hull_record_latest = {{"tick", tick}, {"object", target}, {"section", section}, {"raw_bytes", values},
            {"body_extent", extent}, {"bank_bytes", size}, {"bank_relative_offset", offset},
            {"authored_section_count", authored_damage_sections.at(target_definition)},
            {"completed_response_bits", bits}, {"accumulator_byte", values[2]}, {"unknown_byte3", values[3]},
            {"pending_word", pending}, {"pending_response_index", pending & 7u}, {"pending_countdown", pending >> 3u},
            {"hold_timer", std::bit_cast<std::int16_t>(std::uint16_t(word(memory, address + 6u, 2)))},
            {"scope", "Exact packed8-byte DCFF0/DC660 section record; no section-health float is inferred"}};
        if (hull_record_history.size() < 32 && (hull_record_history.empty() || tick >= last_hull_record_tick + 60u || bits != last_hull_response_bits)) {
            hull_record_history.push_back(hull_record_latest); last_hull_record_tick = tick;
        }
        last_hull_response_bits = bits;
    }
    void observe_hull_effect(std::uint32_t tick, std::uint32_t identity) {
        if (identity==null_datum || hull_effect_history.size()>=64) return;
        try {
            const auto &m=world->source_objects->memory();
            const auto effects=Pool::read(m,0x4f720cu,400), loops=Pool::read(m,0x4f9b5cu,24);
            const auto effect=effects.lookup(identity);
            J row{{"tick",tick},{"effect",identity},{"effect_live",effect!=0}};
            std::uint32_t link=null_datum, flags=null_datum;
            if (effect) {
                link=word(m,effect+8u);const auto definition=word(m,effect+4u);
                row.update({{"effect_flags",word(m,effect+2u,2)},{"definition",definition},{"effect_loop_handle",link},
                    {"effect_location",std::bit_cast<std::int16_t>(std::uint16_t(word(m,effect+0x20u,2)))},
                    {"owner",word(m,effect+0x48u)},{"encoded_first_person",word(m,effect+0x58u)},
                    {"current_event",std::bit_cast<std::int16_t>(std::uint16_t(word(m,effect+0x5cu,2)))},
                    {"elapsed_bits",word(m,effect+0x60u)},{"duration_bits",word(m,effect+0x64u)},
                    {"remainder_bits",word(m,effect+0x68u)},{"control_bits",word(m,effect+0x6cu)},
                    {"primary_control",word(m,effect+0x10u)},{"secondary_control",word(m,effect+0x14u)},
                    {"particle_head_fc",word(m,effect+0xfcu)},{"particle_head_100",word(m,effect+0x100u)}});
                const auto table=word(m,0x4f0414u);
                if (table && definition!=null_datum) {
                    const auto tag=word(m,table+(definition&65535u)*16u+8u);
                    row["tag_root"]=tag;
                    if (tag) {
                        const auto count=word(m,tag+0xcu);
                        const auto location=std::bit_cast<std::int16_t>(std::uint16_t(word(m,tag+0x24u,2)));
                        row.update({{"root_flags",word(m,tag)},{"root_location_count",count},
                            {"root_event_count",word(m,tag+0x14u)},{"root_event_pointer",word(m,tag+0x18u)},
                            {"root_lsnd",word(m,tag+0x20u)},{"root_loop_location",location}});
                        if (location>=0 && unsigned(location)<std::min(count,64u)) {
                            auto at=word(m,effect+0x7cu+unsigned(location)*4u);std::set<std::uint32_t> visited;
                            const auto locations=Pool::read(m,0x4f7208u,60);row["location_chain"]=J::array();
                            while (at!=null_datum && visited.size()<8 && visited.insert(at).second) {
                                const auto body=locations.lookup(at);if (!body) {row["location_invalid"]=at;break;}
                                std::array<std::uint32_t,13> transform{};
                                for(unsigned n=0;n<13;++n)transform[n]=word(m,body+8u+n*4u);
                                row["location_chain"].push_back({{"location",at},{"node_bits",word(m,body+2u,2)},
                                    {"next",word(m,body+4u)},{"transform_bits",transform}});at=word(m,body+4u);
                            }
                        }
                    }
                }
            }
            row["loops_for_effect"]=J::array();
            if (loops.valid)for(unsigned index=0;index<loops.high;++index) {
                const auto handle=loops.identity(index);if(handle==null_datum)continue;
                const auto body=loops.rows+index*24u;if(word(m,body+0x10u)!=identity)continue;
                flags=word(m,body+4u,2);std::array<std::uint32_t,6> words{};
                for(unsigned n=0;n<6;++n)words[n]=word(m,body+n*4u);
                if(row["loops_for_effect"].size()<8)row["loops_for_effect"].push_back({{"loop",handle},{"row_words",words},
                    {"state",word(m,body+2u,1)},{"mode",word(m,body+3u,1)},{"flags",flags}});
            }
            const auto sound=word(m,0x4f2c50u);if(sound)row["sound_device_bytes"]={word(m,sound+0x78u,1),word(m,sound+0x79u,1),word(m,sound+0x7au,1)};
            if(hull_effect_history.empty() || tick>=last_hull_effect_sample_tick+30u || link!=last_effect_link || flags!=last_effect_loop_flags) {
                hull_effect_history.push_back(std::move(row));last_hull_effect_sample_tick=tick;
            }
            last_effect_link=link;last_effect_loop_flags=flags;
        } catch(const std::exception &error) {++diagnostic_errors;if(hull_effect_history.size()<64)hull_effect_history.push_back({{"tick",tick},{"effect",identity},{"read_error",error.what()}});}
    }
    void input(unsigned tick, std::span<const Controls> commands, const MultiplayerState &state) {
        check(world && commands.size() == 1 && slot < state.players.size(), "Actual local input/roster disappeared");
        const auto &p = state.players[slot]; const auto &c = commands[0];
        retain_delivered_input(tick, c);
        last_input_tick = tick; last_state_tick = state.tick; focus();
        PlanningSample diagnostic{*this, state, c}; census(state); mode3.census(state.tick);
        fire_commands += c.fire; reload_commands += c.reload; walk_commands += c.forward != 0;
        precondition_commands += c.grenade;
        fire_streak = c.fire ? fire_streak + 1 : 0; longest_fire_streak = std::max(longest_fire_streak, fire_streak);
        check(!c.melee && !c.pickup && !c.switch_weapon && precondition_commands <= 1 &&
            (!c.grenade || (precondition_requested && !requested_fire)) && (!c.fire || precondition_complete),
            "Unexpected action contaminated the ordinary grenade-precondition then SMG input");
        if (!p.connected || p.dead || p.source_player == null_datum || p.source_unit == null_datum || p.source_weapon == null_datum) {
            diagnostic.phase = "invalid_player";
            fire(false); walk(false); if (identities) ++identity_changes; return;
        }
        if (!identities) {
            player = p.source_player; unit = p.source_unit; weapon = p.source_weapon; identities = true; initial_player_position = p.position;
        } else if (player != p.source_player || unit != p.source_unit || weapon != p.source_weapon) ++identity_changes;
        check(p.weapon == weapon_definition, "Ordinary player lost its authored default SMG");
        if (have_sample && p.source_weapon == weapon) {
            if (std::bit_cast<std::int32_t>(p.shot_tick - last_shot) > 0 && p.loaded < last_loaded) {
                ++shot_transitions;
                rounds_consumed += unsigned(last_loaded - p.loaded);
                if (!mode3.shot_seen) {
                    mode3.shot_seen = true;
                    const auto clock = word(*mode3.memory, 0x51df54u);
                    check(clock != 0, "Actual native SMG shot lacks its source clock");
                    mode3.first_shot_tick = word(*mode3.memory, clock + 8u);
                    mode3.first_shot_weapon_tick = p.shot_tick;
                }
            }
            if (p.loaded > last_loaded && p.reserve < last_reserve && reload_commands) {
                ++reload_transfers; reload_rounds += unsigned(p.loaded - last_loaded); waiting_reload = false;
            }
        }
        if (history.size() < 48 && (!have_sample || p.loaded != last_loaded || p.reload_tick != last_reload) &&
            (!have_sample || history.empty() || state.tick >= history.back().at("tick").get<unsigned>() + 5u))
            history.push_back({{"tick", state.tick}, {"loaded", p.loaded}, {"reserve", p.reserve}, {"shot_tick", p.shot_tick},
                {"reload_tick", p.reload_tick}, {"fire", c.fire}, {"reload", c.reload}});
        last_loaded = p.loaded; last_reserve = p.reserve; last_shot = p.shot_tick; last_reload = p.reload_tick; have_sample = true;
        if (precondition_requested) for (const auto &projectile : state.grenade_projectiles) {
            if (projectile.type != 0 || projectile.player != player || projectile.unit != unit ||
                projectile.definition == projectile_definition) continue;
            check(projectile.datum != null_datum && projectile.datum >> 16u && projectile.definition != null_datum &&
                projectile.definition >> 16u && world->source_objects->object_definition(projectile.datum) == projectile.definition,
                "Actual frag precondition lost its full salted source projectile/definition");
            if (precondition_projectile == null_datum) {
                precondition_projectile = projectile.datum; precondition_definition = projectile.definition;
            }
            check(projectile.datum == precondition_projectile && projectile.definition == precondition_definition,
                  "Frag precondition admitted more than its one actual projectile");
            ++precondition_projectile_samples;
            if (projectile.parent == null_datum && !precondition_released) {
                precondition_released = true; precondition_release_tick = state.tick;
            }
            if (precondition_projectile_history.size() < 8 || (projectile.parent == null_datum &&
                (precondition_projectile_history.back().at("parent") != null_datum || state.tick % 8u == 0u))) {
                if (precondition_projectile_history.size() == 16) precondition_projectile_history.erase(precondition_projectile_history.begin());
                precondition_projectile_history.push_back({{"tick", state.tick}, {"projectile", projectile.datum},
                    {"definition", projectile.definition}, {"type", projectile.type}, {"parent", projectile.parent},
                    {"player", projectile.player}, {"unit", projectile.unit}, {"position", xyz(projectile.position)},
                    {"velocity", xyz(projectile.velocity)}});
            }
        }
        for (const auto &event : state.grenade_detonations) if (event.serial > last_detonation_serial) {
            last_detonation_serial = std::max(last_detonation_serial, event.serial);
            if (precondition_requested && event.projectile == precondition_projectile &&
                event.definition == precondition_definition && event.player == player && event.unit == unit) {
                precondition_detonated = true; precondition_detonation_serial = event.serial; precondition_detonation_tick = event.tick;
                precondition_detonation = {{"serial", event.serial}, {"tick", event.tick}, {"projectile", event.projectile},
                    {"definition", event.definition}, {"player", event.player}, {"unit", event.unit},
                    {"position", xyz(event.position)}, {"source_effect_instance", event.source_effect_instance}};
            }
            if (event.definition == projectile_definition) {
                ++native_detonations; if (event.player == player && event.unit == unit) ++owned_detonations;
            }
        }
        for (const auto &impact : state.bullet_impacts) if (impact.serial > last_impact_serial) {
            last_impact_serial = std::max(last_impact_serial, impact.serial);
            if (impact.weapon == weapon_definition) ++legacy_impacts;
        }
        choose_target(state, p); if (target == null_datum) { diagnostic.phase = "no_target"; fire(false); walk(false); return; }
        const auto vehicle = std::ranges::find_if(state.world_vehicles, [&](const auto &v) { return v.handle == target; });
        const auto pose = std::ranges::find_if(state.source_body_poses, [&](const auto &v) { return v.object == target; });
        if (vehicle == state.world_vehicles.end() || pose == state.source_body_poses.end() || pose->nodes.empty() || !vehicle->regions) {
            diagnostic.phase = "missing_target";
            ++target_missing; fire(false); walk(false); return;
        }
        latest_target_position = vehicle->transform.position;
        mode3.target_owner = target; mode3.target_definition = target_definition;
        observe_hull_record(state.tick);
        const auto hull_region = hull_regions.at(target_definition);
        check(hull_region < vehicle->regions->selection.states.size() && hull_region < vehicle->regions->selection.permutations.size() &&
            hull_region < vehicle->regions->looping_effects.size(), "Actual Warthog hull region publication is incomplete");
        const auto hull_damage = vehicle->regions->selection.states[hull_region].damage;
        const auto hull_permutation = vehicle->regions->selection.permutations[hull_region];
        const auto hull_effect = vehicle->regions->looping_effects[hull_region];
        maximum_hull_damage = std::max(maximum_hull_damage, unsigned(hull_damage));
        if (hull_history.size() < 16 && (hull_damage != last_hull_damage || hull_permutation != last_hull_permutation || hull_effect != last_hull_effect))
            hull_history.push_back({{"tick", state.tick}, {"region", hull_region}, {"damage_state", hull_damage},
                {"permutation", hull_permutation}, {"looping_effect_instance", hull_effect}});
        last_hull_damage = hull_damage; last_hull_permutation = hull_permutation; last_hull_effect = hull_effect;
        observe_hull_effect(state.tick,hull_effect);
        diagnostic.fields.update({{"hull_region", hull_region}, {"hull_damage_state", hull_damage}, {"hull_permutation", hull_permutation},
            {"hull_looping_effect_instance", hull_effect}, {"preferred_damage_section", hull_sections.at(target_definition)}});
        const auto vitality = world->source_objects->object_vitality(target);
        check(std::isfinite(vitality.body) && std::isfinite(vitality.shield), "Actual Warthog vitality is nonfinite");
        if (!have_vitality) {
            initial_body = minimum_body = vitality.body; initial_shield = minimum_shield = vitality.shield; have_vitality = true;
        }
        ++vitality_samples; minimum_body = std::min(minimum_body, vitality.body); minimum_shield = std::min(minimum_shield, vitality.shield);
        if (precondition_detonated) {
            check(requested_fire || vitality.body > 0.f, "Ordinary frag precondition destroyed the actual Warthog; no SMG result is substituted");
            if (vitality.body < precondition_before_body && !precondition_damage) {
                precondition_damage = true; precondition_damage_tick = state.tick; precondition_after_body = vitality.body;
            }
            mode3.precondition_observed = precondition_commands == 1 && precondition_released && precondition_damage;
            check(precondition_damage || state.tick < precondition_detonation_tick + 90u,
                  "Actual frag detonated without damaging the Warthog; ordinary precondition was not reached");
            if (mode3.precondition_observed && state.tick >= precondition_detonation_tick + 30u && !precondition_complete) {
                precondition_complete = true; precondition_complete_tick = state.tick;
            }
        }
        if (requested_fire) minimum_post_fire_body = std::min(minimum_post_fire_body, vitality.body);
        if (mode3.shot_seen && vitality.body < pre_fire_body) { if (!body_damage) first_damage_tick = state.tick; body_damage = true; }
        if (!p.source_player_camera || p.source_player_camera->camera_mode != 0) {
            if (!p.source_player_camera) { ++camera_absent_returns; diagnostic.phase = "no_ordinary_camera"; }
            else {
                ++camera_mode_returns; diagnostic.phase = "nonzero_camera_mode";
                if (p.source_player_camera->camera_mode < camera_mode_return_counts.size())
                    ++camera_mode_return_counts[p.source_player_camera->camera_mode];
            }
            if (!first_camera_block_tick) first_camera_block_tick = state.tick;
            last_camera_block_tick = state.tick;
            fire(false); walk(false); return;
        }
        const auto eye = p.source_player_camera->position; check(finite(eye), "Actual ordinary player camera is nonfinite");
        PosedCollisionHit planned_hit{}; auto surface = visible(*vehicle, *pose, eye, &planned_hit);
        const auto range = length(vehicle->transform.position - p.position);
        diagnostic.fields.update({{"surface_found", bool(surface)}, {"target_range", range},
            {"target_position", xyz(vehicle->transform.position)}, {"target_sphere", xyz(pose->sphere.center)},
            {"target_radius", pose->sphere.radius}, {"planned_surface", surface ? xyz(*surface) : J(nullptr)}});
        if (surface) {
            diagnostic.fields["planned_model_material"] = planned_hit.model_material;
            diagnostic.fields["planned_damage_section"] = models.at(target_definition).materials.at(unsigned(planned_hit.model_material)).damage_section;
            const auto desired_forward = normalized(*surface - eye);
            diagnostic.fields["target_direction"] = xyz(desired_forward);
            diagnostic.fields["target_vs_player_angle_gap"] = std::acos(std::clamp(dot(desired_forward, direction(p.yaw, p.pitch)), -1.f, 1.f));
            diagnostic.fields["target_vs_source_camera_angle_gap"] = std::acos(std::clamp(dot(desired_forward, normalized(p.source_player_camera->forward)), -1.f, 1.f));
        }
        const float desired_range = precondition_complete?3.f:10.f; // Ordinary input choice only.
        if ((!surface || range > desired_range) && (!walking_target || state.tick >= last_plan_tick + 90u)) {
            last_plan_tick = state.tick; ++replan_attempts; diagnostic.important = true;
            diagnostic.fields["previous_walking_target"] = walking_target ? xyz(*walking_target) : J(nullptr);
            const auto replacement = lane->vantage(p.position, vehicle->transform.position, (precondition_complete?.25f:5.f) + pose->sphere.radius,
                [&](Vec3 origin) {
                    const auto foot = origin - Vec3{0, 0, world->content.player.camera_height};
                    // The planner sorts several authored-height rings by
                    // distance from the player. Admit only goals inside the
                    // chosen firing range, including its arrival tolerance.
                    return length(foot - vehicle->transform.position) <= desired_range - world->content.player.radius * 1.5f &&
                        bool(visible(*vehicle, *pose, origin));
                });
            diagnostic.fields["replacement_success"] = bool(replacement);
            diagnostic.fields["replacement_walking_target"] = replacement ? xyz(*replacement) : J(nullptr);
            diagnostic.fields["kept_previous"] = !replacement && bool(walking_target);
            if (replacement) walking_target = replacement;
            else { ++no_walking_vantage; if (walking_target) ++kept_walking_target; }
        }
        if (walking_target && std::hypot(walking_target->x - p.position.x, walking_target->y - p.position.y) < world->content.player.radius * 1.5f)
            walking_target.reset();
        if (surface && range <= desired_range) walking_target.reset();
        if (walking_target) {
            diagnostic.phase = "walking";
            diagnostic.fields["walking_target_xy_distance"] = std::hypot(walking_target->x - p.position.x, walking_target->y - p.position.y);
            const auto point = *walking_target + Vec3{0, 0, world->content.player.camera_height}; aim(p, eye, point);
            const auto toward = point - eye;
            const auto alignment = dot(normalized(Vec3{toward.x, toward.y, 0}), direction(p.yaw));
            diagnostic.fields["walking_alignment"] = alignment; walk(alignment > .9f);
            fire(false); return;
        }
        walk(false); if (!surface) { ++no_surface_returns; diagnostic.phase = "no_visible_surface"; fire(false); return; }
        if (range > desired_range) {
            diagnostic.phase = "no_inside_range_destination"; aim(p, eye, *surface); fire(false); return;
        }
        if (!precondition_requested || !precondition_released) {
            // The first observed throw landed short of the real vehicle. A
            // higher ordinary look input supplies a new arc; native release,
            // motion, collision, fuse and damage remain the only result owners.
            const auto grenade_aim=*surface+Vec3{0,0,1.25f};
            const auto wanted=normalized(grenade_aim-eye);
            aim(p,eye,grenade_aim); fire(false);
            diagnostic.phase="aiming_frag_arc"; diagnostic.fields["grenade_aim_point"]=xyz(grenade_aim);
            if (!precondition_requested && dot(direction(p.yaw,p.pitch),wanted)>.99999f) {
                check(p.grenade_current==0 && p.grenades[0]>0,
                      "Ordinary Coagulation birth lacks its selected authored frag precondition");
                precondition_requested=true;precondition_request_tick=state.tick;precondition_before_body=vitality.body;
                diagnostic.phase="frag_precondition_requested";diagnostic.important=true;grenade();
            }
            return;
        }
        aim(p, eye, *surface);
        const auto &model = models.at(target_definition);
        const auto ray = direction(p.yaw, p.pitch) * (length(*surface - eye) * 2.f);
        const auto current_hit = posed_collision(model, pose->nodes, vehicle->regions->selection.permutations,
            *world->content.original_data, eye, ray, 2u);
        const bool lane_clear = current_hit && lane->clear(eye, eye + ray * current_hit->fraction);
        const bool hull_hit = current_hit && current_hit->model_material >= 0 &&
            std::size_t(current_hit->model_material) < model.materials.size() &&
            model.materials[unsigned(current_hit->model_material)].damage_section == hull_sections.at(target_definition);
        const bool within_mouse_quantum=dot(direction(p.yaw,p.pitch),normalized(*surface-eye))>=std::cos(.003f);
        const bool aligned = (lane_clear && hull_hit) || within_mouse_quantum;
        diagnostic.fields["aim_within_mouse_quantum"]=within_mouse_quantum;
        diagnostic.fields["current_view_hit"] = bool(current_hit); diagnostic.fields["current_view_clear"] = lane_clear;
        diagnostic.fields["current_view_hull_section"] = hull_hit;
        if (current_hit) {
            diagnostic.fields["current_view_material"] = current_hit->model_material;
            if (current_hit->model_material >= 0 && std::size_t(current_hit->model_material) < model.materials.size())
                diagnostic.fields["current_view_damage_section"] = model.materials[unsigned(current_hit->model_material)].damage_section;
        }
        if (!aligned) {
            ++unaligned_returns;
            if (!current_hit) ++current_ray_misses;
            else if (!lane_clear) ++blocked_current_rays;
            else if (!hull_hit) ++non_hull_current_rays;
            diagnostic.phase = "unaligned"; fire(false); return;
        }
        if (!precondition_complete) { diagnostic.phase = "waiting_actual_frag_damage_alive"; fire(false); return; }
        if (p.loaded == 0 && p.reserve > 0 && !waiting_reload) { diagnostic.phase = "reload_requested"; fire(false); reload(); waiting_reload = true; return; }
        if (waiting_reload || p.loaded == 0) { diagnostic.phase = "waiting_reload_or_empty"; fire(false); return; }
        if (!requested_fire) {
            mode3.baseline_generations(); requested_fire = true; first_fire_tick = state.tick;
            pre_fire_body = minimum_post_fire_body = vitality.body;
        }
        diagnostic.phase = "firing"; fire(true);
    }
    void audio(const MultiplayerAudioSubmission &submission) {
        if (!submission.frames || !submission.voice) { ++invalid_packets; return; }
        ++valid_packets;
        if (submission.sound == firing_sound) ++smg_packets;
        if (submission.kind == MultiplayerAudioKind::FirstPerson && reload_sounds.contains(submission.sound) &&
            submission.view == 0 && weapon != null_datum && submission.owner == weapon) ++reload_packets;
        if (submission.kind == MultiplayerAudioKind::Ambient) ++ambient_packets;
    }
    void finish(J &out) const {
        const auto input_bytes = std::filesystem::is_regular_file(delivered_input_path) ? std::filesystem::file_size(delivered_input_path) : 0u;
        out["delivered_inputs"] = {{"path", delivered_input_path.string()}, {"rows", delivered_input_rows}, {"bytes", input_bytes},
            {"closed", delivered_input_closed}, {"io_error", delivered_input_io_error}, {"float_digits", std::numeric_limits<float>::max_digits10},
            {"parser_roundtrip_rows", delivered_roundtrip_rows},
            {"actual_accept_equals_fire_rows", delivered_input_rows}, {"legacy_cli_projection_omits_accept", true},
            {"scope", "Actual delivered Controls at input_observer steps keys; c.yaw/c.pitch, fire/reload/grenade and movement/jump/crouch; no player angles or gameplay results supplied"}};
        out["birth_inventory"] = birth_inventory;
        out["precondition"] = {{"kind", "one_ordinary_frag_before_smg"}, {"requested", precondition_requested},
            {"commands", precondition_commands}, {"request_tick", precondition_request_tick},
            {"projectile", precondition_projectile}, {"definition", precondition_definition}, {"type", 0},
            {"projectile_samples", precondition_projectile_samples}, {"released", precondition_released},
            {"release_tick", precondition_release_tick}, {"detonated", precondition_detonated},
            {"detonation_serial", precondition_detonation_serial}, {"detonation_tick", precondition_detonation_tick},
            {"detonation", precondition_detonation}, {"projectile_history", precondition_projectile_history},
            {"target_owner", target}, {"target_definition", target_definition},
            {"before_body", precondition_before_body}, {"after_body", precondition_after_body},
            {"damage_observed", precondition_damage}, {"damage_tick", precondition_damage_tick},
            {"damaged_alive", precondition_damage && precondition_after_body > 0.f},
            {"complete", precondition_complete}, {"complete_tick", precondition_complete_tick},
            {"scope", "Actual ordinary frag input, full salted native release/detonation and surviving target damage; timing alone does not attribute a looping effect's birth"}};
        out["player"] = {{"slot", slot}, {"source_player", player}, {"source_unit", unit}, {"source_weapon", weapon},
            {"weapon_definition", weapon_definition}, {"identity_changes", identity_changes}, {"magazine_capacity", capacity},
            {"fire_commands", fire_commands}, {"longest_fire_streak", longest_fire_streak}, {"shot_transitions", shot_transitions},
            {"rounds_consumed", rounds_consumed}, {"reload_commands", reload_commands}, {"reload_transfers", reload_transfers},
            {"reload_rounds", reload_rounds}, {"walking_commands", walk_commands}, {"aim_inputs", aim_inputs},
            {"first_fire_request_tick", first_fire_tick}, {"first_shot_tick", mode3.first_shot_tick},
            {"initial_position", xyz(initial_player_position)}, {"history", history}};
        out["target"] = {{"handle", target}, {"definition", target_definition}, {"placement", target_placement},
            {"netgame_row", target_netgame_row}, {"tag", warthog_tag}, {"vitality_samples", vitality_samples},
            {"missing_samples", target_missing}, {"initial_body", initial_body}, {"minimum_body", minimum_body},
            {"pre_fire_body", pre_fire_body}, {"minimum_post_fire_body", minimum_post_fire_body},
            {"initial_shield", initial_shield}, {"minimum_shield", minimum_shield}, {"body_damage_observed", body_damage},
            {"first_damage_tick", first_damage_tick}, {"initial_position", xyz(initial_target_position)},
            {"latest_position", xyz(latest_target_position)}};
        out["target"]["maximum_hull_damage_state"] = maximum_hull_damage;
        out["target"]["hull_region_history"] = hull_history;
        out["target"]["hull_effect_history"] = hull_effect_history;
        out["target"]["effect_stores"]={{"samples",J::array()},{"errors",effect_store_errors},{"overflow",effect_store_overflow},
            {"scope","Passive actual17A0F0..17A560 completed stores for full salted Warthog-owned effects; fixed integer callback, no supplied source outcomes"}};
        for(unsigned n=0;n<effect_store_count;++n){const auto &s=effect_stores[n];
            out["target"]["effect_stores"]["samples"].push_back({{"pc",s.pc},{"cursor",s.cursor},{"tick",s.tick},
                {"address",s.address},{"bytes",s.bytes},{"before",s.before},{"written",s.written},
                {"effect",s.effect},{"definition",s.definition},{"flags",s.flags},{"loop",s.loop},{"owner",s.owner},
                {"primary_control",s.primary},{"secondary_control",s.secondary},{"control18",s.control18},
                {"control6c_bits",s.control6c},{"control70_bits",s.control70},{"owner_header_flags",s.header_flags},
                {"owner_body_flags",s.body_flags},{"owner_parent",s.parent},{"elapsed_bits",s.elapsed},
                {"duration_bits",s.duration},{"remainder_bits",s.remainder}});}
        out["target"]["hull_section_record_latest"] = hull_record_latest;
        out["target"]["hull_section_record_history"] = hull_record_history;
        out["projectiles"] = {{"smg_projectile_definition", projectile_definition}, {"native_smg_detonations", native_detonations},
            {"native_smg_owned_detonations", owned_detonations}, {"legacy_smg_bullet_impacts", legacy_impacts},
            {"scope", "Native projectile effect publications and separate legacy impact census; publications alone do not identify a hit receiver"}};
        out["audio"] = {{"smg_packets", smg_packets}, {"reload_packets", reload_packets}, {"ambient_packets", ambient_packets},
            {"valid_packets", valid_packets}, {"invalid_packets", invalid_packets}, {"driver", "dummy"}, {"mute", false},
            {"scope", "Accepted actual SDL packets; physical speaker output is outside this test"}};
        out["mode3"] = mode3.report(); out["submitted_frames"] = submitted_frames; out["capture_frame_count"] = capture_frame_count;
        out["last_input_tick"] = last_input_tick; out["last_observed_state_tick"] = last_state_tick;
        out["shadow_generate_draws"] = generated_draws; out["shadow_apply_draws"] = applied_draws;
        out["planning"] = {{"samples", planning}, {"latest", latest_planning}, {"sample_limit", 32}, {"minimum_interval_ticks", 60},
            {"scope", "Read-only posed/BSP queries and actual source/input census; these queries do not count as physical hits or damage"},
            {"diagnostic_errors", diagnostic_errors}, {"camera_absent_returns", camera_absent_returns},
            {"nonzero_camera_mode_returns", camera_mode_returns}, {"no_surface_returns", no_surface_returns},
            {"camera_mode_return_counts", camera_mode_return_counts}, {"first_camera_block_tick", first_camera_block_tick},
            {"last_camera_block_tick", last_camera_block_tick},
            {"unaligned_returns", unaligned_returns}, {"current_ray_misses", current_ray_misses}, {"blocked_current_rays", blocked_current_rays},
            {"non_hull_current_rays", non_hull_current_rays},
            {"replan_attempts", replan_attempts}, {"no_walking_vantage", no_walking_vantage}, {"kept_previous_walking_target", kept_walking_target},
            {"aim_calls", aim_calls}, {"aim_zero_inputs", aim_zero_inputs}, {"warthog_candidates", J::array()}};
        for (const auto &[handle, row] : candidates)
            out["planning"]["warthog_candidates"].push_back({{"handle", handle}, {"definition", row.definition},
                {"placement", row.placement}, {"netgame_row", row.netgame_row}, {"first_tick", row.first_tick}, {"last_tick", row.last_tick},
                {"first_pose_tick", row.pose_tick}, {"first_regions_tick", row.regions_tick}, {"source_generation_verified", row.generation_verified},
                {"initial_position", xyz(row.initial_position)}, {"latest_position", xyz(row.latest_position)}});
    }
};

bool capture_image(const Arguments &args, J &report) {
    if (!std::filesystem::is_regular_file(args.screenshot)) return false;
    std::ifstream input(args.screenshot, std::ios::binary); std::string magic, dimensions, maximum;
    std::getline(input, magic); std::getline(input, dimensions); std::getline(input, maximum);
    const auto header = input.tellg(); const auto size = std::filesystem::file_size(args.screenshot);
    report["capture_bytes"] = size;
    return magic == "P6" && dimensions == std::to_string(width) + " " + std::to_string(height) && maximum == "255" &&
        header >= 0 && size == std::uint64_t(header) + std::uint64_t(width) * height * 3u;
}
void exercise(const Arguments &args, J &report) {
    environment(args);
    MultiplayerOptions options; options.map = args.map; options.shared_maps = args.shared;
    options.width = width; options.height = height; options.local_players = 1; options.starting_weapon = 0;
    options.mute = false; options.hidden = false; options.source_object_world = true;
    options.reference_tick_cadence = true; options.capture_tick = args.capture_tick;
    options.screenshot = args.screenshot; options.capture_shader_directory = args.shaders;
    options.source_object_savegame_directory = args.scratch / "smg-warthog-source";
    options.desktop = std::make_shared<DesktopSession>(); options.desktop->display_settings.graphics_mode = pstencil::GraphicsMode::Original;
    Probe probe(args, options);
    options.source_world_observer = [&](const auto &memory) { probe.mode3.bind(memory); };
    options.input_observer = [&](auto tick, auto commands, const auto &state) { probe.input(tick, commands, state); };
    options.audio_observer = [&](const auto &submission) { probe.audio(submission); };
    options.view_state_observer = [&](unsigned view, const auto &state, const auto &camera) {
        if (view == 0) {
            probe.last_view_tick = state.tick; probe.submitted_camera_position = camera.position;
            probe.submitted_camera_forward = camera.forward; probe.submitted_camera_oriented = camera.oriented;
            probe.have_submitted_camera = true;
        }
    };
    options.frame_observer = [&](const auto &frame) {
        if (!frame.submitted) return;
        ++probe.submitted_frames;
        if (probe.last_view_tick == args.capture_tick) ++probe.capture_frame_count;
        probe.generated_draws += frame.renderer.world_native_shadow_generate_prepared_draws;
        probe.applied_draws += frame.renderer.world_native_shadow_apply_prepared_draws;
    };
    try {
        auto prepared = std::make_shared<MultiplayerPreparedWorld>(options); options.prepared_world = prepared; probe.setup(*prepared);
        MultiplayerRuntimeOutcome outcome; const auto result = run_multiplayer(options, outcome);
        probe.unbind_effect_store();
        constexpr std::array exit_names{"headless_limit", "frame_limit", "captured_tick", "window_quit", "escape",
            "session_world_close", "online_postgame", "online_disconnected", "system_link_postgame", "system_link_lobby"};
        report["run_result"] = result; report["exit_reason"] = exit_names.at(unsigned(outcome.reason));
        report["exit_reason_code"] = int(outcome.reason);
        report["exit_reason_captured_tick"] = outcome.reason == MultiplayerExitReason::captured_tick;
        report["local_authority"] = outcome.authority == MultiplayerOutcomeAuthority::local;
        report["final_tick"] = outcome.state ? J(outcome.state->tick) : J(nullptr);
        report["capture_written"] = capture_image(args, report);
        probe.close_delivered_inputs();
        probe.finish(report);
        const J gates{{"runtime_completed", result == 0 && outcome.reason == MultiplayerExitReason::captured_tick &&
                outcome.authority == MultiplayerOutcomeAuthority::local && outcome.state && outcome.state->tick == args.capture_tick},
            {"capture_completed", report["capture_written"] == true && probe.submitted_frames > 0 && probe.capture_frame_count == 1},
            {"ordinary_same_player_weapon", probe.identities && probe.identity_changes == 0},
            {"delivered_input_schedule", probe.delivered_input_closed && !probe.delivered_input_io_error &&
                probe.birth_inventory.at("tick").get<unsigned>() < args.capture_tick &&
                probe.delivered_input_rows == args.capture_tick - probe.birth_inventory.at("tick").get<unsigned>() &&
                probe.delivered_input_rows == probe.last_input_tick + 1u &&
                probe.delivered_roundtrip_rows == probe.delivered_input_rows},
            {"sustained_smg_fire", probe.rounds_consumed > probe.capacity && probe.shot_transitions >= 20 && probe.longest_fire_streak >= 30},
            {"completed_reload", probe.reload_commands > 0 && probe.reload_transfers > 0 && probe.reload_rounds > 0},
            {"actual_frag_damaged_alive_precondition", probe.precondition_complete && probe.precondition_commands == 1 &&
                probe.precondition_projectile_samples >= 2 && probe.precondition_released && probe.precondition_detonated &&
                probe.precondition_damage && probe.precondition_after_body > 0.f &&
                probe.precondition_after_body < probe.precondition_before_body && probe.first_fire_tick >= probe.precondition_complete_tick},
            {"actual_warthog_damage", probe.target != null_datum && probe.vitality_samples >= 2 && probe.body_damage},
            {"native_smg_projectile_publications", probe.native_detonations >= 20 && probe.owned_detonations > 0},
            {"actual_unmuted_audio", probe.invalid_packets == 0 && probe.smg_packets > 0 && probe.reload_packets > 0},
            {"actual_mode3_effect_progression", probe.mode3.target_owned_after_fire > 0 && probe.mode3.ticks >= 2 &&
                probe.mode3.invalid == 0 && probe.mode3.read_failures == 0 && probe.mode3.depth == 0 &&
                probe.mode3.entries == probe.mode3.returns},
            {"ordinary_shadow_draws", probe.generated_draws > 0 && probe.applied_draws > 0}};
        report["gates"] = gates;
        bool passed = true; for (const auto &gate : gates) passed &= gate.get<bool>();
        report["passed"] = passed;
        probe.mode3.binding.reset(); options.prepared_world.reset(); prepared.reset();
        check(passed, "Required live SMG/Warthog/audio/capture evidence is incomplete; inspect gates and measured fields");
    } catch (...) {
        probe.close_delivered_inputs(); probe.mode3.binding.reset(); probe.unbind_effect_store(); probe.finish(report); report["passed"] = false;
        try { report["capture_written"] = capture_image(args, report); } catch (...) { report["capture_written"] = false; }
        throw;
    }
}
} // namespace

int main(int argc, char **argv) {
    std::filesystem::path output;
    J report{{"passed", false}, {"scope", "Ordinary local MCC SMG-to-authored-Warthog runtime with real source owners, SDL inputs/audio and rendered capture; same build cohort, no halo2_native ELF or physical Xbox claim"},
        {"runtime_cache_source", "MCC"}, {"graphics_mode", "Original"}, {"capture_written", false},
        {"capture_frame_count", 0}, {"submitted_frames", 0}, {"retained_large_artifacts", J::array()}};
    // Find only the report destination before validation so an invalid request
    // replaces a stale receipt with an explicit failure as well.
    for (int n = 1; n + 1 < argc; ++n) if (std::string_view(argv[n]) == "--report-output") output = argv[n + 1];
    try {
        const auto args = arguments(argc, argv); output = args.report;
        report.update({{"map", args.map.string()}, {"shared_maps", args.shared.string()}, {"shader_directory", args.shaders.string()},
            {"requested_capture_tick", args.capture_tick}, {"screenshot", args.screenshot.string()}, {"viewport", {width, height}},
            {"capture_root", false}, {"reference_player", false}, {"capture_camera", false}, {"mute", false}});
        exercise(args, report); retain_report(output, report);
        std::cout << "PASS ordinary MCC SMG/Warthog audio runtime: measured shots, reload, target damage, mode3 effect progression and captured tick "
                  << args.capture_tick << '\n'; return 0;
    } catch (const std::exception &error) {
        report["passed"] = false; report["error"] = error.what();
        try { retain_report(output, report); } catch (const std::exception &failure) { std::cerr << failure.what() << '\n'; }
        std::cerr << "FAIL " << error.what() << '\n'; return 1;
    }
}
