#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <bit>
namespace halo2 {
void MultiplayerMatch::adopt_source_devices() {
  if(!simulation_.source_physics_movement_)return;
  auto &devices=simulation_.devices_;
  devices.synchronize(simulation_.scene_);
  auto snapshot=devices.snapshot(simulation_.ticks());
  if(snapshot.owners.empty() && snapshot.groups.empty())return;
  const auto &memory=source_object_world_->memory();
  const auto word=[&](auto address){return Bytes(memory.view(address,4)).u32(0);};
  const auto pool=word(0x4ecbf8u);
  if(!pool || word(pool+0x24u)!=12u || snapshot.groups.size()>word(pool+0x20u))
    throw FormatError("TU1.5 device readback requires its actual original group pool");
  SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
  const auto group=[&](std::uint32_t datum) {
    const auto record=source_raw_datum_lookup(pool,datum,storage);
    if(!record)throw FormatError("TU1.5 device readback lost an actual full group identity");
    const Bytes row(memory.view(record,12));
    return SourceDeviceGroup{std::bit_cast<float>(row.u32(4)),row.u16(2)};
  };
  // Whole10C180 resolves authored named-group ordinals directly through the
  // original12-byte pool rows and rejects their private-group bit4.
  for(unsigned index=0;index<snapshot.groups.size();++index) {
    const auto row=word(pool+0x44u)+index*12u;
    const auto salt=Bytes(memory.view(row,2)).u16(0);
    snapshot.groups[index]=group((std::uint32_t(salt)<<16u)|index);
    if(snapshot.groups[index].flags&4u)throw FormatError("TU1.5 named device group resolves to a private row");
  }
  for(auto &entry:snapshot.owners) {
    auto &owner=entry.state;const auto body=source_object_world_->object_body(owner.handle);
    if(source_object_world_->object_definition(owner.handle)!=entry.definition)
      throw FormatError("TU1.5 device readback changed its actual definition");
    // Whole10C790 binds130/13C and copies134/140. Whole10B840 owns
    // both velocities,12C flags and the signed148 timer.
    owner.motion.position=std::bit_cast<float>(word(body+0x140u));
    owner.motion.position_velocity=std::bit_cast<float>(word(body+0x144u));
    owner.motion.power=std::bit_cast<float>(word(body+0x134u));
    owner.motion.power_velocity=std::bit_cast<float>(word(body+0x138u));
    owner.motion.flags=word(body+0x12cu);
    owner.motion.delay_ticks=std::int16_t(Bytes(memory.view(body+0x148u,2)).u16(0));
    const auto &definition=*content_.devices.at(owner.placement);
    if(definition.placement.power_group<0)owner.private_power=group(word(body+0x130u));
    if(definition.placement.position_group<0)owner.private_position=group(word(body+0x13cu));
    if(definition.machine) {
      // Actual1179C0 reads/writes these machine-specific fields.
      owner.door.flags=word(body+0x1ccu);owner.door.open_ticks=std::bit_cast<std::int32_t>(word(body+0x1d0u));
    }
    if(definition.control)owner.control_flags=word(body+0x1ccu);
  }
  // This is an authority view of already completed source state. It cannot
  // bind a replica, advance another motion clock or produce a group result.
  devices.observe_source_snapshot(snapshot);
}
}
