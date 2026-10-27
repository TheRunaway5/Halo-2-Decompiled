#include "halo2/native_tu15_constraint_clock.hpp"
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using J=nlohmann::json;using namespace halo2;
void check(bool value,const std::string&message){if(!value)throw std::runtime_error(message);}
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
std::uint64_t input(const std::string&bytes){check(bytes.size()==16,"Invalid physical FILETIME bytes");std::uint64_t value{};for(unsigned n=0;n<8;++n)value|=std::stoull(bytes.substr(n*2,2),nullptr,16)<<(n*8u);return value;}
}
int main(int argc,char**argv){try{
    check(argc==2&&std::filesystem::exists(argv[1]),"Required physical time conversion input absent");std::ifstream stream(argv[1]);const auto fixture=J::parse(stream);check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("required_backend")=="unchanged-tu15-native-clock-recorded-filetime-integer-conversion-v1","Required original time authority missing");
    std::size_t count{},clocks{};
    for(const auto&c:fixture.at("division_cases")){
        const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();const auto actual=native_tu15_seconds_from_filetime(c.at("filetime").get<std::uint64_t>());check(actual==c.at("boundary_gp")[0],"Actual original unsigned time conversion differs");check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,"Native integer time conversion changed FP");++count;
    }
    for(const auto&c:fixture.at("real_clock_cases")){
        const auto&t=c.at("actual_time");check(!t.is_null()&&t.at("genuine_original_return").get<bool>(),"Actual original time dependency missing");const auto ticks=input(t.at("filetime"));unsigned calls{};
        alignas(16)std::array<std::byte,108>host{};const long double first=1.234567890123456789L,second=-8.76543210987654321L;long double after_first{},after_second{};asm volatile("fnsave %0\n\tfldt %1\n\tfldt %2":"=m"(host):"m"(first),"m"(second):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();const auto actual=native_tu15_constraint_clock_seconds([&]{++calls;return ticks;});const bool fp_same=control()==cw&&status()==sw&&_mm_getcsr()==mxcsr;
        asm volatile("fstpt %0\n\tfstpt %1\n\tfrstor %2":"=m"(after_second),"=m"(after_first):"m"(host):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        check(actual==t.at("seconds")&&calls==1,"Native time did not consume exactly one actual FILETIME input");check(fp_same&&std::memcmp(&first,&after_first,10)==0&&std::memcmp(&second,&after_second,10)==0,"Native clock changed actual live x87 values/FP status");++clocks;
    }
    bool absent{},failed{};try{native_tu15_constraint_clock_seconds({});}catch(const std::runtime_error&){absent=true;}try{native_tu15_constraint_clock_seconds([]()->std::uint64_t{throw std::logic_error("actual-platform-input-failure");});}catch(const std::logic_error&e){failed=std::string(e.what())=="actual-platform-input-failure";}check(absent&&failed,"Native clock invented missing/failed platform input");check(count>0&&clocks>0,"Empty physical clock qualification");std::cout<<count<<" unchanged original unsigned time conversions and "<<clocks<<" genuine clock FILETIME inputs matched the native time owner with actual x87 live values preserved; world input receipt wiring remains separate\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
