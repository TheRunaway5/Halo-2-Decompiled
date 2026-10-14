#pragma once
#include <cstdint>
#include <memory>
#include <set>
#include <span>
#include <string>
#include <vector>
namespace halo2 {
class Cache;
class MultiplayerObjectRuntime;
class NativeWorldEffects;
// Presentation of completed original weapon/projectile attachment slots.
// Functions, effect allocation, controls, colors and locations belong to the
// source world; this adapter neither creates source effects nor applies damage.
class SourceCombatAttachments {
public:
    SourceCombatAttachments(const Cache &,NativeWorldEffects &,
        std::span<const std::uint32_t> object_definitions);
    ~SourceCombatAttachments();
    std::set<std::uint32_t> synchronize(MultiplayerObjectRuntime &,std::uint64_t tick);
    std::vector<std::string> unavailable() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
