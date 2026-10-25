#include "halo2/source_light_lens_tu15.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool v,const char *why){if(!v)throw FormatError(why);}
std::string hex(Vec3 v) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(const auto b:std::as_bytes(std::span{&v,1})) {
        const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];
    }
    return out;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply original light lens request fixture");
    std::ifstream file(argv[1]);nlohmann::json fixture;file>>fixture;
    check(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original light callback image differs");
    unsigned cases{},requests{};
    for(const auto &row:fixture.at("cases")) {
        EffectLightDefinition definition;definition.flags=row.at("flags");definition.lens=row.at("lens");
        SourceEffectLightRuntime runtime{};runtime.identity=row.at("identity");runtime.owner=0xffffffffu;
        runtime.attached_node=-1;runtime.flags=row.at("active");runtime.position={3,.25f,.5f};runtime.forward={1,0,0};
        runtime.state.diffuse={.25f,.5f,1.5f};
        runtime.lens_fade=std::bit_cast<float>(row.at("fade_bits").get<std::uint32_t>());
        runtime.state.control=std::bit_cast<float>(row.at("opacity_bits").get<std::uint32_t>());
        SourceEffectLightView view;view.records.push_back({runtime,&definition,{}});view.visible.push_back(runtime.identity);
        const auto actual=source_standalone_light_lenses_tu15(view);const auto &expected=row.at("requests");
        check(actual.size()==expected.size(),"Original light callback admission differs");
        for(unsigned n=0;n<actual.size();++n) {
            const auto &a=actual[n];const auto &e=expected.at(n);
            //3E9B0 returns its predicate in AL; the untouched upper EAX bits
            //are pushed by C5539 but2DB80 consumes only that low byte.
            check((e.at("priority").get<std::uint32_t>()&255u)==0&&e.at("category")==2&&e.at("ordinal")==0&&e.at("scale_bits")==0x3f800000u,"Original closed light caller ABI changed");
            check(a.identity==runtime.identity&&(a.identity&65535u)==e.at("index")&&a.lens==e.at("lens"),"Original light lens identities differ");
            check(hex(a.position)==e.at("position").get<std::string>()&&hex(a.forward)==e.at("forward").get<std::string>()&&hex(a.color)==e.at("color").get<std::string>(),"Original light lens vectors differ");
            check(std::bit_cast<std::uint32_t>(a.brightness)==e.at("brightness_bits")&&std::bit_cast<std::uint32_t>(a.opacity)==e.at("opacity_bits"),"Original light lens scalar arithmetic differs");
            ++requests;
        }
        view.visible.clear();check(source_standalone_light_lenses_tu15(view).empty(),"Light outside the actual visible queue issued a lens");
        ++cases;
    }
    check(cases==225&&requests==90,"Original light callback coverage was narrowed");
    std::cout<<"PASS "<<cases<<" unchanged original light callback cases and "<<requests<<" exact requests\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
