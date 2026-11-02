#include "halo2/native_tu15_light_up_vector.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>

namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::vector<std::byte> decode(const std::string& text){std::vector<std::byte> out;for(unsigned n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::array<std::uint32_t,3> vector(std::span<const std::byte> bytes){std::array<std::uint32_t,3> out{};std::memcpy(out.data(),bytes.data(),12);return out;}
struct Constants final:SourcePhysicsContactMemory {
    mutable Json reads=Json::array();
    void read(std::uint32_t address,std::span<std::byte> bytes)const override {
        reads.push_back(address);check(bytes.size()==4,"Unexpected light constant width");
        const auto value=address==0x46a0d0u?0x3f800000u:address==0x46a0ecu?0x38d1b717u:throw std::runtime_error("Unknown light constant");
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8));
    }
    void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Light value kernel wrote constant storage");}
};
struct Host {
    alignas(16)std::array<std::byte,108> saved{};unsigned mxcsr;
    Host():mxcsr(_mm_getcsr()) {
        asm volatile("fnsave %0":"=m"(saved)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(0x9fc0);const long double first=.25L,second=-1.75L;
        asm volatile("fldt %0\n\tfldt %1"::"m"(first),"m"(second):"memory","st","st(1)");
    }
    void verify(bool observer_flags=false) {
        std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
        check(cw==0x37f && (sw&~(observer_flags?0x3fu:0u))==0x3000 &&
            (_mm_getcsr()&~(observer_flags?0x3fu:0u))==0x9fc0,"Light kernel leaked its FP environment");
        long double a{},b{};asm volatile("fstpt %0\n\tfstpt %1":"=m"(a),"=m"(b)::"memory","st","st(1)");
        const long double first=.25L,second=-1.75L;
        check(!std::memcmp(&a,&second,10)&&!std::memcmp(&b,&first,10),"Light kernel changed live host x87 values");
    }
    ~Host(){asm volatile("frstor %0"::"m"(saved):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr);}
};
Json run(const Json& row) {
    Constants memory;auto up=row.at("up").get<std::array<std::uint32_t,3>>();
    const auto forward=row.at("forward").get<decltype(up)>();
    auto fp=NativeTu15PhysicsFrameFp{std::uint16_t(row.value("cw",0x27fu)),std::uint16_t(row.value("status",0u)),row.value("mxcsr",0x1f80u)};
    Json stores=Json::array();NativeTu15LightUpResult result;
    {
        Host host;
        const auto publish=[&](auto stage,unsigned axis,auto value){
            constexpr std::array perpendicular{0x31bfau,0x31bfeu,0x31c03u},normalized{0x30c1bu,0x30c2cu,0x30c31u};
            stores.push_back(Json::array({stage==NativeTu15LightUpStage::perpendicular?perpendicular[axis]:normalized[axis],axis,value}));
        };
        if(row.value("entry",0x31b30u)==0x30bd0u) {
            result.up=up;
            result.magnitude_bits=native_campaign_ai_normalize_vector_value_tu15(memory,result.up,fp,
                [&](unsigned axis,auto value){publish(NativeTu15LightUpStage::normalized,axis,value);});
        } else result=native_tu15_light_orthonormal_up(memory,up,forward,fp,publish);
        host.verify();
    }
    // Observing stage publication must not alter the arithmetic or FP state.
    Constants second;auto plain_fp=NativeTu15PhysicsFrameFp{std::uint16_t(row.value("cw",0x27fu)),std::uint16_t(row.value("status",0u)),row.value("mxcsr",0x1f80u)};
    NativeTu15LightUpResult plain;
    if(row.value("entry",0x31b30u)==0x30bd0u) {
        plain.up=up;plain.magnitude_bits=native_campaign_ai_normalize_vector_value_tu15(second,plain.up,plain_fp);
    } else plain=native_tu15_light_orthonormal_up(second,up,forward,plain_fp);
    check(plain.up==result.up&&plain.magnitude_bits==result.magnitude_bits&&plain_fp.status==fp.status&&plain_fp.mxcsr==fp.mxcsr,"Light stage observer changed the result");
    // FP-using host callbacks must not contaminate the source sticky flags.
    {
        Host host;Constants observed;
        auto observed_fp=NativeTu15PhysicsFrameFp{std::uint16_t(row.value("cw",0x27fu)),std::uint16_t(row.value("status",0u)),row.value("mxcsr",0x1f80u)};
        const auto disturb=[](unsigned,std::uint32_t) {
            auto one=_mm_set_ss(1.f);const auto three=_mm_set_ss(3.f);const float denominator=3.f;
            asm volatile("divss %1,%0":"+x"(one):"x"(three):"memory");
            asm volatile("fld1\n\tfdivs %0\n\tfstp %%st(0)"::"m"(denominator):"memory","st");
        };
        NativeTu15LightUpResult disturbed;
        if(row.value("entry",0x31b30u)==0x30bd0u) {
            disturbed.up=up;disturbed.magnitude_bits=native_campaign_ai_normalize_vector_value_tu15(observed,disturbed.up,observed_fp,disturb);
        } else disturbed=native_tu15_light_orthonormal_up(observed,up,forward,observed_fp,
            [&](auto,unsigned axis,auto value){disturb(axis,value);});
        check(disturbed.up==plain.up&&disturbed.magnitude_bits==plain.magnitude_bits&&
            observed_fp.status==plain_fp.status&&observed_fp.mxcsr==plain_fp.mxcsr,
            "Host observer FP operations contaminated source arithmetic");
        host.verify(true);
    }
    return {{"up",result.up},{"magnitude",result.magnitude_bits},{"fp",Json::array({fp.control_word,fp.status,fp.mxcsr})},{"stores",stores},{"constant_reads",memory.reads},{"host_fp_and_stack_restored",true}};
}
}
int main(int argc,char** argv)try {
    if(argc==3 && std::string(argv[1])=="--fixture") {
        std::ifstream input(argv[2]);const auto fixture=Json::parse(input);unsigned count{};
        for(const auto& c:fixture.at("cases")) {
            const auto bytes=decode(c.at("input"));const auto result=run({{"up",vector(std::span(bytes).first(12))},{"forward",vector(std::span(bytes).subspan(12))}});
            check(result.at("up").get<std::array<std::uint32_t,3>>()==vector(decode(c.at("output"))),"Native light up differs from original finite fixture");++count;
        }
        std::cout<<"PASS "<<count<<" native complete light-up finite reference cases\n";return 0;
    }
    for(std::string line;std::getline(std::cin,line);)if(!line.empty())std::cout<<run(Json::parse(line)).dump()<<'\n'<<std::flush;
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
