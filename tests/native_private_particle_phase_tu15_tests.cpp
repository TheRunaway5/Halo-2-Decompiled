#include "halo2/bsp_atmosphere.hpp"
#include "halo2/kernels.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/muzzle_effects.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "halo2/source_particle_render_state_tu15.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/source_private_particle_phase_tu15.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <vector>
#include <x86intrin.h>
using namespace halo2;
namespace {
using Json = nlohmann::json;
void need(bool value, const char *why) {
  if (!value)
    throw FormatError(why);
}
struct Report {
  const char *path;
  Json data{{"passed", false}, {"full_goal_complete", false}};
  ~Report() { std::ofstream(path) << data.dump(2) << '\n'; }
};
std::uint32_t word(const SourceRuntimeMemory &memory, std::uint32_t at) {
  return Bytes(memory.view(at, 4)).u32(0);
}
struct Frame {
  MultiplayerPreparedWorld &world;
  SourceWorldParticleDrawsTu15 graphics;
  SourcePortalVisibilitySet visibility;
  MuzzleEffectView view{};
  SourceCameraAngles angles;
  Vec3 eye, forward, up{0, 0, 1};
  float fov;
  explicit Frame(MultiplayerPreparedWorld &value, unsigned representation)
      : world(value), graphics(*world.cache, world.content.objects) {
    const auto state = world.match.state();
    const auto &player = state.players.at(world.local_slots.at(0));
    const auto &biped = multiplayer_biped(world.content, representation);
    eye = ordinary_unit_eye(player.position, biped.camera_height,
                            biped.crouch_camera_height, player.crouch);
    forward = direction(player.yaw, player.pitch);
    fov = biped.camera_field_of_view;
    angles = source_camera_angles(fov);
    SourceFrustumProjectionInput input;
    input.projection_angle = angles.projection_angle;
    input.outer = input.safe = {0, 0, 480, 640};
    input.scenario_present = true;
    input.scenario_type = world.cache->load_source_geometry_scenario_type();
    const auto projection = source_frustum_projection(input);
    const auto camera = source_frustum_camera(
        {eye, forward, up, .015625f, 1024.f, projection.clipping_rectangle()});
    SourcePortalMaskState mask;
    mask.initialize(world.cache->load_source_portal_mask_definition(0), {});
    visibility = source_portal_visibility(
        world.cache->load_source_portal_bsp(0), std::span{&camera, 1},
        world.cache->load_bsp_atmosphere(0).cluster_at(eye), mask.words);
    view.position = eye;
    view.forward = forward;
    view.up = up;
    view.width = 640;
    view.height = 480;
    const auto &basis = camera.camera_to_world.basis;
    view.orientation = {basis[0].x, basis[0].y, basis[0].z,
                        basis[1].x, basis[1].y, basis[1].z,
                        basis[2].x, basis[2].y, basis[2].z};
    const auto &weapon = world.content.weapons.at(player.weapon);
    for (const auto &row : weapon.firing_effects)
      for (auto effect : row.effects)
        if (effect != null_datum) {
          const auto tag =
              world.cache->query_tag({.handle = TagHandle{effect}});
          if (tag && tag->group == "effe") {
            graphics.preload_first_person(effect);
            graphics.preload_world(effect);
          }
        }
  }
  std::vector<ParticleDraw> advance(float seconds) {
    auto &native = *world.source_objects;
    native.render_model_view([&](auto &source) {
      graphics.publish_observer(source, 0, eye, forward, up, fov);
      graphics.prepare_storage(source);
    });
    native.prepare_first_person_models(0, eye, forward, up);
    native.advance_effect_frame(seconds);
    std::vector<ParticleDraw> draws;
    native.render_model_view([&](auto &source) {
      graphics.preload_live_world(source.memory());
      graphics.prepare_storage(source);
      source.begin_model_frame();
      draws = graphics.draw_first_person(source, visibility, view, 0,
                                         angles.projection_angle,
                                         angles.lighting_distance_scale);
    });
    return draws;
  }
};
} // namespace

