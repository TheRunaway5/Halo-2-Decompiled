#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace halo2 {
struct Vec3;
struct SourceViewProjectionBank;
struct SourceShaderLodDefinition {
    std::uint16_t bias{}; // Raw selected shader+0x3C, not geometry LOD.
    std::vector<float> thresholds; // Actual selected template's 10-byte LOD+6.
};
struct SourceShaderLodView {
    std::array<float,7> multipliers{}; // Literal 485B28..485B40.
};
// Complete 1BEB0. Top/bottom are the actual signed-short outer camera bounds;
// auxiliary+5 changes only bias6, after the separate bias0 value is stored.
SourceShaderLodView source_shader_lod_view(std::int16_t top,std::int16_t bottom,
    bool auxiliary_present,bool auxiliary_byte5);
// Actual 2C490 literal stored at4670E4 (.85/.3/.25); distinct from4BA028.
float source_shader_lod_player_multiplier(unsigned local_players);
struct SourceShaderCoverageCamera {
    std::array<float,4> inverse_view_z{}; //4B9E20/2C/38/44.
    float viewport_height_metric{}; //4B9ED0, actual2FD90 output+BC.
};
SourceShaderCoverageCamera source_shader_coverage_camera(const SourceViewProjectionBank &);
// Original133390/133520/133790 coverage. Cluster bounds are the literal
// +54..68 min/max bank; instance center/radius are actual+3C/+48; object
// center/radius are the actual cached runtime sphere. No geometry fitting.
float source_shader_cluster_coverage(const std::array<float,6> &bounds,
    const SourceShaderCoverageCamera &);
float source_shader_instance_coverage(Vec3 center,float radius,
    const SourceShaderCoverageCamera &);
// Original13362D..13368F/322B0C. Zero skips the instance command before
// shader selection; the byte is also the source instance alpha payload.
std::uint8_t source_shader_instance_alpha(float actual_coverage);
struct SourceShaderStipple {
    std::uint8_t level{}; // Literal nearest-even1C290 selector,0..16.
    std::array<std::uint32_t,32> rows{}; //1C1B0/3F8CC0 32×32 source pattern.
};
SourceShaderStipple source_shader_instance_stipple(std::uint8_t alpha);
// Main2234E0→2B5D0→12A50 frame descriptor, unrelated to MSAA sample count.
inline constexpr std::int32_t source_shader_gameplay_frame_type=3;
//44370 literal25-layer table; view frame types4..7 force15680 mode0.
bool source_shader_layer_stipple(unsigned source_layer,std::int32_t render_frame_type);
float source_shader_object_coverage(Vec3 cached_center,float cached_radius,
    float actual_4ba028_multiplier,const SourceShaderCoverageCamera &);
// Actual2CBF0 slice/16AE0 floor and wrapping int edge cases, then0..255 clamp.
std::uint8_t source_shader_coverage_byte(float actual_coverage);
// 410F8..4111A: original collector byte, already-resolved 45CE0 part scale,
// and the actual 2C490-produced 4670E4 local-player multiplier.
float source_shader_part_metric(std::uint8_t collector_byte,
    float part_lod_scale,float local_player_multiplier);
struct SourceShaderLodChoice {
    std::int32_t index{};
    bool accepted{}; // Literal 1BF50 AL return. An empty source bank gives -1/true.
};
// Complete ordinary shad-path 1BF50 arithmetic. Bias5 bypasses the threshold
// scan; every other valid bias uses its actual view multiplier and 4BA019.
// Dynamic MTPR/prt3 template virtual services are not supplied by this helper.
SourceShaderLodChoice source_shader_lod_choice(std::span<const float> thresholds,
    unsigned bias,float part_metric,const SourceShaderLodView &,bool view_modifier);
}
