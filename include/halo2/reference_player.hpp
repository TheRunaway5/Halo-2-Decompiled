#pragma once
#include "halo2/appearance.hpp"
#include "halo2/first_person_ik.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_view_clear.hpp"

namespace halo2 {
struct ReferenceDeviceState {
    std::uint32_t source_object_datum{},definition{},render_model{},source_object_flags{};
    unsigned object_type{},scenario_placement{};
    // Authored channel position/power followed by source root position,
    // forward/up and object+A0 scale, all independently retained float words.
    std::array<std::uint32_t,12> words{};
    float position{},power{},root_scale{};
    Vec3 root_position,root_forward,root_up;
};
// Diagnostic original-frame inputs. These values are retained from guest
// memory and the identified GPU draw, never selected from native defaults.
// This fixture does not restore simulation, animation controllers or history.
struct ReferencePlayerState {
    std::string map, map_sha256, executable_sha256;
    unsigned pose_frame{};
    std::uint32_t unit_datum{}, unit_definition{}, weapon_datum{}, weapon_definition{};
    Vec3 camera_position, camera_forward, camera_up;
    float camera_vertical_angle{};
    float camera_scale{}; // 2F800 camera+2C lighting/grid factor, not clip scale.
    Vec3 root_position, root_velocity; // Physics unit+270, not animated object+30.
    // Version3 actual retained OWNER object+30/+3C. Earlier fixture versions
    // leave this absent; physics position or FP geometry cannot supply it.
    std::optional<ShadowCasterSphere> owner_sphere;
    // Version4 actual main132D0 entry inputs. Currently admitted only when
    // this initialization defines target0 color and depth/stencil history.
    std::optional<SourceViewClearState> view_clear;
    ObjectColors colors;
    // Object +126 runtime floats and render context +64 packed colors differ
    // slightly. 33980/131BB0 consumes the latter; colors above is that bank.
    std::array<std::uint32_t,12> object_color_words{};
    std::array<std::uint32_t,4> packed_colors{};
    // 3DB00's exact 84-byte retained state. Last word is flags, not a float.
    std::array<std::uint32_t,21> lighting_words{};
    ObjectLightingState root_lighting;
    // Actual lighting draws c26/c27 independently corroborate the retained
    // record. Base texture passes can retain unrelated unused constants.
    std::array<std::uint32_t,6> draw_light_direction_words{};
    ObjectLightingState first_person_lighting; // Corroborated by lighting draws.
    // Paused final context (HUD), for provenance only. Material shaders use
    // their own authored brightness; these are not FP draw brightness inputs.
    float retained_context_specular_brightness{}, retained_context_ambient_brightness{};
    std::uint32_t aiming_word{};
    FirstPersonAimBlend aiming;
    // Version2 finite image inputs. The solved source device channel phase is
    // an image fixture, not a production timer/controller-state restoration.
    std::vector<ReferenceDeviceState> devices;
};

// Bounded versioned text format; source floats are hexadecimal IEEE754 words.
// Unknown, missing, trailing, nonfinite or inconsistent fields are rejected.
ReferencePlayerState load_reference_player_state(const std::filesystem::path &);
} // namespace halo2
