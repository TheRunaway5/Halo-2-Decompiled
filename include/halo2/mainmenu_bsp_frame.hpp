#pragma once
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/menu_projection.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/source_portal_mask.hpp"

namespace halo2 {
struct MainmenuCameraLocation {
    BspAtmosphere::CameraLocation current{},retained{};
    bool query_succeeded{};
};
// Original2B720 current output and2B790 history commit are separate. On a
// failed query only a valid previous pair becomes the local current output;
// persistent history is unchanged even when that pair is invalid.
MainmenuCameraLocation mainmenu_camera_location(const BspAtmosphere &,Vec3 eye,
    BspAtmosphere::CameraLocation previous);

// Exact16E210 membership from the actual aggregate datum. An absent4E64C8
// word is supported only when both possible bit4 branches have equal outputs.
// Scenario/BSP presence here is established by the source-owned loaded bank.
std::vector<std::uint8_t> mainmenu_bsp_fog_admission(const BspAtmosphere &,
    std::uint32_t planar_datum,std::optional<std::uint32_t> flags_4e64c8);

struct MainmenuBspFrameInput {
    const Camera &camera; // Actual canonical menu_render_camera result.
    BspAtmosphere::CameraLocation previous_location;
    const SourcePortalMaskState &portals; // Persistent actual512bit bank.
    SourceBspShaderLodAuxiliary cinematic; // Actual510C50 presence and byte5.
    std::optional<std::uint32_t> flags_4e64c8;
};
struct MainmenuBspAdmissionFrame {
    BspAtmosphere::CameraLocation current_location{},retained_location{};
    bool location_query_succeeded{};
    SourceViewProjectionBank render_camera;
    SourceFrustumCamera clipping_camera;
    BspFogSample fog;
    SourcePortalVisibilitySet visibility;
    SourceBspAdmission admission;
    SourceBspShaderLodAuxiliary cinematic;
    bool view_modifier{};
    // References into this frame. The caller must keep it alive for selection.
    SourceBspShaderView shader_view() const &;
    SourceBspShaderView shader_view() const && = delete;
};
// Source-owned canonical standard-video, one-player normal-main-mode0 producer:
// 2F970/2FB70/2FD90,2B720/14A280,12E5E0,441B0,173130 and134390. Actual Cache
// metadata supplies every authored bank. No screen ID, captured owner ID or
// final draw packet is an input. It requires a successful current inside-BSP
// query; outside fog/visibility retention and native aspect extension remain
// unsupported. This does not produce partial subpart masks or activate draws.
class MainmenuBspFrameProducer {
public:
    MainmenuBspFrameProducer(const Cache &,std::size_t bsp);
    MainmenuBspAdmissionFrame prepare(const MainmenuBspFrameInput &) const;
private:
    BspAtmosphere atmosphere_;
    SourcePortalBsp portals_;
    SourceBspAdmissionBsp geometry_;
};
}
