#pragma once
#include "halo2/native_tu15_activation_services.hpp"
#include <map>
namespace halo2 {
struct NativeTu15ContactBodyUpdateScratch {
    std::uint32_t point_velocity{},impulse{}; // Distinct caller-owned, aligned16 DATA packets.
};
struct NativeTu15ContactBodyUpdateServices {
    std::function<std::uint32_t(std::uint32_t manager,NativeTu15ContactVirtualOwner,
        std::uint16_t contact,NativeTu15PhysicsFrameFp&,unsigned)> point;
    std::function<std::uint32_t(std::uint32_t manager,NativeTu15ContactVirtualOwner,
        std::uint16_t contact,NativeTu15PhysicsFrameFp&,unsigned)> properties;
    std::function<void(std::uint32_t motion,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&,unsigned)> inspect_motion_type;
    std::function<void(std::uint32_t motion,NativeTu15ContactVirtualOwner,
        std::uint32_t point,std::uint32_t output,NativeTu15PhysicsFrameFp&,unsigned)> point_velocity;
    // Actual1E1170 semantic boundary: source body, impulse16B and contact point16B.
    // The supplied native owner must perform real wake and motion impulse work.
    std::function<void(NativeTu15PhysicsBody,std::uint32_t impulse,std::uint32_t point,
        NativeTu15PhysicsFrameFp&,unsigned)> apply_point_impulse;
};
struct NativeTu15ContactBodyUpdateStats {
    std::uint64_t calls{},point_queries{},property_queries{},motion_type_queries{},
        property_publications{},reversed_normals{},object_flags{},point_velocity_queries{},
        impulse_requests{},unqualified_services{},unsupported_fp_profiles{};
    std::map<std::uint32_t,std::uint64_t> unqualified_owners;
};
struct NativeTu15ContactBodyImpulseServices {
    std::function<void(NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&,unsigned)> activate_body;
    std::function<void(std::uint32_t motion,NativeTu15ContactVirtualOwner,
        std::uint32_t impulse,std::uint32_t point,NativeTu15PhysicsFrameFp&,unsigned)> motion_impulse;
};
// Actual323020/323040. Current point-ID indirection and backing reads stay in
// source order. Returned identities are typed DATA pointers, never CPU state.
std::uint32_t native_tu15_contact_manager_point(const SourcePhysicsContactMemory&,
    std::uint32_t manager,std::uint16_t contact);
std::uint32_t native_tu15_contact_manager_properties(const SourcePhysicsContactMemory&,
    std::uint32_t manager,std::uint16_t contact);
// Actual1D7B60, including original component/object/tag and vehicle flag gates.
void native_tu15_publish_contact_body_property(SourcePhysicsContactMemory&,
    std::uint32_t component_datum,std::uint32_t row,std::uint32_t properties,
    const SourcePhysicsContactRecordObserver& = {});
// Actual323A10. Numeric inputs/output are ordinary caller-owned source DATA;
// preserve the cross-product store before rereading current linear velocity.
void native_tu15_contact_motion_point_velocity(SourcePhysicsContactMemory&,
    std::uint32_t motion,std::uint32_t point,std::uint32_t output,
    NativeTu15PhysicsFrameFp&,const SourcePhysicsContactRecordObserver& = {});
// Actual324F80, including source-ordered impulse reread after linear velocity
// publication and current inertia/rotation/angular-velocity rereads.
void native_tu15_apply_contact_motion_point_impulse(SourcePhysicsContactMemory&,
    std::uint32_t motion,std::uint32_t impulse,std::uint32_t point,
    NativeTu15PhysicsFrameFp&,const SourcePhysicsContactRecordObserver& = {});
// Actual1E1170 plus its caller-used315C90 boolean. A sleeping body delegates
// to the genuine qualified315CB0 service; unknown motion owners refuse.
void native_tu15_apply_contact_body_point_impulse(SourcePhysicsContactMemory&,
    NativeTu15PhysicsBody,std::uint32_t impulse,std::uint32_t point,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15ContactBodyImpulseServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactBodyUpdateStats* = nullptr);
// Complete1D77D0 semantic algorithm. Unknown virtual/native impulse owners
// remain explicit typed requirements; no original executor or GP image exists.
void native_tu15_update_contact_body(SourcePhysicsContactMemory&,std::uint32_t manager,
    std::uint32_t row,bool skip_impulse,bool reversed,std::uint32_t component_datum,
    std::uint32_t body_slot,NativeTu15ContactBodyUpdateScratch,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15ContactBodyUpdateServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactBodyUpdateStats* = nullptr);
}
