#pragma once
#include "halo2/source_runtime_memory.hpp"
#include <array>
#include <filesystem>

namespace halo2 {
// Actual externally owned kernel export322 data. These bytes are input from
// the caller's named kernel/hardware resource, never a selected flag profile.
// The loader copies them into immutable retained backing and resolves only
// TU1.5's authenticated data import cell. This does not execute startup.
struct SourceRuntimeKernelHardwareInfo {
  std::uint32_t address;
  std::array<std::byte, 8> bytes;
  std::string owner;
};
// TU1.1/TU1.5 ordinal156 is a volatile DWORD data import. Its native owner supplies
// retained mutable backing and refreshes it from an actual monotonic clock.
struct SourceRuntimeKernelTickCount {
  std::uint32_t address;
  std::array<std::byte,4> bytes;
  std::string owner;
};
// Authenticated selected TU1.1/u5 image backing at original virtual addresses, with
// section-declared writable data and loader-zeroed virtual tails. The literal
// XBE header is mounted read-only as a native input policy, not a recovered
// Xbox header-page protection contract. This imports
// operands/method tables/BSS storage only. Every startup initializer, pointer
// binding, pool, tag loader and runtime constructor remains an actual owner.
// Algorithms must select the same image as the retained bank.
SourceRuntimeMemory load_source_runtime_image(const std::filesystem::path &,
                                             SourcePhysicsContactImage);
SourceRuntimeMemory load_source_runtime_image(const std::filesystem::path &,
    SourcePhysicsContactImage, const SourceRuntimeKernelHardwareInfo &);
SourceRuntimeMemory load_source_runtime_image(const std::filesystem::path &,
    SourcePhysicsContactImage, const SourceRuntimeKernelTickCount &);
// Existing explicit TU1.1 entry keeps its selected-image contract.
SourceRuntimeMemory
load_source_runtime_image_1_1(const std::filesystem::path &);
} // namespace halo2
