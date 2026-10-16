#include "halo2/native_vehicle_camera.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/death_camera_binding.hpp"
#include <bit>
#include <algorithm>

namespace halo2 {
namespace {
template<class Address> std::uint32_t word(const SourceRuntimeMemory &bank,Address address) {
  return Bytes(bank.view(address,4)).u32(0);
}
float scalar(const SourceObserverState &s,unsigned offset) {
  return std::bit_cast<float>(s.at(offset/4));
}
Vec3 vector(const SourceObserverState &s,unsigned offset) {
  return {scalar(s,offset),scalar(s,offset+4),scalar(s,offset+8)};
}
}
NativeVehicleCamera::NativeVehicleCamera(MultiplayerObjectRuntime &runtime,const OriginalData &data)
    :runtime_(runtime),data_(data) {
  if(data.image!=OriginalData::Image::TitleUpdate15 || !runtime.owns_physics() ||
      !data.observer_constants || !data.observer_world_constants || !data.death_camera_constants)
    throw FormatError("Native vehicle camera requires authenticated TU1.5 live owners");
  random_=word(runtime_.memory(),word(runtime_.memory(),0x4f3cd8u)+4u);
}
std::optional<NativeDeathCameraView> NativeVehicleCamera::update(unsigned index,
    std::uint32_t unit,std::uint16_t flags,float seconds) {
  if(index>=views_.size())throw FormatError("Native vehicle camera view exceeds its actual local bank");
  const auto player=unit==0xffffffffu?views_[index].player:
      word(runtime_.memory(),runtime_.object_body(unit)+0x13cu);
  if(player==0xffffffffu)return {};
  return update_player(index,player,flags,seconds);
}
std::optional<NativeDeathCameraView> NativeVehicleCamera::update_player(unsigned index,
    std::uint32_t player,std::uint16_t flags,float seconds,bool cycle_pressed) {
  if(index>=views_.size())throw FormatError("Native player camera view exceeds its actual local bank");
  if(player==0xffffffffu){views_[index]=View{};return {};}
  const auto players=source_death_camera_players(runtime_.memory(),runtime_.player_pool());
  const auto own=std::find_if(players.begin(),players.end(),[&](const auto &p){return p.datum==player;});
  if(own==players.end())throw FormatError("Native player camera lost its full source player identity");
  const auto unit=own->current_unit;
  const auto &c=*data_.observer_constants;const auto &w=*data_.observer_world_constants;
  const auto &d=*data_.death_camera_constants;
  auto &view=views_[index];
  const auto frame=runtime_.observer_world_frame(std::int32_t(index),unit,flags,saved_clusters_);
  if(view.player!=player) {
    view=View{};view.player=player;
    source_observer_reset(view.observer,std::int16_t(frame.bsp),c,w);
  }
  std::uint8_t camera_mode{};
  auto packet=runtime_.player_camera_packet(player,seconds,view.controller,view.observer,random_,cycle_pressed,&camera_mode);
  const bool death=death_view(index);
  const bool seated=view.controller[0xc/4]==0x245ea0u;
  bool transitioning=scalar(view.observer,0x9c)>0 || view.seated ||
      std::bit_cast<float>(view.controller[4/4])>0;
  for(unsigned group=1;group<6;++group)transitioning|=scalar(view.observer,0x9c+4*group)>0;
  //157FE0/171FF0 already repaired and registered the actual packet. Adopt
  // and integrate it through the retained complete six-channel observer.
  source_observer_register_adopt(view.observer,packet,c);
  source_observer_motion(view.observer,source_scope_camera_dt(seconds,c.scalar),c);
  // Death and respawn use the same retained six observer channels. Keep
  // presenting their pending transitions after158450 restores the normal
  // producer, just as we do after a vehicle-seat exit.
  view.handing_off=seated || death || (view.handing_off && transitioning);
  view.seated=seated;
  source_observer_world(view.observer,frame,runtime_.observer_world_services(),c,d,w);
  if(!view.handing_off && !death)return {};
  return NativeDeathCameraView{vector(view.observer,0xb8),vector(view.observer,0xd8),
      vector(view.observer,0xe4),scalar(view.observer,0xf0),scalar(view.observer,0x100),camera_mode};
}
} // namespace halo2
