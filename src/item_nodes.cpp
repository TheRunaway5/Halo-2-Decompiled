#include "halo2/item_nodes.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
template<class T> T read(std::span<const std::byte> bytes, std::size_t offset) {
  if (offset + sizeof(T) > bytes.size()) throw std::runtime_error("Incomplete original item node storage");
  T value; std::memcpy(&value, bytes.data() + offset, sizeof value); return value;
}
void store(std::span<std::byte> bytes, bool fallback, std::size_t offset,
    std::uint32_t value, std::uint32_t instruction, const SourceItemNodeObserver &observer) {
  SourceMatrixMathStore event{instruction, offset,
      {bytes.begin() + offset, bytes.begin() + offset + 4}, std::vector<std::byte>(4)};
  std::memcpy(event.after.data(), &value, 4); std::memcpy(bytes.data() + offset, &value, 4);
  if (observer) observer({fallback, event});
}
} // namespace
void source_item_current_nodes(SourceItemNodesStorage storage,
    const SourceNodeMatrixInputs &input, std::span<const std::uint32_t> callbacks,
    const SourceItemNodeObserver &observer) {
  auto body = storage.body;
  if (read<std::uint32_t>(body, 0x14) != 0xffffffffu ||
      read<std::uint32_t>(body, 0xb4) != 0xffffffffu ||
      read<std::uint32_t>(body, 0xd4) != 0xffffffffu ||
      read<std::uint8_t>(storage.definition, 0) != 2)
    throw std::runtime_error("Original parent/physical/network/type node producer unavailable");
  if (read<std::uint32_t>(storage.definition, 0x38) == 0xffffffffu ||
      read<std::uint32_t>(storage.model, 4) == 0xffffffffu)
    throw std::runtime_error("Unmeasured original model-NONE node branch");
  const auto current_size = read<std::int16_t>(body, 0x114);
  const auto current_offset = read<std::int16_t>(body, 0x116);
  const auto count = read<std::int32_t>(storage.render_model, 0x48);
  if (count <= 0 || current_size < count * 0x34 || current_offset < 0 ||
      unsigned(current_offset) + unsigned(current_size) > body.size() ||
      std::size_t(count) * 0x60 > storage.render_nodes.size())
    throw std::runtime_error("Original full current/render node bank unavailable");
  const auto flags = read<std::uint32_t>(body, 4) & 0xbfffffffu;
  store(body, false, 4, flags, 0xbdfd8, observer);
  const auto local_offset = read<std::int16_t>(body, 0x112);
  std::span<std::byte> local;
  if (local_offset == -1) local = storage.fallback_nodes;
  else {
    const auto size = read<std::int16_t>(body, 0x110);
    if (size < count * 0x20 || local_offset < 0 || unsigned(local_offset) + unsigned(size) > body.size())
      throw std::runtime_error("Original local item node bank unavailable");
    local = body.subspan(unsigned(local_offset), unsigned(size));
  }
  if (std::size_t(count) * 0x20 > local.size())
    throw std::runtime_error("Incomplete original fallback item node bank");
  if (!(flags & 0x20000000u)) {
    // The measured branch has no animation context. An animation pose is not
    // replaced with these default records when its source producer is needed.
    if (read<std::int16_t>(body, 0x12a) != -1)
      throw std::runtime_error("Original animated item local-node producer unavailable");
    constexpr std::array<std::uint32_t, 8> instructions{
        0x16f3ef, 0x16f3f4, 0x16f3fa, 0x16f400,
        0x16f40b, 0x16f410, 0x16f416, 0x16f41d};
    for (int i = 0; i < count; ++i) {
      const auto node = storage.render_nodes.subspan(unsigned(i) * 0x60, 0x60);
      for (unsigned word = 0; word < 8; ++word) {
        const auto value = word < 4 ? read<std::uint32_t>(node, 0x18 + word * 4) :
            word < 7 ? read<std::uint32_t>(node, 0x0c + (word - 4) * 4) :
            std::bit_cast<std::uint32_t>(input.one);
        const auto offset = unsigned(i) * 0x20 + word * 4;
        if (local_offset == -1) store(local, true, offset, value, instructions[word], observer);
        else store(body, false, unsigned(local_offset) + offset, value, instructions[word], observer);
      }
    }
  }
  std::array<std::byte, 0x34> root, node0;
  source_item_root_matrix(std::span<const std::byte, 12>(body.data() + 0x7c, 12),
      std::span<const std::byte, 12>(body.data() + 0x70, 12),
      std::span<const std::byte, 12>(body.data() + 0x64, 12),
      read<float>(body, 0xa0), (flags >> 10) & 0xffffff01u, input.one, root);
  source_item_node_matrix(std::span<const std::byte, 0x20>(local.data(), 0x20), node0, input);
  auto current = body.subspan(unsigned(current_offset), unsigned(current_size));
  auto at = [&](unsigned i) { return std::span<std::byte, 0x34>(current.data() + i * 0x34, 0x34); };
  auto observe_at = [&](unsigned i) -> SourceMatrixMathObserver {
    return [&, i](const SourceMatrixMathStore &value) {
      auto absolute = value; absolute.offset += unsigned(current_offset) + i * 0x34;
      if (observer) observer({false, absolute});
    };
  };
  source_compose_item_matrix(root, node0, at(0), observe_at(0));
  // Actual 10D260 entries are all NULL. Positive type callback behavior remains
  // a separate producer; the source callback roster/order is retained.
  for (auto callback : callbacks) if (callback)
    throw std::runtime_error("Original positive current-node callback unavailable");
  for (unsigned i = 1; i < unsigned(count); ++i) {
    source_item_node_matrix(std::span<const std::byte, 0x20>(local.data() + i * 0x20, 0x20),
        at(i), input, observe_at(i));
    const auto parent = read<std::int16_t>(storage.render_nodes, i * 0x60 + 4);
    if (parent < 0 || unsigned(parent) >= i)
      throw std::runtime_error("Original current-node parent bank unavailable");
    source_compose_item_matrix(at(unsigned(parent)), at(i), at(i), observe_at(i));
  }
}
} // namespace halo2
