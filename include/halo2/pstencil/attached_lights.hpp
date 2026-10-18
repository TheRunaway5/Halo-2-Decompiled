#pragma once
#include "halo2/effect_light.hpp"
#include "halo2/transform.hpp"
#include <cstdint>
#include <vector>
namespace halo2 {
class SourceRuntimeMemory;
namespace pstencil {
// Completed TU1.5 C0CD0 attachment and C2CB0 light state. This is a read-only
// presentation snapshot; the native allocation, function/color and timing
// owners remain authoritative, including its actual attachment transform.
struct AttachedLight {
    unsigned slot{};
    std::uint32_t identity=0xffffffffu,definition=0xffffffffu;
    std::uint16_t flags{};
    EffectLightState state;
    Transform frame;
};
std::vector<AttachedLight> attached_lights_tu15(const SourceRuntimeMemory&,
    std::uint32_t object,std::uint32_t definition);
}
}
