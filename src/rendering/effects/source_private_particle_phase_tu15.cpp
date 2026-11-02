#include "halo2/source_private_particle_phase_tu15.hpp"
#include "gameplay/animation/source_first_person_context_memory.hpp"
#include "halo2/death_cycle_runtime_tu15.hpp"
#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/physics_phantom_world_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/source_effect_particle_frame_tu15.hpp"
#include "halo2/source_particle_swarm_tu15.hpp"
#include <cmath>
#include <cstring>
#include <sstream>

namespace halo2 {
namespace {
using Memory = SourcePhysicsContactMemory;
std::uint32_t word(const Memory &memory, std::uint32_t at) {
  std::array<std::byte, 4> bytes;
  memory.read(at, bytes);
  return Bytes(bytes).u32(0);
}
struct Extent {
  std::uint32_t address, bytes;
};
bool contains(Extent range, std::uint32_t at, std::size_t bytes) {
  return at >= range.address && std::uint64_t(at) + bytes <=
                                    std::uint64_t(range.address) + range.bytes;
}
constexpr std::array<std::pair<std::uint32_t, unsigned>, 6> pools{
    {{0x4f7208u, 60},
     {0x4f720cu, 400},
     {0x51df74u, 84},
     {0x52d71cu, 52},
     {0x52d718u, 76},
     {0x52d714u, 64}}};
struct PrivateMemory final : Memory {
  first_person_context_detail::ContextMemory data;
  std::vector<Extent> writable;
  Extent stack{}, exception_head{};
  std::array<std::byte, 4> exception_bytes{};
  std::vector<std::byte> caller;
  std::uint32_t instruction{};
  void read(std::uint32_t at, std::span<std::byte> bytes) const override {
    if (contains(stack, at, bytes.size()))
      std::memcpy(bytes.data(), caller.data() + (at - stack.address),
                  bytes.size());
    else if (contains(exception_head, at, bytes.size()))
      std::memcpy(bytes.data(),
                  exception_bytes.data() + (at - exception_head.address),
                  bytes.size());
    else
      data.read(at, bytes);
  }
  void write(std::uint32_t at, std::span<const std::byte> bytes) override {
    if (contains(stack, at, bytes.size())) {
      std::memcpy(caller.data() + (at - stack.address), bytes.data(),
                  bytes.size());
      return;
    }
    if (contains(exception_head, at, bytes.size())) {
      std::memcpy(exception_bytes.data() + (at - exception_head.address),
                  bytes.data(), bytes.size());
      return;
    }
    // Actual30AA40 increments the retained collision-cache use counter at
    // this+24. It is shared resource metadata in the input World; retain only
    // this reached source-written word privately, never the borrowed resource.
    if (instruction == 0x30aa40u && bytes.size() == 4 &&
        !data.fully_owned(at, 4)) {
      data.pin_range(at, 4, true);
      writable.push_back({at, 4});
    }
    if (!std::ranges::any_of(writable, [&](auto range) {
          return contains(range, at, bytes.size());
        })) {
      std::ostringstream error;
      error << "Private particle phase requires another storage owner at 0x"
            << std::hex << at << " at 0x" << instruction;
      throw FormatError(error.str());
    }
    data.write(at, bytes);
  }
};
struct Borrow {
  PrivateMemory &memory;
  Borrow(PrivateMemory &m, const Memory &inputs) : memory(m) {
    memory.data.borrow(&inputs);
  }
  ~Borrow() { memory.data.borrow(nullptr); }
};
} // namespace
struct SourcePrivateParticlePhaseTu15::Impl {
  PrivateMemory memory;
  std::array<std::uint32_t, 6> roots{};
  std::uint32_t random{}, clock{}, tick{}, game{}, kind{}, tags{};
  bool faulted{};
  void pin(std::uint32_t at, std::uint32_t bytes, bool writable) {
    memory.data.pin_range(at, bytes, true);
    if (writable)
      memory.writable.push_back({at, bytes});
  }
  explicit Impl(const Memory &source) {
    Borrow borrow(memory, source);
    clock = word(source, 0x51df54);
    tick = word(source, clock + 8);
    game = word(source, 0x4f3218);
    kind = word(source, game + 8);
    tags = word(source, 0x4f0414);
    if (!clock || !game || !tags)
      throw FormatError("Private particle seed lacks its genuine native roots");
    unsigned n{};
    for (const auto [global, stride] : pools) {
      const auto pool = word(source, global),
                 capacity = word(source, pool + 0x20);
      if (!pool || !capacity || capacity > 65536 ||
          word(source, pool + 0x24) != stride)
        throw FormatError(
            "Private particle seed has another native pool layout");
      for (const auto offset : {0x34u, 0x38u, 0x3cu})
        if (word(source, pool + offset) > capacity)
          throw FormatError(
              "Private particle seed exceeds actual allocator bounds");
      roots[n++] = pool;
      pin(global, 4, false);
      pin(pool, 0x4c, false);
      memory.writable.push_back({pool + 0x34, 14});
      pin(word(source, pool + 0x44), capacity * stride, true);
      pin(word(source, pool + 0x48), (capacity + 31) / 32 * 4, true);
    }
    random = word(source, 0x4f3cd8);
    pin(0x4f3cd8, 4, false);
    pin(random, 8, true);
    // The actual13A6B0 sprite/model interface instances retain two mutable
    // selector words. Their vtable words remain borrowed immutable inputs.
    for (const auto instance : {0x48603cu, 0x486048u})
      pin(instance + 4, 8, true);
    // Native16C620/24DC90 collision traversals own their query generations,
    // active flags and visited stamps. Copy the actual existing storage:
    // 512 BSP clusters,1024 instanced geometry rows and2048 object slots.
    // These extents match native_tu15_world_memory/object_storage_data.
    for (const auto range : std::array<Extent, 9>{{{0x4f3ce1u, 1},
                                                   {0x4f3ce4u, 4},
                                                   {0x4f3ce8u, 512 * 4},
                                                   {0x4f44e8u, 1},
                                                   {0x4f44ecu, 4},
                                                   {0x4f44f0u, 1024 * 4},
                                                   {0x4eabc8u, 1},
                                                   {0x4eabccu, 4},
                                                   {0x4eabd0u, 2048 * 4}}})
      pin(range.address, range.bytes, true);
  }
  void validate(const Memory &source) {
    if (faulted)
      throw FormatError(
          "Private particle owner stopped inside a previous native call");
    if (word(source, 0x51df54) != clock || word(source, clock + 8) != tick ||
        word(source, 0x4f3218) != game || word(source, game + 8) != kind ||
        word(source, 0x4f0414) != tags || word(source, 0x4f3cd8) != random)
      throw FormatError("Private particle inputs require their same admitted "
                        "native tick/layout");
    unsigned n{};
    for (const auto [global, stride] : pools) {
      const auto pool = roots[n++];
      if (word(source, global) != pool)
        throw FormatError("Private particle input pool root differs");
      std::array<std::byte, 0x34> before, after;
      memory.read(pool, before);
      source.read(pool, after);
      if (before != after)
        throw FormatError("Private particle input pool layout differs");
      std::array<std::byte, 10> tail_before, tail_after;
      memory.read(pool + 0x42, tail_before);
      source.read(pool + 0x42, tail_after);
      if (tail_before != tail_after)
        throw FormatError("Private particle input pool backing differs");
    }
  }
};
SourcePrivateParticlePhaseTu15::SourcePrivateParticlePhaseTu15(
    const Memory &source)
    : impl_(std::make_unique<Impl>(source)) {}
SourcePrivateParticlePhaseTu15::~SourcePrivateParticlePhaseTu15() = default;
void SourcePrivateParticlePhaseTu15::advance(
    const Memory &inputs, SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsWorldRuntimeServices &cpu,
    const SourcePhysicsContactRecordObserver &observe) {
  auto &owner = *impl_;
  Borrow borrow(owner.memory, inputs);
  owner.validate(inputs);
  if (abi.gp[4] < 0x10000 || abi.gp[4] > 0xfffeffffu)
    throw FormatError(
        "Private particle caller scratch wraps its native address");
  const auto seconds = std::bit_cast<float>(word(inputs, abi.gp[4] + 4));
  if (!std::isfinite(seconds) || seconds < 0)
    throw FormatError(
        "Private particle caller lacks a finite nonnegative accepted delta");
  owner.memory.stack = {abi.gp[4] - 0x10000, 0x20000};
  // Caller scratch is transient input, not persistent particle state. Copy it
  // from this call's actual input owner; reading the private overlay here
  // would silently retain the preceding call's delta and other arguments.
  owner.memory.caller.resize(owner.memory.stack.bytes);
  inputs.read(owner.memory.stack.address, owner.memory.caller);
  // Actual2E9790 brackets its selected SEH frame through FS:0. This is
  // transient caller state, copied from this genuine ABI's thread owner.
  if (abi.fs_base > 0xfffffffbu)
    throw FormatError("Private particle FS head wraps");
  owner.memory.exception_head = {abi.fs_base, 4};
  inputs.read(abi.fs_base, owner.memory.exception_bytes);
  SourcePhysicsWorldRuntimeServices services;
  services.instruction = [&](auto pc) {
    owner.memory.instruction = pc;
    if (cpu.instruction)
      cpu.instruction(pc);
  };
  services.reciprocal_sqrt = cpu.reciprocal_sqrt;
  services.timestamp_counter = cpu.timestamp_counter;
  services.required_owner = [&](auto &memory, const auto &call, auto &a,
                                const auto &o) {
    if (!a.floating_point)
      throw FormatError("Private particle callee lost its native FP controls");
    const auto fp = *a.floating_point;
    // Match the actual World's precedence for overlapping complete owners.
    if (native_weapon_effect_dispatch_tu15_owns_entry(call.target))
      native_weapon_effect_dispatch_tu15_run(memory, call.target, a, services,
                                             o);
    else if (source_combat_action_tu15_owns_entry(call.target))
      source_run_combat_action_tu15(memory, call.target, a, fp, services, o);
    else if (source_death_cycle_runtime_tu15_owns_entry(call.target))
      source_run_death_cycle_runtime_tu15(memory, call.target, a, fp, services,
                                          o);
    else if (source_physics_live_support_tu15_owns_entry(call.target))
      source_run_physics_live_support_tu15(memory, call.target, a, fp, services,
                                           o);
    else if (source_particle_swarm_tu15_owns_entry(call.target))
      source_run_particle_swarm_tu15(memory, call.target, a, fp, services, o);
    else if (source_effect_particle_frame_tu15_owns_entry(call.target))
      source_run_effect_particle_frame_tu15(memory, call.target, a, fp,
                                            services, o);
    else if (call.target == 0x2e9790u)
      // Actual24DCEE enters the selected TU1.5 instanced collision owner.
      // Its public entry ID remains canonical2E5CE0; selected observations
      // retain original2E9790 instruction/store addresses and the real ABI.
      source_run_physics_world_runtime_selected_observations(
          memory, 0x2e5ce0u, a, SourcePhysicsContactImage::TitleUpdate15, fp,
          services, o);
    else if (call.target == 0x18b1d0u) {
      // Actual18B017's material/event selection follows the World's Phantom
      // composition, including its returned native floating-point controls.
      auto control = fp;
      source_run_physics_phantom_world_tu15_runtime(memory, call.target, a,
                                                    control, services, o);
      a.floating_point = control;
    } else {
      std::ostringstream error;
      error << "Private particle phase requires actual owner 0x" << std::hex
            << call.target << " from 0x" << call.instruction;
      throw FormatError(error.str());
    }
  };
  try {
    native_weapon_effect_dispatch_tu15_run(owner.memory, 0x179df0, abi,
                                           services, observe);
  } catch (...) {
    owner.faulted = true;
    throw;
  }
}
std::vector<SourcePrivateParticlePhaseTu15::Range>
SourcePrivateParticlePhaseTu15::state() const {
  std::vector<Range> result;
  for (const auto &range : impl_->memory.data.ranges()) {
    result.push_back({range.address, range.bytes});
  }
  return result;
}
} // namespace halo2
