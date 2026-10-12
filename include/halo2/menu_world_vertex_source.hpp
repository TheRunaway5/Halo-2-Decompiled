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
  std::vector<unsigned> helper_operations;
};
// Independently implemented from this project's existing MIT vertex decoder
// and arithmetic. Bounded to the ordinary class0/class1 operation/input corridor
// covered by the retained title tests; unsupported token patterns are errors.
// No GPL/LGPL generator/helper implementation is imported. Original source
// behavior is verification evidence; this is not a clean-room claim.
MenuWorldVertexSource menu_world_vertex_source(const VertexProgram &);
} // namespace halo2
