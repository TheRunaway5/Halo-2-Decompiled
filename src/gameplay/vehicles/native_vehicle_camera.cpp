#include "halo2/native_vehicle_camera.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/death_camera_binding.hpp"
#include "halo2/source_original_camera_seed.hpp"
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
    std::uint32_t player,std::uint16_t flags,float seconds,bool cycle_pressed,
    std::optional<NativeDeathCameraView> *completed) {
  return update_player_impl(index,player,flags,seconds,cycle_pressed,nullptr,completed);
}
std::array<std::byte,0x5c> NativeVehicleCamera::collect_player_input(unsigned index,
    std::uint32_t player,float frame_seconds,float game_seconds) {
  if(index>=views_.size() || player==0xffffffffu)
    throw FormatError("Native input requires a live local player/view");
  const auto players=source_death_camera_players(runtime_.memory(),runtime_.player_pool());
  const auto own=std::find_if(players.begin(),players.end(),[&](const auto&p){return p.datum==player;});
  if(own==players.end())throw FormatError("Native input lost its full source player identity");
  auto&view=views_[index];
  if(view.player!=player) {
    view=View{};view.player=player;
    // Before the first normal post-frame camera phase the genuine157EB0
    // default remains mode4.1AD4F0 must see that state, including rejection.
    view.controller=runtime_.tu15_world().original_camera_seed().controller_;
    const auto rows=word(runtime_.memory(),word(runtime_.memory(),0x4f54f4u)+0x44u);
    const auto flags=word(runtime_.memory(),rows+(player&0xffffu)*0x21cu);
    const auto frame=runtime_.observer_world_frame(std::int32_t(index),own->current_unit,
        std::uint16_t(flags>>16),saved_clusters_);
    source_observer_reset(view.observer,std::int16_t(frame.bsp),*data_.observer_constants,
        *data_.observer_world_constants);
  }
  return runtime_.tu15_world().collect_native_input(player,index,frame_seconds,game_seconds,
      view.controller,view.observer);
}
std::optional<NativeDeathCameraView> NativeVehicleCamera::update_player_impl(unsigned index,
    std::uint32_t player,std::uint16_t flags,float seconds,bool cycle_pressed,const SourceOriginalCameraSeed *seed,
    std::optional<NativeDeathCameraView> *completed) {
  if(completed)completed->reset();
  if(seed&&!seed->available())throw FormatError("Admitted camera lacks its locally initialized original seed");
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
    // A typed admitted owner uses only the locally produced157EB0 default.
    // Its active byte prevents the lower adapter's zero-controller fallback
    // from selecting a controller out of an imported mutable native bank.
    if(seed)view.controller=seed->controller_;
    source_observer_reset(view.observer,std::int16_t(frame.bsp),c,w);
  }
  if(seed&&!(view.controller[0]&255u))
    throw FormatError("Admitted camera lost its locally initialized controller");
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
  const NativeDeathCameraView result{vector(view.observer,0xb8),vector(view.observer,0xd8),
      vector(view.observer,0xe4),scalar(view.observer,0xf0),scalar(view.observer,0x100),camera_mode};
  // The ordinary245060 ->CD4A0 ->E18D0 owner corrects the eye from the
  // actual posed head as pitch changes. Retain its completed observer too;
  // root plus standing/crouching height cannot replace that source result.
  if(completed)*completed=result;
  if(!view.handing_off && !death)return {};
  return result;
}
} // namespace halo2
