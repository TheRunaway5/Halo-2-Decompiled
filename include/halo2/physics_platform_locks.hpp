#pragma once
#include "halo2/physics_contact_record.hpp"
#include <functional>
#include <mutex>
#include <stdexcept>
#include <map>
#include <set>
#include <memory>
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Native platform operations for the source-visible 28-byte critical section.
// The layout and transition contract are checked against pinned compatibility
// source, not recovered Xbox kernel instructions. Thread values are opaque
// equality tokens supplied by a real host identity owner, never KTHREAD records.
enum class PhysicsPlatformLockBoundaryKind { CurrentThread, Wait, Wake };
struct PhysicsPlatformLockBoundary : std::runtime_error {
    PhysicsPlatformLockBoundaryKind kind;
    std::uint32_t critical_section;
    PhysicsPlatformLockBoundary(PhysicsPlatformLockBoundaryKind, std::uint32_t);
};
struct PhysicsPlatformLockServices {
    std::function<std::uint32_t()> current_thread;
    // Exact required wait: this critical section's event, reason/mode zero,
    // non-alertable, no timeout. The owner returns only after satisfying it.
    std::function<void(std::uint32_t)> wait;
    // Exact required set-event: this event, priority increment one, Wait false.
    std::function<void(std::uint32_t)> wake;
};
struct PhysicsPlatformLockStore {
    std::uint32_t address;
    std::vector<std::byte> before, written;
    bool operator==(const PhysicsPlatformLockStore &) const = default;
};
using PhysicsPlatformLockObserver = std::function<void(const PhysicsPlatformLockStore &)>;
class PhysicsPlatformLocks {
public:
    // Callers sharing source memory must share this synchronization owner.
    // These operations do not own an x86 callee ABI or manufacture register
    // clobbers. That adapter requires separate original caller evidence.
    void initialize(SourcePhysicsContactMemory &, std::uint32_t,
        const PhysicsPlatformLockObserver & = {});
    void enter(SourcePhysicsContactMemory &, std::uint32_t,
        const PhysicsPlatformLockServices &, const PhysicsPlatformLockObserver & = {});
    void leave(SourcePhysicsContactMemory &, std::uint32_t,
        const PhysicsPlatformLockServices &, const PhysicsPlatformLockObserver & = {});
    struct OwnershipCheckpoint {
        std::uint32_t version=1;
        std::vector<std::uint32_t> critical_sections;
        bool operator==(const OwnershipCheckpoint &) const = default;
    };
    class PreparedOwnership {
    public:
        ~PreparedOwnership();
        PreparedOwnership(PreparedOwnership &&) noexcept;
        PreparedOwnership &operator=(PreparedOwnership &&) noexcept;
        bool for_owner(const PhysicsPlatformLocks &,const SourceRuntimeMemory &) const noexcept;
    private:
        friend class PhysicsPlatformLocks;
        PreparedOwnership();
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
    // Export only at the enclosing world's barrier. Consumed critical-section
    // records must remain alive; held/contended records and active native calls
    // reject. Equality tokens/record bytes remain in the owned source bank.
    OwnershipCheckpoint ownership_checkpoint(const SourcePhysicsContactMemory &) const;
    PreparedOwnership prepare_ownership(SourceRuntimeMemory &,const OwnershipCheckpoint &,
                                        const SourceRuntimeMemory::Checkpoint &);
    // Publish only after the admitted bank. Keep the local mutex/services;
    // commit/cancel on the preparing barrier thread. Other banks stay local.
    bool commit_ownership(PreparedOwnership &&) noexcept;
private:
    struct ActiveCall {
        PhysicsPlatformLocks &owner;
        explicit ActiveCall(PhysicsPlatformLocks &);
        ~ActiveCall();
    };
    mutable std::mutex mutex_;
    mutable std::recursive_mutex call_gate_;
    std::size_t active_calls_{};
    bool staging_{};
    std::map<const SourcePhysicsContactMemory *,std::set<std::uint32_t>> known_;
};
} // namespace halo2
