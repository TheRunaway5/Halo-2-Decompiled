#pragma once
#include <cstdint>
#include <array>
namespace halo2 {
class PhysicsLiveWorldTu15;
struct SourceLiveModelDrawContextTu15;
struct SourceViewProjectionInput;
struct BspFogSample;
// Actual main caller local-user identity, before model/command collection.
// This is distinct from a viewport index and may be-1 for an actual non-user view.
void source_live_shadow_main_user_tu15(PhysicsLiveWorldTu15&,std::int32_t actual_local_user);
// Actual containing BSP cluster from the same completed camera location/fog
// query used for ordinary visibility. Literal2BA31 publishes its EDx caller
// value; zero is not a default camera location. No visibility result supplied.
void source_live_shadow_main_location_tu15(PhysicsLiveWorldTu15&,std::int32_t actual_cluster);
// Four consumed ordinary-collection fog fields from the actual camera sample.
//2BA96 copies these inside its caller fog block;135E38..135E83 consumes them.
// This does not reconstruct the full0x120-byte fog block or supply query flags.
void source_live_shadow_main_collection_fog_tu15(PhysicsLiveWorldTu15&,const BspFogSample&);
// Consumed2BAA1/2BAEF/132B0 camera fields for the actual ordinary2B9F0
// argument2==0 caller, where the global and submitting cameras share inputs.
// This does not replace a full0x74 camera/view state or execute132B0's GPU calls.
// Alternate/scaled views need their distinct caller owner and are rejected.
void source_live_shadow_main_camera_tu15(PhysicsLiveWorldTu15&,
    const SourceViewProjectionInput&,std::array<std::int16_t,4> actual_viewport,
    std::int16_t actual_view_mode,bool actual_scaled);
// Original ordinary caller publications and whole2C470/1BE90, before both
// main native model collection and secondary queries. No queue result supplied.
void source_live_shadow_main_inputs_tu15(PhysicsLiveWorldTu15&,
    const SourceLiveModelDrawContextTu15&);
}