int main(int argc, char **argv) try {
  need(argc == 4 || argc == 7, "Supply MCC map, shared maps, report and "
                               "optional weapon/representation/hold ticks");
  if (!std::filesystem::is_regular_file(argv[1]))
    return 77;
  need(std::getenv("HALO2_TEST_SCRATCH"), "Use owned storage");
  Report report{argv[3]};
  const auto option = argc == 7 ? unsigned(std::stoul(argv[4])) : 5u;
  const auto representation = argc == 7 ? unsigned(std::stoul(argv[5])) : 2u;
  const auto hold_ticks = argc == 7 ? unsigned(std::stoul(argv[6])) : 1u;
  need(option >= 3 && option <= 20 &&
           (representation == 2 || representation == 3) && hold_ticks >= 1 &&
           hold_ticks <= 120,
       "Use declared actual weapon/character/trigger inputs");
  report.data.update({{"option", option},
                      {"representation", representation},
                      {"primary_trigger_hold_ticks", hold_ticks}});
  MultiplayerOptions options;
  options.map = argv[1];
  options.shared_maps = argv[2];
  options.local_players = 1;
  options.starting_weapon = option;
  if (representation == 3) {
    auto profile = source_default_player_configuration();
    profile[0x44] = 1;
    options.selected_profiles = {profile};
  }
  options.source_object_world = true;
  options.host_port = 1;
  options.mute = true;
  options.session_random_seed = 0x105240;
  options.variant = {"Slayer", 0, {{2, 0}, {10, 0}, {89, int(option)}}};
  options.source_object_savegame_directory =
      std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH")) /
      "private-particle-savegames";
  MultiplayerPreparedWorld world(options);
  world.bind_source_local_views(world.local_slots);
  need(world.cache->is_mcc(), "Use actual MCC resources");
  std::array<Controls, multiplayer_capacity> controls{};
  for (unsigned n = 0; n < 60; ++n)
    world.match.step(controls);
  Frame graphics(world, representation);
  (void)graphics.advance(1.f / 30.f);
  const auto slot = world.local_slots[0];
  const auto before = world.match.state();
  bool admitted{};
  unsigned trigger_ticks{};
  MultiplayerState fired;
  // Use actual trigger inputs only. A plasma-pistol effect arrives on release;
  // ammo or elapsed time never substitutes for a source-scheduled birth.
  for (; trigger_ticks < hold_ticks + 120 && !admitted; ++trigger_ticks) {
    controls[slot].fire = trigger_ticks < hold_ticks;
    world.match.step(controls);
    fired = world.match.state();
    for (const auto &event : fired.weapon_effects)
      if (event.weapon == fired.players[slot].source_weapon &&
          event.source_scheduled && event.source_instance != null_datum)
        admitted = true;
  }
  report.data["trigger_ticks"] = trigger_ticks;
  const auto tag =
      world.cache->query_tag({.handle = TagHandle{fired.players[slot].weapon}});
  report.data["weapon"] = tag ? tag->name : "unknown";
  need(admitted,
       "Need actual native effect allocation from primary trigger/release");
  if (argc == 4)
    need(fired.players[slot].loaded + 1 == before.players[slot].loaded,
         "Need one genuine SMG shot");
  auto &bank = world.source_objects->memory();
  auto &native = world.source_objects->tu15_world();
  SourcePrivateParticlePhaseTu15 candidate(bank);
  SourcePhysicsWorldRuntimeServices cpu;
  cpu.reciprocal_sqrt = [](auto value) {
    return std::bit_cast<std::uint32_t>(
        _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(value)))));
  };
  cpu.timestamp_counter = []() { return __rdtsc(); };
  const auto state_read = [](const auto &state, std::uint32_t at,
                             unsigned bytes) {
    std::vector<std::byte> result(bytes);
    std::size_t n{};
    while (n < bytes) {
      bool found{};
      for (const auto &range : state)
        if (at + n >= range.address &&
            std::uint64_t(at + n) <
                std::uint64_t(range.address) + range.bytes.size()) {
          const auto offset = at + n - range.address,
                     count = std::min(bytes - n, range.bytes.size() - offset);
          std::memcpy(result.data() + n, range.bytes.data() + offset, count);
          n += count;
          found = true;
          break;
        }
      need(found, "Private snapshot lost an owned native byte");
    }
    return result;
  };
  const auto unchanged = [&](const auto &a, const auto &b) {
    need(a.size() == b.size(), "Rejected input changed private range count");
    for (unsigned n = 0; n < a.size(); ++n)
      need(a[n].address == b[n].address && a[n].bytes == b[n].bytes,
           "Rejected input changed private state");
  };
  struct Inputs final : SourcePhysicsContactMemory {
    const SourceRuntimeMemory &source;
    std::optional<SourceEffectCompiledSymbolsTu15> symbols;
    std::uint32_t at{}, value{};
    Inputs(const SourceRuntimeMemory &s, bool inherit_symbols = true) : source(s) {
      if (inherit_symbols)
        symbols.emplace(*this, s);
    }
    void read(std::uint32_t p, std::span<std::byte> out) const override {
      source.read(p, out);
      for (unsigned n = 0; n < out.size(); ++n)
        if (p + n >= at && p + n < at + 4)
          out[n] = std::byte(value >> ((p + n - at) * 8));
    }
    void write(std::uint32_t, std::span<const std::byte>) override {
      throw FormatError("Readonly actual input control");
    }
  };
  unsigned refused{};
  for (unsigned mode = 0; mode < 4; ++mode) {
    const auto saved = candidate.state();
    const auto args = std::array{std::bit_cast<std::uint32_t>(1.f / 60.f)};
    auto a = native.prepare_source_call(args);
    Inputs bad(bank, mode != 3);
    bad.at = mode == 3   ? 0
             : mode == 0 ? word(bank, 0x51df54) + 8
             : mode == 1 ? 0x4f720c
                         : 0x4f3cd8;
    bad.value = mode == 3 ? 0 : word(bank, bad.at) + 1;
    bool rejected{};
    try {
      candidate.advance(bad, a, cpu);
    } catch (const FormatError &) {
      rejected = true;
    }
    need(rejected, "Private particle owner accepted another tick/root/namespace");
    bool read_rejected{}, called{};
    try {
      candidate.with_state_inputs(bad, [&](const auto &) { called = true; });
    } catch (const FormatError &) {
      read_rejected = true;
    }
    need(read_rejected && !called,
         "Private particle readback accepted another tick/root/namespace");
    unchanged(saved, candidate.state());
    ++refused;
  }
  for (const auto seconds :
       std::array{-1.f, std::numeric_limits<float>::infinity(),
                  std::numeric_limits<float>::quiet_NaN()}) {
    const auto saved = candidate.state();
    const auto args = std::array{std::bit_cast<std::uint32_t>(seconds)};
    auto a = native.prepare_source_call(args);
    bool rejected{};
    try {
      candidate.advance(bank, a, cpu);
    } catch (const FormatError &) {
      rejected = true;
    }
    need(rejected,
         "Private particle owner accepted a nonfinite/negative delta");
    unchanged(saved, candidate.state());
    ++refused;
  }
  {
    const auto saved = candidate.state();
    const auto original = bank.checkpoint();
    bool entered{}, interrupted{};
    try {
      candidate.with_state_inputs(bank, [&](const auto &) {
        entered = true;
        throw FormatError("Declared readonly-consumer interruption");
      });
    } catch (const FormatError &) {
      interrupted = true;
    }
    need(entered && interrupted,
         "Readonly consumer interruption was not reached");
    unchanged(saved, candidate.state());
    need(bank.checkpoint() == original,
         "Readonly consumer interruption changed borrowed World");
  }
  {
    bool nested_refused{}, nested_entered{};
    candidate.with_state_inputs(
        bank, [&](const SourcePhysicsContactMemory &inputs) {
          try {
            candidate.with_state_inputs(
                bank, [&](const auto &) { nested_entered = true; });
          } catch (const FormatError &) {
            nested_refused = true;
          }
          std::array<std::byte, 4> bytes;
          inputs.read(0x51df74u, bytes);
          need(Bytes(bytes).u32(0) == word(bank, 0x51df74u),
               "Rejected nested borrow lost the outer readonly input owner");
        });
    need(nested_refused && !nested_entered,
         "Private particle input borrow reentered");
  }
  unsigned render_callbacks{}, render_states{}, distinct_unadvanced_readbacks{};
  const auto render_values = [&](const SourcePhysicsContactMemory &memory,
                                 std::uint32_t system, std::uint32_t location) {
    const auto state = source_particle_render_state_tu15(
        *world.cache, memory, system, location,
        {graphics.eye, graphics.forward, graphics.up, {}});
    ParticleRenderValueCache values;
    values.system_identity = system;
    values.location_identity = location;
    Json result = Json::array();
    const auto bits = [](float value) {
      return std::bit_cast<std::uint32_t>(value);
    };
    const auto vector = [&](Vec3 v) {
      return std::array{bits(v.x), bits(v.y), bits(v.z)};
    };
    for_each_particle_render_input(
        state.definition, state.state, values,
        *world.cache->shader_function_tables(),
        [&](auto particle, const ParticleSpriteInput &input,
            ParticleValueCache &computed, const ParticleValueSources &) {
          std::vector<std::uint32_t> slots, inputs, color;
          for (auto value : computed.values)
            slots.push_back(bits(value));
          for (auto value : input.values)
            inputs.push_back(bits(value));
          for (auto value : input.color)
            color.push_back(bits(value));
          result.push_back(
              {{"particle", particle},
               {"evaluated", computed.evaluated},
               {"values", slots},
               {"inputs", inputs},
               {"position", vector(input.position)},
               {"direction", vector(input.direction)},
               {"scale", bits(input.scale)},
               {"color", color},
               {"random", std::array{input.random1, input.random2}},
               {"seed_color", input.seed_color}});
        });
    return result;
  };
  using RenderSnapshot =
      std::map<std::pair<std::uint32_t, std::uint32_t>, Json>;
  const auto render_snapshot = [&](const SourcePhysicsContactMemory &inputs) {
    const auto read = [&](std::uint32_t at, unsigned bytes = 4) {
      std::array<std::byte, 4> data{};
      inputs.read(at, std::span(data).first(bytes));
      return Bytes(data).u32(0);
    };
    RenderSnapshot result;
    const auto pool = read(0x51df74u), rows = read(pool + 0x44u),
               high = read(pool + 0x38u), occupancy = read(pool + 0x48u);
    need(high <= read(pool + 0x20u) && read(pool + 0x24u) == 84,
         "Readonly particle system pool layout differs");
    for (unsigned n = 0; n < high; ++n)
      if (read(occupancy + (n / 32u) * 4u) & (1u << (n % 32u))) {
        const auto record = rows + n * 84u,
                   system = read(record, 2) * 65536u + n;
        std::set<std::uint32_t> visited;
        for (auto location = read(record + 0x30u); location != null_datum;) {
          need(visited.insert(location).second,
               "Readback location chain cycles");
          const auto locations = read(0x52d71cu),
                     row = read(locations + 0x44u) + (location & 65535u) * 52u;
          if (read(row + 2u, 1))
            result.emplace(std::pair{system, location},
                           render_values(inputs, system, location));
          location = read(row + 0xcu);
        }
      }
    return result;
  };
  const auto initial_query_generation = word(bank, 0x4f3ce4u);
  unsigned compared{}, cpu_estimates{}, cpu_cycles{};
  Json samples = Json::array();
  const auto estimate = cpu.reciprocal_sqrt;
  cpu.reciprocal_sqrt = [&](auto value) {
    ++cpu_estimates;
    return estimate(value);
  };
  const auto cycle = cpu.timestamp_counter;
  cpu.timestamp_counter = [&]() {
    ++cpu_cycles;
    return cycle();
  };
  unsigned retired{};
  {
    SourcePrivateParticlePhaseTu15 stopped(bank);
    const auto seed = stopped.state();
    const auto args = std::array{std::bit_cast<std::uint32_t>(1.f / 60.f)};
    auto a = native.prepare_source_call(args);
    const auto original = bank.checkpoint();
    bool changed_owned{};
    const SourcePhysicsContactRecordObserver interrupt =
        [&](const auto &store) {
          if (store.before == store.written)
            return;
          for (const auto &range : seed)
            if (store.address >= range.address &&
                std::uint64_t(store.address) + store.written.size() <=
                    std::uint64_t(range.address) + range.bytes.size()) {
              changed_owned = true;
              throw FormatError("Declared post-store interruption after "
                                "changed native owned state");
            }
        };
    bool interrupted{};
    try {
      stopped.advance(bank, a, cpu, interrupt);
    } catch (const FormatError &) {
      interrupted = true;
    }
    need(interrupted && changed_owned,
         "Native partial-write interruption control was not reached");
    const auto saved = stopped.state();
    bool private_changed = seed.size() != saved.size();
    for (unsigned n = 0; n < std::min(seed.size(), saved.size()); ++n)
      private_changed |= seed[n].address != saved[n].address ||
                         seed[n].bytes != saved[n].bytes;
    need(private_changed,
         "Post-store interruption lost its actual partial native write");
    unsigned resumed_instructions{};
    auto resume = cpu;
    resume.instruction = [&](auto) { ++resumed_instructions; };
    bool refused_again{};
    a = native.prepare_source_call(args);
    try {
      stopped.advance(bank, a, resume);
    } catch (const FormatError &e) {
      refused_again =
          std::string(e.what()) ==
          "Private particle owner stopped inside a previous native call";
    }
    need(refused_again && !resumed_instructions,
         "Partially executed native owner was reused");
    bool read_rejected{}, read_entered{};
    try {
      stopped.with_state_inputs(bank,
                                [&](const auto &) { read_entered = true; });
    } catch (const FormatError &) {
      read_rejected = true;
    }
    need(read_rejected && !read_entered,
         "Partially executed native owner exposed render inputs");
    unchanged(saved, stopped.state());
    need(bank.checkpoint() == original,
         "Retired native owner modified borrowed World");
    ++retired;
  }
  for (unsigned frame = 0; frame < 24; ++frame) {
    const auto seconds =
        std::array{0.f, 1.f / 60.f, 1.f / 30.f, 1.f / 120.f}[frame % 4];
    const auto args = std::array{std::bit_cast<std::uint32_t>(seconds)};
    auto a = native.prepare_source_call(args);
    const auto original = bank.checkpoint();
    report.data["attempt"] = {{"frame", frame}, {"delta_bits", args[0]}};
    std::vector<std::uint32_t> private_trace, actual_trace;
    cpu.instruction = [&](auto pc) { private_trace.push_back(pc); };
    std::string private_error;
    try {
      candidate.advance(bank, a, cpu);
    } catch (const std::exception &e) {
      private_error = e.what();
      report.data["native_error"] = private_error;
    }
    cpu.instruction = {};
    need(bank.checkpoint() == original,
         "Private native phase modified borrowed World inputs");
    RenderSnapshot private_render;
    if (private_error.empty()) {
      const auto saved = candidate.state();
      try {
        candidate.with_state_inputs(
            bank, [&](const SourcePhysicsContactMemory &inputs) {
              private_render = render_snapshot(inputs);
            });
      } catch (const std::exception &e) {
        private_error = e.what();
        report.data["private_readback_error"] = private_error;
      }
      if (private_error.empty() && private_render != render_snapshot(bank))
        ++distinct_unadvanced_readbacks;
      unchanged(saved, candidate.state());
      need(bank.checkpoint() == original,
           "Readonly private particle readback changed its input World");
    }
    SourcePhysicsWorldRuntimeAbi actual{};
    bool returned{};
    try {
      world.source_objects->advance_effect_particles(
          seconds, [&](bool before, const auto &result, const auto &,
                       auto instructions, auto) {
            if (!before) {
              actual = result;
              returned = true;
              actual_trace.assign(instructions.begin(), instructions.end());
            }
          });
    } catch (const std::exception &e) {
      report.data["actual_error"] = e.what();
      throw;
    }
    if (!private_error.empty()) {
      std::size_t common{};
      while (common < private_trace.size() && common < actual_trace.size() &&
             private_trace[common] == actual_trace[common])
        ++common;
      const auto tail = [&](const auto &trace) {
        const auto first = common > 16 ? common - 16 : 0,
                   last = std::min(trace.size(), common + 32);
        return std::vector<std::uint32_t>(trace.begin() + first,
                                          trace.begin() + last);
      };
      report.data.update({{"actual_reference_completed", returned},
                          {"common_instruction_prefix", common},
                          {"private_trace_size", private_trace.size()},
                          {"actual_trace_size", actual_trace.size()},
                          {"private_divergence", tail(private_trace)},
                          {"actual_divergence", tail(actual_trace)}});
      throw FormatError(private_error);
    }
    need(returned && a.gp == actual.gp &&
             !((a.eflags ^ actual.eflags) & 0x8d5u),
         "Private phase differs from actual World GP/flags");
    need(a.floating_point && actual.floating_point &&
             a.floating_point->x87_control_word ==
                 actual.floating_point->x87_control_word &&
             a.floating_point->mxcsr == actual.floating_point->mxcsr,
         "Private phase FP controls differ");
    need(a.x87_depth == actual.x87_depth && a.x87_status == actual.x87_status &&
             a.xmm == actual.xmm &&
             bool(a.active_mmx) == bool(actual.active_mmx),
         "Private phase live FP/MMX file differs");
    for (unsigned n = 0; n < a.x87_depth; ++n)
      need(!std::memcmp(&a.x87[n], &actual.x87[n], 10),
           "Private phase live extended lane differs");
    if (a.active_mmx)
      need(a.active_mmx->physical == actual.active_mmx->physical &&
               a.active_mmx->known_mask == actual.active_mmx->known_mask,
           "Private phase MMX lanes differ");
    const auto state = candidate.state();
    for (const auto [global, stride] :
         std::array<std::pair<std::uint32_t, unsigned>, 6>{{{0x4f7208u, 60},
                                                            {0x4f720cu, 400},
                                                            {0x51df74u, 84},
                                                            {0x52d71cu, 52},
                                                            {0x52d718u, 76},
                                                            {0x52d714u, 64}}}) {
      const auto pool = word(bank, global), capacity = word(bank, pool + 0x20);
      for (const auto [at, bytes] :
           std::array<std::pair<std::uint32_t, unsigned>, 3>{
               {{pool, 0x4c},
                {word(bank, pool + 0x44), capacity * stride},
                {word(bank, pool + 0x48), (capacity + 31) / 32 * 4}}}) {
        const auto private_bytes = state_read(state, at, bytes);
        const auto actual_bytes = bank.view(at, bytes);
        if (!std::ranges::equal(private_bytes, actual_bytes)) {
          unsigned offset{};
          while (offset < bytes &&
                 private_bytes[offset] == actual_bytes[offset])
            ++offset;
          report.data["first_pool_difference"] = {
              {"frame", frame},
              {"delta_bits", args[0]},
              {"pool_global", global},
              {"address", at + offset},
              {"private_byte",
               std::to_integer<unsigned>(private_bytes[offset])},
              {"actual_byte", std::to_integer<unsigned>(actual_bytes[offset])}};
          throw FormatError("Private native pool bytes differ");
        }
      }
    }
    const auto random = word(bank, 0x4f3cd8);
    need(std::ranges::equal(state_read(state, random, 8), bank.view(random, 8)),
         "Private native RNG pair differs");
    for (const auto &range : state) {
      std::vector<std::byte> actual_bytes(range.bytes.size());
      bank.read(range.address, actual_bytes);
      need(range.bytes == actual_bytes, "Private native owned state differs, "
                                        "including query/accessor scratch");
    }
    const auto world_before_read = bank.checkpoint();
    const auto actual_render = render_snapshot(bank);
    need(private_render == actual_render,
         "Private native FP render-value callbacks differ from actual World");
    for (const auto &[identity, values] : actual_render) {
      (void)identity;
      render_callbacks += values.size();
      ++render_states;
    }
    need(bank.checkpoint() == world_before_read,
         "Readonly actual particle readback changed its World");
    samples.push_back({{"frame", frame}, {"delta_bits", args[0]}});
    ++compared;
  }
  need(!cpu_estimates && !cpu_cycles,
       "Unpaired live CPU services cannot establish state equality");
  if (argc == 4)
    need(render_callbacks > 0, "SMG positive control produced no private "
                               "native render-value callbacks");
  if (argc == 4)
    need(distinct_unadvanced_readbacks > 0,
         "SMG private readback did not distinguish the unadvanced borrowed "
         "World");
  const auto final_state = candidate.state();
  std::size_t owned_bytes{};
  for (const auto &range : final_state)
    owned_bytes += range.bytes.size();
  report.data.update(
      {{"passed", true},
       {"retirement_controls", retired},
       {"retirement_after_changed_owned_store", true},
       {"cpu_estimates", cpu_estimates},
       {"cpu_cycles", cpu_cycles},
       {"owned_bytes", owned_bytes},
       {"initial_query_generation", initial_query_generation},
       {"final_query_generation", word(bank, 0x4f3ce4u)},
       {"native_frames", compared},
       {"native_fp_render_states", render_states},
       {"native_fp_render_callbacks", render_callbacks},
       {"readonly_render_values_equal", true},
       {"private_readback_before_actual_world_phase", true},
       {"readbacks_distinct_from_unadvanced_world",
        distinct_unadvanced_readbacks},
       {"readonly_input_refusals", 4},
       {"compiled_symbol_namespace_retained", true},
       {"readonly_consumer_interruption", true},
       {"nested_borrow_refused", true},
       {"input_refusals", refused},
       {"source_admission", admitted},
       {"actual_loaded_delta",
        before.players[slot].loaded - fired.players[slot].loaded},
       {"borrowed_world_unchanged_by_private_owner", true},
       {"samples", samples},
       {"scope",
        "Private native179DF0 phase over genuine six-pool/RNG state compared "
        "with the actual World, including zero and three positive accepted "
        "deltas and input refusals. Same gameplay tick and readonly full World "
        "inputs remain explicit; no current-client, render-queue or "
        "full-lifecycle acceptance."}});
  std::cout << Json{{"passed", true},
                    {"frames", compared},
                    {"refused_inputs", refused}}
                   .dump()
            << std::endl;
} catch (const std::exception &e) {
  std::cerr << e.what() << std::endl;
  return 1;
}
