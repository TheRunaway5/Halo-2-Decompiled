#pragma once
#include "halo2/model.hpp"
namespace halo2 {
// Exact authored one-frame maximum-to-new-minimum scale reset, followed by
// an explicit hold to the clip's end. No magnitude or ratio threshold.
struct ModelScaleReset {
    unsigned node{};
    float visible_min{},visible_max{},hidden{};
    bool operator==(const ModelScaleReset &)const=default;
};
std::vector<ModelScaleReset> model_terminal_scale_resets(const RenderModel &,const AnimationClip &);
bool model_scale_reset(const ModelScaleReset &,float before,float after);
}
