#include "halo2/path_tracing/source_ambient.hpp"
#include "halo2/path_tracing/material_alias.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/world_material.hpp"
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
    unsigned materials{},cases{},uv_cases{},detail_materials{},capital_materials{},honor_guard_materials{};
    unsigned combined_materials{},prophet_robes{},prophet_regalia{},council_thrones{};
    bool flat_ambient_alias{};
    unsigned fixed_materials{},fixed_cases{};
    float fixed_error{};
    float maximum{},uv_error{};bool mutated=true;nlohmann::json rows=nlohmann::json::array();
    for(const auto&tag:cache.tags()) {
        if(tag.group!="shad")continue;
        auto material=cache.load_material(tag.datum);
        if(tag.name.ends_with("\\truth_helmet") || tag.name.ends_with("\\truth_body") ||
           tag.name.ends_with("\\elite_honor_armor")) {
            auto fixed=material;
            if(!prepare_default_object_material(cache,fixed,0.f))throw FormatError("Missing fixed ambient source");
            const auto surface=pt::qualified_surface(fixed);
            const auto ambient=surface?pt::source_fixed_accumulated_ambient(fixed,surface->family):std::nullopt;
            if(!ambient)throw FormatError("Missing qualified fixed ambient source: "+tag.name);
            auto words=fixed.environment_pass->forward_program.words;
            words[11]&=0xff000000u;words[19]&=0xff000000u;
            const auto rgb=pt::surface_detail::argb(*ambient);
            for(unsigned sample=0;sample<256;++sample) {
                std::array<ShaderVector,4> textures;
                for(auto& texture:textures)for(float& c:texture)c=unit(random);
                textures[1]={}; // Do not sample the combined environment cube.
                ShaderVector v0,v1;for(float& c:v0)c=unit(random);for(float& c:v1)c=unit(random);
                const auto actual=halo2_test::scalar_combiners(words,textures,v0,v1);
                for(unsigned c=0;c<3;++c)
                    fixed_error=std::max(fixed_error,std::abs(actual[c]-std::clamp(2.f*textures[0][c]*rgb[c],0.f,1.f)));
                ++fixed_cases;
            }
            fixed.environment_pass->forward_program.words[8]^=1u;
            mutated&=!pt::source_fixed_accumulated_ambient(fixed,surface->family);
            ++fixed_materials;
        }
        if(!prepare_dynamic_material(cache,material,{},true,0.f))continue;
        const auto* owner=pt::source_ambient_owner(material);if(!owner)continue;
        const bool detail=owner==&material;const auto selected=owner->forward_pass_index;
        const auto ambient_word=pt::source_ambient_color_word(*owner);
        const bool flat=ambient_word==16u;
        const auto family=pt::qualified_surface(material)->family;
        float material_error{};
        if(detail)++detail_materials;
        if(detail && (tag.name.ends_with("\\capital_ship_a") || tag.name.ends_with("\\capital_ship_b")))++capital_materials;
        if(const auto source=pt::qualified_surface(material);
            source && source->family==pt::SurfaceFamily::TextureHonorGuard)++honor_guard_materials;
        if(flat)++combined_materials;
        if(tag.name.starts_with("objects\\characters\\prophet\\shaders\\")) {
            if(!flat && (tag.name.ends_with("\\mercy_body") || tag.name.ends_with("\\regret_body") ||
                         tag.name.ends_with("\\truth_body")))++prophet_robes;
            if(flat && (tag.name.ends_with("_helmet") || tag.name.ends_with("\\headdress")))++prophet_regalia;
        }
        if(flat && tag.name.ends_with("\\throne_dull"))++council_thrones;
        if(flat && tag.name.ends_with("\\truth_helmet")) {
            std::array<Vertex,3> triangle{};
            triangle[1].position={1,0,0};triangle[2].position={0,1,0};
            const auto roots=std::span(&material,1);
            auto scene=pt::build_scene(triangle,roots);
            WorldMaterialUniform primary{},lighting{};
            primary.program=material.forward_program.words;
            primary.forward_transforms=material.forward_transforms;
            lighting.program=owner->forward_program.words;
            lighting.forward_transforms=owner->forward_transforms;
            lighting.program[ambient_word]=0x5c3726u;
            const std::array inputs{pt::MaterialAliasInput{{1,0,0},&primary,nullptr,nullptr,&lighting}};
            const auto aliases=pt::refresh_material_aliases(scene,roots,inputs);
            flat_ambient_alias=aliases.size()==1 && scene.material_alias_images.empty() &&
                scene.materials[aliases.front().material].color_guide==std::array<std::uint32_t,4>{3,0,0x5c3726u,0};
        }
        const auto vertex_program=decode_vertex_program(owner->vertex_program);
        for(unsigned sample=0;sample<256;++sample) {
            auto lighting=*owner;ObjectLightingState state;state.ambient={unit(random),unit(random),unit(random)};
            lighting.ambient_brightness=unit(random)*2-1;apply_object_lighting(lighting,state);
            auto words=lighting.forward_program.words;
            // Preserve all native stages; only the independent authored
            // directional/specular constants are zero in this source state.
            std::array<ShaderVector,4> textures;
            for(auto&t:textures)for(auto&c:t)c=unit(random);
            if(flat)textures[1]={}; // Isolate the ambient term from the environment cube.
            ShaderVector v0,v1;for(auto&c:v0)c=unit(random);for(auto&c:v1)c=unit(random);
            const auto actual=halo2_test::scalar_combiners(words,textures,v0,v1);
            const auto ambient=pt::source_ambient_irradiance(words,ambient_word);
            const auto albedo=detail?pt::surface_color(family,pt::surface_constants(words,family),textures):textures[0];
            for(unsigned c=0;c<3;++c) {
                const float bump_z=flat?1.f:std::max(2.f*textures[2][2]-1.f,0.f);
                material_error=std::max(material_error,std::abs(actual[c]-std::clamp(albedo[c]*ambient[c]*bump_z,0.f,1.f)));
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
        maximum=std::max(maximum,material_error);
        rows.push_back({{"name",tag.name},{"lighting_pass",owner->passes[selected].name},
            {"detail",detail},{"flat_ambient",flat},{"maximum_error",material_error}});++materials;
    }
    const bool passed=materials>4 && cases>1000 && detail_materials>0 && capital_materials==2 &&
        honor_guard_materials==3 && combined_materials>4 && prophet_robes==3 && prophet_regalia==4 &&
        council_thrones==1 && flat_ambient_alias && maximum<2.e-6f && uv_error==0 && mutated && fixed_materials==3 && fixed_error<2.e-6f;
    nlohmann::json report{{"passed",passed},{"materials",rows},{"cases",cases},{"maximum_error",maximum},
        {"vertex_uv_cases",uv_cases},{"maximum_uv_error",uv_error},{"control_mutations_passed",mutated},
        {"dynamic_detail_materials",detail_materials},{"capital_ship_materials",capital_materials},{"honor_guard_materials",honor_guard_materials},
        {"combined_materials",combined_materials},{"prophet_robes",prophet_robes},{"prophet_regalia",prophet_regalia},
        {"council_thrones",council_thrones},{"flat_ambient_alias",flat_ambient_alias},
        {"fixed_materials",fixed_materials},{"fixed_cases",fixed_cases},{"maximum_fixed_error",fixed_error},
        {"scope","Exact dynamic source ambient transfer with current source brightness/packing and each program's normal-map response. Complete native T2 UV execution; arbitrary texture alpha and vertex lighting outputs. No directional/specular energy included."}};
    std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return passed?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
