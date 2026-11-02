#pragma once
#include "halo2/native_tu15_physics_frame.hpp"

namespace halo2 {
struct NativeTu15RenderPrepareServices {
    std::function<void(std::uint32_t object,std::uint32_t mode,std::uint32_t flags,NativeTu15PhysicsFrameFp&)> update_d7230;
    std::function<void(std::uint32_t mode,std::uint32_t object,NativeTu15PhysicsFrameFp&)> attached_41970;
};
// Whole41C10 child-before-sibling walk. Each body pointer is pinned before
// callbacks, but its child/sibling fields are read at the original phase.
// Object datums retain their high half in service arguments; pool rows use
// only the unsigned low half. The explicit return stack contains data only.
void native_tu15_render_object_prepare_41c10(const SourcePhysicsContactMemory&,
    std::uint32_t mode,std::uint32_t object,std::uint32_t flags,NativeTu15PhysicsFrameFp&,
    const NativeTu15RenderPrepareServices&);
// Whole41C70: the caller's context/mode remain pinned; queue pointer, signed
// count and row pointers are refreshed only where the source refreshes them.
void native_tu15_render_prepare_41c70(const SourcePhysicsContactMemory&,
    std::uint32_t context,std::uint32_t mode,NativeTu15PhysicsFrameFp&,
    const NativeTu15RenderPrepareServices&);
using NativeTu15RenderGeometryRequest=std::function<bool(std::uint32_t header,NativeTu15PhysicsFrameFp&)>;
using NativeTu15RenderUnitReader=std::function<std::uint32_t(std::uint32_t offset)>;
// Whole44930. The reader resolves the current unit, in a native state or
// the original bank. The instance definition read precedes BSP+13C here.
std::uint32_t native_tu15_render_section_44930(const SourcePhysicsContactMemory&,
    const NativeTu15RenderUnitReader&,std::uint8_t selector);
std::uint32_t native_tu15_render_section_44930(const SourcePhysicsContactMemory&,
    std::uint32_t unit,std::uint8_t selector);
// Whole449D0. Request/owner consume their low byte. Optional requests use
// mode3 at the genuine130980 boundary, then reread the pinned section's
// published geometry. A refused request returns zero without that final read.
std::uint32_t native_tu15_render_geometry_449d0(const SourcePhysicsContactMemory&,
    std::uint32_t unit,std::uint8_t request,std::uint8_t owner,NativeTu15PhysicsFrameFp&,
    const NativeTu15RenderGeometryRequest& = {});
std::uint32_t native_tu15_render_geometry_449d0(const SourcePhysicsContactMemory&,
    const NativeTu15RenderUnitReader&,std::uint8_t request,std::uint8_t owner,NativeTu15PhysicsFrameFp&,
    const NativeTu15RenderGeometryRequest& = {});
class PhysicsLiveWorldTu15;
void source_native_render_prepare_tu15(PhysicsLiveWorldTu15&,std::uint32_t context,std::uint32_t mode,NativeTu15PhysicsFrameFp&);
void source_native_render_object_prepare_tu15(PhysicsLiveWorldTu15&,std::uint32_t mode,std::uint32_t object,
    std::uint32_t flags,NativeTu15PhysicsFrameFp&);
std::uint32_t source_native_render_geometry_tu15(PhysicsLiveWorldTu15&,std::uint32_t unit,
    std::uint8_t request,std::uint8_t owner,NativeTu15PhysicsFrameFp&);
}
