#include "halo2/source_weather_execution.hpp"
#include <stdexcept>
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
#include <xmmintrin.h>
#endif

namespace halo2 {
SourceWeatherExecution::SourceWeatherExecution() {
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
    if(std::fegetenv(&previous_)!=0)
        throw std::runtime_error("Cannot retain native weather floating-point environment");
    mxcsr_=_mm_getcsr();
    if(std::fesetenv(FE_DFL_ENV)!=0) {
        std::fesetenv(&previous_);
        throw std::runtime_error("Cannot enter native weather floating-point environment");
    }
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    const std::uint16_t control=0x023f;
    asm volatile("fldcw %0"::"m"(control));
#endif
    _mm_setcsr(0x1f80);
#else
    throw std::runtime_error("Native weather reference-emulator profile requires an SSE host");
#endif
}
SourceWeatherExecution::~SourceWeatherExecution() {
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
    std::fesetenv(&previous_);
    _mm_setcsr(mxcsr_);
#endif
}
}
