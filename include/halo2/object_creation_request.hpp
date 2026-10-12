#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
struct ObjectCreationRequestOperands {
  // Immutable executable operands only. Mutable pool/header/body backing is
  // supplied separately by its actual constructor owner.
  std::uint32_t one_bits;
  std::array<std::byte,12> basis_forward, basis_up;
};
struct ObjectCreationRequestServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
};
struct ObjectCreationRequestStore {
  std::uint32_t instruction,address;
  unsigned bytes;
  std::uint32_t before,after;
  friend bool operator==(const ObjectCreationRequestStore &,
                         const ObjectCreationRequestStore &)=default;
};
using ObjectCreationRequestObserver=
    std::function<void(const ObjectCreationRequestStore &)>;

// Complete BBCB0: raw pool high-water, stride, nonzero signed salt, type mask
// and header body pointer. A missing backing provider is an error, not NONE.
std::uint32_t source_object_body_from_datum(std::uint32_t object_pool,
    std::uint32_t datum,std::uint32_t type_mask,
    const ObjectCreationRequestServices &);

// Complete B8820: initialize the original 196-byte request, then propagate
// actual owner's flags/player/team and optional raw 12-byte location. This
// produces a request only; B8A30 allocation/constructors remain separate.
void source_object_creation_request_initialize(std::uint32_t request,
    std::uint32_t definition,std::uint32_t owner,std::uint32_t optional_location,
    std::uint32_t object_pool,const ObjectCreationRequestOperands &,
    const ObjectCreationRequestServices &,const ObjectCreationRequestObserver & = {});
} // namespace halo2
