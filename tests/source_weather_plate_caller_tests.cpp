#include "halo2/source_weather_plate_caller.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
#include <xmmintrin.h>
#endif

namespace {
using namespace halo2;
using Input=std::array<std::uint32_t,21>;
using Output=std::array<std::uint32_t,65>;
void require(bool condition,const char *message) {if(!condition)throw std::runtime_error(message);}
SourceWeatherPlateCallerInput decode(const Input &words) {
    SourceWeatherPlateCallerInput in;
    std::copy_n(words.begin(),13,in.compositor.words.begin());
    in.depth_start=std::bit_cast<float>(words[13]);in.depth_end=std::bit_cast<float>(words[14]);
    in.selection_near=std::bit_cast<float>(words[15]);in.selection_far=std::bit_cast<float>(words[16]);
    in.projection_near=std::bit_cast<float>(words[17]);in.projection_far=std::bit_cast<float>(words[18]);
    require(words[19]<=65535,"caller FPCW exceeds storage");
    in.source_fpcw=static_cast<std::uint16_t>(words[19]);in.source_mxcsr=words[20];return in;
}
Output flatten(const SourceWeatherPlateCallerResult &result) {
    Output out{};out[1]=static_cast<unsigned>(result.profile);
    out[2]=out[3]=out[4]=0xdeadbeef;
    for(unsigned i=0;i<60;++i)out[5+i]=0xc0de0000u+i;
    if(!result.program) {
        require(result.profile==SourceWeatherPlateCallerProfile::NoDraw&&!result.depth&&!result.projected_zw,
            "source false return published an incomplete caller result");return out;
    }
    require(result.depth.has_value()&&result.projected_zw.has_value(),"successful caller omitted depth/projection");
    out[0]=1;out[2]=std::bit_cast<std::uint32_t>(*result.depth);
    for(unsigned i=0;i<2;++i)out[3+i]=std::bit_cast<std::uint32_t>((*result.projected_zw)[i]);
    std::copy(result.program->words.begin(),result.program->words.end(),out.begin()+5);return out;
}
unsigned controls() {
    SourceWeatherPlateCallerInput baseline;
    baseline.compositor.words[0]=0x4b7fff01;
    for(unsigned i:{2u,5u,8u})baseline.compositor.words[i]=std::bit_cast<std::uint32_t>(.5f);
    for(unsigned i:{10u,11u,12u})baseline.compositor.words[i]=std::bit_cast<std::uint32_t>(1.f);
    baseline.depth_start=10;baseline.depth_end=30;baseline.selection_near=baseline.projection_near=1.f/64;
    baseline.selection_far=baseline.projection_far=1024;
    const auto before=flatten(source_weather_plate_caller_profile(baseline));
    std::vector<SourceWeatherPlateCallerInput> rejected;
    const auto add=[&](const std::function<void(SourceWeatherPlateCallerInput &)> &edit){auto input=baseline;edit(input);rejected.push_back(input);};
    for(unsigned flag:{0u,2u})add([=](auto &v){v.compositor.words[0]=flag;});
    for(unsigned i:{1u,3u,4u,6u,7u,9u})add([=](auto &v){v.compositor.words[i]=0x80000000;});
    for(unsigned i:{2u,5u,8u,10u,11u,12u})for(float value:{-.1f,1.1f})
        add([=](auto &v){v.compositor.words[i]=std::bit_cast<std::uint32_t>(value);});
    add([](auto &v){v.compositor.words[5]=0x7fc00000;});
    add([](auto &v){v.projection_near=std::numeric_limits<float>::infinity();});
    add([](auto &v){v.projection_far=std::numeric_limits<float>::quiet_NaN();});
    add([](auto &v){v.projection_near=v.projection_far;});
    add([](auto &v){v.projection_near=v.projection_far=0;});
    add([](auto &v){v.projection_near=-std::numeric_limits<float>::max();v.projection_far=std::numeric_limits<float>::max();});
    add([](auto &v){v.projection_far=std::numeric_limits<float>::max();v.projection_near=std::nextafter(v.projection_far,0.f);});
    add([](auto &v){v.depth_start=v.selection_near=0;v.depth_end=8;});
    add([](auto &v){v.selection_far=0;});
    add([](auto &v){v.source_fpcw=0x037f;});
    add([](auto &v){v.source_mxcsr|=0x8000;});
    add([](auto &v){v.depth_start=std::numeric_limits<float>::quiet_NaN();});
    for(const auto &input:rejected) {
        bool caught=false;try{(void)source_weather_plate_caller_profile(input);}catch(const std::invalid_argument &){caught=true;}
        require(caught,"unsupported caller classification accepted");
        require(flatten(source_weather_plate_caller_profile(baseline))==before,"caller rejection changed subsequent retained input output");
    }
    unsigned count=static_cast<unsigned>(rejected.size());
    auto input=baseline;input.compositor.words[0]=0xdeadbe01;
    require(flatten(source_weather_plate_caller_profile(input))==before,"source padding bytes were consumed");++count;
    input=baseline;input.depth_start=1.f/64;input.depth_end=input.depth_start+8;
    require(source_weather_plate_caller_profile(input).profile==SourceWeatherPlateCallerProfile::NearDepthDisabledUnsupported,
        "near raster profile was admitted as W depth");++count;
    input=baseline;input.depth_end=input.depth_start;
    require(source_weather_plate_caller_profile(input).profile==SourceWeatherPlateCallerProfile::NoDraw,
        "collapsed source range emitted a caller packet");++count;
    const auto rounding=std::fegetround();require(std::fesetround(FE_DOWNWARD)==0,"cannot set rounding control");
    bool caught=false;try{(void)source_weather_plate_caller_profile(baseline);}catch(const std::invalid_argument &){caught=true;}
    require(std::fesetround(rounding)==0,"cannot restore rounding control");require(caught,"non-nearest host was accepted");++count;
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
    const auto csr=_mm_getcsr();
    for(unsigned bits:{0x40u,0x8000u}) {
        _mm_setcsr(csr|bits);caught=false;
        try{(void)source_weather_plate_caller_profile(baseline);}catch(const std::invalid_argument &){caught=true;}
        _mm_setcsr(csr);require(caught,"host DAZ/FTZ was accepted");++count;
    }
#endif
    return count;
}
}
int main(int argc,char **argv) {try {
    require(argc==2,"source caller fixture path or --controls required");
    if(std::string(argv[1])=="--controls") {
        std::cout<<"source caller profile controls: "<<controls()<<'\n';return 0;
    }
    std::ifstream file(argv[1]);require(bool(file),"source caller fixture unavailable");
    std::string line;unsigned cases=0,above=0,near=0,false_returns=0;
    while(std::getline(file,line)) {
        if(line.empty()||line[0]=='#')continue;
        std::istringstream row(line);row>>std::hex;Input input{};
        for(auto &word:input)require(bool(row>>word),"incomplete source caller input");
        char split{};require(bool(row>>split)&&split=='|',"source caller fixture separator missing");
        const auto actual=flatten(source_weather_plate_caller_profile(decode(input)));
        for(auto word:actual){std::uint32_t expected{};require(bool(row>>expected),"incomplete source caller expected output");require(word==expected,"complete original caller word mismatch");}
        std::string extra;require(!(row>>extra),"source caller fixture trailing data");
        ++cases;false_returns+=actual[1]==0;above+=actual[1]==1;near+=actual[1]==2;
    }
    require(cases>0,"source caller fixture is empty");
    std::cout<<"complete original caller cases: "<<cases<<" above="<<above<<" near-unsupported="<<near<<" no-draw="<<false_returns<<'\n';return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
