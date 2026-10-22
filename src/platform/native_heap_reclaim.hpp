#pragma once
#include <cstdlib>
#if defined(__GLIBC__)
#include <malloc.h>
#endif

namespace halo2 {
// Advisory cleanup of freed host allocator buffers at resource publication
// boundaries. Live banks and their native addresses retain their owners.
inline void reclaim_unused_host_heap_pages() {
#if defined(__GLIBC__)
  (void)::malloc_trim(0);
#endif
}
} // namespace halo2
