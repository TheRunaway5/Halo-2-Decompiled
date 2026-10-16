#pragma once
#include <string_view>
namespace halo2::pstencil {
// User-directed later-asset treatment. These are the current armor/helmet
// materials, deliberately excluding Arbiter cloth, flesh, eyes and teeth.
inline bool arbiter_metal(std::string_view shader) {
    return shader=="objects\\characters\\dervish\\shaders\\dervish_armor" ||
        shader=="objects\\characters\\dervish\\shaders\\dervish_helmet";
}
inline constexpr float arbiter_specular_companion=1.f;
inline constexpr unsigned arbiter_specular_copies=3;
inline constexpr float arbiter_environment_gain=2.f;
}
