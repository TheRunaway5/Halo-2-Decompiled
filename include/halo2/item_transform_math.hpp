#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace halo2 {
struct SourceMatrixMathStore {
  std::uint32_t instruction;
  std::size_t offset;
  std::vector<std::byte> before, after;
  friend bool operator==(const SourceMatrixMathStore &,
                         const SourceMatrixMathStore &) = default;
};
using SourceMatrixMathObserver = std::function<void(const SourceMatrixMathStore &)>;
struct SourceNodeMatrixInputs {
  float one;
  std::array<std::uint32_t, 4> sign_mask;
  std::span<std::byte, 8> lazy_constants;
  // Original RCPSS instruction boundary. A modern hardware approximation is
  // not substituted for the independently measured original instruction.
  std::function<float(float)> reciprocal;
};

// Original 1441D0 read/write order, including in-place left/right aliases.
void source_compose_item_matrix(std::span<const std::byte, 0x34> left,
    std::span<const std::byte, 0x34> right, std::span<std::byte, 0x34> output,
    const SourceMatrixMathObserver & = {});
// Measured 143960 length-positive branch. Lazy constants must already have
// been produced by the source initializer; missing initialization fails.
void source_item_node_matrix(std::span<const std::byte, 0x20> node,
    std::span<std::byte, 0x34> output, const SourceNodeMatrixInputs &,
    const SourceMatrixMathObserver & = {});
// Measured BEB30 root matrix with no parent and low-byte handedness false.
void source_item_root_matrix(std::span<const std::byte, 12> up,
    std::span<const std::byte, 12> forward, std::span<const std::byte, 12> point,
    float scale, std::uint32_t handedness, float one,
    std::span<std::byte, 0x34> output, const SourceMatrixMathObserver & = {});
} // namespace halo2
