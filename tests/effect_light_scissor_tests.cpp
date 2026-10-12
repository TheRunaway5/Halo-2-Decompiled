#include "halo2/effect_light_scissor.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw FormatError(why);}
template<class T> void load(std::istream &in,T &value){in.read(reinterpret_cast<char*>(&value),sizeof(value));require(bool(in),"source light scissor fixture truncated");}
void same(std::span<const float> a,std::span<const float> b,unsigned row,const char* label) {
    require(a.size()==b.size(),"source light scissor comparison width differs");
    for(unsigned i=0;i<a.size();++i)if(std::bit_cast<std::uint32_t>(a[i])!=std::bit_cast<std::uint32_t>(b[i])) {
        std::cerr<<"case "<<row<<" "<<label<<" lane "<<i<<" native "<<std::hex
            <<std::bit_cast<std::uint32_t>(a[i])<<" original "<<std::bit_cast<std::uint32_t>(b[i])<<'\n';
        throw FormatError("source light projected/scissor word differs");
    }
}
}
int main(int argc,char** argv)try {
    require(argc==1 || argc==2,"source light scissor test accepts one optional fixture path");
    static_assert(sizeof(Transform)==52 && sizeof(Vec3)==12);
    const auto file=argc==2?std::filesystem::path(argv[1]):std::filesystem::path(HALO2_SOURCE_DIR)/"tests/fixtures/effect_light_scissor.original.bin";
    std::ifstream stream(file,std::ios::binary);std::array<std::uint32_t,3> header;load(stream,header);
    require(header[0]==0x534c3248 && header[1]==1 && header[2]==680,"source light scissor fixture header differs");
    unsigned projected=0,absent=0,bounded=0;
    for(unsigned row=0;row<header[2];++row) {
        EffectLightScissorInput in;load(stream,in.inverse_view);load(stream,in.center);load(stream,in.radius);load(stream,in.clipping_rectangle);load(stream,in.near_distance);load(stream,in.far_distance);load(stream,in.viewport);
        std::uint32_t present;load(stream,present);in.light_present=present!=0;
        std::array<float,6> seed,expected_projection,expected_scissor;std::uint32_t expected_valid,expected_mode;
        load(stream,seed);load(stream,expected_valid);load(stream,expected_projection);load(stream,expected_scissor);load(stream,expected_mode);
        std::uint16_t before{},after{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("fnstcw %0":"=m"(before));
#endif
        const auto projection=source_effect_light_sphere_projection(in.inverse_view,in.center,in.radius);
        require(bool(projection)==bool(expected_valid),"source light projected admission differs");
        same(projection?std::span<const float>(*projection):std::span<const float>(seed),expected_projection,row,"projection");
        const auto scissor=source_effect_light_scissor(in);require(scissor.light_present==bool(expected_mode),"source light scissor mode differs");same(scissor.bounds,expected_scissor,row,"scissor");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("fnstcw %0":"=m"(after));
        require(before==after,"source light projection leaked its x87 control word");
#endif
        projected+=expected_valid;absent+=!present;
        const std::array<float,6> fallback{float(in.viewport[1]),float(in.viewport[3]),float(in.viewport[0]),float(in.viewport[2]),0.f-in.far_distance,0.f-in.near_distance};
        bounded+=scissor.bounds!=fallback;
    }
    require(stream.peek()==std::char_traits<char>::eof(),"source light scissor trailing bytes");
    std::cout<<"PASS "<<header[2]<<" original sphere/scissor cases / "<<projected<<" projected / "<<bounded<<" bounded / "<<absent<<" absent / "<<header[2]*14<<" exact output words\n";
    return 0;
} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
