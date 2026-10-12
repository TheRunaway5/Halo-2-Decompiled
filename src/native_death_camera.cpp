#include "halo2/native_death_camera.hpp"
#include "halo2/death_camera_binding.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
float scalar(const SourceObserverState &s,unsigned offset) {
  return std::bit_cast<float>(s.at(offset/4));
}
Vec3 vector(const SourceObserverState &s,unsigned offset) {
  return {scalar(s,offset),scalar(s,offset+4),scalar(s,offset+8)};
}
void vector(DeathCameraPacket &p,unsigned offset,Vec3 v) {
  p.at(offset/4)=std::bit_cast<std::uint32_t>(v.x);
  p.at(offset/4+1)=std::bit_cast<std::uint32_t>(v.y);
  p.at(offset/4+2)=std::bit_cast<std::uint32_t>(v.z);
}
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t address) {
  const auto bytes=m.view(address,4);std::uint32_t value{};
  for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);
  return value;
}
}
NativeDeathCamera::NativeDeathCamera(MultiplayerObjectRuntime &runtime,
    const OriginalData &data,std::uint32_t &random)
    :runtime_(runtime),data_(data),random_(random) {
  if(!data_.observer_constants || !data_.observer_world_constants || !data_.death_camera_constants)
    throw FormatError("Native death view requires authenticated original camera operands");
}
std::optional<NativeDeathCameraView> NativeDeathCamera::update(unsigned index,
    std::uint32_t own,std::uint16_t flags,bool team_only,float seconds,
    bool game_in_progress,bool clock_running,bool cycle_pressed,
    const NativeDeathCameraView &ordinary) {
  if(index>=views_.size())throw FormatError("Native death camera view is outside its local bank");
  const auto players=source_death_camera_players(runtime_.memory(),runtime_.player_pool());
  const auto player=std::find_if(players.begin(),players.end(),[&](const auto &p){return p.datum==own;});
  if(player==players.end())throw FormatError("Native death view lost its actual source player");
  const auto &c=*data_.observer_constants;
  const auto &w=*data_.observer_world_constants;
  const auto &d=*data_.death_camera_constants;
  auto &view=views_[index];
  const auto frame=runtime_.observer_world_frame(std::int32_t(index),player->current_unit,flags,saved_clusters_);
  if(view.player!=own) {
    view=View{};view.player=own;
    source_observer_reset(view.observer,std::int16_t(frame.bsp),c,w);
  }
  if(!(seconds>0.f))return view.death?view.presented:std::nullopt;
  const auto dt=source_scope_camera_dt(seconds,c.scalar);
  if(player->current_unit!=0xffffffffu || !view.presented) {
    // Native ordinary-camera packet delivery. Zero durations are actual native
    // caller inputs; the complete observer owns their adoption and derivatives.
    DeathCameraPacket packet{};packet[0]=1;
    vector(packet,4,ordinary.position);vector(packet,0x2c,ordinary.forward);
    vector(packet,0x38,ordinary.up);
    packet[0x28/4]=std::bit_cast<std::uint32_t>(ordinary.horizontal_fov);
    source_observer_repair_packet(packet,c,d,w);
    source_observer_register_adopt(view.observer,packet,c);
    source_observer_motion(view.observer,dt,c);
    view.presented=ordinary;
    if(player->current_unit!=0xffffffffu){view.death.reset();return {};}
  }
  //1573B0's multiplayer current-unit test selects23FB00 once; a new current
  //unit restores the native ordinary camera above. No source view row is forged.
  if(!view.death)view.death=source_death_camera_create(view.presented->position,
      std::int8_t(index),own,players,random_,d);
  const auto clock=runtime_.source_clock();
  const auto rate=std::bit_cast<std::int16_t>(std::uint16_t(word(runtime_.memory(),clock)>>16));
  const DeathCameraFrame input{seconds,rate,view.presented->position,view.presented->up,
      cycle_pressed,team_only,game_in_progress,clock_running};
  auto packet=source_death_camera_packet(*view.death,players,
      [this](auto object){return runtime_.camera_target(object);},input,d,c,w);
  source_observer_register_adopt(view.observer,packet,c);
  source_observer_motion(view.observer,dt,c);
  source_observer_world(view.observer,frame,runtime_.observer_world_services(),c,d,w);
  view.presented=NativeDeathCameraView{vector(view.observer,0xb8),
      vector(view.observer,0xd8),vector(view.observer,0xe4),scalar(view.observer,0xf0)};
  return view.presented;
}
} // namespace halo2
