#pragma once
#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
namespace halo2 {
// Migration composition: actual collision319D50 residual, then proper native
// world319F90/island320B70/bodyforce3252D0. The caller supplies the true bridge
// continuation and an owned stack interval below its currently active frame.
// The step packet stays above every residual frame for this entire call.
// Actual returned x87 values/FP/MMX context are preserved; no lifted31B1A0 or
//319F90 root and no fabricated dead GPR/XMM/flags postimage exists here.
void native_tu15_step_physics_world_with_residuals(SourcePhysicsContactMemory&,
  NativeTu15PhysicsWorld,const NativeTu15PhysicsStep&,NativeTu15PhysicsFrameFp&,
  SourcePhysicsContactRuntimeAbi& actual_state,std::uint32_t true_continuation,
  NativeTu15PhysicsFrameScratch,const NativeTu15PhysicsFrameResidualCall&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15WorldIntegrationStats* = nullptr,
  std::uint64_t* residual_calls = nullptr,const std::function<std::uint64_t()>& timestamp_counter = {},
  const std::function<std::uint64_t()>& actual_recorded_filetime = {});
} // namespace halo2
