#include "halo2/item_transform_math.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
template<class T> T read(std::span<const std::byte> bytes, std::size_t offset) {
  T value; std::memcpy(&value, bytes.data() + offset, sizeof value); return value;
}
float scalar(std::span<const std::byte> bytes, std::size_t offset) {
  return read<float>(bytes, offset);
}
void write(std::span<std::byte> output, std::size_t offset,
    std::span<const std::byte> value, std::uint32_t instruction,
    const SourceMatrixMathObserver &observer) {
  SourceMatrixMathStore store{instruction, offset,
      {output.begin() + offset, output.begin() + offset + value.size()},
      {value.begin(), value.end()}};
  std::copy(value.begin(), value.end(), output.begin() + offset);
  if (observer) observer(store);
}
void word(std::span<std::byte> output, std::size_t offset, float value,
    std::uint32_t instruction, const SourceMatrixMathObserver &observer) {
  write(output, offset, std::as_bytes(std::span(&value, 1)), instruction, observer);
}
void pair(std::span<std::byte> output, std::size_t offset, float a, float b,
    std::uint32_t instruction, const SourceMatrixMathObserver &observer) {
  const std::array<float, 2> values{a, b};
  write(output, offset, std::as_bytes(std::span(values)), instruction, observer);
}
void local_word(std::span<std::byte> output, std::size_t offset, float value) {
  std::memcpy(output.data() + offset, &value, sizeof value);
}
} // namespace

void source_compose_item_matrix(std::span<const std::byte, 0x34> left,
    std::span<const std::byte, 0x34> right, std::span<std::byte, 0x34> output,
    const SourceMatrixMathObserver &observer) {
  const auto scale = scalar(left, 0);
  const auto product_scale = scalar(right, 0) * scale;
  std::array<std::array<float, 3>, 3> basis;
  std::array<float, 3> translation;
  // All left inputs precede the first write, as in MOVSS/MOVHPS.
  for (unsigned column = 0; column < 3; ++column)
    for (unsigned row = 0; row < 3; ++row)
      basis[column][row] = scalar(left, 4 + column * 12 + row * 4);
  const std::array<float, 3> right_translation{
      scalar(right, 0x28), scalar(right, 0x2c), scalar(right, 0x30)};
  for (unsigned row = 0; row < 3; ++row) {
    const auto a = right_translation[0] * basis[0][row];
    const auto b = right_translation[1] * basis[1][row];
    const auto c = right_translation[2] * basis[2][row];
    const auto ab = a + b; const auto abc = ab + c;
    const auto scaled = abc * scale;
    translation[row] = scaled + scalar(left, 0x28 + row * 4);
  }
  word(output, 0x28, translation[0], 0x14423d, observer);
  pair(output, 0x2c, translation[1], translation[2], 0x144242, observer);
  // Source processes right columns in reverse order; later reads can alias
  // output. Do not snapshot/reorder the entire right operand.
  constexpr std::array<std::uint32_t, 3> first{0x1442d1, 0x1442a3, 0x144270};
  constexpr std::array<std::uint32_t, 3> second{0x1442d6, 0x1442a8, 0x144275};
  for (int column = 2; column >= 0; --column) {
    const auto offset = 4 + unsigned(column) * 12;
    const std::array<float, 3> rhs{scalar(right, offset),
        scalar(right, offset + 4), scalar(right, offset + 8)};
    std::array<float, 3> result;
    for (unsigned row = 0; row < 3; ++row) {
      const auto z = rhs[2] * basis[2][row];
      const auto y = rhs[1] * basis[1][row];
      const auto x = rhs[0] * basis[0][row];
      const auto yz = y + z; result[row] = x + yz;
    }
    word(output, offset, result[0], first[column], observer);
    pair(output, offset + 4, result[1], result[2], second[column], observer);
  }
  word(output, 0, product_scale, 0x1442da, observer);
}

