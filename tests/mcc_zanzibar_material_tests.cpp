#include "halo2/cache.hpp"
#include "halo2/world_material.hpp"
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
}
int main(int argc,char** argv)try {
    if((argc!=2 && argc!=3) || !std::filesystem::is_regular_file(argv[1]) ||
       (argc==3 && !std::filesystem::is_regular_file(argv[2])))return 77;
    Cache cache(argv[1]);
    require(cache.is_mcc(),"Zanzibar material fixture requires an owned MCC map");
    const auto* shader=cache.query_tag({.group=tag_groups::shader,
        .name="scenarios\\shaders\\multi\\zanzibar\\zanzibar_cliff1",.require_address=true});
    require(shader,"Zanzibar cliff source shader is absent");
    auto material=cache.load_material(shader->datum);
    require(prepare_world_material(cache,material),"Zanzibar cliff material has no native world pass");
    require(material.rendering_gaps.empty(),"Zanzibar cliff material has unresolved source resources");
    const auto mesh=cache.load_bsp(0,true);
    for(const auto& warning:mesh.material_warnings)std::cerr<<warning<<'\n';
    require(mesh.material_warnings.empty(),"Zanzibar BSP has failed material admissions");
    unsigned cliff_sections{};
    for(const auto& candidate:mesh.materials)if(candidate.name==material.name) {
        require(candidate.world_pass,"Zanzibar cliff section lost its native world pass");
        ++cliff_sections;
    }
    require(cliff_sections==5,"Zanzibar cliff fixture no longer covers all five affected sections");
    if(argc==3) {
        Cache original(argv[2]);
        const std::string name="scenarios\\objects\\nature\\boulders\\granite\\bitmaps\\granite_bump";
        const auto* source=original.query_tag({.group=tag_groups::bitmap,.name=name,.require_address=true});
        const auto* restored=cache.query_tag({.group=tag_groups::bitmap,.name=name,.require_address=true});
        require(source && restored,"Zanzibar original/restored granite bitmap owner is absent");
        const auto a=original.load_bitmap(source->datum,0),b=cache.load_bitmap(restored->datum,0);
        require(a.width==b.width && a.height==b.height && a.depth==b.depth && a.layers==b.layers &&
            a.format==b.format && a.pixels==b.pixels && a.mips.size()==b.mips.size(),
            "Restored Zanzibar granite bitmap differs from the complete original texture");
        for(unsigned i=0;i<a.mips.size();++i) {
            const auto& x=a.mips[i];const auto& y=b.mips[i];
            require(x.width==y.width && x.height==y.height && x.depth==y.depth && x.offset==y.offset && x.size==y.size,
                "Restored Zanzibar granite bitmap changed an original mip extent");
        }
        std::cout<<"Original granite bitmap exact: mips="<<b.mips.size()<<" bytes="<<b.pixels.size()<<'\n';
    }
    std::cout<<"PASS all five Zanzibar cliff sections retain complete native world materials\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
