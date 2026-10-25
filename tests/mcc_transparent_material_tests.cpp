#include "halo2/cache.hpp"
#include <bit>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
bool texture(const Texture& a,const Texture& b) {
    if(a.width!=b.width || a.height!=b.height || a.depth!=b.depth || a.layers!=b.layers ||
       a.format!=b.format || a.pixels!=b.pixels || a.mips.size()!=b.mips.size())return false;
    for(unsigned i=0;i<a.mips.size();++i) {
        const auto& x=a.mips[i];const auto& y=b.mips[i];
        if(x.width!=y.width || x.height!=y.height || x.depth!=y.depth || x.offset!=y.offset || x.size!=y.size)return false;
    }
    return true;
}
nlohmann::json mip_differences(const Texture& actual,const Texture& expected) {
    nlohmann::json result=nlohmann::json::array();
    if(actual.format!=11 || expected.format!=11 || actual.layers!=1 || expected.layers!=1 ||
       actual.depth || expected.depth || actual.mips.size()!=expected.mips.size())return result;
    for(unsigned index=0;index<actual.mips.size();++index) {
        const auto& a=actual.mips[index];const auto& b=expected.mips[index];
        if(a.width!=b.width || a.height!=b.height || a.size!=b.size || a.size%4)continue;
        std::array<std::uint64_t,4> changes{},sums{};std::array<unsigned,4> maximum{};
        for(unsigned at=0;at<a.size;++at) {
            const auto x=std::to_integer<int>(actual.pixels[a.offset+at]);
            const auto y=std::to_integer<int>(expected.pixels[b.offset+at]);
            const auto difference=unsigned(std::abs(x-y)),channel=at%4;
            changes[channel]+=difference!=0;sums[channel]+=difference;maximum[channel]=std::max(maximum[channel],difference);
        }
        result.push_back({{"mip",index},{"width",a.width},{"height",a.height},
            {"RGBA_changed",changes},{"RGBA_maximum_error",maximum},{"RGBA_absolute_error_sum",sums}});
    }
    return result;
}
}
int main(int argc,char** argv)try {
    if((argc!=3 && argc!=4) || !*argv[1] || !*argv[2])return 77;
    Cache original(argv[1]),mcc(argv[2]);
    check(!original.is_mcc() && mcc.is_mcc(),"Expected original and prepared MCC Elongation inputs");
    const auto reference=original.load_bsp(0),actual=mcc.load_bsp(0);
    unsigned checked{},texture_failures{};std::set<std::string> reported;
    nlohmann::json report={{"original_map",argv[1]},{"prepared_map",argv[2]},
        {"bitmap_differences",nlohmann::json::array()}};
    for(unsigned slot:{24u,27u,30u,35u,52u,55u,68u,75u}) {
        const auto& a=actual.materials.at(slot);const auto& b=reference.materials.at(slot);
        check(b.name==R"(scenarios\shaders\human\military\glass\glass_interior)" &&
            a.name==R"(scenarios\shaders\human\military\glass\glass_inte_dlc)",
            "Actual Elongation BSP window shader identity changed");
        check(a.forward_pass && b.forward_pass,"MCC Elongation BSP glass still uses the opaque fallback");
        check(mcc.is_preserved_native_shader({a.source_shader_datum}),"BSP glass lacks its native owner/pointer map publication");
        check(a.forward_raster.depth_write==0 && a.forward_raster==b.forward_raster &&
            a.forward_blend==b.forward_blend && a.forward_vertex==b.forward_vertex &&
            a.forward_blend_color==b.forward_blend_color,
            "Elongation glass lost original alpha/depth/cull state");
        check(a.pixel_constants==b.pixel_constants && a.vertex_constants==b.vertex_constants &&
            a.forward_program.words==b.forward_program.words && a.program_constants==b.program_constants &&
            a.vertex_program==b.vertex_program && a.texture_states==b.texture_states,
            "Projected window parameters/programs differ from original cooked glass");
        check(a.bitmaps.size()==b.bitmaps.size(),"Glass bitmap slot count changed");
        for(unsigned i=0;i<a.bitmaps.size();++i) {
            const auto& x=a.bitmaps[i];const auto& y=b.bitmaps[i];
            const auto* xt=mcc.find_tag({x.datum},tag_groups::bitmap);
            const auto* yt=original.find_tag({y.datum},tag_groups::bitmap);
            check(xt && yt && xt->name==yt->name && x.index==y.index &&
                std::bit_cast<std::uint32_t>(x.log_dimension)==std::bit_cast<std::uint32_t>(y.log_dimension),
                "Projected window changed an authored bitmap/frame/log word");
            const auto imported=mcc.load_bitmap(x.datum,x.index),source=original.load_bitmap(y.datum,y.index);
            if(!texture(imported,source)) {
                ++texture_failures;
                if(reported.insert(xt->name).second)report["bitmap_differences"].push_back({
                    {"slot",i},{"name",xt->name},{"frame",x.index},
                    {"actual_fnv64",imported.pixels.fingerprint()},{"original_fnv64",source.pixels.fingerprint()},
                    {"width",imported.width},{"height",imported.height},{"format",imported.format},
                    {"layers",imported.layers},{"mips",imported.mips.size()},{"bytes",imported.pixels.size()},
                    {"per_mip",mip_differences(imported,source)}});
            }
        }
        if(!(texture(a.diffuse,b.diffuse) && texture(a.detail,b.detail) &&
            texture(a.bump,b.bump) && texture(a.auxiliary,b.auxiliary)))++texture_failures;
        check(a.rendering_gaps.empty(),"Projected window still has an unresolved material gap");
        ++checked;
    }
    report["metadata_slots_passed"]=checked;report["texture_failures"]=texture_failures;
    report["passed"]=texture_failures==0;
    if(argc==4)std::ofstream(argv[3])<<report.dump(2)<<'\n';
    std::cout<<report.dump()<<'\n';
    std::cout<<(texture_failures?"FAIL ":"PASS ")<<checked<<" actual Elongation BSP glass slots: native publication, original programs/constants, non-writing depth/alpha state; texture failures="<<texture_failures<<'\n';
    return texture_failures?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
