#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Addresses are retained opaque 32-bit tokens. The four pointer slots must
// have their actual owning storage; this owner rereads them after callbacks.
struct SourceObjectConstructionState {
  std::uint32_t object_pool_slot, tag_table_slot, world_slot, clock_slot;
  // Private C8-byte invocation frame. No initial contents are assumed. Only
  // original locals/temporary manager are written; this is not a Unit body.
  std::uint32_t private_frame;
};
struct ObjectConstructionOperands {
  std::uint32_t one_bits;
  std::uint16_t sentinel_word;
};
// Actual caller SSE state, shared with reached lower owners. Only COMISS
// status bits change here. Unmasked exceptions stop before later stores.
struct ObjectConstructionFloatingPointContext { std::uint32_t mxcsr; };
// Original COMISS against +0, including source DAZ/status controls. Used at
// B90F9/B910C; it throws at an unmasked exception rather than completing the
// subsequent source stores. It does not translate a host/Xbox exception ABI.
bool source_object_construction_positive_scalar(std::uint32_t bits,
    ObjectConstructionFloatingPointContext &);
enum class ObjectConstructionRegister : unsigned { Eax, Ecx, Edx, Ebx, Esi, Edi };
struct ObjectConstructionCall {
  // Canonical TU1.1 identities; complete u5 correspondence is separate evidence.
  std::uint32_t instruction, entry;
  std::array<std::uint32_t,6> registers{};
  unsigned register_mask{};
  std::array<std::uint32_t,5> arguments{};
  unsigned argument_count{};
  bool xmm0_zero{};
};
struct ObjectConstructionStore {
  std::uint32_t instruction, address;
  unsigned bytes;
  std::uint32_t before, after;
  friend bool operator==(const ObjectConstructionStore &,
                         const ObjectConstructionStore &) = default;
};
using ObjectConstructionObserver = std::function<void(const ObjectConstructionStore &)>;
struct SourceGenericObjectConstructionServices {
  std::function<std::span<const std::byte>(std::uint32_t,std::size_t)> read;
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Literal selected-image type record +8 word, not a native sizeof(Unit).
  std::function<std::uint16_t(std::int16_t)> base_extent;
  // Every reached complete lower owner is mandatory. Defined EAX/AL and real
  // memory effects come from that owner; missing providers must stop/throw.
  // Registers listed in register_mask are actual owning inputs only. No
  // undefined caller-saved register contents or synthetic success are added.
  std::function<std::uint32_t(const ObjectConstructionCall &,
                             ObjectConstructionFloatingPointContext &)> invoke;
};
// Complete B8A30 direct owner. Includes allocation/append/type failures and
// cleanup scheduling. This does not itself implement the required physical,
// node/pose, world insertion, activation, effect or resource providers.
std::uint32_t construct_source_object(const SourceObjectConstructionState &,
    std::uint32_t request, const ObjectConstructionOperands &,
    ObjectConstructionFloatingPointContext &,
    const SourceGenericObjectConstructionServices &, const ObjectConstructionObserver & = {});
} // namespace halo2
