#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <functional>
namespace halo2 {
struct SourceFirstPersonNativeActionTu15 {
    std::uint32_t tick,unit,weapon,view,action,hand;
    std::uint32_t instruction{};
};
// Passive observer of the authentic F0D50/F0BE0 ->16B736 entry boundaries.
// Its scope borrows the exact bank, and neither ABI nor source state is changed.
class SourceFirstPersonActionObserverTu15 {
public:
    using Observer=std::function<void(const SourcePhysicsContactMemory&,
        const SourcePhysicsWorldRuntimeAbi&,const SourcePhysicsWorldRuntimeServices&,
        const SourceFirstPersonNativeActionTu15&)>;
    using Phase=std::function<void(const SourcePhysicsContactMemory&,
        const SourcePhysicsWorldRuntimeAbi&,const SourcePhysicsWorldRuntimeServices&)>;
    SourceFirstPersonActionObserverTu15(const SourcePhysicsContactMemory&,Observer,Phase={});
    ~SourceFirstPersonActionObserverTu15();
    SourceFirstPersonActionObserverTu15(const SourceFirstPersonActionObserverTu15&)=delete;
    SourceFirstPersonActionObserverTu15&operator=(const SourceFirstPersonActionObserverTu15&)=delete;
    static void observe(const SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
        const SourcePhysicsWorldRuntimeAbi&,const SourcePhysicsWorldRuntimeServices&);
    static void entry(const SourcePhysicsContactMemory&,const SourcePhysicsWorldRuntimeAbi&,
        const SourcePhysicsWorldRuntimeServices&);
    static bool phase_bound(const SourcePhysicsContactMemory&)noexcept;
    static void phase(const SourcePhysicsContactMemory&,const SourcePhysicsWorldRuntimeAbi&,
        const SourcePhysicsWorldRuntimeServices&);
private:
    const SourcePhysicsContactMemory &memory_;
    Observer observer_;
    Phase phase_;
    SourceFirstPersonActionObserverTu15 *previous_;
};
// Read-only inputs at the authentic169C46 ->16A4DD boundary. The temporal
// owner advances its shared feedback clock after composing both hands, so a
// later completed-state capture is not an equivalent sampling input.
class SourceFirstPersonCompositionObserverTu15 {
public:
    using Observer=std::function<void(const SourcePhysicsContactMemory&,
        const SourcePhysicsWorldRuntimeAbi&,unsigned view,unsigned hand)>;
    SourceFirstPersonCompositionObserverTu15(const SourcePhysicsContactMemory&,Observer);
    ~SourceFirstPersonCompositionObserverTu15();
    SourceFirstPersonCompositionObserverTu15(const SourceFirstPersonCompositionObserverTu15&)=delete;
    SourceFirstPersonCompositionObserverTu15&operator=(const SourceFirstPersonCompositionObserverTu15&)=delete;
    static void entry(const SourcePhysicsContactMemory&,const SourcePhysicsWorldRuntimeAbi&);
private:
    const SourcePhysicsContactMemory &memory_;
    Observer observer_;
    SourceFirstPersonCompositionObserverTu15 *previous_;
};
}
