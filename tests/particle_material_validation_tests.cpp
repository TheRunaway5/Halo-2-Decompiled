#include "halo2/particle_material_validation.hpp"
#include "helpers/particle_admission_material.hpp"
#include <iostream>
#include <limits>

namespace {using namespace halo2;
void check(bool value,const char* message){if(!value)throw FormatError(message);}
}
int main()try {
    const auto original=tests::particle_admission_material();
    const auto admitted=validate_particle_material(original);
    check(admitted.classification==7 && admitted.instructions==51 && admitted.program_hash==0x2b67638b8d57aa48ull &&
        !admitted.external_scene_texture && !admitted.distortion && !admitted.fragment_kernel.empty(),
        "Complete particle contract was not admitted");
    for(unsigned classification:{8u,9u}) {
        auto changed=original;changed.vertex.classification=classification;
        const auto code=compiled_particle_vertex_signature(classification);changed.vertex.program.assign(code.begin(),code.end());
        check(validate_particle_material(changed).classification==classification,"Other compiled sprite class was rejected");
    }
    unsigned controls{};
    const auto rejects=[&](const auto& mutate) {
        auto changed=original;mutate(changed);bool rejected=false;
        try{(void)validate_particle_material(changed);}catch(const ParticleResourceError&){rejected=true;}
        check(rejected,"Unsupported particle content passed complete material admission");++controls;
    };
    rejects([](auto& p){p.vertex.classification=6;});
    rejects([](auto& p){p.vertex.program.back()^=std::byte{1};});
    rejects([](auto& p){p.pass.program.words[0]^=1u;});
    rejects([](auto& p){p.textures[0].reset();});
    rejects([](auto& p){p.pass.texture_states.clear();});
    rejects([](auto& p){p.textures[0]->mips.clear();});
    rejects([](auto& p){p.textures[0]->mips[0].offset=std::numeric_limits<std::size_t>::max();});
    rejects([](auto& p){p.textures[0]->mips[0].size=3;});
    rejects([](auto& p){p.textures[0]->mips[0].width=2;});
    rejects([](auto& p){p.textures[0]->layers=6;});
    rejects([](auto& p){p.textures[0]->format=14;});
    rejects([](auto& p){p.pass.texture_states[0].color_key_op=1;});
    rejects([](auto& p){p.pass.texture_states[0].color_sign=0x10000000u;p.textures[0]->pixels.push_back(std::byte{});});
    rejects([](auto& p){p.pass.texture_states[0].min_filter=0;});
    rejects([](auto& p){p.pass.texture_states[0].address_v=0;});
    rejects([](auto& p){p.pass.texture_states[0].lod_bias=0x7fc00000u;});
    rejects([](auto& p){p.pass.texture_states[0].border_color=0x01020304u;});
    rejects([](auto& p){p.pass.textures[1].stage=2;});
    rejects([](auto& p){p.pass.textures[1].stage=0;});
    rejects([](auto& p){p.pass.textures[1].external=23;});
    rejects([](auto& p){p.pass.state_externs.push_back({0,0,0});});
    rejects([](auto& p){p.sampler_writes.push_back({4,22,0});});
    rejects([](auto& p){p.sampler_writes.push_back({1,21,0});});
    rejects([](auto& p){p.sampler_writes.push_back({1,22,0x7fc00000u});});
    rejects([](auto& p){p.pass.states.push_back({255,0});});
    rejects([](auto& p){p.pass.states.push_back({74,0});});
    rejects([](auto& p){p.pass.states.push_back({147,0x902});});
    rejects([](auto& p){p.pass.states.push_back({62,2});});
    std::cout<<"PASS complete material admission, three compiled sprite classes and "<<controls<<" unsupported/missing-resource controls\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
