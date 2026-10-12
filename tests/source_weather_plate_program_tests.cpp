#include "halo2/source_weather_plate_program.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <fstream>
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
using InputWords=std::array<std::uint32_t,25>;
using OutputWords=std::array<std::uint32_t,63>;
void require(bool condition,const char *message) {
    if(!condition)throw std::runtime_error(message);
}
SourceWeatherPlateProgramInput decode(const InputWords &v) {
    SourceWeatherPlateProgramInput in;
    std::copy_n(v.begin(),13,in.compositor.words.begin());
    in.depth_start=std::bit_cast<float>(v[13]);in.depth_end=std::bit_cast<float>(v[14]);
    in.near_clip=std::bit_cast<float>(v[15]);in.far_clip=std::bit_cast<float>(v[16]);
    in.mode=v[17];in.center=v[18];in.anchor_end=v[19];in.anchor_start=v[20];in.skip_state=v[21];
    require(v[22]<=1 && v[23]<=65535,"plate program fixture boolean/control exceeds storage");
    in.depth_requested=v[22]!=0;in.source_fpcw=static_cast<std::uint16_t>(v[23]);in.source_mxcsr=v[24];
    return in;
}
OutputWords probe(const InputWords &v) {
    const auto in=decode(v);OutputWords out{};
    // Explicit prior banks allow source false returns and null depth pointers
    // to be compared with unchanged original sentinels, not zero-filled data.
    out[2]=0xdeadbeef;
    for(unsigned i=0;i<60;++i)out[3+i]=0xc0de0000u+i;
    const auto result=source_weather_plate_program(in);
    out[0]=result.program.has_value();out[1]=result.depth.has_value();
    if(result.depth)out[2]=std::bit_cast<std::uint32_t>(*result.depth);
    if(result.program)std::copy(result.program->words.begin(),result.program->words.end(),out.begin()+3);
    return out;
}
void print(const OutputWords &v) {
    std::cout<<std::hex;
    for(auto word:v)std::cout<<word<<' ';
    std::cout<<'\n';
}
unsigned admission() {
    SourceWeatherPlateProgramInput baseline;
    baseline.depth_start=10;baseline.depth_end=30;baseline.near_clip=1.f/64;baseline.far_clip=1024;
    baseline.compositor.words[0]=1;
    for(unsigned i=1;i<13;++i)baseline.compositor.words[i]=std::bit_cast<std::uint32_t>(.5f);
    const auto retained=source_weather_plate_program(baseline);
    require(retained.program.has_value(),"baseline program unavailable");
    std::vector<SourceWeatherPlateProgramInput> invalid;
    auto append=[&](auto edit){auto v=baseline;edit(v);invalid.push_back(v);};
    for(unsigned mode:{0u,1u,2u,4u,0xffffffffu})append([=](auto &v){v.mode=mode;});
    for(unsigned flag:{0u,0x100u,0xffff0000u})append([=](auto &v){v.skip_state=flag;});
    for(float far:{0.f,-1.f,std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::denorm_min()})
        append([=](auto &v){v.far_clip=far;});
    append([](auto &v){v.depth_start=std::numeric_limits<float>::infinity();});
    append([](auto &v){v.depth_end=std::numeric_limits<float>::quiet_NaN();});
    append([](auto &v){v.near_clip=std::numeric_limits<float>::infinity();});
    append([](auto &v){v.depth_start=1e30f;});
    append([](auto &v){v.depth_end=1e30f;});
    append([](auto &v){v.compositor.words[1]=0x7fc00000;});
    append([](auto &v){v.compositor.words[12]=std::bit_cast<std::uint32_t>(1e30f);});
    for(unsigned cw:{0x033fu,0x063fu,0x023eu})append([=](auto &v){v.source_fpcw=cw;});
    for(unsigned mx:{0x3f80u,0x9f80u,0x1fc0u,0x80001f80u})append([=](auto &v){v.source_mxcsr=mx;});
    unsigned rejected=0;
    for(const auto &v:invalid) {
        auto bank=retained;
        try {bank=source_weather_plate_program(v);}
        catch(const std::invalid_argument &) {++rejected;}
        require(bank.program.has_value() && bank.program->words==retained.program->words &&
            bank.depth==retained.depth,"rejected pure preparation changed retained bank");
    }
    require(rejected==invalid.size(),"unsupported classification admitted");
    const auto round=std::fegetround();require(std::fesetround(FE_DOWNWARD)==0,"rounding control unavailable");
    bool host_rejected=false;
    try {(void)source_weather_plate_program(baseline);}
    catch(const std::invalid_argument &) {host_rejected=true;}
    std::fesetround(round);require(host_rejected,"unsupported host rounding admitted");
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
    const auto mxcsr=_mm_getcsr();
    for(unsigned bit:{0x40u,0x8000u}) {
        _mm_setcsr(mxcsr|bit);bool rejected_flush=false;
        try {(void)source_weather_plate_program(baseline);}
        catch(const std::invalid_argument &) {rejected_flush=true;}
        _mm_setcsr(mxcsr);require(rejected_flush,"unsupported host DAZ/FTZ admitted");
    }
    return rejected+3;
#else
    return rejected+1;
#endif
}
}
int main(int argc,char **argv) {try {
    require(argc==2,"plate program fixture path, --probe or --admission required");
    const bool probing=std::string(argv[1])=="--probe";
    if(std::string(argv[1])=="--admission") {
        std::cout<<"weather plate unsupported classifications rejected: "<<admission()<<'\n';return 0;
    }
    std::ifstream file;
    if(!probing) {file.open(argv[1]);require(bool(file),"plate program fixture unavailable");}
    auto &stream=probing?std::cin:file;std::string line;unsigned cases=0,failures=0,successes=0;
    if(!probing)admission();
    while(std::getline(stream,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);row>>std::hex;InputWords input{};
        for(auto &word:input)require(bool(row>>word),"incomplete plate program fixture input");
        const auto actual=probe(input);successes+=actual[0]!=0;failures+=actual[0]==0;
        if(probing)print(actual);
        else {
            char split{};require(bool(row>>split) && split=='|',"plate program fixture separator missing");
            for(unsigned i=0;i<actual.size();++i) {
                std::uint32_t expected{};require(bool(row>>expected),"incomplete plate program fixture output");
                if(actual[i]!=expected)throw std::runtime_error("plate program case "+std::to_string(cases)+
                    " output "+std::to_string(i)+" differs from complete unchanged original");
            }
        }
        std::string extra;require(!(row>>extra),"plate program fixture trailing data");++cases;
    }
    require(cases>0,"empty plate program fixture");
    if(!probing)std::cout<<"complete original weather plate mode3 cases: "<<cases<<
        "; successes: "<<successes<<"; source failures preserving prior banks: "<<failures<<'\n';
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