void source_item_node_matrix(std::span<const std::byte, 0x20> node,
    std::span<std::byte, 0x34> output, const SourceNodeMatrixInputs &input,
    const SourceMatrixMathObserver &observer) {
  if (!(read<std::uint32_t>(input.lazy_constants, 4) & 1u))
    throw std::runtime_error("Original quaternion lazy-constant initializer unavailable");
  std::array<float, 4> q, squared;
  for (unsigned i = 0; i < 4; ++i) { q[i] = scalar(node, i * 4); squared[i] = q[i] * q[i]; }
  const auto xz = squared[0] + squared[2];
  const auto yw = squared[1] + squared[3];
  const auto length = xz + yw;
  if (!(length > scalar(input.lazy_constants, 0)))
    throw std::runtime_error("Unmeasured original degenerate quaternion branch");
  if (!input.reciprocal) throw std::runtime_error("Original RCPSS producer unavailable");
  auto inverse = input.reciprocal(length);
  auto correction = length * inverse;
  correction = correction * inverse;
  inverse = inverse + inverse;
  inverse = inverse - correction;
  inverse = inverse + inverse;
  std::array<float, 4> scaled, x, y, w;
  for (unsigned i = 0; i < 4; ++i) {
    scaled[i] = q[i] * inverse;
    x[i] = q[0] * scaled[i];
    y[i] = q[1] * scaled[i];
    w[i] = q[3] * scaled[i];
  }
  const auto zz = scaled[2] * q[2];
  constexpr std::array<unsigned, 4> x_shuffle{1, 2, 1, 2};
  constexpr std::array<unsigned, 4> w_shuffle{2, 1, 2, 1};
  std::array<float, 4> mixed;
  for (unsigned i = 0; i < 4; ++i) {
    const auto signed_w = std::bit_cast<float>(
        std::bit_cast<std::uint32_t>(w[w_shuffle[i]]) ^ input.sign_mask[i]);
    mixed[i] = x[x_shuffle[i]] + signed_w;
  }
  pair(output, 8, mixed[2], mixed[3], 0x143a41, observer);
  word(output, 0x10, mixed[0], 0x143a45, observer);
  pair(output, 0x18, mixed[0], mixed[1], 0x143a4a, observer);
  const auto one_minus_zz = input.one - zz;
  const auto diagonal_x = one_minus_zz - y[1];
  word(output, 4, diagonal_x, 0x143a69, observer);
  const auto diagonal_y = one_minus_zz - x[0];
  word(output, 0x14, diagonal_y, 0x143a72, observer);
  const auto one_minus_xx = input.one - x[0];
  const auto diagonal_z = one_minus_xx - y[1];
  word(output, 0x24, diagonal_z, 0x143a87, observer);
  const auto yz_minus_wx = y[2] - w[0];
  word(output, 0x20, yz_minus_wx, 0x143a94, observer);
  const auto yz_plus_wx = y[2] + w[0];
  word(output, 0x18, yz_plus_wx, 0x143a9d, observer);
  word(output, 0, scalar(node, 0x1c), 0x143aa7, observer);
  pair(output, 0x28, scalar(node, 0x10), scalar(node, 0x14), 0x143aaf, observer);
  word(output, 0x30, scalar(node, 0x18), 0x143ab8, observer);
}

void source_item_root_matrix(std::span<const std::byte, 12> up,
    std::span<const std::byte, 12> forward, std::span<const std::byte, 12> point,
    float scale, std::uint32_t handedness, float one,
    std::span<std::byte, 0x34> output, const SourceMatrixMathObserver &observer) {
  if (handedness & 0xffu)
    throw std::runtime_error("Unmeasured original handed root-matrix branch");
  const auto cross_x = scalar(up, 4) * scalar(forward, 8) -
      scalar(forward, 4) * scalar(up, 8);
  const auto cross_y = scalar(forward, 0) * scalar(up, 8) -
      scalar(up, 0) * scalar(forward, 8);
  const auto cross_z = scalar(forward, 4) * scalar(up, 0) -
      scalar(forward, 0) * scalar(up, 4);
  std::array<std::byte, 0x34> translation{}, orientation{}, scaling{};
  for (auto *matrix : {&translation, &orientation, &scaling}) {
    local_word(*matrix, 0, one);
    for (unsigned i = 0; i < 3; ++i) local_word(*matrix, 4 + i * 16, one);
  }
  std::copy(point.begin(), point.end(), translation.begin() + 0x28);
  std::copy(forward.begin(), forward.end(), orientation.begin() + 4);
  local_word(orientation, 0x10, cross_x); local_word(orientation, 0x14, cross_y);
  local_word(orientation, 0x18, cross_z);
  std::copy(up.begin(), up.end(), orientation.begin() + 0x1c);
  if (scale != one) {
    local_word(scaling, 0, scale);
    source_compose_item_matrix(orientation, scaling, orientation);
  }
  source_compose_item_matrix(translation, orientation, output, observer);
}
} // namespace halo2
