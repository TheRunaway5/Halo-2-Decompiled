#pragma once
#include "halo2/vista_session_birth.hpp"

namespace halo2 {
// Actual retained transport owner bindings, in original argument4..7 order.
// Words identify the caller's bindings; they are never cast to native pointers.
// owner5 is the source 36-byte-per-session registration bank; owner6 is the
// source four-byte-per-session registry. Both must be genuine caller-owned
// storage with bindings resolving the supplied session/owner words.
struct VistaSessionStorageBindings {
  std::array<std::uint32_t,4> owner_words;
  std::uint32_t session_word;
  std::span<std::byte> owner5;
  std::span<std::byte> owner6;
};
// Literal5C1E65 including actual5E5996 and5BD60C registration writes. Preserves
// every original opaque byte, notably +0,+20,+48. No transport owner, handle,
// key, identity, or quiescent-byte value is constructed. Unsupported owner
// extents and truncated banks reject before any bank changes.
void vista_session_storage_initialize(std::span<std::byte> actual_session,
    std::uint32_t slot,std::uint32_t mode,std::uint32_t network_slot,
    const VistaSessionStorageBindings& actual_bindings);
} // namespace halo2
