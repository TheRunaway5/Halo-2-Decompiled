#pragma once
#include "halo2/death_startup_runtime.hpp"
#include "halo2/source_runtime_host_physical_memory.hpp"
#include <filesystem>
#include <memory>
#include <stop_token>

namespace halo2 {
struct SourceDeathStartupNativeSchedule {
  // Optional host scheduling controls for deterministic original comparisons.
  // They run before real acquisition/yield; neither grants a permit nor writes
  // source completion. Normal runtime leaves these empty.
  std::function<void(std::uint32_t, std::stop_token)> before_worker_wait;
  std::function<void()> before_main_yield;
};
struct SourceDeathStartupNativeConfiguration {
  std::filesystem::path savegame_directory;
  std::uint32_t worker_stack_address;
  SourcePhysicsFactoryFloatingPoint floating_point;
  SourceDeathStartupNativeSchedule schedule;
  // Explicit opaque token namespace for reached static virtual allocations.
  // Zero leaves that platform owner unavailable until actually requested.
  std::uint32_t static_allocation_address = 0;
};
// Native resource composition for complete original death/object startup.
// Retains real physical pages, mutexes/semaphore, one immortal source worker
// and read/write savegame cache files. The selected image and main call stack
// remain caller-owned; no game banks, ready flags or construction results are
// imported. Source requests create each resource at its actual call boundary.
// Native stack/TLS/scheduling and Linux nice(-1) for the source priority1
// request are explicit host policies, not Xbox platform equivalence. A real
// priority failure returns false, which the original constructor ignores.
// Stop/join this owner before releasing its bank or callback captures.
class SourceDeathStartupNative {
public:
  SourceDeathStartupNative(SourceRuntimeMemory &,
      SourceDeathStartupNativeConfiguration, SourceDeathWorldSchedulerServices,
      SourcePhysicsContactRecordObserver = {});
  ~SourceDeathStartupNative();
  SourceDeathStartupNative(const SourceDeathStartupNative &) = delete;
  SourceDeathStartupNative &operator=(const SourceDeathStartupNative &) = delete;
  SourcePhysicsContactMemory &memory();
  SourceDeathWorldSchedulerServices services();
  // Native imported-cache file inputs, in encoded-location order: current,
  // mainmenu, shared, single_player_shared. Opens actual readonly files and
  // binds their handles to19D8C0's file selectors. This is a native mount;
  // it does not claim Xbox disk staging19DAD0 ran or publish resource readiness.
  void bind_cache_files(const std::array<std::filesystem::path,4> &);
  void run(std::uint32_t entry, SourceUnitAnimationRuntimeAbi &);
  void stop(); // Native teardown; never a successful source worker return.
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
