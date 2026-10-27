#pragma once
#include "halo2/render_program.hpp"
#include "halo2/cache.hpp"
namespace halo2 {
struct UnsupportedVertexProgram : FormatError {
  using FormatError::FormatError;
};
using VertexInstruction = std::array<std::uint32_t,4>;
// 136 instruction slots are available in the original NV2A transform engine.
struct VertexProgram {
  std::array<VertexInstruction,136> instructions{};
  std::uint32_t count{};
  bool relative_constants{};
};
VertexProgram decode_vertex_program(std::span<const std::byte>);
struct VertexProgramState {
  std::array<ShaderVector,192> constants{};
  std::array<ShaderVector,16> attributes{}, outputs{};
};
// Offline test-only arithmetic reference for original vertex programs. The
// shipped renderer has no implementation or call to this interpreter.
// Native float arithmetic
// is not a claim of NV2A reciprocal precision or hardware rounding parity.
void execute_vertex_program(const VertexProgram &, VertexProgramState &);
}
