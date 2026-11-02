#pragma once
#include "halo2/render_interpolation.hpp"
#include <algorithm>
#include <cstdint>

namespace halo2 {
// Phase of one already completed native FP pair. This never evaluates an
// animation or establishes authority: the caller supplies ModelNodeInterpolation's
// genuine pair admission and owns all unit/model/lifetime discontinuities.
// The local command serial belongs to the same presentation continuity value
// as this clock. An unrelated input-delivery counter must not anchor restores.
class CompletedFirstPersonPairClock {
public:
    void reset()noexcept{*this=CompletedFirstPersonPairClock{};}
    float sample(std::uint64_t completed_source_tick,bool genuinely_paired,
        std::uint64_t owned_command_serial,float local_fraction) {
        validate_render_fraction(local_fraction);
        if(!genuinely_paired) {
            tick_=completed_source_tick;serial_=owned_command_serial;
            initialized_=true;paired_=false;fraction_=1.f;return fraction_;
        }
        if(!initialized_||!paired_||tick_!=completed_source_tick) {
            tick_=completed_source_tick;serial_=owned_command_serial;
            initialized_=paired_=true;fraction_=local_fraction;return fraction_;
        }
        if(owned_command_serial<serial_)
            throw FormatError("Completed first-person pair lost its restored local command clock");
        // Once the local fixed step crosses this pair's first submitted
        // phase, hold the completed endpoint. A held source tick cannot
        // replay its old pair at every local accumulator wrap.
        fraction_=owned_command_serial>serial_?1.f:std::max(fraction_,local_fraction);
        serial_=owned_command_serial;
        return fraction_;
    }
private:
    std::uint64_t tick_{},serial_{};
    float fraction_=1.f;
    bool initialized_{},paired_{};
};
}
