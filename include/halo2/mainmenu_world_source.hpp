#pragma once
#include "halo2/mainmenu_bsp_frame.hpp"
#include "halo2/mainmenu_bsp_ranges.hpp"
#include "halo2/menu_world_source_binding.hpp"
#include "halo2/menu_world_source_pass.hpp"
#include <map>
#include <memory>
#include <string_view>

namespace halo2 {
enum class MainmenuWorldSourceReject {
    not_selected, non_instance, partial_instance, incomplete_bsp_ranges, instance_fade,
    unsupported_base_pass, unsupported_fog_pass, missing_fog_material,
    unsupported_class1, external_attribute, missing_constant
};
std::string_view mainmenu_world_source_reject_name(MainmenuWorldSourceReject);
struct MainmenuWorldSourceFog {
    unsigned implementation{};
    // The actual selected child, not a reload of its unanimated shader tag.
    std::shared_ptr<const Material> material;
    MenuWorldSourceBinding binding;
    MenuWorldSourcePass pass;
};
struct MainmenuWorldSourceRange {
    MenuWorldSourceBinding base;
    MenuWorldSourcePass base_pass;
    std::optional<MainmenuWorldSourceFog> fog;
};
struct MainmenuWorldSourceCandidate {
    std::size_t catalogue_index{};
    SourceBspDrawKey original;
    std::uint32_t selected_material{};
    // One original4DAA0 call per entry. A complete empty vector owns the
    // source key with no submissions, suppressing its whole-part fallback.
    std::vector<MainmenuWorldSourceRange> ranges;
};
struct MainmenuWorldSourceFrame {
    std::vector<MainmenuWorldSourceCandidate> candidates;
    // Exactly one rejection per excluded resident catalogue entry. These are
    // supported-boundary diagnostics, not a substitute for draw admission.
    std::map<MainmenuWorldSourceReject,std::size_t> rejected;
};
// CPU-only assembly for actual fully-contained BSP instances on the existing
// classification3 layer3 path, paired with source-admitted layer16 fog.
// The Cache and resident catalogue must outlive this object. Materials retain
// their original indices; the caller supplies their current animated banks.
// Device/catalogue/texture/raster/context/aspect admission remains the graphics
// consumer's responsibility. This does not activate a draw or partial ranges.
class MainmenuWorldSourcePlan {
public:
    MainmenuWorldSourcePlan(const Cache &,const SourceBspResidentPlan &,std::size_t bsp=0);
    ~MainmenuWorldSourcePlan();
    MainmenuWorldSourcePlan(const MainmenuWorldSourcePlan &)=delete;
    MainmenuWorldSourcePlan &operator=(const MainmenuWorldSourcePlan &)=delete;
    // c0..17 come from the actual source view/fog producers. Each selected
    // animated material supplies c18..33; binding supplies c50 and c170..172.
    // Only consumed constants must be finite. Unconsumed NaN bits survive.
    // The frame must be from this same Cache/BSP's MainmenuBspFrameProducer;
    // frame.shader_view() refers into the frame for the duration of this call.
    MainmenuWorldSourceFrame prepare(const MainmenuBspAdmissionFrame &,
        std::span<const Material>,const MenuWorldSourceConstants &);
    // CPU assembly against this same frame's completed BSP-only range result.
    // It retains every ordered original4DAA0 call, including complete-empty
    // parts. This prefix has not incorporated dynamic133720's shared-arena
    // interaction, so the live renderer must continue to use prepare above.
    MainmenuWorldSourceFrame prepare_bsp_prefix(const MainmenuBspAdmissionFrame &,
        const MainmenuBspRangeFrame &,std::span<const Material>,const MenuWorldSourceConstants &);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
