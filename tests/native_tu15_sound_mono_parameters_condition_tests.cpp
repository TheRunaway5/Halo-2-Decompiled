#include "halo2/native_tu15_sound_mono_parameters.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <iostream>
#include <xmmintrin.h>

using namespace halo2::tu15_mono_parameter_conversion;
namespace {
std::uint16_t sw() { std::uint16_t out; asm volatile("fnstsw %0":"=m"(out)); return out; }
std::uint16_t cw() { std::uint16_t out; asm volatile("fnstcw %0":"=m"(out)); return out; }
struct RawEnvironment { alignas(16) std::array<unsigned char,32> bytes; };
RawEnvironment retain() {
    RawEnvironment value{};
    asm volatile("fnstenv %0; fldenv %0":"=m"(value)::"memory");
    return value;
}
void restore(const RawEnvironment& value) { asm volatile("fldenv %0"::"m"(value):"memory"); }
}
int main() {
    const auto initial = retain();
    const auto mx = _mm_getcsr();
    const std::array<std::uint32_t,7> values{1052199879,1166843904,0,1065353216,1166843904,0,1065353216};
    unsigned cases = 0, failures = 0;
    std::cout << "{\"observations\":[";
    for (auto mode : {FpMode::source_pretable_53,FpMode::double_53,FpMode::extended_64})
        for (unsigned pattern = 0; pattern < 16; ++pattern) {
            auto supplied = initial;
            std::uint16_t supplied_status;
            std::memcpy(&supplied_status,supplied.bytes.data()+4,2);
            supplied_status = std::uint16_t((supplied_status & ~0x4700u)
                | ((pattern&1)?0x100:0) | ((pattern&2)?0x200:0)
                | ((pattern&4)?0x400:0) | ((pattern&8)?0x4000:0));
            std::memcpy(supplied.bytes.data()+4,&supplied_status,2);
            restore(supplied);
            const auto before_status=sw(),before_control=cw();
            const auto before_mx=_mm_getcsr();
            const auto output=convert_bounded_values(values,mode);
            const auto after_status=sw(),after_control=cw();
            const auto after_mx=_mm_getcsr();
            if(cases++)std::cout<<',';
            const bool passed=before_status==after_status&&before_control==after_control&&before_mx==after_mx;
            if(!passed)++failures;
            std::cout<<"{\"mode\":"<<unsigned(mode)<<",\"pattern\":"<<pattern
                <<",\"before_sw\":"<<before_status<<",\"after_sw\":"<<after_status
                <<",\"passed\":"<<(passed?"true":"false")<<",\"gain\":"<<output.gain.bits<<'}';
        }
    restore(initial);_mm_setcsr(mx);
    std::cout<<"],\"cases\":"<<cases<<",\"failures\":"<<failures<<"}\n";
    return failures?1:0;
}
