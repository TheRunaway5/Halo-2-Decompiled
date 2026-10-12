#pragma once
#include "halo2/unit_animation_resolution.hpp"
#include <functional>
namespace halo2 {
struct SourceAnimationAsyncBindings {
  // Genuinely owned32-bit address tokens from the selected image/storage owner.
  std::uint32_t nodes,free_head,work_head,free_mutex_slot,work_mutex_slot,
      semaphore_slot,worker_slot,worker_storage_descriptor,worker_storage,
      worker_entry;
};
struct SourceAnimationAsyncConstructionServices {
  SourceAnimationResourceMemory memory;
  SourceAnimationAsyncBindings bindings;
  // Explicit native adaptations at the XAPI call boundary. Actual owned alias
  // or actual creation failure0; these do not assert Xbox NTSTATUS/TLS parity.
  std::function<std::uint32_t(std::uint32_t security,
      std::uint32_t initial_owner,std::uint32_t name)> native_create_mutex;
  std::function<std::uint32_t(std::uint32_t security,
      std::uint32_t initial_count,std::uint32_t maximum_count,
      std::uint32_t name)> native_create_semaphore;
  // Real worker/TLS/callback ownership required before either is installed.
  std::function<std::uint32_t(std::uint32_t security,
      std::uint32_t stack_bytes,std::uint32_t entry,std::uint32_t context,
      std::uint32_t flags,std::uint32_t thread_id_out)> native_create_worker;
  std::function<void(std::uint32_t actual_worker,std::int32_t priority)>
      native_set_worker_priority;
};
// Complete1221B0 CPU store/call sequence. It initializes all150x64 node bytes,
// preserves both counters, and stores actual supplied creation outcomes even0.
// Missing required owner stops after all preceding original stores. No handles,
// readiness, worker, file completion, body offset or successful result default.
void construct_source_animation_async_queue(
    const SourceAnimationAsyncConstructionServices &,
    const AnimationResourceObserver & = {});
} // namespace halo2
