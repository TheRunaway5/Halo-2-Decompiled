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
} // namespace halo2
