#pragma once
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>

namespace halo2 {
// Native output mapping for the admitted 30 Hz campaign and the actual 48 kHz
// SDL presentation clock. The caller supplies both observed epoch values.
// It adds no lookahead, changes no source sleeps, and never rebases a late cue.
class CampaignAudioTimeline {
    struct Epoch {std::uint64_t tick,frame;};
    std::optional<Epoch> epoch_;
public:
    bool anchored() const noexcept {return epoch_.has_value();}
    void clear() noexcept {epoch_.reset();}
    void anchor(std::uint64_t tick,std::uint64_t frame) noexcept {epoch_=Epoch{tick,frame};}
    std::uint64_t frame(std::uint64_t tick) const {
        if(!epoch_ || tick<epoch_->tick)
            throw std::invalid_argument("Campaign audio requires its current source/device epoch");
        constexpr std::uint64_t frames_per_tick=48000/30;
        const auto ticks=tick-epoch_->tick;
        if(ticks>(std::numeric_limits<std::uint64_t>::max()-epoch_->frame)/frames_per_tick)
            throw std::overflow_error("Campaign audio sample deadline exceeds its output clock");
        return epoch_->frame+ticks*frames_per_tick;
    }
};
}
