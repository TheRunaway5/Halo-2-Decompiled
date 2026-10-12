#pragma once
#include "halo2/animation_async_constructor.hpp"
#include "halo2/source_bitmap_async_read.hpp"
#include <functional>
namespace halo2 {
struct SourceAnimationAsyncWorkerBindings {
  SourceAnimationAsyncBindings queue;
  std::uint32_t counter,requests_added,current_node,read_callback;
};
struct SourceAnimationAsyncStore {
  std::uint32_t instruction,address,before,after;
  unsigned bytes;
  friend bool operator==(const SourceAnimationAsyncStore &,
                         const SourceAnimationAsyncStore &)=default;
};
using SourceAnimationAsyncObserver=std::function<void(const SourceAnimationAsyncStore &)>;
struct SourceAnimationAsyncWorkerServices {
  SourceAnimationResourceMemory memory;
  SourceAnimationAsyncWorkerBindings bindings;
  // Named native synchronization adaptation, not Xbox wait/status emulation.
  // A wait must own the actual permit/exclusion before returning. Failure or
  // host shutdown throws BEFORE the source caller's subsequent stores. The
  // original ignores wrapper status; returning a failure cannot grant access.
  std::function<void(std::uint32_t handle,std::uint32_t milliseconds)> wait;
  std::function<void(std::uint32_t handle)> release_mutex;
  std::function<void(std::uint32_t handle,std::uint32_t count)> release_semaphore;
  std::function<void()> yield;
  // Full actual callback dispatch. Payload is its original node+8 token.
  // Missing callback/file/IO ownership must stop, never return success.
  std::function<std::int32_t(std::uint32_t callback,std::uint32_t payload)> callback;
};
// Complete121FD0/122020/1220E0/122150 over actual retained raw pointers.
// Null is0; node IDs are source indices/salted counter words, never host slots.
std::uint32_t lease_source_animation_async_node(const SourceAnimationAsyncWorkerServices &,
    const SourceAnimationAsyncObserver & = {});
void insert_source_animation_async_node(std::uint32_t node,
    const SourceAnimationAsyncWorkerServices &,const SourceAnimationAsyncObserver & = {});
std::uint32_t publish_source_animation_async_node(std::uint32_t node,
    const SourceAnimationAsyncWorkerServices &,const SourceAnimationAsyncObserver & = {});
void unlink_source_animation_async_node(std::uint32_t node,
    const SourceAnimationAsyncWorkerServices &,const SourceAnimationAsyncObserver & = {});
// Complete19D8C0 file-bank routing and1A4870 ten-word request/bind/publish.
// File tokens/paths and source buffer/done backing remain actual owner inputs.
std::uint32_t submit_source_animation_async_read(const SourceBitmapReadRequest &,
    const SourceBitmapFileBank &,const SourceAnimationAsyncWorkerServices &,
    const SourceAnimationAsyncObserver & = {});
// Complete immortal122360. There is no source shutdown or successful return.
[[noreturn]] void run_source_animation_async_worker(const SourceAnimationAsyncWorkerServices &,
    const SourceAnimationAsyncObserver & = {});
// Complete1A47D0 CPU stores/result using the existing real owned-file backend.
// Caller provides its actual file token/path binding, buffer backing and chunk
// limit. This adapter requires disjoint task metadata/IO buffer backing and
// rejects aliasing before IO. Done/result1 is terminal, including failed/short reads.
using SourceAnimationAsyncFileObserver=std::function<void(const SourceAsyncReadExecution &)>;
SourceAsyncReadExecution read_source_animation_async_file(std::uint32_t payload,
    std::uint32_t actual_chunk_limit,const SourceAsyncOwnedBuffer &,SourceAsyncOwnedFiles &,
    const SourceAnimationResourceMemory &,const SourceAnimationAsyncObserver & = {},
    const SourceAnimationAsyncFileObserver & = {});
} // namespace halo2
