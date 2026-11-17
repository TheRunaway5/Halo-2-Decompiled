#pragma once

namespace halo2 {
enum class NativeTu15SolverHullStorage { stride_32,stride_80,stride_4 };

// Destroy the three actual caller-backed buffers in the authored exception
// frame. Each capacity is freshly acquired after the preceding release.
// The live allocator table owns the actual release and its returned ABI.
template<class View>void native_tu15_solver_hull_cleanup_kernel(View&view){
    using Storage=NativeTu15SolverHullStorage;
    view.open_frame();
    if(view.owns_storage(Storage::stride_32))view.release_stride_32();
    if(view.owns_storage(Storage::stride_80))view.release_stride_80();
    if(view.owns_storage(Storage::stride_4))view.release_stride_4();
    view.close_frame();
}
} // namespace halo2
