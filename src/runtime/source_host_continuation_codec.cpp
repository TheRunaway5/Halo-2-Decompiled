#include "network/multiplayer_host_continuation_data.hpp"
#include "runtime/continuation_wire.hpp"
#include "halo2/source_authority_binding.hpp"
#include <boost/pfr/core.hpp>
#include <limits>

namespace halo2 {
namespace {
using namespace continuation_wire;
constexpr std::size_t host_limit=512*1024;
constexpr std::string_view host_magic="H2HOST02";
void valid_binding(const LanHost::ContinuationBinding &b){
  need(b.context.host<16&&(b.context.peers&(1u<<b.context.host))&&
      std::ranges::any_of(b.context.session,[](auto v){return v!=std::byte{};})&&b.world.id&&b.world.bytes&&
      std::ranges::any_of(b.world.digest,[](auto v){return v!=std::byte{};}));
}
void binding(Writer &w,const LanHost::ContinuationBinding &b){
  w.octets(b.context.session);w.u(b.context.epoch,4);w.u(b.context.membership_revision,8);
  w.u(b.context.peers,2);w.u(b.context.host,1);w.u(b.world.id,8);w.u(b.world.tick,8);w.u(b.world.bytes,8);w.octets(b.world.digest);
}
LanHost::ContinuationBinding binding(Reader &r){
  LanHost::ContinuationBinding b;r.octets(b.context.session);b.context.epoch=r.u(4);b.context.membership_revision=r.u(8);
  b.context.peers=r.u(2);b.context.host=r.u(1);b.world.id=r.u(8);b.world.tick=r.u(8);b.world.bytes=r.u(8);r.octets(b.world.digest);return b;
}
void intent(Writer &w,const std::optional<SourcePlayerActionTarget> &v){
  w.boolean(v.has_value());if(v){w.u(std::bit_cast<std::uint16_t>(v->action),2);w.u(std::bit_cast<std::uint16_t>(v->index),2);w.u(v->object,4);}
}
auto intent(Reader &r){
  std::optional<SourcePlayerActionTarget> v;if(r.boolean()){
    SourcePlayerActionTarget t;t.action=std::bit_cast<std::int16_t>(std::uint16_t(r.u(2)));
    t.index=std::bit_cast<std::int16_t>(std::uint16_t(r.u(2)));t.object=r.u(4);
    need(t.action==0?(t.index==0&&t.object==null_datum):(t.action>=4&&t.action<=6&&t.index>=0&&t.index<64&&t.object!=null_datum));v=t;
  }return v;
}
void controls(Writer &w,const Controls &c){
  static_assert(boost::pfr::tuple_size_v<Controls> ==27);
  for(float v:{c.forward,c.right,c.yaw,c.pitch})w.real(v);
  for(bool v:{c.jump,c.crouch,c.fire,c.reload,c.skip_cinematic})w.boolean(v);
  w.u(std::bit_cast<std::uint16_t>(c.zoom),2);w.u(c.zoom_reset_serial,4);
  for(bool v:{c.grenade,c.pickup,c.dual_wield,c.switch_weapon,c.fire_secondary,c.switch_grenade})w.boolean(v);
  w.u(c.pickup_milliseconds,2);w.u(c.dual_wield_milliseconds,2);
  for(bool v:{c.accept,c.cancel,c.melee,c.flashlight,c.start,c.back})w.boolean(v);
  w.real(c.look_stick);intent(w,c.vehicle_interaction_target);
}
Controls controls(Reader &r){
  Controls c;c.forward=r.real();c.right=r.real();c.yaw=r.real();c.pitch=r.real();
  c.jump=r.boolean();c.crouch=r.boolean();c.fire=r.boolean();c.reload=r.boolean();c.skip_cinematic=r.boolean();
  c.zoom=std::bit_cast<std::int16_t>(std::uint16_t(r.u(2)));c.zoom_reset_serial=r.u(4);
  c.grenade=r.boolean();c.pickup=r.boolean();c.dual_wield=r.boolean();c.switch_weapon=r.boolean();
  c.fire_secondary=r.boolean();c.switch_grenade=r.boolean();c.pickup_milliseconds=r.u(2);c.dual_wield_milliseconds=r.u(2);
  c.accept=r.boolean();c.cancel=r.boolean();c.melee=r.boolean();c.flashlight=r.boolean();c.start=r.boolean();c.back=r.boolean();
  c.look_stick=r.real();c.vehicle_interaction_target=intent(r);
  need(std::abs(c.forward)<=1&&std::abs(c.right)<=1&&std::abs(c.yaw)<=10000&&std::abs(c.pitch)<=10000);return c;
}
void camera_view(Writer &w,const std::optional<NativeDeathCameraView> &v){
  static_assert(boost::pfr::tuple_size_v<NativeDeathCameraView> ==6);
  w.boolean(v.has_value());if(v){for(float f:{v->position.x,v->position.y,v->position.z,v->forward.x,v->forward.y,v->forward.z,
      v->up.x,v->up.y,v->up.z,v->horizontal_fov,v->aim_displacement})w.real(f);w.u(v->camera_mode,1);}
}
auto camera_view(Reader &r){
  std::optional<NativeDeathCameraView> out;if(r.boolean()){
    NativeDeathCameraView v;v.position={r.real(),r.real(),r.real()};v.forward={r.real(),r.real(),r.real()};
    v.up={r.real(),r.real(),r.real()};v.horizontal_fov=r.real();v.aim_displacement=r.real();v.camera_mode=r.u(1);
    need(v.horizontal_fov>0&&v.horizontal_fov<3.14159265358979323846f&&
        std::abs(length(v.forward)-1.f)<=.01f&&std::abs(length(v.up)-1.f)<=.01f&&std::abs(dot(v.forward,v.up))<=.01f);out=v;
  }return out;
}
auto native_roster(std::span<const SourcePlayerAdmission> input){
  std::vector<SourcePlayerAdmission> result(input.begin(),input.end());need(!result.empty()&&result.size()<=16);
  std::ranges::sort(result,{},&SourcePlayerAdmission::slot);std::uint16_t slots{};
  for(const auto &p:result){need(p.slot<16&&p.player!=null_datum&&(p.player>>16)&&(p.player&65535u)==p.slot&&!(slots&(1u<<p.slot)));
    slots|=std::uint16_t(1u<<p.slot);}return result;
}
bool same_admissions(std::span<const OnlineAdmission> a,std::span<const OnlineAdmission> b){
  if(a.size()!=b.size())return false;for(unsigned i=0;i<a.size();++i)
    if(a[i].slot!=b[i].slot||a[i].name!=b[i].name||a[i].team!=b[i].team||a[i].identity!=b[i].identity||
        online_admission_peer(a[i])!=online_admission_peer(b[i])||a[i].guest!=b[i].guest)return false;return true;
}
auto view_admissions(const auto &saved,const auto &peer){
  std::vector<SourcePlayerAdmission> views;for(auto slot:peer.ids){
    auto found=std::ranges::find(saved.roster,slot,&SourcePlayerAdmission::slot);need(found!=saved.roster.end());views.push_back(*found);
  }return views;
}
void valid_data(const auto &s,const LanHost::ContinuationBinding &b){
  valid_binding(b);need(s.binding==source_authority_binding(b.context)&&s.tick==b.world.tick&&
      s.configuration.size()>=10&&s.configuration.size()<=65536&&s.admissions.size()<=16&&!s.admissions.empty()&&s.peers.size()<=16);
  need(native_roster(s.roster)==s.roster);std::uint16_t seats{},local{},endpoints{},remote{};
  for(const auto &p:s.admissions){const auto owner=online_admission_peer(p);
    // Admission/PCR rows outlive active transport membership. A departed host
    // remains an independently accepted report identity in the complete cut.
    // Matchmaking uses one placement group per player in FFA (0..15).
    // These admission groups are separate from the native selected-team
    // requests below, which still require -1 or an actual source team0..7.
    need(p.slot<16&&owner<16&&!(seats&(1u<<p.slot))&&p.name.size()<=1024&&p.team>=-1&&p.team<=15);
    seats|=std::uint16_t(1u<<p.slot);}
  for(const auto &p:s.roster)need(seats&(1u<<p.slot));
  for(auto player:s.local_players)if(player!=null_datum){
    const auto p=std::ranges::find(s.roster,player,&SourcePlayerAdmission::player);need(p!=s.roster.end()&&!(local&(1u<<p->slot)));
    const auto a=std::ranges::find(s.admissions,p->slot,&OnlineAdmission::slot);
    need(a!=s.admissions.end()&&online_admission_peer(*a)==b.context.host);local|=std::uint16_t(1u<<p->slot);
  }
  for(const auto &p:s.peers){need(p.endpoint<16&&p.endpoint!=b.context.host&&(b.context.peers&(1u<<p.endpoint))&&
      !(endpoints&(1u<<p.endpoint))&&!p.ids.empty()&&p.ids.size()<=4&&p.selected_teams.size()==p.ids.size()&&p.profiles.size()==p.ids.size());
    endpoints|=std::uint16_t(1u<<p.endpoint);std::vector<unsigned> ids;
    if(p.last_snapshot){const auto &v=*p.last_snapshot;
      need(v.fingerprint==s.fingerprint&&v.nonce==p.nonce&&v.cookie==p.cookie&&v.sequence==p.snapshot_sequence&&
          v.tick<=s.tick&&v.bytes>0&&v.bytes<=8u*1024u*1024u);}
    for(const auto &a:s.admissions)if(online_admission_peer(a)==p.endpoint)ids.push_back(a.slot);need(ids==p.ids);
    (void)view_admissions(s,p);
    for(unsigned i=0;i<p.ids.size();++i){const auto slot=p.ids[i];need(!(remote&(1u<<slot))&&!(local&(1u<<slot)));
      remote|=std::uint16_t(1u<<slot);need(p.selected_teams[i]>=-1&&p.selected_teams[i]<=7);}
    for(unsigned view=0;view<4;++view){
      need(!(p.vehicle_views[view]&&p.death_views[view]));
      if(p.vehicle_views[view])need(p.vehicle_views[view]->camera_mode==0||p.vehicle_views[view]->camera_mode==1||p.vehicle_views[view]->camera_mode==3);
      if(view>=p.ids.size())need(!p.vehicle_views[view]&&!p.death_views[view]);
    }
    if(!p.camera){need(!p.camera_tick);for(unsigned i=0;i<4;++i)need(!p.vehicle_views[i]&&!p.death_views[i]);}
  }
}
auto idle_nanoseconds(std::chrono::nanoseconds value){
  need(value>=decltype(value)::zero()&&value.count()<=std::numeric_limits<std::int64_t>::max());return std::uint64_t(value.count());
}
auto idle_duration(std::uint64_t count){
  need(count<=std::uint64_t(std::numeric_limits<std::int64_t>::max()));const auto ns=std::chrono::nanoseconds(std::int64_t(count));
  return ns;
}
}
std::vector<std::byte> LanHost::encode_continuation(const Continuation &cut,const ContinuationBinding &bound){
  using namespace continuation_wire;need(bool(cut.data_));const auto &s=*cut.data_;valid_data(s,bound);
  static_assert(boost::pfr::tuple_size_v<Continuation::Data> ==8);
  static_assert(boost::pfr::tuple_size_v<Continuation::Data::Peer> ==21);
  static_assert(boost::pfr::tuple_size_v<MultiplayerSnapshotStamp> ==7);
  static_assert(boost::pfr::tuple_size_v<OnlineAdmission> ==6&&boost::pfr::tuple_size_v<SourcePlayerAdmission> ==3);
  Writer w{host_limit,{}};w.u(native_multiplayer_protocol,2);binding(w,bound);w.octets(s.binding);w.u(s.fingerprint,8);w.u(s.tick,8);
  w.blob(s.configuration,65536);for(auto p:s.local_players)w.u(p,4);w.u(s.admissions.size(),1);
  for(const auto &p:s.admissions){w.u(p.slot,1);w.string(p.name);w.i32(p.team);w.octets(p.identity);w.u(p.peer,4);w.boolean(p.guest);}
  w.u(s.roster.size(),1);for(const auto &p:s.roster){w.u(p.slot,1);w.u(p.player,4);w.octets(p.identity);}
  w.u(s.peers.size(),1);for(const auto &p:s.peers){
    w.u(p.endpoint,1);w.u(p.nonce,8);w.u(p.cookie,8);w.u(p.ids.size(),1);for(auto slot:p.ids)w.u(slot,1);
    for(auto team:p.selected_teams)w.u(std::bit_cast<std::uint8_t>(team),1);
    for(const auto &profile:p.profiles){w.boolean(profile.has_value());if(profile)w.octets(*profile);}
    w.u(p.sequence,4);w.u(p.snapshot_sequence,4);w.boolean(p.last_snapshot.has_value());
    if(p.last_snapshot){const auto &v=*p.last_snapshot;w.u(v.fingerprint,8);w.u(v.nonce,8);w.u(v.cookie,8);
      w.u(v.sequence,4);w.u(v.tick,4);w.u(v.bytes,4);w.octets(v.digest);}
    w.boolean(p.seen);w.u(idle_nanoseconds(p.idle),8);
    for(const auto &c:p.controls)controls(w,c);
    for(const auto &bank:p.received_presses)for(auto v:bank)w.u(v,4);
    for(const auto &bank:p.delivered_presses)for(auto v:bank)w.u(v,4);
    for(const auto &bank:p.completed)for(auto v:bank)w.u(v,2);
    for(const auto &v:p.completed_vehicle)intent(w,v);
    w.boolean(p.camera.has_value());if(p.camera)w.blob(NativeVehicleCamera::encode_checkpoint(*p.camera),8192);
    w.boolean(p.camera_tick.has_value());if(p.camera_tick)w.u(*p.camera_tick,4);
    for(const auto &v:p.vehicle_views)camera_view(w,v);for(const auto &v:p.death_views)camera_view(w,v);
    for(auto serial:p.camera_jump_serial)w.u(serial,4);
  }
  return frame(host_magic,w.bytes,host_limit);
}
LanHost::Continuation LanHost::decode_continuation(std::span<const std::byte> bytes,const ContinuationBinding &expected,
    std::uint64_t fingerprint,std::span<const OnlineAdmission> admissions,std::span<const SourcePlayerAdmission> roster){
  using namespace continuation_wire;valid_binding(expected);const auto accepted=native_roster(roster);
  need(admissions.size()<=16&&!admissions.empty());Reader r{unframe(bytes,host_magic,host_limit)};
  need(r.u(2)==native_multiplayer_protocol&&binding(r)==expected);auto s=std::make_shared<Continuation::Data>();
  r.octets(s->binding);s->fingerprint=r.u(8);s->tick=r.u(8);const auto configuration=r.blob(65536);
  s->configuration.assign(configuration.begin(),configuration.end());for(auto &p:s->local_players)p=r.u(4);
  const auto count=r.u(1);need(count>0&&count<=16);s->admissions.resize(count);
  for(auto &p:s->admissions){p.slot=r.u(1);p.name=r.string();p.team=r.i32();r.octets(p.identity);p.peer=r.u(4);p.guest=r.boolean();}
  const auto players=r.u(1);need(players>0&&players<=16);s->roster.resize(players);
  for(auto &p:s->roster){p.slot=r.u(1);p.player=r.u(4);r.octets(p.identity);}
  need(s->fingerprint==fingerprint&&s->binding==source_authority_binding(expected.context)&&s->tick==expected.world.tick&&
      same_admissions(s->admissions,admissions)&&s->roster==accepted);
  const auto peers=r.u(1);need(peers<=16);s->peers.resize(peers);
  for(auto &p:s->peers){
    p.endpoint=r.u(1);p.nonce=r.u(8);p.cookie=r.u(8);const auto views=r.u(1);need(views>0&&views<=4);p.ids.resize(views);
    for(auto &slot:p.ids)slot=r.u(1);p.selected_teams.resize(views);for(auto &team:p.selected_teams)team=std::bit_cast<std::int8_t>(std::uint8_t(r.u(1)));
    p.profiles.resize(views);for(auto &profile:p.profiles)if(r.boolean()){profile.emplace();r.octets(*profile);}
    p.sequence=r.u(4);p.snapshot_sequence=r.u(4);
    if(r.boolean()){auto &v=p.last_snapshot.emplace();v.fingerprint=r.u(8);v.nonce=r.u(8);v.cookie=r.u(8);
      v.sequence=r.u(4);v.tick=r.u(4);v.bytes=r.u(4);r.octets(v.digest);}
    p.seen=r.boolean();p.idle=idle_duration(r.u(8));
    for(auto &c:p.controls)c=controls(r);
    for(auto &bank:p.received_presses)for(auto &v:bank)v=r.u(4);
    for(auto &bank:p.delivered_presses)for(auto &v:bank)v=r.u(4);
    for(auto &bank:p.completed)for(auto &v:bank)v=r.u(2);
    for(auto &v:p.completed_vehicle)v=intent(r);
    if(r.boolean())p.camera=NativeVehicleCamera::decode_checkpoint(r.blob(8192),s->binding,std::uint32_t(s->tick),view_admissions(*s,p));
    if(r.boolean())p.camera_tick=r.u(4);
    for(auto &v:p.vehicle_views)v=camera_view(r);for(auto &v:p.death_views)v=camera_view(r);
    for(auto &serial:p.camera_jump_serial)serial=r.u(4);
  }
  r.end();valid_data(*s,expected);Continuation result;result.data_=std::move(s);return result;
}
}
