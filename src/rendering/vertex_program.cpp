#include "halo2/vertex_program.hpp"
#include "halo2/cache.hpp"
namespace halo2 {
VertexProgram decode_vertex_program(std::span<const std::byte> bytes) {
  const Bytes data(bytes);
  if (data.u16(0) != 0x2078)
    throw UnsupportedVertexProgram("unsupported Xbox vertex-program header");
  VertexProgram result;
  result.count = data.u16(2);
  if (!result.count || result.count > result.instructions.size() ||
      bytes.size() != 4 + result.count * 16)
    throw FormatError("invalid Xbox vertex-program length");
  bool unsupported = false;
  for (unsigned i = 0; i < result.count; ++i) {
    for (unsigned w = 0; w < 4; ++w)
      result.instructions[i][w] = data.u32(4 + i * 16 + w * 4);
    const auto &t = result.instructions[i];
    result.relative_constants |= (t[3] & 2) != 0;
    unsupported |= ((t[1] >> 21) & 15) > 13 ||
                   ((t[3] & 0xf000) &&
                    (!(t[3] & 0x800) || ((t[3] >> 3) & 255) > 15));
    if ((t[3] & 1) != (i + 1 == result.count))
      throw FormatError("invalid vertex-program final marker");
  }
  if (unsupported)
    throw UnsupportedVertexProgram(
        "unsupported vertex opcode or constant destination");
  return result;
}
} // namespace halo2
