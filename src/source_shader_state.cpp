#include "halo2/source_shader_state.hpp"
#include "halo2/source_shader_rgb.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
std::vector<ShaderState> remap_source_shader_states(
    std::span<std::uint32_t> state_words,std::span<const float> scalars,
    std::span<const ShaderVector> colors,std::span<const ShaderRemap> mappings,
    std::uint16_t control) {
    // Literal executable descriptors, not variant or weapon special cases.
    struct Descriptor {std::uint8_t state,conversion;};
    constexpr Descriptor table[]{{75,0},{75,0},{61,2},{77,1},{78,1},{157,1},{138,0}};
    std::vector<ShaderState> writes;
    for(const auto &mapping:mappings) {
        const auto row=mapping.destination&255u;
        if(row>=std::size(table))throw FormatError("Shader state extern requires an admitted descriptor row");
        const auto descriptor=table[row];
        if(descriptor.state>=state_words.size())throw FormatError("Shader state word unavailable");
        auto value=state_words[descriptor.state];
        if((mapping.destination>>8)==1) {
            if(mapping.source>=scalars.size() || !std::isfinite(scalars[mapping.source]))
                throw FormatError("Shader state scalar unavailable or nonfinite");
            const auto scalar=scalars[mapping.source];
            switch(descriptor.conversion) {
            case 0:value=source_shader_alpha_state_word(value,scalar,control);break;
            case 1:value=std::bit_cast<std::uint32_t>(scalar);break;
            case 2:value=source_shader_integer(scalar,control);break;
            }
        } else {
            if(mapping.source>=colors.size())throw FormatError("Shader state RGB source unavailable");
            const auto &color=colors[mapping.source];
            value=source_shader_rgb_state_word(value,{color[0],color[1],color[2]},control);
        }
        state_words[descriptor.state]=value;
        writes.push_back({descriptor.state,value});
    }
    return writes;
}
}
