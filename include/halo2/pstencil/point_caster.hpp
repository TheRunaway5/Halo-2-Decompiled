#pragma once
#include "halo2/menu_world_source_binding.hpp"
#include <functional>

namespace halo2 {
class PhysicsLiveWorldTu15;
struct SourceDrawUnitState;
struct SourceCullDrawUnit;
struct SourceEffectLightRuntime;
}

namespace halo2::pstencil {
// Owned source position inputs from the actual light query, before color-view
// availability and receiver range filtering. Shadow-only faces need no color
// material/pass. The immutable geometry is shared; current palettes are copied.
struct PointCaster {
    MenuWorldSourceBinding source;
    std::uint32_t light_identity{},light_domain{};
    Vec3 light_position;
    std::uint8_t opacity=255;
    bool color_available=true;
};
// Lazy position-only cache. Constructed only by the optional pStencil path;
// no visible material/shader pass or main-camera resource bank is required.
class PointCasterBank {
public:
    PointCasterBank(const Cache&,unsigned bsp);
    ~PointCasterBank();
    void collect(const SourceDrawUnitState&,PhysicsLiveWorldTu15*,
        const SourceEffectLightRuntime&,const MenuWorldSourceConstants&,
        std::uint32_t domain,const std::function<bool(const SourceCullDrawUnit&)>& color_available,
        std::vector<PointCaster>&);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
