#include "halo2/cache.hpp"
#include <iostream>

namespace {
using namespace halo2;
void check(bool value,const char* text){if(!value)throw FormatError(text);}
const Tag& find(const Cache& cache,const std::string& group,const std::string& name) {
    const auto* tag=cache.query_tag({.group=TagGroup::from_string(group),.name=name,.require_address=true});
    if(!tag)throw FormatError("Missing comparison tag: "+name);return *tag;
}
void compare(const Cache& original,const Cache& converted,const std::string& name) {
    const auto& a=find(original,"shad",name);const auto& b=find(converted,"shad",name);
    const auto x=original.load_material(a.datum),y=converted.load_material(b.datum);
    check(x.template_name==y.template_name,"Existing shader template changed");
    check(x.vertex_constants==y.vertex_constants && x.pixel_constants==y.pixel_constants,"Existing shader constants changed");
    check(x.shader_switches==y.shader_switches && x.passes.size()==y.passes.size(),"Existing shader switches/pass count changed");
    check(x.bitmaps.size()==y.bitmaps.size(),"Existing shader bitmap binding count changed");
    for(std::size_t n=0;n<x.bitmaps.size();++n) {
        const auto* xt=original.query_tag({.handle=TagHandle{x.bitmaps[n].datum}});
        const auto* yt=converted.query_tag({.handle=TagHandle{y.bitmaps[n].datum}});
        if(!xt && !yt && x.bitmaps[n].datum==y.bitmaps[n].datum && x.bitmaps[n].index==y.bitmaps[n].index)continue;
        if(!xt || !yt || xt->name!=yt->name)std::cerr<<name<<" bitmap "<<n<<": "
            <<(xt?xt->name:"null")<<" -> "<<(yt?yt->name:"null")<<'\n';
        check(xt && yt && xt->group==yt->group && xt->name==yt->name && x.bitmaps[n].index==y.bitmaps[n].index,
              "Existing shader texture identity/frame changed");
    }
    for(std::size_t n=0;n<x.passes.size();++n) {
        const auto& xp=x.passes[n];const auto& yp=y.passes[n];
        check(xp.name==yp.name && xp.definition.words==yp.definition.words && xp.program.words==yp.program.words,
              "Existing NV2A shader program changed");
        check(xp.vertex_shader==yp.vertex_shader && xp.vertex_code==yp.vertex_code && xp.constants==yp.constants,
              "Existing vertex shader/constant bindings changed");
    }
    check(x.rendering_gaps==y.rendering_gaps,"Preservation introduced a rendering gap");
    std::cout<<name<<": original shader programs, constants and texture bindings retained\n";
}
}
int main(int argc,char** argv) {
    if(argc!=3 || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    try {
        const halo2::Cache original(argv[1]),converted(argv[2]);
        for(const auto* leaf:{"metal_roof","basic_concrete","zanzibar_seawall","massive_concrete"})
            compare(original,converted,std::string("scenarios\\shaders\\multi\\zanzibar\\")+leaf);
        const std::string bitmap="scenarios\\bitmaps\\multi\\zanzibar\\massive_concrete_bump";
        const auto x=original.load_bitmap(find(original,"bitm",bitmap).datum,0);
        const auto y=converted.load_bitmap(find(converted,"bitm",bitmap).datum,0);
        check(x.width==y.width && x.height==y.height && x.mips.size()==y.mips.size() && x.pixels==y.pixels,
              "Restored Zanzibar bump differs from original decoded mip chain");
        std::cout<<"All ten restored bump mips match the original decoded pixels\n";return 0;
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
