#pragma once
#include "halo2/native_tu15_constraint_step.hpp"
#include "halo2/native_tu15_constraint_solver.hpp"
#include "halo2/native_tu15_constraint_solver_initializer.hpp"
#include "halo2/native_tu15_constraint_custom_schema.hpp"
#include "halo2/native_tu15_constraint_clock.hpp"
#include "halo2/native_tu15_constraint_base.hpp"
#include <optional>
namespace halo2 {
// Migration transport for named unconverted callees only. Return values must
// come from that actual owner; this interface supplies no physics outcomes.
using NativeTu15ConstraintResidualCall=std::function<std::uint32_t(
  std::uint32_t owner,std::span<const std::uint32_t> arguments,
  std::optional<std::uint32_t> receiver,unsigned callee_argument_bytes,
  NativeTu15PhysicsFrameFp&)>;
struct NativeTu15ConstraintBindingActivity {
  NativeTu15ConstraintStepStats step;
  std::uint64_t native_dynamic_setups{},native_motion_commits{},residual_calls{};
  std::array<std::uint64_t,3> native_motion_commit_kinds{};
  NativeTu15ConstraintSolverStats solver;
  NativeTu15ConstraintSolverInitializerStats initializer;
  NativeTu15ConstraintCustomSchemaStats custom_schemas;
  NativeTu15ConstraintProducerStats producers;
  NativeTu15ConstraintBaseStats base;
  // Count completed direct emissions only after the enclosing native builder
  // returns. These are distinct from the generated32A390 ABI scope counters.
  std::array<std::uint64_t,3> completed_producer_emissions{};
  std::uint64_t custom_schema_refusals{};
  std::uint64_t native_clock_calls{};
};
// The caller owns sixteen ordinary writable bytes for the retained mutable
// builder packet, outside its active residual call/argument frame.
void native_tu15_step_island_constraints_with_residuals(SourcePhysicsContactMemory&,
  const NativeTu15PhysicsConstraintInput&,NativeTu15PhysicsFrameFp&,
  unsigned actual_preserved_x87_depth,std::uint32_t retained_builder_packet,
  const NativeTu15ConstraintResidualCall&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintBindingActivity* = nullptr,
  const NativeTu15SystemFiletimeSource& actual_recorded_filetime = {});
} // namespace halo2
