#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace halo2 {
// Complete authored control identity selects a compiled kernel. Only packed
// color constants are variable; no opcode is uploaded to or decoded by it.
std::string_view native_fragment_kernel_path(std::string_view family,
    std::span<const std::uint32_t> complete_program);
std::vector<std::string_view> native_fragment_kernel_paths(std::string_view family);
inline std::array<std::uint32_t,60> native_fragment_uniform_words(
    std::span<const std::uint32_t> complete_program) {
    std::array<std::uint32_t,60> colors{};
    if(complete_program.size()!=colors.size())
        throw std::invalid_argument("Native fragment requires all 60 source words");
    for(unsigned i=10;i<26;++i)colors[i]=complete_program[i];
    colors[43]=complete_program[43];colors[44]=complete_program[44];
    return colors;
}
}
