#pragma once
#include "halo2/physics_game_arena_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <filesystem>
#include <memory>
#include <stop_token>
namespace halo2 {
struct SourcePhysicsGameArenaSchedule {
  // Optional declared host schedule, used for original differential execution.
  // Hooks do not grant a permit, complete source work or alter source state.
  std::function<void(std::uint32_t,std::stop_token)> before_worker_wait;
  std::function<void()> before_main_yield;
};
struct SourcePhysicsGameArenaConfiguration {
  std::filesystem::path savegame_directory;
  std::uint32_t worker_stack_address;
  SourcePhysicsFactoryFloatingPoint floating_point;
  SourcePhysicsGameArenaSchedule schedule;
};
// Fresh TU1.5 arena resources: native physical pages, mutexes/semaphore,
// immortal source worker and real savegame files. Source instructions own all
// queue links, job completion, CRC, offsets and arena/allocator construction.
// Native ABI/token/stack/priority/scheduling policies are explicit adaptation,
// not Xbox kernel equivalence. The calling bank and main stack are retained
// caller inputs. Stop/join this owner before releasing the bank or callbacks.
class SourcePhysicsGameArenaNative {
public:
  SourcePhysicsGameArenaNative(SourceRuntimeMemory &,
      SourcePhysicsGameArenaConfiguration,const SourcePhysicsWorldRuntimeServices & = {},
      const SourcePhysicsContactRecordObserver & = {});
  ~SourcePhysicsGameArenaNative();
  SourcePhysicsGameArenaNative(const SourcePhysicsGameArenaNative &)=delete;
  SourcePhysicsGameArenaNative &operator=(const SourcePhysicsGameArenaNative &)=delete;
  SourcePhysicsContactMemory &memory();
  void run(std::uint32_t canonical_entry,SourcePhysicsWorldRuntimeAbi &);
  void stop();
  // Protect an existing retained physical allocation; source cache callbacks
  // own the requested address and extent after startup has joined its worker.
  void protect_pages(std::uint32_t address,std::uint32_t bytes,std::uint32_t protection);
private:
  struct Impl;std::unique_ptr<Impl> impl_;
};
} // namespace halo2
