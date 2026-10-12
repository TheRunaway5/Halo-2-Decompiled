#include "halo2/effect_light.hpp"
#include "halo2/effect_light_draw.hpp"
#include "halo2/texture_program.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <tuple>

using namespace halo2;
namespace {
void require(bool value,const std::string&message){if(!value)throw FormatError(message);}
struct Lod {unsigned index{},range{},mask{},height{};};
using Shaders=std::map<std::uint32_t,std::vector<Lod>>;
}
int main(){try {
    const std::filesystem::path root=HALO2_SOURCE_DIR;
    std::ifstream fixture(root/"tests/fixtures/retail_bsp_shader_lods.tsv");
    require(bool(fixture),"Original BSP shader LOD fixture missing");
    std::map<std::string,Shaders> maps;std::string line;unsigned records=0;
    while(std::getline(fixture,line)) {
        if(line.empty()||line[0]=='#')continue;
        std::istringstream row(line);std::string map;unsigned datum;Lod lod;
        require(bool(row>>map>>std::hex>>datum>>lod.index>>lod.range>>lod.mask>>lod.height),"Malformed original BSP shader LOD fixture");
        maps[map][datum].push_back(lod);++records;
    }
    require(maps.size()==21&&records==1362,"Original21-map BSP LOD coverage changed");
    unsigned draws=0,eligible=0,absent=0;std::vector<std::string> unresolved;
    for(const auto &[name,shaders]:maps) {
        const auto disc=root/"assets/disc/maps"/(name+".map");
        const auto path=std::filesystem::exists(disc)?disc:root/"assets/map-pack/maps"/(name+".map");
        if(!std::filesystem::exists(path))return 77;
        Cache cache(path);unsigned map_draws=0,map_eligible=0;
        for(const char *grenade:{"effects\\objects\\weapons\\grenade\\frag_grenade\\grenade_frag_explosion",
                                "effects\\objects\\weapons\\grenade\\plasma_grenade\\plasma_grenade_explosion"}) {
            const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto&t){return t.group=="ligh"&&t.name==grenade;});
            require(tag!=cache.tags().end(),name+": original grenade light missing");
            const auto light=load_effect_light(cache,tag->datum);
            for(const auto &[datum,lods]:shaders) {
                const auto original=cache.load_material(datum);
                for(const auto &lod:lods) {
                    auto source=original;source.selected_shader_lod=lod.index;
                    const auto &actual=source.lods.at(lod.index);
                    require(actual.layers.packed==lod.range&&actual.available_layers==lod.mask,
                        name+": BSP light selected LOD differs from original raw fixture");
                    const bool present=(lod.mask&(1u<<12))!=0;
                    if(present){++eligible;++map_eligible;}else ++absent;
                    for(unsigned shift=0;shift<3;++shift) {
                        EffectLightUniforms uniforms{};uniforms.color_shift=shift;
                        for(unsigned i=0;i<9;++i)uniforms.constants[i]={float(i),float(i)+.25f,float(i)+.5f,float(i)+.75f};
                        uniforms.constants[6]={.75f,.5f,.25f,0};uniforms.constants[7]={0,0,0,0};
                        try {
                            const auto passes=prepare_effect_light_diffuse_material(cache,source,light,uniforms);
                            require(present?!passes.empty():passes.empty(),"Authored sphere diffuse layer admission differs");
                            for(const auto &pass:passes) {
                                const auto &raw=source.passes.at(pass.forward_pass_index);
                                require(pass.source_shader_datum==datum&&pass.selected_shader_lod==lod.index&&
                                    pass.vertex_program==raw.vertex_code&&raw.texture_states.size()<=pass.texture_states.size()&&
                                    std::equal(raw.texture_states.begin(),raw.texture_states.end(),pass.texture_states.begin()),
                                    "Sphere diffuse pass changed source shader/LOD/VP/sampler identity");
                                require(texture_program_supported(pass.forward_program),"Sphere diffuse addressing is unsupported");
                                require(pass.auxiliary.depth==32&&pass.auxiliary.width==32&&
                                    pass.auxiliary.height==32&&pass.auxiliary.mips.size()==6,
                                    "Sphere diffuse pass lost authored attenuation volume/mips");
                                const auto draw=source_effect_light_draw(0,pass,{});
                                require(draw.vertex_bank.range==std::array<std::uint32_t,4>{34,6,0,0},
                                    "Sphere diffuse extended constant range changed");
                                for(unsigned i=0;i<3;++i)require(pass.program_constants[1][13+i]==uniforms.constants[i],
                                    "Sphere diffuse c31..33 material overlap changed");
                                ++draws;++map_draws;
                            }
                        }catch(const FormatError&error) {
                            std::ostringstream message;message<<name<<" shader="<<std::hex<<datum<<std::dec<<" LOD="<<lod.index
                                <<" shift="<<shift<<" light="<<grenade<<": "<<error.what();unresolved.push_back(message.str());
                        }
                    }
                }
            }
        }
        std::cout<<name<<" authored_grenade_lods="<<map_eligible<<" completed_passes="<<map_draws<<'\n';
    }
    for(const auto &message:unresolved)std::cerr<<message<<'\n';
    std::cout<<"21 maps, "<<eligible<<" grenade/LOD light layers, "<<absent<<" absent layers, "<<draws<<" passes, "<<unresolved.size()<<" unresolved bindings; asset/service scope, not gameplay appearance\n";
    require(unresolved.empty(),"Original grenade BSP material bindings remain unresolved");
    require(draws>1000&&eligible>300,"Sphere diffuse asset fixture has no meaningful coverage");
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
