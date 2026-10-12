#pragma once
#include <cfenv>
#include <cstdint>

namespace halo2 {
// Native-thread arithmetic scope for the explicitly selected reference-
// emulator weather profile. Restores the caller's complete FP environment.
// The source kernels still receive/check their literal023F/1F80 inputs.
class SourceWeatherExecution {
public:
    SourceWeatherExecution();
    ~SourceWeatherExecution();
    SourceWeatherExecution(const SourceWeatherExecution&)=delete;
private:
    std::fenv_t previous_{};
    std::uint32_t mxcsr_{};
};
}
