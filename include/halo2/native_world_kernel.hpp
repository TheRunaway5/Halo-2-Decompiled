#pragma once
#include "halo2/source_vertex_declaration.hpp"
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace halo2 {
struct VertexProgramState;
// CPU-side world-position extraction uses the same AOT rule. Program words
// select a complete fixed function; they are never executed by an opcode loop.
struct NativeCpuPositionKernelEntry {
    std::span<const std::array<std::uint32_t,4>> program;
    void (*run)(VertexProgramState&);
};
// Source-owned vertex identity and its numerical fragment reference. Runtime
// vertex lookup matches the complete bytes/declaration/bank; the independent
// fragment catalog matches family plus all control words. These paths contain
// fixed compiled operations, never an opcode/register-combiner interpreter.
struct NativeWorldKernelEntry {
    std::span<const std::uint8_t> vertex_program;
    std::array<std::uint32_t,60> controls;
    PackedSourceVertexDeclaration declaration;
    bool extended_vertex_bank;
    std::string_view vertex_path,fragment_path;
    std::string_view normal_vertex_path,normal_utility_vertex_path;
    unsigned default_texture_w_mask=15;
    bool relative_constants=false;
};
}
