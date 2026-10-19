#pragma once
#include "halo2/source_campaign_camera_tu15.hpp"

namespace halo2 {
struct CinematicCameraFrame {
    std::uint64_t tick{},command{},restores{},bsp{};
    bool paired{};
    float fraction{};
    SourceCampaignCameraViewTu15 completed;
};
// Native presentation history only. Source camera/tick ownership stays intact.
class CinematicCameraInterpolation {
public:
    explicit CinematicCameraInterpolation(bool reference_held=false):reference_held_(reference_held){}
    void clear(){valid_=paired_=false;}
    SourceCampaignCameraViewTu15 sample(const CinematicCameraFrame &);
private:
    SourceCampaignCameraViewTu15 before_{},current_{};
    std::uint64_t tick_{},command_{},restores_{},bsp_{};
    bool valid_{},paired_{},reference_held_{};
    bool observed_subframe_{};
    unsigned eligible_{},stable_{31};
    float fraction_{};
};
}
