#include "halo2/pstencil/material.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char** argv)try {
    if(argc!=3)throw FormatError("Supply current map and new evidence directory");
    const std::filesystem::path out=std::filesystem::absolute(argv[2]);
    const auto* owned=std::getenv("HALO2_TEST_SCRATCH");
    if(!owned)throw FormatError("Run this probe through tools/run_test.py");
    const auto relative=out.lexically_relative(std::filesystem::absolute(owned));
    if(relative.empty() || *relative.begin()=="..")throw FormatError("Probe output must stay under HALO2_TEST_SCRATCH");
    if(std::filesystem::exists(out))throw FormatError("Evidence exists");
    std::filesystem::create_directories(out);Cache cache(argv[1]);nlohmann::json rows=nlohmann::json::array();
    for(const auto& tag:cache.tags())if(tag.group=="shad" &&
        (tag.name=="objects\\characters\\masterchief\\shaders\\masterchief" ||
         tag.name=="objects\\characters\\masterchief\\shaders\\masterchief_visor" ||
         tag.name=="objects\\characters\\elite\\shaders\\torso" ||
         tag.name=="objects\\characters\\dervish\\shaders\\dervish_armor")) {
        auto authored=cache.load_material(tag.datum);authored.selected_shader_lod=0;auto m=pstencil::bind_texture_mapping_pass(cache,authored);
        if(!m)throw FormatError("Texture mapping inputs unresolved: "+tag.name);
        const auto& pass=m->passes.at(m->forward_pass_index);
        const auto vertex_file=std::to_string(rows.size())+"-current-vs.bin";
        std::ofstream f(out/vertex_file,std::ios::binary);f.write(reinterpret_cast<const char*>(m->vertex_program.data()),m->vertex_program.size());
        nlohmann::json textures=nlohmann::json::array();
        for(unsigned stage=0;stage<4;++stage) {
            const auto* t=stage==0?&m->diffuse:stage==1?&m->detail:stage==2?&m->bump:&m->auxiliary;
            textures.push_back({{"stage",stage},{"format",t->format},{"width",t->width},{"height",t->height},{"depth",t->depth},{"layers",t->layers}});
        }
        rows.push_back({{"shader",tag.name},{"template",authored.template_name},{"slot",m->forward_pass_index},
            {"vertex",pass.vertex_shader},{"vertex_file",vertex_file},{"program",m->forward_program.words},
            {"constants",m->program_constants},{"bank_first",m->vertex_bank?m->vertex_bank->first_constant:0},{"bank_constants",m->vertex_bank?m->vertex_bank->constants:std::vector<ShaderVector>{}},
            {"textures",textures}});
    }
    if(rows.size()!=4)throw FormatError("Texture material coverage incomplete");
    std::ofstream(out/"inputs.json")<<rows.dump(2)<<'\n';std::cout<<rows.size()<<" authored texture pass bindings\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
