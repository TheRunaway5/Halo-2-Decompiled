#pragma once
#include "halo2/unit_constructors.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Original selected-image type records and their +84 callback banks. Every
// pointer is an opaque32-bit original address, resolved only by bounded owned
// backing. These are not the separate60-entry unit-action descriptors.
struct SourceObjectConstructionMetadata {
  // Actual tag-table group word indexed by signed low16, as10CA30 reads it.
  // This is not the object definition's first signed type field.
  std::function<std::uint32_t(std::int16_t index)> tag_group;
  std::function<std::uint32_t(std::int16_t type)> type_address;
  std::function<std::span<const std::byte>(std::uint32_t, std::size_t)> read;
};
enum class ObjectConstructionCallback { Prepare, Construct };
struct SourceObjectConstructionCall {
  std::uint32_t entry, unit;
  ObjectConstructionCallback callback;
  // The same retained C4 request throughout the owning wrapper. Construct's
  // separate failure byte is live mutable storage, not a manufactured result.
  std::span<std::byte> request;
  std::span<std::byte> failure;
};
struct SourceObjectConstructionServices {
  UnitConstructorObjectPool objects;
  SourceObjectConstructionMetadata metadata;
  // Required actual complete callback. Prepare ignores its scalar; Construct
  // consumes low AL. Never return success or replay final body bytes in live
  // providers. Header/body/descriptor backing must remain source-valid.
  std::function<std::uint8_t(const SourceObjectConstructionCall &)> invoke;
};

// Complete10CA30: group match in all13 original type records. Returns-1 when
// no group matches; its actual index input is signed low16 of the definition.
std::int16_t
source_object_definition_type(std::uint32_t definition,
                              const SourceObjectConstructionMetadata &);
// Complete10CBD0. Walks actual +84 pointers through its first NULL, calls each
// non-NULL +28 method in original order with the same mutable request.
void prepare_source_object_type(std::span<std::byte> request,
                                const SourceObjectConstructionServices &);
// Complete10CC20. Reads source-produced signed body+AA; scans every bank slot
// even after a false callback, but makes no subsequent callback once false.
// Internally indexes low16, without adding a salted caller-admission gate.
bool construct_source_object_type(std::uint32_t unit,
                                  std::span<std::byte> request,
                                  std::span<std::byte> failure,
                                  const SourceObjectConstructionServices &);
} // namespace halo2
