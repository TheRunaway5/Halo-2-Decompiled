#pragma once
#include "halo2/physics_contact_record.hpp"
#include <optional>

namespace halo2 {
// Defined inputs of an original owner. Unspecified volatile registers are
// incidental, and are never silently supplied as dependencies.
struct SourceRuntimeCall {
  std::uint32_t entry{}, entry_esp{};
  std::optional<std::uint32_t> eax, ecx, edx, esi, edi;
  std::optional<std::uint32_t> ebx;
  std::vector<std::uint32_t> arguments;
  std::optional<std::uint8_t> dl;
  // Optional masks for stack cells whose callee owns only part of the input.
  // An omitted mask vector means every argument is a full DWORD. Original
  // callers can push uninitialized upper bytes for a callee's byte argument;
  // those bytes are not runtime dependencies and must never be fabricated.
  std::vector<std::uint32_t> argument_masks;
  std::optional<std::uint8_t> cl;
};
using SourceRuntimeOwner = std::function<std::uint32_t(
    SourcePhysicsContactMemory &, const SourceRuntimeCall &,
    const SourcePhysicsContactRecordObserver &)>;
} // namespace halo2
