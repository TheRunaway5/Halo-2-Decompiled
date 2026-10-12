#pragma once
#include "halo2/unit_animation_selection.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
enum class AnimationResolutionOutput { requested, resolved, packed };
struct AnimationResolutionStore {
  std::uint32_t instruction;
  AnimationResolutionOutput output;
  std::size_t offset;
  unsigned bytes;
  std::uint32_t before, after;
  friend bool operator==(const AnimationResolutionStore &,
                         const AnimationResolutionStore &) = default;
};
using AnimationResolutionObserver =
    std::function<void(const AnimationResolutionStore &)>;
struct SourceAnimationResolutionBuffers {
  std::span<std::byte> requested, resolved, packed;
};
struct SourceAnimationResourceServices {
  // Literal runtime518410/518411 bytes from their owning initialization/state.
  // A missing producer is an unavailable service, never an enabled/idle
  // default.
  std::function<std::array<std::uint8_t, 2>()> state;
  // Required complete124e40 when a descriptor's actual byte10 admits demand.
  // Descriptor tokens address mutable source postprocess/residency state.
  std::function<void(std::uint32_t descriptor, std::uint8_t demand)> request;
};
// Complete1e13c0/1e1230/1de9b0: signed packed inheritance/base/variant links,
// authored resource flag and runtime descriptor byte10, preserving call order.
void demand_source_animation(std::uint32_t graph_address, std::uint32_t packed,
                             const SourceAnimationGraphMemory &,
                             const SourceAnimationResourceServices &);
// Complete1deb60. Optional output spans are absent iff their source pointer is
// NULL. ClassNONE writes packedNONE only. Valid requests execute all resource
// sweeps, including repeated requested/resolved root and inherited demands.
void lookup_source_animation_request(
    std::uint32_t graph_address, SourceAnimationIdentifiers,
    std::span<std::byte> packed,
    std::array<std::span<std::byte>, 3> optional_resolved,
    const SourceAnimationGraphMemory &, const SourceAnimationResourceServices &,
    const AnimationResolutionObserver & = {});
// Complete1cfde0, with complete1cfce0 stores. Manager is read-only. Output
// spans must be disjoint; unsupported host aliasing fails explicitly. Defined
// AL is returned, and every required service runs only at its actual source
// branch.
bool resolve_source_animation_request(std::span<const std::byte> manager,
                                      SourceAnimationIdentifiers,
                                      std::uint32_t selection_flags,
                                      SourceAnimationResolutionBuffers,
                                      const SourceAnimationGraphMemory &,
                                      const SourceAnimationResourceServices &,
                                      const AnimationResolutionObserver & = {});

struct SourceAnimationResourceMemory {
  std::function<std::span<const std::byte>(std::uint32_t, std::size_t)> read;
  // Writes must be applied to genuinely owned mutable backing, with bounds.
  std::function<void(std::uint32_t, std::span<const std::byte>)> write;
};
struct SourceAnimationResourceBindings {
  // Original address tokens supplied by the image/runtime binding owner.
  std::uint32_t active_byte, queued_pool_pointer, cache_pointer,
      warning_deadline, priority_table;
};
struct SourceAnimationQueuedAllocation {
  std::uint32_t datum, pool_address;
};
struct SourceAnimationDemandServices {
  SourceAnimationResourceMemory memory;
  SourceAnimationResourceBindings bindings;
  // Exact16d2c0 EAX datum/EDX pool output. Allocation failure is actualNONE.
  std::function<SourceAnimationQueuedAllocation(std::uint32_t pool_address)>
      allocate_queued;
  // Full125120 owner: cache allocation/retirement/IO remain required services.
  std::function<void()> schedule;
  // OriginalGetTickCount provider; exhaustion branch calls twice if required.
  std::function<std::uint32_t()> tick_count;
};
struct AnimationResourceStore {
  std::uint32_t instruction, address, before, after;
  friend bool operator==(const AnimationResourceStore &,
                         const AnimationResourceStore &) = default;
};
using AnimationResourceObserver =
    std::function<void(const AnimationResourceStore &)>;
// Complete124dd0 lookup over actual raw pool header/elements/occupied bitmap.
std::uint32_t
find_source_queued_animation(std::uint32_t descriptor,
                             const SourceAnimationDemandServices &);
// Complete124e40 direct owner. Missing backing/allocator/scheduler/time fails
// at that boundary after preceding source stores. No IO/residency success is
// substituted. This function alone does not create a resource cache or pools.
void request_source_animation_resource(std::uint32_t descriptor,
                                       std::uint8_t demand,
                                       const SourceAnimationDemandServices &,
                                       const AnimationResourceObserver & = {});
} // namespace halo2
