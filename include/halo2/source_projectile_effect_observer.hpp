#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
namespace halo2 {
class SourceRuntimeMemory;
class SourcePhysicsContactMemory;
struct SourceProjectileDetonationTu15;
struct SourceProjectileEffectListener;
// Scoped read-only observation of actual FDEB0/FFA60 contact-effect creation.
// The token must retire before its source world or callback target.
class SourceProjectileEffectObserver {
public:
    SourceProjectileEffectObserver(SourceRuntimeMemory &,
        std::function<void(const SourceProjectileDetonationTu15 &)>);
    ~SourceProjectileEffectObserver();
    SourceProjectileEffectObserver(const SourceProjectileEffectObserver &)=delete;
private:
    std::shared_ptr<SourceProjectileEffectListener> listener_;
};
struct SourceProjectileContactCapture {
    std::uint32_t projectile{},definition{},effect{},player{},unit{};
    std::shared_ptr<SourceProjectileEffectListener> listener;
};
class SourceProjectileUpdateScope {
public:
    SourceProjectileUpdateScope(SourcePhysicsContactMemory &,std::uint32_t actual_projectile);
    ~SourceProjectileUpdateScope();
    SourceProjectileUpdateScope(const SourceProjectileUpdateScope &)=delete;
private:
    const SourcePhysicsContactMemory *memory_{};
};
std::optional<SourceProjectileContactCapture> source_begin_projectile_contact_effect_tu15(
    SourcePhysicsContactMemory &,std::uint32_t original_frame,std::uint32_t original_stack,
    std::uint32_t actual_call=0x101fd1u);
void source_complete_projectile_contact_effect_tu15(SourcePhysicsContactMemory &,
    const SourceProjectileContactCapture &,std::uint32_t actual_instance);
}
