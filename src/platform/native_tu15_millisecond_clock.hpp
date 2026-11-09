#pragma once
#include "halo2/native_tu15_millisecond_clock_kernel.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <cstdlib>

namespace halo2::native_millisecond_clock_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_MILLISECOND_CLOCK");return p&&*p;}();return enabled;}
template<class State>bool admitted(const State&s){return !reference_mode()&&s.image==SourcePhysicsContactImage::TitleUpdate15;}
// Native service observations retain all three source boundaries, including
// the genuine 595 refresh. These are not translated-body instruction steps.
template<class State>void run_admitted(State&s) {
    s.instruction_literal(0x33b590u);
    const auto sample=native_tu15_sample_millisecond_clock(s.memory,[&](std::uint32_t cell){
        s.gp[0]=cell;s.instruction_literal(0x33b595u);return s.gp[0];
    });
    s.gp[0]=sample.milliseconds;s.instruction_literal(0x33b597u);s.pop();
}
template<class State>bool bridge(State&s){if(!admitted(s))return false;run_admitted(s);return true;}
} // namespace halo2::native_millisecond_clock_detail
