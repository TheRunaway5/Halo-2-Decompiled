#include "halo2/native_vehicle_camera.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
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
}
NativeVehicleCamera::NativeVehicleCamera(MultiplayerObjectRuntime &runtime,const OriginalData &data)
    :runtime_(runtime),data_(data) {
  if(data.image!=OriginalData::Image::TitleUpdate15 || !runtime.owns_physics() ||
      !data.observer_constants || !data.observer_world_constants || !data.death_camera_constants)
    throw FormatError("Native vehicle camera requires authenticated TU1.5 live owners");
}
std::optional<NativeDeathCameraView> NativeVehicleCamera::update(unsigned index,
    std::uint32_t unit,std::uint16_t flags,float yaw,float pitch,float seconds,
    const NativeDeathCameraView &ordinary) {
  if(index>=views_.size())throw FormatError("Native vehicle camera view exceeds its actual local bank");
  const auto &c=*data_.observer_constants;const auto &w=*data_.observer_world_constants;
  const auto &d=*data_.death_camera_constants;
  auto &view=views_[index];
  const auto frame=runtime_.observer_world_frame(std::int32_t(index),unit,flags,saved_clusters_);
  if(view.unit!=unit) {
    view=View{};view.unit=unit;
    source_observer_reset(view.observer,std::int16_t(frame.bsp),c,w);
  }
  std::optional<DeathCameraPacket> seated;
  if(unit!=0xffffffffu) {
    if(!view.seated) {
      // Literal1584D0..158512 producer construction: zero flags/deltas,
      // NONE parent, signed NONE seat, and the source unit zoom scale one.
      view.producer={};view.producer[2]=0xffffffffu;view.producer[3]=0xffffu;
      view.producer[6]=std::bit_cast<std::uint32_t>(c.scalar.one);
    }
    seated=runtime_.vehicle_camera_packet(index,unit,yaw,pitch,view.producer);
  }
  DeathCameraPacket packet{};
  if(seated)packet=*seated;
  else {
    packet[0]=1;vector(packet,4,ordinary.position);vector(packet,0x2c,ordinary.forward);
    vector(packet,0x38,ordinary.up);
    packet[0x28/4]=std::bit_cast<std::uint32_t>(ordinary.horizontal_fov);
  }
  source_observer_repair_packet(packet,c,d,w);
  source_observer_register_adopt(view.observer,packet,c);
  source_observer_motion(view.observer,source_scope_camera_dt(seconds,c.scalar),c);
  view.seated=bool(seated);
  if(!seated)return {};
  source_observer_world(view.observer,frame,runtime_.observer_world_services(),c,d,w);
  return NativeDeathCameraView{vector(view.observer,0xb8),vector(view.observer,0xd8),
      vector(view.observer,0xe4),scalar(view.observer,0xf0)};
}
} // namespace halo2
