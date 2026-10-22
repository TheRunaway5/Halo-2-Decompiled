#include "halo2/source_unit_cinematic_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include <bit>
namespace halo2 {
float source_unit_cinematic_emotion_seconds_tu15(std::int16_t frames,float factor) {
    volatile float seconds=float(frames)*factor;return seconds;
}
void source_unit_cinematic_emotion_tu15(PhysicsLiveWorldTu15 &w,std::uint32_t object,
    std::uint32_t name,float weight,std::int16_t frames) {
    if(object==0xffffffffu)return; // Original2AD979..2AD97E admission.
    const auto factor=Bytes(w.memory().view(0x46a17cu,4)).f32(0);
    const auto seconds=source_unit_cinematic_emotion_seconds_tu15(frames,factor);
    auto a=w.prepare_source_call(std::array{std::bit_cast<std::uint32_t>(weight),std::bit_cast<std::uint32_t>(seconds)});
    a.gp[0]=object;a.gp[1]=name;w.run_actual(0x111080u,a);
}
}
