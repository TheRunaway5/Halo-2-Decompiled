#include "halo2/source_shader_fog.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <limits>
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86_FP)
#include <xmmintrin.h>
#endif
using namespace halo2;
namespace {
void check(bool value,const std::string &message){if(!value)throw FormatError(message);}
float number(const nlohmann::json &bits){return std::bit_cast<float>(bits.get<std::uint32_t>());}
template<class F>void rejected(F function,const char *message) {
    bool failed=false;try{function();}catch(const FormatError &){failed=true;}check(failed,message);
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply unchanged original fog-provider fixture");
    std::ifstream file(argv[1]);check(bool(file),"Original fog-provider fixture missing");
    const auto fixture=nlohmann::json::parse(file);
    unsigned pairs=0,valid=0,defaults=0;
    for(std::size_t index=0;index<fixture.at("inputs").size();++index) {
        const auto &row=fixture.at("inputs").at(index);
        SourceShaderFogContext context{row.at("datum"),number(row.at("near_bits")),number(row.at("far_bits")),{}, {}};
        for(unsigned axis=0;axis<3;++axis) {
            context.camera_forward[axis]=number(row.at("forward_bits").at(axis));
            context.default_color[axis]=number(row.at("default_rgb_bits").at(axis));
        }
        const auto actual=source_shader_fog_externs(context);
        if(context.datum!=0xffffffffu && context.far_distance>context.near_distance)++valid;else ++defaults;
        for(const auto &[version,original]:fixture.at("versions").items()) {
            const auto &expected=original.at("results").at(index);
            const auto label=version+" fog context "+std::to_string(index);
            check(std::bit_cast<std::uint32_t>(actual.scalar39)==expected.at("scalar39_bits"),label+": scalar39 differs");
            for(unsigned axis=0;axis<3;++axis)
                check(std::bit_cast<std::uint32_t>(actual.color41[axis])==expected.at("color41_bits").at(axis),label+": color41 differs");
            check(expected.at("default_before_hex")==expected.at("default_after_hex"),label+": original default changed");
            ++pairs;
        }
    }
    check(pairs==2304 && valid==264 && defaults==504,"Original fog-provider coverage changed");
    SourceShaderFogContext context{7,10,30,{1,-.5f,.25f},{.1f,.2f,.3f}};
    const auto before=std::fegetround();
    for(const auto rounding:{FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO}) {
        check(std::fesetround(rounding)==0,"Cannot set host rounding control");
        rejected([&]{(void)source_shader_fog_externs(context);},"Unadmitted host rounding accepted");
    }
    check(std::fesetround(before)==0,"Cannot restore host rounding control");
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86_FP)
    const auto mxcsr=_mm_getcsr();
    for(const auto control:{mxcsr|0x40u,mxcsr|0x8000u,mxcsr&~0x80u}) {
        _mm_setcsr(control);
        rejected([&]{(void)source_shader_fog_externs(context);},"Unadmitted host gradual/mask control accepted");
    }
    _mm_setcsr(mxcsr);
#endif
    context.near_distance=std::numeric_limits<float>::quiet_NaN();
    rejected([&]{(void)source_shader_fog_externs(context);},"Nonfinite fog input accepted");
    context.near_distance=0;context.far_distance=std::numeric_limits<float>::denorm_min();
    rejected([&]{(void)source_shader_fog_externs(context);},"Nonfinite fog result accepted");
    std::cout<<pairs<<" unchanged original scalar39/color41 pairs passed (4608 whole dispatch calls); "
        <<valid<<" valid and "<<defaults<<" default contexts; host/input admission controls passed\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
