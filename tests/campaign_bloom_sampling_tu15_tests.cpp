#include "halo2/source_bloom_sampling.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <xmmintrin.h>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
std::uint32_t bits(float value){return std::bit_cast<std::uint32_t>(value);}
}
int main(int argc,char **argv)try {
    if(argc!=2)throw FormatError("Supply unchanged original filter-coordinate fixture");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Json original;{std::ifstream f(argv[1]);f>>original;}
    check(original.at("status")=="PASS_UNCHANGED_ORIGINAL_FILTER_PIXEL_COORDINATES_FIXED_LOGICAL_DIMENSIONS" &&
        original.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        original.at("executed_distinct_instructions")==210 && original.at("cases").size()==5 &&
        original.at("code_patches").empty() && original.at("service_returns").empty(),
        "Original filter-coordinate authority or coverage differs");
    std::fesetround(FE_TONEAREST);_mm_setcsr(0x1f80);
    unsigned bloom_cases{},observations{},old_denominator_mismatches{};
    for(const auto &c:original.at("cases")) {
        if(c.at("name")=="dof-filter")continue;
        check(c.at("radius_packet").at("destination_dimensions")==Json::array({160,120}),
            "Original bloom target dimensions changed");
        const auto radius=c.at("radius").get<float>();
        const auto &origin=c.at("radius_packet").at("corners")[0];
        check(origin.at("corner")==Json::array({0.,0.}) && origin.at("coordinates").size()==4,
            "Original origin/tap coverage differs");
        for(unsigned scale:{1u,2u,3u}) {
            const Vec2 physical{float(160*scale),float(120*scale)};
            const auto controls=source_bloom_filter_controls(radius,physical,Vec2{160,120});
            const auto old=source_bloom_filter_controls(radius,physical);
            check(controls[0]==radius && controls[3]==0,"Bloom uniform changed radius/padding");
            for(const auto &tap:origin.at("coordinates"))for(unsigned axis=0;axis<2;++axis) {
                const auto stage=tap.at("stage").get<unsigned>();
                const float sign=stage&(1u<<axis)?1.f:-1.f;
                // Same scalar operations as production bloom_color.glsl.
                const float actual=sign*((controls[0]*.5f)/controls[axis+1]);
                const float previous=sign*((old[0]*.5f)/old[axis+1]);
                const float expected=tap.at("analyzed_normalized")[axis].get<float>();
                check(bits(actual)==bits(expected),"Logical bloom tap differs from unchanged original");
                old_denominator_mismatches+=bits(previous)!=bits(expected);++observations;
            }
            // Composite has radius0 and retains its full physical dimensions.
            const auto composite=source_bloom_filter_controls(0,{float(640*scale),float(480*scale)});
            check(composite==ShaderVector{0,float(640*scale),float(480*scale),0},
                "Composite controls inherited the low-resolution tap extent");
        }
        ++bloom_cases;
    }
    check(bloom_cases==4 && observations==96 && old_denominator_mismatches==64,
        "Bloom radius or independent old-denominator coverage differs");
    // Unadmitted view corridors keep their existing physical dimensions.
    for(const Vec2 physical:std::array<Vec2,4>{{{214,120},{160,60},{80,60},{320,240}}}) {
        const auto unchanged=source_bloom_filter_controls(1,physical);
        check(unchanged==ShaderVector{1,physical.x,physical.y,0},
            "Unadmitted wide/split/detail/DoF denominator changed");
    }
    std::cout<<"PASS96 original bloom tap components at1x/2x/3x;64 independent old-denominator mismatches; physical composite/fallback controls retained\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
