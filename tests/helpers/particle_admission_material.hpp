#pragma once
#include "halo2/compiled_particle_vertex_signature.hpp"
#include "halo2/particle_material.hpp"

namespace halo2::tests {
// The complete already-compiled alpha-multiply-add contract, with a declared
// one-texel image. This supplies no native effect or simulation outcome.
inline ParticleMaterial particle_admission_material() {
    ParticleMaterial result;
    result.vertex.classification=7;
    const auto signature=compiled_particle_vertex_signature(7);
    result.vertex.program.assign(signature.begin(),signature.end());
    result.pass.name="particle admission fixture";
    result.pass.vertex_shader="custom_vulkan\\particle";
    result.pass.vertex_code=result.vertex.program;
    result.pass.definition.words={873993241u,404029440u,0,0,0,0,0,0,135593984u,6272u,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2048u,128u,0,0,0,0,0,0,
        872944649u,134545408u,0,0,0,0,0,0,0,0,0,2048u,128u,0,0,0,0,0,0,69890u,33u,0,0,0,0,0};
    result.pass.program=result.pass.definition;
    result.pass.states={{67,0x10101u},{59,1},{62,1},{63,0x303u},{74,0x8006u},
        {64,0},{60,1},{61,1},{58,0x204u},{147,0}};
    result.pass.textures={{0,0,0,0},{0,0,1,0}};
    result.pass.texture_states.resize(2);
    Texture image{1,1,11,{std::byte{255},std::byte{255},std::byte{255},std::byte{255}},{{1,1,0,4}}};
    result.textures[0]=image;result.textures[1]=std::move(image);
    return result;
}
}
