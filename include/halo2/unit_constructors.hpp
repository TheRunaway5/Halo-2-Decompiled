#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
enum class UnitConstructorBuffer { Manager, Header, Unit, Failure };
struct UnitConstructorStore {
  std::uint32_t instruction;
  UnitConstructorBuffer buffer;
  std::size_t offset;
  unsigned bytes;
  std::uint32_t before, after;
  friend bool operator==(const UnitConstructorStore &,
                         const UnitConstructorStore &) = default;
};
using UnitConstructorObserver =
    std::function<void(const UnitConstructorStore &)>;
struct UnitConstructorLeaf {
  std::uint32_t entry;
  // EAX is represented only where it is an actual owning input. ECX of
  // 1d0a60 is the supplied manager span, not an invented guest pointer.
  std::uint32_t eax{};
  bool eax_input{};
  std::array<std::uint32_t, 7> arguments{};
  unsigned count{};
  friend bool operator==(const UnitConstructorLeaf &,
                         const UnitConstructorLeaf &) = default;
};
struct UnitConstructorObjectPool {
  // Original 12-byte headers. These routines index low16 directly; salted
  // object validity is the caller's prerequisite, not a check they perform.
  std::span<std::byte> headers;
  std::uint32_t capacity{};
  std::function<std::span<std::byte>(std::uint16_t index)> body;
  // Actual18fd70 contract: EAX signed new size, EBX=&header+8, stack(heap,0,0).
  // The storage owner may relocate the body and update header+8. It must not
  // change header+6. No allocation-success or backing default is provided.
  std::function<bool(std::uint32_t datum, std::int32_t requested_bytes)> grow;
};
struct UnitConstructorServices {
  UnitConstructorObjectPool objects;
  // Literal TU1.1 original465030 word imported from the selected executable.
  // Constructors perform no arithmetic with it: they copy its exact bits.
  std::uint32_t one_bits;
  std::function<std::int16_t(std::uint32_t graph)> graph_nodes;
  std::function<std::int32_t(std::uint32_t model)> model_nodes;
  // Required1d0a60 owner; invokes its actual graph/default selection.
  std::function<void(std::span<std::byte> manager, const UnitConstructorLeaf &)>
      select_default;
  std::function<std::span<const std::byte>(std::uint32_t tag)> unit_definition;
  // Full source model node block: signed count+78 and records stride5c.
  std::function<std::span<const std::byte>(std::uint32_t model)>
      model_node_records;
  std::function<std::uint32_t()> game_type;
  // Required110a20 /116680 /110bf0 owners. Their return values are ignored.
  std::function<void(const UnitConstructorLeaf &)> invoke;
};

// Complete1ce8c0 (including its tail-jumped1ce9b0) and complete1ce9b0.
// Both preserve manager84..8f. Reset additionally preserves bytes62 and66.
void construct_source_unit_animation_manager(
    std::span<std::byte> manager, std::uint32_t one_bits,
    const UnitConstructorObserver & = {});
void reset_source_unit_animation_manager(std::span<std::byte> manager,
                                         std::uint32_t one_bits,
                                         const UnitConstructorObserver & = {});
// Complete1ceac0 direct owner; nested default selection remains required.
bool bind_source_unit_animation_manager(std::span<std::byte> manager,
                                        std::uint32_t graph,
                                        std::uint32_t model, bool flag,
                                        const UnitConstructorServices &,
                                        const UnitConstructorObserver & = {});
// CompleteBD270: signed descriptor/size, alignment-byte shift and padded
// append clearing. Host bounds failures throw; there is no fabricated AL.
bool append_source_unit_component(std::uint32_t unit, std::int16_t descriptor,
                                  std::int16_t bytes, std::uint8_t alignment,
                                  const UnitConstructorObjectPool &,
                                  const UnitConstructorObserver & = {});
// CompleteC52C0 direct owner, including all three append requests. The unit
// must already have source-owned body/animation-manager storage from B8A30.
// No full body fill, graph binding, pose or successful nested service is added.
bool construct_source_unit(std::uint32_t unit,
                           std::span<const std::byte> descriptor,
                           std::span<std::byte> failure,
                           const UnitConstructorServices &,
                           const UnitConstructorObserver & = {});
} // namespace halo2
