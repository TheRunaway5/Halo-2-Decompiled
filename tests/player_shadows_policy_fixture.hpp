#pragma once
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_live_model_draws_tu15.hpp"
#include "halo2/source_live_shadow_main_inputs_tu15.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_geometry_lod.hpp"
#include <bit>
#include <nlohmann/json.hpp>

namespace halo2::shadow_test {
inline nlohmann::json graphics_policy(const halo2::SourceRuntimeMemory &memory) {
    const auto word=[&](std::uint32_t at){return halo2::Bytes(memory.view(at,4)).u32(0);};
    const auto features=memory.view(0x4c6840u,6);
    std::vector<unsigned> bytes;for(auto value:features)bytes.push_back(std::to_integer<unsigned>(value));
    return {{"outer_view_count",word(0x4c686cu)},{"feature_bytes",bytes},
        {"ordinary_maximum",word(0x4c683cu)},{"coverage_bits",word(0x4c6848u)},
        {"secondary_bits",word(0x4c684cu)},{"effect_bits",word(0x473644u)}};
}
inline void require_graphics_policy(const nlohmann::json &actual,unsigned players) {
    const auto expected=halo2::source_geometry_lod_policy(players);
    if(actual["outer_view_count"]!=players || actual["feature_bytes"]!=std::vector<unsigned>(
        expected.feature_bytes.begin(),expected.feature_bytes.end()) ||
        actual["ordinary_maximum"]!=expected.ordinary_maximum ||
        actual["coverage_bits"]!=std::bit_cast<std::uint32_t>(expected.coverage_multiplier) ||
        actual["secondary_bits"]!=std::bit_cast<std::uint32_t>(expected.secondary_scale) ||
        actual["effect_bits"]!=std::bit_cast<std::uint32_t>(expected.effect_scale))
        throw halo2::FormatError("Actual native main caller did not publish its complete recipient view policy/count");
}
// Declared ordinary view0 camera for these test-only caller controls. Executes
// whole native2C470/1BE90; no output bank, feature byte or4C686C is supplied.
// It establishes source policy inputs, not a completed GPU camera receipt.
inline nlohmann::json main_caller_policy(halo2::MultiplayerObjectRuntime &objects,unsigned players,halo2::Vec3 eye) {
    using namespace halo2;
    if(players!=1&&players!=2)throw FormatError("View-policy fixture supplies only its declared one/two-player layout");
    auto &world=objects.tu15_world();world.begin_model_frame();
    SourceLiveModelDrawContextTu15 context;context.local_players=players;
    context.entry.eye=context.draw.eye=eye;context.entry.distance_scale=1;context.forward={1,0,0};
    // Native signed-short rectangles are top/left/bottom/right.
    context.draw.viewport={0,0,std::int16_t(players==1?480:240),640};
    SourceFrustumProjectionInput projection;projection.projection_angle=1.2217304706573486f;
    projection.outer=projection.safe=context.draw.viewport;
    context.draw.rendering=source_view_projection({eye,context.forward,{0,0,1},.015625f,1024.f,{},
        source_frustum_projection(projection).projection_program});
    source_live_shadow_main_user_tu15(world,0);
    source_live_shadow_main_inputs_tu15(world,context);
    const auto result=graphics_policy(world.memory());require_graphics_policy(result,players);return result;
}
}
