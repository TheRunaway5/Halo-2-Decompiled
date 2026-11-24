#pragma once
// Native TU1.5 tag group fixups, entered at 11EF70 (physics live support
// unit): the map loader's pass over every tag, by its group, before the
// netgame and combat postprocessing.
//
//   tag_fixups::run   11EF70()
//
// The tag iterator (125790, its 0x14-byte state in the frame: +0xC zero,
// +0x10 NONE) answers each tag's handle until NONE. By the tag's group (+0
// of its 16-byte instance at [4F0414], by the handle's sign-extended index):
// a creature's block (+0xD4 of its data, +8) and a biped's (+0x264) go to
// 1CAF40 (EDX), a collision model to 21A580, a physics model to 1F2C70 (0
// pushed) and a vehicle's block (+0x2AC) to 20D810 (EAX); other groups are
// passed over. 11F060 (netgame postprocessing) and 165B30 (combat
// postprocessing) follow.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the iterator's state in the original's
// frame; the boundary adapter (native_tu15_tag_fixups.hpp) maps the role to
// the original instruction and owns the ABI of the children.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_tag_fixups {
using native_object_update::widen16;
inline constexpr std::uint32_t none=0xffffffffu;
namespace globals {inline constexpr std::uint32_t tag_instances=0x4f0414u;}
inline constexpr std::uint32_t instance_size=16u,instance_group=0u,instance_data=8u;
namespace group {
inline constexpr std::uint32_t creature=0x63726561u,biped=0x62697064u,collision=0x636f6c6cu,physics=0x70686d6fu,vehicle=0x76656869u;
}
namespace block {inline constexpr std::uint32_t creature=0xd4u,biped=0x264u,vehicle=0x2acu;}
namespace iterator {inline constexpr std::uint32_t position=0xcu,current=0x10u;}

namespace tag_fixups {
enum class Access {position_clear,current_clear,instances,group,creature_data,biped_data,vehicle_data};
enum class Outcome {done};
// Frame:
//   iterator(offset)      the iterator's state now
//   next() -> handle      125790(EDI the iterator)
//   creature_block(block), biped_block(block)   1CAF40(EDX the block)
//   collision(handle)     21A580
//   physics(handle)       1F2C70(0 pushed)
//   vehicle_block(block)  20D810(EAX the block)
//   postprocess()         11F060, then 165B30
template<class Memory> std::uint32_t data_of(Memory &memory,Access role,std::uint32_t handle,std::uint32_t instances) {
    return memory.load32(role,instance_size*(handle&0xffffu)+instances+instance_data);
}
template<class Memory,class Frame> Outcome run(Memory &memory,Frame &frame) {
    using enum Access;
    memory.store32(position_clear,frame.iterator(iterator::position),0u);
    memory.store32(current_clear,frame.iterator(iterator::current),none);
    for(auto handle=frame.next();handle!=none;handle=frame.next()) {
        const auto instances_address=memory.load32(instances,globals::tag_instances);
        switch(memory.load32(Access::group,instance_size*widen16(handle)+instances_address+instance_group)) {
        case group::creature:frame.creature_block(data_of(memory,creature_data,handle,instances_address)+block::creature);break;
        case group::biped:frame.biped_block(data_of(memory,biped_data,handle,instances_address)+block::biped);break;
        case group::collision:frame.collision(handle);break;
        case group::physics:frame.physics(handle);break;
        case group::vehicle:frame.vehicle_block(data_of(memory,vehicle_data,handle,instances_address)+block::vehicle);break;
        default:break;
        }
    }
    frame.postprocess();
    return Outcome::done;
}
} // namespace tag_fixups
} // namespace halo2::native_tag_fixups
