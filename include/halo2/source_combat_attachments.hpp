#pragma once
#include <cstdint>
#include <memory>
#include <optional>
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
    struct Impl;
public:
    SourceCombatAttachments(const Cache &,NativeWorldEffects &,
        std::span<const std::uint32_t> object_definitions);
    ~SourceCombatAttachments();
    std::set<std::uint32_t> synchronize(MultiplayerObjectRuntime &,std::uint64_t tick);
    std::vector<std::string> unavailable() const;
    class Checkpoint {
    public:
        std::optional<std::uint64_t> tick()const noexcept;
        std::size_t active_instances()const noexcept;
    private:
        friend class SourceCombatAttachments;
        struct Data;std::shared_ptr<const Data> data_;
    };
    class PreparedRestore {
    public:
        ~PreparedRestore();
        PreparedRestore(PreparedRestore &&)noexcept;
        PreparedRestore &operator=(PreparedRestore &&)noexcept;
    private:
        friend class SourceCombatAttachments;
        PreparedRestore();std::unique_ptr<Impl> replacement_;
        std::shared_ptr<const int> owner_;
    };
    // Restore only with the same local world-effect owner's matching state.
    // Neither snapshot nor preparation calls into the source world or renderer.
    Checkpoint checkpoint()const;
    PreparedRestore prepare_restore(const Checkpoint &)const;
    bool restore_ready(const PreparedRestore &)const noexcept;
    bool commit_restore(PreparedRestore &&)noexcept;
private:
    std::unique_ptr<Impl> impl_;
    std::shared_ptr<const int> owner_=std::make_shared<const int>(0);
};
}
