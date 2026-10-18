#include "halo2/cache.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/pstencil/combined_environment.hpp"
#include "halo2/pstencil/environment.hpp"
#include "halo2/source_render_policy.hpp"
#include "../scalar_combiners.hpp"
#include <iostream>
#include <random>
using namespace halo2;
namespace {
void require(bool v,const char* message){if(!v)throw FormatError(message);}
const Material* independent(const Material& m) {
    if(pstencil::supports_environment_response(m))return &m;
    if(m.environment_pass)if(auto* p=independent(*m.environment_pass))return p;
    for(const auto& c:m.world_overlays_after)if(auto* p=independent(*c))return p;
    for(const auto& c:m.world_overlays_before)if(auto* p=independent(*c))return p;
    return nullptr;
}
}
int main(int argc,char** argv)try {
    require(argc==2,"Supply actual Cairo/opening cache");Cache cache(argv[1]);
    const auto find=[&](std::string_view suffix) {
        for(const auto& t:cache.tags())if(t.group=="shad" && t.name==suffix)return cache.load_material(t.datum);
        throw FormatError("Actual Chief shader missing");
    };
    auto body=find("objects\\characters\\masterchief\\shaders\\masterchief");
    auto visor=find("objects\\characters\\masterchief\\shaders\\masterchief_visor");
    body.selected_shader_lod=0;
    require(prepare_dynamic_material(cache,body,{},true,0.f),"Actual independent carrier did not prepare");
    const auto* carrier=independent(body);require(carrier,"Missing actual independent environment carrier");
    unsigned checked{},cube_changes{},semantic_cases{};
    std::mt19937 random(0x2003cafe);std::uniform_real_distribution<float> unorm(0,1);
    for(unsigned lod=0;lod<visor.lods.size();++lod) {
        auto root=visor;root.selected_shader_lod=lod;
        require(prepare_dynamic_material(cache,root,{},true,0.f),"Visor LOD did not prepare");
        const auto* source=pstencil::combined_visor_environment(root);
        require(source,"Actual visor LOD was not admitted");
        const auto before=source->forward_program.words;
        auto base=source->forward_program;pstencil::suppress_combined_visor_environment(base);
        for(unsigned n=0;n<60;++n)require(base.words[n]==
            ((n==17 || n==25)?before[n]&0xffffffu:before[n]),"Reflection separation changed diffuse controls");
        const auto extra=pstencil::separate_visor_environment(root,*carrier);
        require(!source_material_layer(extra) && source_material_layer(*carrier)==8,
            "Own companion claimed another authored template LOD/layer");
        require(extra.name==root.name && extra.source_shader_datum==root.source_shader_datum &&
            extra.selected_shader_lod==lod,"Companion changed source material/LOD ownership");
        require(extra.diffuse.pixels==source->bump.pixels && extra.auxiliary.pixels==source->detail.pixels &&
            extra.bump.pixels==source->auxiliary.pixels,"Companion substituted a current authored image");
        require(extra.program_constants[1][0]==source->program_constants[1][0] &&
            extra.program_constants[1][2]==source->program_constants[1][2],"Companion changed actual bump UV transforms");
        require(extra.vertex_program==carrier->vertex_program &&
            extra.vertex_declaration==carrier->vertex_declaration,"Companion changed the validated vertex interface");
        require(extra.texture_frames==std::array<std::uint32_t,4>{source->texture_frames[2],0,
            source->texture_frames[3],source->texture_frames[1]},"Companion changed the actual bound bitmap frames");
        require(extra.animated_textures[3].size()==source->animated_textures[1].size(),
            "Companion retained another material's cube frame bank");
        for(unsigned n=0;n<extra.animated_textures[3].size();++n)
            require(extra.animated_textures[3][n].pixels==source->animated_textures[1][n].pixels,
                "Companion substituted an authored cube frame");
        auto compatible=extra;compatible.forward_blend=8;
        require(pstencil::supports_environment_response(compatible),"Companion lost the recovered response interface");
        auto broken=*source;broken.forward_program.words[34]^=1;
        require(!pstencil::supports_combined_visor_environment(broken),"Foreign compiled controls were admitted");
        broken=*source;broken.name="objects\\characters\\elite\\shaders\\torso";
        require(!pstencil::supports_combined_visor_environment(broken),"Visor policy escaped its exact source shader");
        for(unsigned n=0;n<1024;++n) {
            auto program=source->forward_program;
            for(unsigned c=10;c<=25;++c)program.words[c]=random();
            std::array<ShaderVector,4> samples;ShaderVector v0,v1;
            for(auto& s:samples)for(auto& c:s)c=unorm(random);
            for(auto& c:v0)c=unorm(random);for(auto& c:v1)c=unorm(random);
            auto zero=program;pstencil::suppress_combined_visor_environment(zero);
            const auto full=halo2_test::scalar_combiners(program.words,samples,v0,v1);
            const auto base=halo2_test::scalar_combiners(zero.words,samples,v0,v1);
            auto alternate=samples;for(auto& c:alternate[1])c=unorm(random);
            require(base==halo2_test::scalar_combiners(zero.words,alternate,v0,v1),
                "Separated current diffuse still depends on the reflection cube");
            require(base[3]==full[3],"Reflection separation changed current output alpha");
            alternate[1]={};
            require(halo2_test::scalar_combiners(program.words,alternate,v0,v1)==
                halo2_test::scalar_combiners(zero.words,alternate,v0,v1),
                "Reflection strengths changed diffuse with the cube absent");
            cube_changes+=full!=halo2_test::scalar_combiners(program.words,alternate,v0,v1);
            ++semantic_cases;
        }
        ++checked;
    }
    require(cube_changes>0,"Current reflection isolation has no positive cube controls");
    std::cout<<"PASS "<<checked<<" actual visor LODs: source ownership, bump/cube, UV, independent vertex interface; "
        <<semantic_cases<<" diffuse/alpha isolation cases, "<<cube_changes<<" positive cube controls\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
