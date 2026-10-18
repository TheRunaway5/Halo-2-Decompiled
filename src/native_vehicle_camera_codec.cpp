#include "native_vehicle_camera_continuation_data.hpp"
#include "continuation_wire.hpp"
#include <boost/pfr/core.hpp>

namespace halo2 {
namespace {constexpr std::size_t camera_limit=8192;constexpr std::string_view camera_magic="H2CAM001";}
std::vector<std::byte> NativeVehicleCamera::encode_checkpoint(const Checkpoint &cut){
  using namespace continuation_wire;need(bool(cut.data_));const auto &c=*cut.data_;
  static_assert(boost::pfr::tuple_size_v<Checkpoint::Data> ==6);
  static_assert(boost::pfr::tuple_size_v<View> ==5);
  static_assert(std::tuple_size_v<SourceObserverState> ==214&&std::tuple_size_v<SourceCameraControllerState> ==80);
  Writer w{camera_limit,{}};w.octets(c.binding);w.u(c.tick,4);
  for(unsigned index=0;index<4;++index){const auto &v=c.views[index];
    w.u(v.player,4);w.boolean(v.seated);w.boolean(v.handing_off);w.octets(c.identities[index]);
    for(auto word:v.observer)w.u(word,4);for(auto word:v.controller)w.u(word,4);
  }
  w.i32(c.clusters.bsp);for(auto cluster:c.clusters.clusters)w.i32(cluster);w.u(c.random,4);
  return frame(camera_magic,w.bytes,camera_limit);
}
NativeVehicleCamera::Checkpoint NativeVehicleCamera::decode_checkpoint(std::span<const std::byte> bytes,
    AuthorityBinding binding,std::uint32_t tick,std::span<const SourcePlayerAdmission> expected){
  using namespace continuation_wire;
  need(!expected.empty()&&expected.size()<=4&&std::ranges::any_of(binding,[](auto b){return b!=std::byte{};}));
  std::uint16_t slots{};for(const auto &p:expected){need(p.slot<16&&p.player!=0xffffffffu&&(p.player>>16)&&
      (p.player&65535u)==p.slot&&!(slots&(1u<<p.slot)));slots|=std::uint16_t(1u<<p.slot);}
  Reader r{unframe(bytes,camera_magic,camera_limit)};auto c=std::make_shared<Checkpoint::Data>();r.octets(c->binding);c->tick=r.u(4);
  need(c->binding==binding&&c->tick==tick);
  for(unsigned index=0;index<4;++index){auto &v=c->views[index];v.player=r.u(4);
    v.seated=r.boolean();v.handing_off=r.boolean();r.octets(c->identities[index]);
    for(auto &word:v.observer)word=r.u(4);for(auto &word:v.controller)word=r.u(4);
    if(v.player==0xffffffffu){need(v==View{}&&c->identities[index]==SourcePlayerIdentity{});}
    else need(index<expected.size()&&v.player==expected[index].player&&c->identities[index]==expected[index].identity);
  }
  c->clusters.bsp=r.i32();for(auto &cluster:c->clusters.clusters)cluster=r.i32();c->random=r.u(4);r.end();
  Checkpoint result;result.data_=std::move(c);return result;
}
}
