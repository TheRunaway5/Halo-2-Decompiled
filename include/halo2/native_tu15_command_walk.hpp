#pragma once
#include "halo2/native_tu15_physics_frame.hpp"

namespace halo2 {
// These services run synchronously against the same retained memory. The
// genuine 45CD0 packet is caller-owned and survives every intervening call;
// unwritten fields and padding must never be synthesized by the adapter.
struct NativeTu15CommandPartServices {
    std::uint32_t packet{};
    std::function<std::uint32_t(std::uint32_t unit,std::uint32_t owner_bits,NativeTu15PhysicsFrameFp&)> geometry_449d0;
    std::function<bool(std::uint32_t unit,std::uint32_t part,std::uint32_t matrix,NativeTu15PhysicsFrameFp&)> prepare_45cd0;
    std::function<void(std::uint32_t mode,std::uint32_t coverage,std::uint32_t shader,bool grouped,
        std::uint32_t unit,std::uint32_t part,NativeTu15PhysicsFrameFp&)> emit_41030;
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
void native_tu15_command_frame_40e80(SourcePhysicsContactMemory&,std::uint32_t selector,
    NativeTu15PhysicsFrameFp&,const NativeTu15CommandFrameServices&);
class PhysicsLiveWorldTu15;
NativeTu15PhysicsFrameFp source_native_command_frame_tu15(PhysicsLiveWorldTu15&,std::uint32_t selector=0);
}
