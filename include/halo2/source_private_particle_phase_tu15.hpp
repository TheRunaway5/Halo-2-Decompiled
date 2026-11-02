#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <memory>
#include <vector>

namespace halo2 {
// Private native effect/system/location/emitter/particle state, seeded from
// a quiescent admitted TU1.5 bank. Each call borrows coherent inputs from that
// same native layout and gameplay tick. The caller retains resource, actor,
// view and physics ownership. This owner is not a snapshot admission or a
// replacement for current-client inputs, gameplay or the render queue.
class SourcePrivateParticlePhaseTu15 {
public:
  explicit SourcePrivateParticlePhaseTu15(const SourcePhysicsContactMemory &);
  ~SourcePrivateParticlePhaseTu15();
  SourcePrivateParticlePhaseTu15(const SourcePrivateParticlePhaseTu15 &) =
      delete;
  SourcePrivateParticlePhaseTu15 &
  operator=(const SourcePrivateParticlePhaseTu15 &) = delete;
  // Use an actual prepared native caller: its first stack argument is the
  // finite nonnegative accepted delta. The original179DF0/177830 owners
  // consume it and return the complete ABI. Lower owners execute normally;
  // unsupported dependencies throw and retire this partial private owner.
  void advance(const SourcePhysicsContactMemory &inputs,
               SourcePhysicsWorldRuntimeAbi &,
               const SourcePhysicsWorldRuntimeServices &cpu = {},
               const SourcePhysicsContactRecordObserver &observe = {});
  struct Range {
    std::uint32_t address;
    std::vector<std::byte> bytes;
  };
  // In-process inspection of actual owned bytes, excluding transient caller
  // stack. These ranges neither serialize authority nor authorize imports.
  std::vector<Range> state() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
