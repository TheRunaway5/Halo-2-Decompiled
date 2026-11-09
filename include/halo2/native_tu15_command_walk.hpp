#pragma once
#include "halo2/native_tu15_command_emit.hpp"

namespace halo2 {
// These services run synchronously against the same retained memory. The
// 45CD0 packet is caller-owned and survives every intervening call;
// unwritten fields and padding must never be synthesized by the adapter.
struct NativeTu15CommandPartServices {
    std::uint32_t packet{};
    std::function<std::uint32_t(std::uint32_t unit,std::uint32_t owner_bits,NativeTu15PhysicsFrameFp&)> geometry_449d0;
    std::function<bool(std::uint32_t unit,std::uint32_t part,std::uint32_t matrix,NativeTu15PhysicsFrameFp&)> prepare_45cd0;
    std::function<void(const NativeTu15CommandEmitArgs&,NativeTu15PhysicsFrameFp&)> emit_41030;
};
struct NativeTu15CommandFrameServices {
    NativeTu15CommandPartServices parts;
    std::function<void(std::uint32_t context,std::uint32_t mode,NativeTu15PhysicsFrameFp&)> prepare_41c70;
    std::function<void(NativeTu15PhysicsFrameFp&)> producer_179a40,producer_179b40,
        producer_1770c0,producer_42090,producer_41630;
    std::function<void(std::uint32_t mask,NativeTu15PhysicsFrameFp&)> producer_42750;
};
// Whole TU1.5 functions. 460C0 resolves live geometry and reads signed part
// ranges directly. 44AB0 pins unit inputs at the source instructions, then
// rereads masks and the original geometry's part count after each service.
bool native_tu15_command_part_visible_460c0(const SourcePhysicsContactMemory&,
    std::uint32_t unit,std::uint32_t part,std::uint32_t mask);
void native_tu15_command_parts_44ab0(SourcePhysicsContactMemory&,std::uint32_t mode,
    std::uint32_t flag_mask,std::uint32_t group,NativeTu15PhysicsFrameFp&,const NativeTu15CommandPartServices&);
struct NativeTu15CommandGroup44c80 {
    std::uint32_t mode,flag_mask,section,mask,add,opaque_mask,opaque_add,use_override;
};
// 44C80 updates the caller's mask/add argument slots when it encounters
// an opaque unit. Their replacement survives all later units in this call.
std::uint32_t native_tu15_command_group_44c80(SourcePhysicsContactMemory&,
    NativeTu15CommandGroup44c80,std::uint32_t caller_stack,NativeTu15PhysicsFrameFp&,
    const NativeTu15CommandPartServices&);
// Group non-overlapping sections behind each unvisited kind-1 seed.
unsigned native_tu15_queue_camouflage_42090(SourcePhysicsContactMemory&,NativeTu15PhysicsFrameFp&);
// Append kind-3 sections in source order. Returns the number of pool rows
// emitted and leaves all record fields outside the original stores intact.
unsigned native_tu15_queue_sections_42750(SourcePhysicsContactMemory&,std::uint32_t mask,NativeTu15PhysicsFrameFp&);
void native_tu15_command_frame_40e80(SourcePhysicsContactMemory&,std::uint32_t selector,
    NativeTu15PhysicsFrameFp&,const NativeTu15CommandFrameServices&);
class PhysicsLiveWorldTu15;
struct SourcePhysicsContactRuntimeAbi;
NativeTu15PhysicsFrameFp source_native_command_frame_tu15(PhysicsLiveWorldTu15&,std::uint32_t selector=0);
void source_native_transparent_collection_tu15(PhysicsLiveWorldTu15&,SourcePhysicsContactRuntimeAbi&);
// Try the retained 3BC20 command selection frame. Refusal makes no writes.
bool source_native_command_selection_3baa0(PhysicsLiveWorldTu15&,SourcePhysicsContactRuntimeAbi&,const void* camera_receipts);
}
