#pragma once
#include "halo2/physics_contact_record.hpp"
#include <functional>
#include <mutex>
#include <stdexcept>

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
private:
    std::mutex mutex_;
};
} // namespace halo2
