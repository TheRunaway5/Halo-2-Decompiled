#include "halo2/path_tracing/source_ambient.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/vertex_program.hpp"
#include "../scalar_combiners.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <random>
using namespace halo2;
namespace pt=halo2::path_tracing;
int main(int argc,char**argv)try {
    if(argc!=3)throw FormatError("Supply prepared introduction cache and report path");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1]);std::mt19937 random(0x1450a11u);std::uniform_real_distribution<float> unit(0,1);
    unsigned materials{},cases{},uv_cases{},detail_materials{},capital_materials{};
    float maximum{},uv_error{};bool mutated=true;nlohmann::json rows=nlohmann::json::array();
    for(const auto&tag:cache.tags()) {
        if(tag.group!="shad")continue;
        auto material=cache.load_material(tag.datum);
        if(!prepare_dynamic_material(cache,material,{},true,0.f))continue;
        const auto* owner=pt::source_ambient_owner(material);if(!owner)continue;
        const bool detail=owner==&material;const auto selected=owner->forward_pass_index;
        if(detail)++detail_materials;
        if(detail && (tag.name.ends_with("\\capital_ship_a") || tag.name.ends_with("\\capital_ship_b")))++capital_materials;
        const auto vertex_program=decode_vertex_program(owner->vertex_program);
        for(unsigned sample=0;sample<256;++sample) {
            auto lighting=*owner;ObjectLightingState state;state.ambient={unit(random),unit(random),unit(random)};
            lighting.ambient_brightness=unit(random)*2-1;apply_object_lighting(lighting,state);
            auto words=lighting.forward_program.words;
            // Preserve all native stages; only the independent authored
            // directional/specular constants are zero in this source state.
            std::array<ShaderVector,4> textures;
            for(auto&t:textures)for(auto&c:t)c=unit(random);
            ShaderVector v0,v1;for(auto&c:v0)c=unit(random);for(auto&c:v1)c=unit(random);
            const auto actual=halo2_test::scalar_combiners(words,textures,v0,v1);
            const auto ambient=pt::source_ambient_irradiance(words);
            for(unsigned c=0;c<3;++c) {
                const float albedo=detail?std::clamp(2.f*textures[0][c]*textures[1][c],0.f,1.f):textures[0][c];
                const float bump_z=std::max(2.f*textures[2][2]-1.f,0.f);
                maximum=std::max(maximum,std::abs(actual[c]-std::clamp(albedo*ambient[c]*bump_z,0.f,1.f)));
            }
            ++cases;
            VertexProgramState vertex;
            for(auto& a:vertex.attributes)for(auto& c:a)c=unit(random)*4-2;
            for(auto& a:vertex.constants)for(auto& c:a)c=unit(random)*4-2;
            vertex.constants[172]={1,1,0,0};
            const auto input=vertex.attributes[1],base=vertex.constants[18],bump=vertex.constants[20];
            execute_vertex_program(vertex_program,vertex);
            for(unsigned c=0;c<2;++c) {
                const float expected=(input[c]*base[c]+base[c+2])*bump[c]+bump[c+2];
                uv_error=std::max(uv_error,std::abs(vertex.outputs[11][c]-expected));
            }
            ++uv_cases;
        }
        for(unsigned word=0;word<60;++word) {
            auto changed=material;
            if(!detail)changed.environment_pass=std::make_shared<Material>(*owner);
            auto& lighting=detail?changed:*changed.environment_pass;
            lighting.forward_program.words[word]^=1u;
            const bool color=(word>=10 && word<=25)||word==43||word==44;
            mutated&=bool(pt::source_ambient_owner(changed))==color;
        }
        for(unsigned policy=0;policy<3;++policy) {
            auto changed=material;
            if(!detail)changed.environment_pass=std::make_shared<Material>(*owner);
            auto& lighting=detail?changed:*changed.environment_pass;
            if(policy==0)lighting.texture_states[2].color_sign=1;
            if(policy==1)lighting.texture_states[2].color_key_op=1;
            if(policy==2)lighting.texture_states[2].alpha_kill=4;
            mutated&=!pt::source_ambient_owner(changed);
        }
        rows.push_back({{"name",tag.name},{"lighting_pass",owner->passes[selected].name},{"detail",detail}});++materials;
    }
    const bool passed=materials>4 && cases>1000 && detail_materials>0 && capital_materials==2 && maximum<2.e-6f && uv_error==0 && mutated;
    nlohmann::json report{{"passed",passed},{"materials",rows},{"cases",cases},{"maximum_error",maximum},
        {"vertex_uv_cases",uv_cases},{"maximum_uv_error",uv_error},{"control_mutations_passed",mutated},
        {"dynamic_detail_materials",detail_materials},{"capital_ship_materials",capital_materials},
        {"scope","Exact dynamic source ambient transfer with current source brightness/packing and signed bump-map Z. Complete native T2 UV execution; arbitrary texture alpha and vertex lighting outputs. No directional/specular energy included."}};
    std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return passed?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
