// Offline arithmetic reference for compiler and GPU differential tests.
// Never linked by the native game or renderer.
#include "halo2/vertex_program.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace halo2 {
void execute_vertex_program(const VertexProgram &program,
                            VertexProgramState &state) {
  // NV vertex arithmetic defines zero times any value as zero, including
  // reciprocal infinities. IEEE multiplication alone makes zero light NaN.
  const auto multiply = [](float a, float b) {
    return a == 0 || b == 0 ? 0.f : a * b;
  };
  std::array<ShaderVector, 13> temp{};
  int address = 0;
  for (auto &out : state.outputs)
    out = {0, 0, 0, 1};
  for (unsigned i = 0; i < program.count; ++i) {
    const auto t = program.instructions[i];
    const auto mac = (t[1] >> 21) & 15, ilu = (t[1] >> 25) & 7;
    auto source = [&](unsigned which) {
      unsigned mux, reg, neg;
      std::array<unsigned, 4> swizzle;
      if (which == 0) {
        mux = t[2] >> 26 & 3;
        reg = t[2] >> 28;
        neg = t[1] >> 8 & 1;
        swizzle = {t[1] >> 6 & 3, t[1] >> 4 & 3, t[1] >> 2 & 3, t[1] & 3};
      } else if (which == 1) {
        mux = t[2] >> 11 & 3;
        reg = t[2] >> 13 & 15;
        neg = t[2] >> 25 & 1;
        swizzle = {t[2] >> 23 & 3, t[2] >> 21 & 3, t[2] >> 19 & 3,
                   t[2] >> 17 & 3};
      } else {
        mux = t[3] >> 28 & 3;
        reg = (t[2] & 3) * 4 + (t[3] >> 30);
        neg = t[2] >> 10 & 1;
        swizzle = {t[2] >> 8 & 3, t[2] >> 6 & 3, t[2] >> 4 & 3, t[2] >> 2 & 3};
      }
      ShaderVector value{}, result{};
      if (mux == 1) {
        if (reg > 12)
          throw FormatError("vertex temporary outside register file");
        value = reg == 12 ? state.outputs[0] : temp[reg];
      } else if (mux == 2)
        value = state.attributes[t[1] >> 9 & 15];
      else if (mux == 3) {
        const int c = int(t[1] >> 13 & 255) + ((t[3] & 2) ? address : 0);
        if (c < 0 || unsigned(c) >= state.constants.size())
          throw FormatError("vertex constant outside register file");
        value = state.constants[c];
      } else if (mac || ilu)
        throw FormatError("invalid vertex operand register type");
      for (unsigned a = 0; a < 4; ++a)
        result[a] = (neg ? -1.f : 1.f) * value[swizzle[a]];
      return result;
    };
    const auto a = mac ? source(0) : ShaderVector{};
    const auto b = (mac == 2 || (mac >= 4 && mac <= 12))
                       ? source(1) : ShaderVector{};
    const auto c = (ilu || mac == 3 || mac == 4) ? source(2) : ShaderVector{};
    ShaderVector m{}, l{};
    float dot = 0;
    if (mac == 5 || mac == 6 || mac == 7) {
      dot =
          (multiply(a[0], b[0]) + multiply(a[1], b[1])) + multiply(a[2], b[2]);
      if (mac == 6)
        dot += b[3];
      if (mac == 7)
        dot += multiply(a[3], b[3]);
    }
    for (unsigned axis = 0; axis < 4; ++axis)
      switch (mac) {
      case 0:
        break;
      case 1:
        m[axis] = a[axis];
        break;
      case 2:
        m[axis] = multiply(a[axis], b[axis]);
        break;
      case 3:
        m[axis] = a[axis] + c[axis];
        break;
      case 4:
        m[axis] = multiply(a[axis], b[axis]) + c[axis];
        break;
      case 5:
      case 6:
      case 7:
        m[axis] = dot;
        break;
      case 8:
        m = {1, multiply(a[1], b[1]), a[2], b[3]};
        break;
      case 9:
        m[axis] = std::min(a[axis], b[axis]);
        break;
      case 10:
        m[axis] = std::max(a[axis], b[axis]);
        break;
      case 11:
        m[axis] = a[axis] < b[axis] ? 1 : 0;
        break;
      case 12:
        m[axis] = a[axis] >= b[axis] ? 1 : 0;
        break;
      case 13:
        // ARL writes A0.x even when the ordinary result masks are zero.
        // Its scalar source uses the same sign/swizzle as other operands.
        // NVIDIA NV_vertex_program 2.14.1.10.1 defines floor, not truncation.
        break;
      default:
        throw FormatError("unsupported vertex arithmetic opcode");
      }
    float scalar = 0;
    switch (ilu) {
    case 0:
      break;
    case 1:
      l = c;
      break;
    case 2:
      scalar = 1 / c[0];
      l.fill(scalar);
      break;
    case 3:
      scalar = 1 / c[0];
      scalar = std::copysign(std::clamp(std::abs(scalar), 0x1p-64f, 0x1p64f),
                             scalar);
      l.fill(scalar);
      break;
    case 4:
      scalar = 1 / std::sqrt(std::abs(c[0]));
      l.fill(scalar);
      break;
    case 5:
      l = {std::exp2(std::floor(c[0])), c[0] - std::floor(c[0]),
           std::exp2(c[0]), 1};
      break;
    case 6:
      scalar = std::log2(std::abs(c[0]));
      l = {std::floor(scalar), std::abs(c[0]) / std::exp2(std::floor(scalar)),
           scalar, 1};
      break;
    case 7:
      l = {1, std::max(c[0], 0.f),
           c[0] > 0
               ? std::pow(std::max(c[1], 0.f), std::clamp(c[3], -128.f, 128.f))
               : 0,
           1};
      break;
    }
    if (mac == 13) {
      const double loaded = std::floor(double(a[0]));
      if (!std::isfinite(loaded) ||
          loaded < std::numeric_limits<int>::min() ||
          loaded > std::numeric_limits<int>::max())
        throw FormatError("vertex address outside signed register range");
      address = int(loaded);
    }
    const auto reg = t[3] >> 20 & 15;
    auto write = [](ShaderVector &out, const ShaderVector &value,
                    unsigned mask) {
      for (unsigned axis = 0; axis < 4; ++axis)
        if (mask & (8 >> axis))
          out[axis] = value[axis];
    };
    const auto mm = t[3] >> 24 & 15, im = t[3] >> 16 & 15, om = t[3] >> 12 & 15;
    if (reg > 12 && (mm || im))
      throw FormatError("vertex destination outside register file");
    if (mac && mac != 13 && mm && !(ilu && reg == 1))
      write(reg == 12 ? state.outputs[0] : temp[reg], m, mm);
    if (ilu && im) {
      const auto ir = mac ? 1 : reg;
      write(ir == 12 ? state.outputs[0] : temp[ir], l, im);
    }
    if (om) {
      if (!(t[3] & 2048))
        throw FormatError("vertex constant writes are unsupported");
      write(state.outputs[t[3] >> 3 & 15], (t[3] & 4) ? l : m, om);
    }
  }
}
} // namespace halo2

namespace halo2::pstencil {
VertexProgram position_prefix(const VertexProgram& program) {
    for(unsigned i=0;i<program.count;++i) {
        const auto& t=program.instructions[i];
        const auto mac=(t[1]>>21)&15u;
        // Authored world projection: DPH/DP4 oPos.x, world, c0. Capture
        // the actual source operand before projection, retaining every
        // preceding decompression, relative palette read and skin operation.
        if((mac==6 || mac==7) && ((t[2]>>11)&3u)==3 &&
           ((t[1]>>13)&255u)==0 && !(t[3]&2u) &&
           (t[3]&0xf800u)==0x8800u && ((t[3]>>3)&15u)==0 && !(t[3]&4u)) {
            auto prefix=program;prefix.count=i+1;prefix.relative_constants=false;
            for(unsigned n=0;n<i;++n)prefix.instructions[n][3]&=~1u;
            auto& probe=prefix.instructions[i];
            probe[1]=(t[1]&0x1fffffu)|(1u<<21); // MOV, same source A.
            probe[3]=0xf800u|(9u<<3)|1u; // Full oT0, last instruction.
            return prefix;
        }
    }
    throw FormatError("pStencil position adapter lacks an authored world/c0 projection boundary");
}
} // namespace halo2::pstencil
