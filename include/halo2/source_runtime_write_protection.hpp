#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
namespace halo2 {
class SourceRuntimeMemory;
class SourceRuntimePolicyChange;
// Policy exclusion only; neither class grants native execution authority or
// retains the borrowed bank. The enclosing owner must keep that bank alive.
// Acquire the hold after installing ordinary write/topology guards and before
// timing/device locks. Downgrade to a change before unregistering those guards.
class SourceRuntimePolicyHold final {
public:
    explicit SourceRuntimePolicyHold(const SourceRuntimeMemory &);
    // Atomically replace the sole active admission, without opening a gap for
    // another publisher during installation or rollback of ordinary guards.
    explicit SourceRuntimePolicyHold(SourceRuntimePolicyChange &);
    ~SourceRuntimePolicyHold();
    SourceRuntimePolicyHold(const SourceRuntimePolicyHold &)=delete;
    SourceRuntimePolicyHold(SourceRuntimePolicyHold &&)=delete;
private:
    const SourceRuntimeMemory *const memory_;
    const std::thread::id executor_=std::this_thread::get_id();
    bool active_=true;
    friend class SourceRuntimePolicyChange;
};
// Keeps admission live through all side effects and outgoing capture retirement.
// Execution-input bindings retain it for their entire lifetime, including on
// other threads. A publication cannot start between admission and mutation.
class SourceRuntimePolicyChange final {
public:
    explicit SourceRuntimePolicyChange(const SourceRuntimeMemory &);
    // Atomic, allocation-free downgrade of this creator's active hold. Keep
    // the returned admission alive through guard removal and capture release.
    explicit SourceRuntimePolicyChange(SourceRuntimePolicyHold &);
    ~SourceRuntimePolicyChange();
    SourceRuntimePolicyChange(const SourceRuntimePolicyChange &)=delete;
    SourceRuntimePolicyChange(SourceRuntimePolicyChange &&)=delete;
private:
    const SourceRuntimeMemory *const memory_;
    const std::thread::id executor_=std::this_thread::get_id();
    bool active_=true;
    friend class SourceRuntimePolicyHold;
};
// Native resource owners can add access constraints without replacing or
// splitting retained banks. They must unregister before the bank dies. Mutable
// borrowed spans must not be retained across a protection transition.
using SourceRuntimeWriteGuard=std::function<void(std::uint32_t,std::size_t)>;
void source_runtime_register_write_guard(const SourceRuntimeMemory &,const void *owner,
    SourceRuntimeWriteGuard);
// For a guard with no effect on writes ending at or below inert_through,
// dispatch skips those writes instead of calling it.
void source_runtime_register_write_guard(const SourceRuntimeMemory &,const void *owner,
    SourceRuntimeWriteGuard,std::uint64_t inert_through);
void source_runtime_unregister_write_guard(const SourceRuntimeMemory &,const void *owner);
void source_runtime_validate_write(const SourceRuntimeMemory &,std::uint32_t,std::size_t);
// Mutation auditing is separate from resource permission. Validation-only
// admission still runs every access policy without claiming a mutable borrow.
void source_runtime_register_write_observer(const SourceRuntimeMemory &,const void *owner,SourceRuntimeWriteGuard);
void source_runtime_unregister_write_observer(const SourceRuntimeMemory &,const void *owner);
void source_runtime_validate_write(const SourceRuntimeMemory &,std::uint32_t,std::size_t,bool observe);
// Equivalent to a write observer that only increments its own counter: counts
// every observed write admitted by all write guards since construction. One
// per-bank counter serves every subscriber, without a callback per store.
class SourceRuntimeWriteSerial final {
public:
    explicit SourceRuntimeWriteSerial(const SourceRuntimeMemory &);
    ~SourceRuntimeWriteSerial();
    SourceRuntimeWriteSerial(const SourceRuntimeWriteSerial &)=delete;
    SourceRuntimeWriteSerial &operator=(const SourceRuntimeWriteSerial &)=delete;
    std::uint64_t load() const noexcept;
    struct Counters;
private:
    const SourceRuntimeMemory *memory_;
    std::shared_ptr<const Counters> counters_;
    std::uint64_t base_{};
};
// Permission is checked for all owners before any topology notification or
// backing change. Observers must not throw and never grant permission.
void source_runtime_register_topology_guard(const SourceRuntimeMemory &,const void *owner,SourceRuntimeWriteGuard);
void source_runtime_unregister_topology_guard(const SourceRuntimeMemory &,const void *owner);
void source_runtime_validate_topology(const SourceRuntimeMemory &,std::uint32_t,std::size_t);
void source_runtime_register_topology_observer(const SourceRuntimeMemory &,const void *owner,SourceRuntimeWriteGuard);
void source_runtime_unregister_topology_observer(const SourceRuntimeMemory &,const void *owner);
void source_runtime_observe_topology(const SourceRuntimeMemory &,std::uint32_t,std::size_t) noexcept;
} // namespace halo2
