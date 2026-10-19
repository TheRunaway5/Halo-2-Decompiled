#include "halo2/source_hologram_material.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
template<class F>void rejects(F &&fn) {
    try{fn();}catch(const FormatError &){return;}
    throw FormatError("unowned hologram dependency was accepted");
}
}
int main(int argc,char **argv)try {
    require(argc==2,"Supply original opening map");
    if(!std::filesystem::exists(argv[1]))return 77;
    Cache cache(argv[1]);
    auto authored=cache.load_material(0xe87106fcu);
    const MenuWorldSourcePassChoice choice{0,21,0,0};
    const auto bound=prepare_hologram_material(cache,authored,choice);
    require(bound.pass.shader()==0xe87106fcu && bound.pass.vertex_shader()==0xe87406ffu &&
        bound.pass.implementation_address()==2152690944u,"actual Regret implementation identity changed");
    const auto &material=bound.material;
    require(material.forward_pass && material.forward_vertex==10 && material.forward_blend==1 &&
        bound.external_targets==std::array<unsigned,4>{19,0,0,0},"authored hologram copy/blend contract lost");
    const auto &pass=authored.passes.at(material.forward_pass_index);
    require(material.forward_program.words==pass.program.words &&
        material.forward_program.fragment_offsets==pass.program.fragment_offsets &&
        material.program_constants==pass.constants &&
        std::ranges::equal(material.texture_states,pass.texture_states),
        "hologram binding changed original program/constants/samplers");
    require(!prepare_independent_object_material(cache,authored),"layer21 was substituted for an ordinary body");
    rejects([&]{auto bad=choice;bad.layer=15;(void)prepare_hologram_material(cache,authored,bad);});
    rejects([&]{auto bad=choice;bad.shader_lod=1;(void)prepare_hologram_material(cache,authored,bad);});
    rejects([&]{auto bad=authored;bad.passes.at(material.forward_pass_index).textures.at(0).external=8;
        (void)prepare_hologram_material(cache,bad,choice);});
    rejects([&]{auto bad=authored;bad.passes.at(material.forward_pass_index).pixel_externs.push_back({0,0,17});
        (void)prepare_hologram_material(cache,bad,choice);});
    std::cout<<"PASS original Regret layer21 material/implementation/target19; 4 unresolved-owner controls; no produced copy or live rendering claim\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
