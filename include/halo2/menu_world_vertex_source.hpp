#pragma once
#include "halo2/vertex_program.hpp"

namespace halo2 {
// A pure code-generation result. The future graphics consumer supplies
// vec4 vpInput[16], vec4 vpOut[16], and vec4 vpConstant(uint). No interface,
// resource binding, source input conversion, camera, epilogue or pipeline is
// selected here. helpers belongs at global scope; statements inside a function.
struct MenuWorldVertexSource {
  std::string helpers, statements;
  std::vector<unsigned> consumed_inputs, consumed_constants;
  // MAC opcodes retain their existing VertexProgram values; ILU operations
  // use 16+opcode. These are actual emitted static calls, including repeated
  // output/temporary writes, rather than a count of instruction slots.
  // The explicit ABI4 renderer corridor records each computed MAC/ILU result
  // once; its arithmetic can use statements instead of helper calls.
  std::vector<unsigned> helper_operations;
};
// Independently implemented from this project's existing MIT vertex decoder
// and arithmetic. Bounded to the ordinary class0/class1 operation/input corridor
// covered by the retained title tests; unsupported token patterns are errors.
// No GPL/LGPL generator/helper implementation is imported. Original source
// behavior is verification evidence; this is not a clean-room claim.
MenuWorldVertexSource menu_world_vertex_source(const VertexProgram &);
// Original local model corridor: v0..6, ARL and c[A0+50..52]. Relative reads
// retain the whole c50..169 palette dependency; no current pose is selected.
MenuWorldVertexSource menu_world_skinned_vertex_source(const VertexProgram &);
// Explicit constant-attribute corridor. V10/V11 are supplied by the selected
// material's separate attribute bank, never by an invented geometry stream.
MenuWorldVertexSource menu_world_material_vertex_source(const VertexProgram &,bool skinned);
// Explicit renderer arithmetic corridor. All operands snapshot the previous
// token state before paired MAC/ILU results or output aliases are committed.
// The existing title/material emitters retain their ABI2/3 source unchanged.
MenuWorldVertexSource menu_world_renderer_vertex_source(const VertexProgram &,bool skinned);
// Build-time XYZW read masks (bits0..3), including masks and operand swizzles.
// Runtime packets use the immutable catalog metadata, not these analyzers.
std::array<std::uint8_t,16> menu_world_vertex_input_components(const VertexProgram &,bool skinned);
std::array<std::uint8_t,16> menu_world_renderer_vertex_input_components(const VertexProgram &,bool skinned);
// Bits0..3 identify texture outputs whose W is never written by an already
// admitted straight-line program. Original output initialization leaves these
// lanes exactly1; preserve that constant through the fragment interface rather
// than introducing perspective-interpolator rounding into textureProj.
std::uint32_t menu_world_vertex_default_texture_w_mask(const VertexProgram &);
} // namespace halo2
