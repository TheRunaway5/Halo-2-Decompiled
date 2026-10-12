#pragma once
#include <array>
#include <cstdint>

namespace halo2 {
struct Vec3;
//133790 retains x87 double precision through squared distance, square root
//and camera+2C multiplication, then stores the single source metric.
float source_geometry_lod_distance(Vec3 cached_sphere_center,Vec3 camera_position,
                                  float camera_distance_scale);
//2F970 writes4BA019 from the actual camera+58/+68 bytes and optionalscnr+10.
bool source_geometry_lod_view_modifier(bool camera_58,bool camera_68,
                                      bool scenario_present,std::int16_t scenario_type);
struct SourceGeometryLodDefinition {
    bool model_present{};
    // Literal hlmt+28..48.4C450 validates descending +34..44 and zero +48;
    // invalid/absent banks select the original10/8/6/4/2/0 defaults.
    std::array<float,9> model_bank{};
};
struct SourceGeometryLodChoice {int lod{};float transition{};};
//4C450: draw_metric is the actual caller's normalized draw distance. Previous
//LOD is the retained signed byte for this object and source view, NONE=-1.
SourceGeometryLodChoice source_geometry_lod_choice(const SourceGeometryLodDefinition &,
    float draw_metric,std::int8_t previous_lod=-1);
struct SourceGeometryLodPolicy {
    int ordinary_maximum{};
    float coverage_multiplier{},secondary_scale{},effect_scale{};
    std::array<std::uint8_t,6> feature_bytes{}; // Original4BA020..25.
};
//2C490's actual local-player-count input, including its default branch.
SourceGeometryLodPolicy source_geometry_lod_policy(unsigned local_players);
//4BCC0 world-object branch: actual admitted PRT flag chooses cap4;
//object+4 bit19 forces5. This does not emulate residency or visibility.
int source_geometry_lod_cap(SourceGeometryLodChoice,const SourceGeometryLodPolicy &,
                            std::uint32_t object_flags,bool admitted_prt);
}
