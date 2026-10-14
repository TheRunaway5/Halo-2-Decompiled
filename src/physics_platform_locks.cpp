#include "halo2/physics_platform_locks.hpp"
#include <array>

namespace halo2 {
namespace {
// Authenticated 32-bit platform layout: event +0, count +16, depth +20,
// identity +24. See PHYSICS_PLATFORM_LOCKS_NATIVE.md for primary source pins.
std::uint32_t read_word(const SourcePhysicsContactMemory &memory, std::uint32_t p) {
    std::array<std::byte,4> bytes;
    memory.read(p, bytes);
    std::uint32_t result = 0;
    for (unsigned i=0; i<4; ++i) result |= std::to_integer<std::uint32_t>(bytes[i]) << (i*8);
    return result;
}
void store(SourcePhysicsContactMemory &memory, std::uint32_t p,
    std::uint32_t value, unsigned size, const PhysicsPlatformLockObserver &observe) {
    PhysicsPlatformLockStore change{p, std::vector<std::byte>(size), std::vector<std::byte>(size)};
    memory.read(p, change.before);
    for (unsigned i=0; i<size; ++i) change.written[i] = std::byte(value >> (i*8));
    memory.write(p, change.written);
    if (observe) observe(change);
}
}
PhysicsPlatformLockBoundary::PhysicsPlatformLockBoundary(
    PhysicsPlatformLockBoundaryKind boundary, std::uint32_t p)
    : std::runtime_error("Required native physics platform lock owner unavailable"),
      kind(boundary), critical_section(p) {}
PhysicsPlatformLocks::ActiveCall::ActiveCall(PhysicsPlatformLocks &owner):owner(owner) {
    std::lock_guard gate(owner.call_gate_);
    if(owner.staging_)throw std::runtime_error("Physics critical sections have a prepared restore");
    ++owner.active_calls_;
}
PhysicsPlatformLocks::ActiveCall::~ActiveCall() {
    std::lock_guard gate(owner.call_gate_);--owner.active_calls_;
}

void PhysicsPlatformLocks::initialize(SourcePhysicsContactMemory &memory, std::uint32_t p,
    const PhysicsPlatformLockObserver &observe) {
    ActiveCall call(*this);
    std::lock_guard guard(mutex_);
    known_[&memory].insert(p);
    // Preserve the event's Absolute and Inserted bytes, as the source does.
    store(memory,p,1,1,observe);
    store(memory,p+2,4,1,observe);
    store(memory,p+4,0,4,observe);
    store(memory,p+12,p+8,4,observe);
    store(memory,p+8,p+8,4,observe);
    store(memory,p+16,0xffffffffu,4,observe);
    store(memory,p+20,0,4,observe);
    store(memory,p+24,0,4,observe);
}
void PhysicsPlatformLocks::enter(SourcePhysicsContactMemory &memory, std::uint32_t p,
    const PhysicsPlatformLockServices &services, const PhysicsPlatformLockObserver &observe) {
    ActiveCall call(*this);
    if (!services.current_thread)
        throw PhysicsPlatformLockBoundary(PhysicsPlatformLockBoundaryKind::CurrentThread,p);
    const auto identity = services.current_thread();
    if (!identity) throw std::runtime_error("Native physics platform thread identity must be nonzero");
    std::unique_lock guard(mutex_);
    known_[&memory].insert(p);
    const auto count = read_word(memory,p+16)+1u;
    store(memory,p+16,count,4,observe);
    if (count != 0) {
        const auto owner = read_word(memory,p+24);
        if (owner == identity) {
            store(memory,p+20,read_word(memory,p+20)+1u,4,observe);
            return;
        }
        if (owner != 0) {
            if (!services.wait)
                throw PhysicsPlatformLockBoundary(PhysicsPlatformLockBoundaryKind::Wait,p);
            // The original increment precedes the wait. Do not roll it back
            // or retain the host guard while another thread releases the lock.
            guard.unlock();
            services.wait(p);
            guard.lock();
        }
    }
    store(memory,p+24,identity,4,observe);
    store(memory,p+20,1,4,observe);
}
void PhysicsPlatformLocks::leave(SourcePhysicsContactMemory &memory, std::uint32_t p,
    const PhysicsPlatformLockServices &services, const PhysicsPlatformLockObserver &observe) {
    ActiveCall call(*this);
    std::unique_lock guard(mutex_);
    known_[&memory].insert(p);
    const auto depth = read_word(memory,p+20)-1u;
    store(memory,p+20,depth,4,observe);
    const auto count = read_word(memory,p+16)-1u;
    store(memory,p+16,count,4,observe);
    if (depth == 0) {
        store(memory,p+24,0,4,observe);
        if ((count & 0x80000000u) == 0) {
            if (!services.wake)
                throw PhysicsPlatformLockBoundary(PhysicsPlatformLockBoundaryKind::Wake,p);
            guard.unlock();
            services.wake(p);
        }
    }
}
} // namespace halo2
