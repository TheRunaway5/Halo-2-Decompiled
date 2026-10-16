#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
namespace halo2 {
class SourceRuntimeMemory;
// Native resource owners can add access constraints without replacing or
// splitting retained banks. They must unregister before the bank dies. Mutable
// borrowed spans must not be retained across a protection transition.
using SourceRuntimeWriteGuard=std::function<void(std::uint32_t,std::size_t)>;
void source_runtime_register_write_guard(const SourceRuntimeMemory &,const void *owner,
    SourceRuntimeWriteGuard);
void source_runtime_unregister_write_guard(const SourceRuntimeMemory &,const void *owner);
void source_runtime_validate_write(const SourceRuntimeMemory &,std::uint32_t,std::size_t);
// Mutation auditing is separate from resource permission. Validation-only
// admission still runs every access policy without claiming a mutable borrow.
void source_runtime_register_write_observer(const SourceRuntimeMemory &,const void *owner,SourceRuntimeWriteGuard);
void source_runtime_unregister_write_observer(const SourceRuntimeMemory &,const void *owner);
void source_runtime_validate_write(const SourceRuntimeMemory &,std::uint32_t,std::size_t,bool observe);
// Permission is checked for all owners before any topology notification or
// backing change. Observers must not throw and never grant permission.
void source_runtime_register_topology_guard(const SourceRuntimeMemory &,const void *owner,SourceRuntimeWriteGuard);
void source_runtime_unregister_topology_guard(const SourceRuntimeMemory &,const void *owner);
void source_runtime_validate_topology(const SourceRuntimeMemory &,std::uint32_t,std::size_t);
void source_runtime_register_topology_observer(const SourceRuntimeMemory &,const void *owner,SourceRuntimeWriteGuard);
void source_runtime_unregister_topology_observer(const SourceRuntimeMemory &,const void *owner);
void source_runtime_observe_topology(const SourceRuntimeMemory &,std::uint32_t,std::size_t) noexcept;
} // namespace halo2
