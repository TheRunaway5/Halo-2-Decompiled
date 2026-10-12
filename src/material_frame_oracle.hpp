#pragma once
#include "halo2/scene.hpp"
#include <cstring>
#include <unordered_set>

namespace halo2 {
// Profiling/test-only differential oracle. Capture the selected outputs, then
// rerun the original full animation bank at the exact same clock and inputs.
template<class FullBank> void verify_material_frame(std::span<const Material> materials,
    std::span<const unsigned> active,FullBank full_bank) {
    struct State {
        const Material *material{};
        PixelProgram program;
        ShaderConstants constants;
        std::array<ShaderVector,8> transforms;
        ShaderVector diffuse,specular;
        std::array<std::uint32_t,4> frames;
    };
    std::vector<State> states;std::unordered_set<const Material *> visited;
    const auto capture=[&](auto &self,const Material &m)->void {
        if(!visited.insert(&m).second)return;
        states.push_back({&m,m.forward_program,m.program_constants,m.forward_transforms,
            m.forward_diffuse,m.forward_specular,m.texture_frames});
        if(m.environment_pass)self(self,*m.environment_pass);
        for(const auto &p:m.world_overlays_before)self(self,*p);
        for(const auto &p:m.world_overlays_after)self(self,*p);
        for(const auto &p:m.world_bloom_passes)self(self,*p);
    };
    for(const auto root:active)capture(capture,materials[root]);
    full_bank();
    const auto bytes=[](const auto &a,const auto &b) {return std::memcmp(&a,&b,sizeof(a))==0;};
    for(const auto &s:states) {
        const auto &m=*s.material;
        if(s.program.words!=m.forward_program.words || s.program.fragment_offsets!=m.forward_program.fragment_offsets ||
           !bytes(s.constants,m.program_constants) || !bytes(s.transforms,m.forward_transforms) ||
           !bytes(s.diffuse,m.forward_diffuse) || !bytes(s.specular,m.forward_specular) || s.frames!=m.texture_frames)
            throw FormatError("referenced material frame differs from full-bank animation oracle");
    }
}
}
