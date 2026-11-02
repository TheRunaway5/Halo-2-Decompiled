#pragma once
#include "halo2/native_tu15_convex_simplex_solver.hpp"
#include <utility>
namespace halo2 {
// Synchronous read-only diagnostics for the current executor. The observer
// receives ordinary typed inputs before the solver writes; no CPU register,
// private control frame, callback result or replacement owner is exposed.
using NativeTu15ConvexSimplexInputObserver=std::function<void(
    const SourcePhysicsContactMemory&,const NativeTu15ConvexSimplexSolverInput&,
    const NativeTu15PhysicsFrameFp&,unsigned)>;
inline NativeTu15ConvexSimplexInputObserver&native_tu15_convex_simplex_input_observer(){
    static thread_local NativeTu15ConvexSimplexInputObserver current;return current;
}
class NativeTu15ConvexSimplexInputObserverBinding {
    NativeTu15ConvexSimplexInputObserver previous_;
public:
    explicit NativeTu15ConvexSimplexInputObserverBinding(NativeTu15ConvexSimplexInputObserver observer)
        :previous_(std::exchange(native_tu15_convex_simplex_input_observer(),std::move(observer))){}
    ~NativeTu15ConvexSimplexInputObserverBinding(){native_tu15_convex_simplex_input_observer()=std::move(previous_);}
    NativeTu15ConvexSimplexInputObserverBinding(const NativeTu15ConvexSimplexInputObserverBinding&)=delete;
};
} // namespace halo2
