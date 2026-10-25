#include "halo2/effect_light.hpp"
#include "halo2/effect_light_draw.hpp"
#include "halo2/world_material.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <iostream>
#include <set>

using namespace halo2;
namespace {void check(bool b,const char* why){if(!b)throw FormatError(why);}}
int main(int argc,char** argv)try {
    if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());
    check(cache.is_mcc(),"Supply the prepared MCC gameplay map");
    const auto light=[&](std::string_view name) {
        const auto at=std::ranges::find_if(cache.tags(),[&](const auto& t){return t.group=="ligh" && t.name==name;});
        check(at!=cache.tags().end(),"Authored light absent from the admitted map");
        return load_effect_light(cache,at->datum);
    };
    const auto control=light("effects\\objects\\weapons\\grenade\\frag_grenade\\grenade_frag_explosion");
    const std::array actual{
        light("effects\\generic_lights\\overcharge_fp"),
        light("objects\\weapons\\pistol\\plasma_pistol\\plasma_pistol_muzzle")};
    for(const auto& definition:actual)
        check((definition.flags&0x20u) && Bytes(definition.bytes).u16(4)==0,
            "The real first-person regression definitions ceased to be spheres");
    const auto metadata=cache.load_source_bsp_metadata(0);
    const Bytes root(metadata.bytes_at(metadata.source_address(),metadata.size()));
    const auto count=root.u32(16+0xa4),address=root.u32(16+0xa8);
    const Bytes materials(metadata.bytes_at(address,std::size_t(count)*32));
    std::set<std::uint32_t> seen;
    for(unsigned n=0;n<count;++n) {
        const auto shader=materials.u32(n*32+12);
        if(shader==0xffffffffu || !seen.insert(shader).second)continue;
        auto authored=cache.load_material(shader);
        for(unsigned lod=0;lod<authored.lods.size();++lod) {
            authored.selected_shader_lod=lod;
            EffectLightUniforms uniforms{};uniforms.constants[6]={.5f,.25f,.125f,0};
            // First establish that this actual receiver supports the existing
            // ordinary sphere path. Unsupported receivers cannot hide an FP
            // failure or create an expected light/driver admission result.
            try {
                if(prepare_effect_light_diffuse_material(cache,authored,control,uniforms).empty())continue;
            }catch(const FormatError&){continue;}
            unsigned passes{};
            for(const auto& definition:actual)for(unsigned shift=0;shift<3;++shift) {
                uniforms.color_shift=shift;
                const auto completed=prepare_effect_light_diffuse_material(cache,authored,definition,uniforms);
                check(!completed.empty(),"Actual first-person sphere lost its supported receiver");
                for(const auto& pass:completed) {
                    // Whole31150:311FD/31200 sets feature bit0 for a sphere.
                    // Whole30D80:30D85 returns implementation1 before it ever
                    // reads the first-person definition flag at30D97.
                    check(pass.passes.at(pass.forward_pass_index).implementation==1,
                        "First-person sphere selected a projected implementation");
                    (void)source_effect_light_draw(0,pass,{});++passes;
                }
            }
            check(passes>=6,"First-person sphere regression covered no real passes");
            std::cout<<"Actual prepared MCC first-person sphere material: shader="<<shader
                <<" LOD="<<lod<<" passes="<<passes<<"; source31150/30D80 implementation1\n";
            return 0;
        }
    }
    throw FormatError("No supported actual sphere receiver was tested");
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
