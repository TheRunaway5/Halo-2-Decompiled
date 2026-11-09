#include "halo2/source_effect_lighting_colors_tu15.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <cfenv>
#if defined(__linux__) && (defined(__i386__) || defined(__x86_64__))
#include <xmmintrin.h>
#endif
using namespace halo2;
namespace {
void check(bool value,const char*why){if(!value)throw std::runtime_error(why);}
bool same(SourceEffectColors a,SourceEffectColors b){return a.a==b.a&&a.b==b.b;}
#if defined(__linux__) && (defined(__i386__) || defined(__x86_64__))
std::uint16_t cw(){std::uint16_t v;asm volatile("fnstcw %0":"=m"(v));return v;}
std::uint16_t sw(){std::uint16_t v;asm volatile("fnstsw %0":"=a"(v));return v;}
#endif
void model(){
    SourceEffectColors colors{0x10203040u,0x50607080u};
    SourceEffectLightingParametersTu15 parameters{5,{0x11223344u,0x55667788u},0};
    unsigned roots{},probes{};
    SourceEffectLightingServicesTu15 services;
    services.render_cache=[&]()->std::optional<SourceEffectLightingSampleTu15>{++roots;return SourceEffectLightingSampleTu15{0xfedcba98u,0x01234567u};};
    services.probe=[&](const auto&)->std::optional<SourceEffectColors>{++probes;throw std::runtime_error("Unexpected probe on cached color branch");};
    check(source_effect_lighting_colors_tu15(colors,&parameters,true,services)&&same(colors,{0x01234567u,0xfedcba98u})&&
        parameters.flags==13u&&same(parameters.colors,colors)&&roots==1u&&probes==0u,"Cached sample order/packed words/write-back differs");
    services.render_cache=[&]()->std::optional<SourceEffectLightingSampleTu15>{throw std::runtime_error("Cached request re-queried root");};
    parameters.colors={0x89abcdefu,0x76543210u};
    check(source_effect_lighting_colors_tu15(colors,&parameters,true,services)&&same(colors,parameters.colors),"Cached parameters did not own colors");
    parameters.flags=5;parameters.probe_reference=0x12345678u;const auto before=parameters.colors;
    services.render_cache=[&]()->std::optional<SourceEffectLightingSampleTu15>{++roots;return std::nullopt;};
    check(!source_effect_lighting_colors_tu15(colors,&parameters,false,services)&&same(colors,before)&&parameters.flags==5u,
        "Disabled probe manufactured color stores");
    services.probe=[&](const SourceEffectLightingProbeTu15&input)->std::optional<SourceEffectColors>{
        ++probes;check(input.reference==0x12345678u&&std::bit_cast<std::uint32_t>(input.incident.x)==0u&&
            std::bit_cast<std::uint32_t>(input.incident.y)==0u&&std::bit_cast<std::uint32_t>(input.incident.z)==0xbf800000u&&
            input.sample.lightmap_rgb==0xff404040u&&input.sample.diffuse_rgb==0xff404040u,"Probe lost original initializer or reference");
        return std::nullopt;
    };
    check(!source_effect_lighting_colors_tu15(colors,&parameters,true,services)&&same(colors,before)&&parameters.flags==5u,
        "Completed no-store probe changed prior colors");
    services.probe=[&](const auto&)->std::optional<SourceEffectColors>{return SourceEffectColors{0x12345678u,0x90abcdefu};};
    check(source_effect_lighting_colors_tu15(colors,&parameters,true,services)&&same(colors,{0x12345678u,0x90abcdefu})&&
        same(parameters.colors,colors)&&parameters.flags==13u,"Completed probe/write-back differs");
    parameters.flags=5;parameters.colors=before;
    services.render_cache=[&]()->std::optional<SourceEffectLightingSampleTu15>{parameters.flags|=8u;return SourceEffectLightingSampleTu15{7,9};};
    check(source_effect_lighting_colors_tu15(colors,&parameters,true,services)&&same(colors,{9,7})&&same(parameters.colors,before),
        "Color selector failed to reload caller flags after producer");
    services.render_cache=[]()->std::optional<SourceEffectLightingSampleTu15>{return std::nullopt;};services.probe={};
    bool refused{};try{(void)source_effect_lighting_colors_tu15(colors,nullptr,true,services);}catch(const FormatError&){refused=true;}
    check(refused,"Missing native probe owner supplied colors");
    refused=false;try{(void)source_effect_lighting_colors_tu15(colors,nullptr,false,{});}catch(const FormatError&){refused=true;}
    check(refused,"Missing native render-cache owner admitted");
}
void original(const char*path){
    using J=nlohmann::json;std::ifstream file(path);const auto fixture=J::parse(file);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original color authority");
    unsigned cases{};
    for(const auto&row:fixture.at("cases")){
        SourceEffectColors colors{row.at("initial")[0],row.at("initial")[1]};
        SourceEffectLightingParametersTu15 params{row.at("flags"),{row.at("parameter_colors")[0],row.at("parameter_colors")[1]},0};
        unsigned queries{};SourceEffectLightingServicesTu15 services;
        services.render_cache=[&]()->std::optional<SourceEffectLightingSampleTu15>{++queries;if(row.at("cache").is_null())return std::nullopt;return SourceEffectLightingSampleTu15{row.at("cache")[0],row.at("cache")[1]};};
        const auto saved_rounding=std::fegetround();
        const std::array<int,4> modes{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
        check(std::fesetround(modes.at(row.at("rounding").get<unsigned>()))==0,"Cannot apply declared FP rounding");
        const auto rounding=std::fegetround(),exceptions=std::fetestexcept(FE_ALL_EXCEPT);
#if defined(__linux__) && (defined(__i386__) || defined(__x86_64__))
        const auto control=cw(),status=sw();const auto mxcsr=_mm_getcsr();
#endif
        const auto completed=source_effect_lighting_colors_tu15(colors,row.at("parameters_present").get<bool>()?&params:nullptr,row.at("probe"),services);
        check(std::fegetround()==rounding&&std::fetestexcept(FE_ALL_EXCEPT)==exceptions,"Native color selector changed FP environment");
#if defined(__linux__) && (defined(__i386__) || defined(__x86_64__))
        check(cw()==control&&sw()==status&&_mm_getcsr()==mxcsr,"Native color selector changed FP policy/status");
#endif
        check(colors.a==row.at("expected")[0]&&colors.b==row.at("expected")[1]&&params.flags==row.at("expected_flags")&&
            params.colors.a==row.at("expected_parameter_colors")[0]&&params.colors.b==row.at("expected_parameter_colors")[1]&&
            queries==row.at("expected_queries")&&completed==row.at("expected_completed"),"Native colors differ from unchanged original cached branch");
        check(std::fesetround(saved_rounding)==0,"Cannot restore host rounding");++cases;
    }
    check(cases>=32,"Insufficient physical original cache/parameter controls");std::cout<<"PASS "<<cases<<" physical original cached color cases\n";
}
}
int main(int argc,char**argv)try{model();if(argc==2)original(argv[1]);else check(argc==1,"Unexpected color fixture arguments");std::cout<<"PASS native color selection, lazy producers, no-store and missing-owner controls\n";}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
