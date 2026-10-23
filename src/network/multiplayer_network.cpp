#include <cstdlib>
#include "network/multiplayer_host_continuation_data.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/native_request_protocol.hpp"
#include "halo2/lan_admission.hpp"
#include "halo2/source_world_pose_wire.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/source_player_departure.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <random>
#include <set>
#include <vector>
#include <thread>
#include <limits>
#include <mutex>
#include <deque>
#include <type_traits>
#include <openssl/sha.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
namespace halo2 {
namespace {
constexpr unsigned packet_limit = 1400;
// Species-aware consumers are required even though the complete profile was
// already carried: older clients render every received participant as Spartan.
constexpr unsigned protocol_version = native_multiplayer_protocol;
// This is the native port's transport, not the Xbox System Link wire format.
// Keep every datagram below the existing limit and publish complete snapshots
// atomically instead of omitting state to fit a single datagram.
constexpr unsigned snapshot_header = 35;
constexpr unsigned snapshot_chunk = packet_limit - snapshot_header;
// Detached corpses share the source's2048-object lifetime pool. Wider native
// fragment headers retain complete banks across respawns and repeated deaths.
constexpr unsigned snapshot_limit = 8*1024*1024;
enum Type : unsigned {
  hello = 1,
  input = 2,
  snapshot = 3,
  leave = 4,
  reject = 5,
  session_query = 6,
  session_info = 7,
  admission_challenge = 8,
  admission_request = 9
};
constexpr unsigned session_header=31,session_chunk=packet_limit-session_header;
constexpr unsigned session_limit=65536;
using Packet = std::vector<std::byte>;
using Clock = std::chrono::steady_clock;
#ifdef _WIN32
using Socket = SOCKET;
constexpr auto invalid_socket = INVALID_SOCKET;
void close_socket(Socket s) { closesocket(s); }
#else
using Socket = int;
constexpr auto invalid_socket = -1;
void close_socket(Socket s) { close(s); }
#endif
struct Udp {
  Socket fd = invalid_socket;
  std::shared_ptr<MultiplayerDatagrams> relay;
  explicit Udp(std::shared_ptr<MultiplayerDatagrams> transport) : relay(std::move(transport)) {}
  explicit Udp(unsigned port) {
    if (port > 65535)
      throw FormatError("Invalid LAN port");
#ifdef _WIN32
    static const bool started = [] {
      WSADATA data;
      return WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }();
    if (!started)
      throw FormatError("Could not initialize Winsock");
#endif
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == invalid_socket)
      throw FormatError("Could not create LAN UDP socket");
    // Full source-world snapshots can exceed the platform's small default
    // queue before the receiver reassembles them. Reserve bounded queue space;
    // operating systems may clamp this request to their configured cap.
    const int receive_buffer=int(snapshot_limit*2u);
    if(setsockopt(fd,SOL_SOCKET,SO_RCVBUF,reinterpret_cast<const char*>(&receive_buffer),sizeof(receive_buffer))!=0) {
      close_socket(fd);fd=invalid_socket;
      throw FormatError("Could not reserve LAN snapshot receive buffer");
    }
    sockaddr_in at{};
    at.sin_family = AF_INET;
    at.sin_addr.s_addr = htonl(INADDR_ANY);
    at.sin_port = htons(port);
    if (bind(fd, reinterpret_cast<sockaddr *>(&at), sizeof(at)) != 0) {
      close_socket(fd);
      fd = invalid_socket;
      throw FormatError("Could not bind LAN port " + std::to_string(port));
    }
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
#endif
  }
  ~Udp() {
    if (fd != invalid_socket)
      close_socket(fd);
  }
  void send(const Packet &p, const sockaddr_in &to) {
    if (relay) { relay->send(p, ntohl(to.sin_addr.s_addr)-1); return; }
    sendto(fd, reinterpret_cast<const char *>(p.data()), int(p.size()), 0,
           reinterpret_cast<const sockaddr *>(&to), sizeof(to));
  }
  bool receive(Packet &p, sockaddr_in &from) {
    if (relay) {
      unsigned peer{}; if (!relay->receive(p, peer)) return false;
      from={}; from.sin_family=AF_INET; from.sin_addr.s_addr=htonl(peer+1); return true;
    }
    p.resize(packet_limit + 1);
#ifdef _WIN32
    int size = sizeof(from);
#else
    socklen_t size = sizeof(from);
#endif
    const auto n =
        recvfrom(fd, reinterpret_cast<char *>(p.data()), int(p.size()), 0,
                 reinterpret_cast<sockaddr *>(&from), &size);
    if (n <= 0)
      return false;
    p.resize(n);
    return true;
  }
  unsigned port() const {
    if (relay) return 0;
    sockaddr_in at{};
#ifdef _WIN32
    int size = sizeof(at);
#else
    socklen_t size = sizeof(at);
#endif
    if (getsockname(fd, reinterpret_cast<sockaddr *>(&at), &size))
      throw FormatError("Could not read LAN port");
    return ntohs(at.sin_port);
  }
};
bool same(sockaddr_in a, sockaddr_in b) {
  return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}
std::uint64_t random_id() {
  std::random_device random;
  return (std::uint64_t(random()) << 32) | random();
}
void put(Packet &p, std::uint64_t n, unsigned bytes) {
  for (unsigned i = 0; i < bytes; ++i)
    p.push_back(std::byte(n >> (8 * i)));
}
void put_float(Packet &p, float f) {
  put(p, std::bit_cast<std::uint32_t>(f), 4);
}
Packet header(Type t) {
  Packet p;
  put(p, 0x324e3248, 4);
  put(p, protocol_version, 2);
  put(p, t, 1);
  return p;
}
struct Reader {
  const Packet &p;
  std::size_t at{};
  std::uint64_t get(unsigned n) {
    if (n > 8 || at + n > p.size())
      throw FormatError("Truncated LAN packet");
    std::uint64_t v = 0;
    for (unsigned i = 0; i < n; ++i)
      v |= std::uint64_t(std::to_integer<unsigned>(p[at++])) << (8 * i);
    return v;
  }
  int integer() { return std::bit_cast<std::int32_t>(std::uint32_t(get(4))); }
  float real() {
    const float f = std::bit_cast<float>(std::uint32_t(get(4)));
    if (!std::isfinite(f))
      throw FormatError("Non-finite LAN value");
    return f;
  }
  Type type() {
    if (p.size() > packet_limit || get(4) != 0x324e3248 || get(2) != protocol_version)
      throw FormatError("Unknown LAN protocol");
    return Type(get(1));
  }
  void end() {
    if (at != p.size())
      throw FormatError("Trailing LAN packet data");
  }
};
bool newer(std::uint32_t a, std::uint32_t b) {
  return std::bit_cast<std::int32_t>(a - b) > 0;
}
// Preflight one complete reserved group before feeding unchanged packets into
// the ordinary decoder. A partial group cannot advance the receive baseline.
std::vector<std::span<const std::byte>> candidate_packets(std::span<const std::byte> archive,
    std::uint64_t nonce,std::uint64_t cookie,std::uint32_t previous){
  std::size_t at{};
  const auto read=[&](unsigned n){
    if(n>8||at>archive.size()||n>archive.size()-at)throw FormatError("Truncated candidate packet archive");
    std::uint64_t value{};for(unsigned i=0;i<n;++i)value|=std::uint64_t(std::to_integer<unsigned>(archive[at++]))<<(8*i);
    return value;
  };
  if(archive.size()>9ull*1024*1024||read(4)!=0x31534348u||read(2)!=1)throw FormatError("Invalid candidate packet archive");
  const auto count=unsigned(read(2));
  if(!count||count>(snapshot_limit+snapshot_chunk-1)/snapshot_chunk)throw FormatError("Invalid candidate fragment count");
  std::vector<std::span<const std::byte>> result(count);
  std::uint32_t selected{};unsigned extent{};
  for(unsigned i=0;i<count;++i){
    const auto bytes=read(2);
    if(bytes<snapshot_header||bytes>packet_limit||at>archive.size()||bytes>archive.size()-at)
      throw FormatError("Invalid archived native packet extent");
    const auto packet=archive.subspan(at,bytes);at+=bytes;
    Packet copy(packet.begin(),packet.end());Reader native{copy};
    if(native.type()!=snapshot||native.get(8)!=nonce||native.get(8)!=cookie)throw FormatError("Candidate has a foreign native connection");
    const auto sequence=std::uint32_t(native.get(4));const auto index=unsigned(native.get(2));
    const auto fragments=unsigned(native.get(2)),total=unsigned(native.get(4));
    if(!total||total>snapshot_limit||fragments!=count||count!=(total+snapshot_chunk-1)/snapshot_chunk||index>=count||
        packet.size()-native.at!=std::min(snapshot_chunk,total-index*snapshot_chunk)||!newer(sequence,previous)||
        (i&&(sequence!=selected||total!=extent))||!result[index].empty())
      throw FormatError("Candidate is not one complete future native snapshot");
    selected=sequence;extent=total;result[index]=packet;
  }
  if(at!=archive.size())throw FormatError("Trailing candidate packet archive data");
  return result;
}
Packet encode_session_variant(const UiVariant &variant) {
  if(variant.name.size()>1024 || variant.values.size()>4096)
    throw FormatError("Native session configuration exceeds its transport bounds");
  Packet out;put(out,variant.name.size(),2);
  for(unsigned char c:variant.name)put(out,c,1);
  put(out,variant.engine,4);put(out,variant.values.size(),4);
  for(const auto &[parameter,value]:variant.values) {
    put(out,parameter,4);put(out,std::uint32_t(value),4);
  }
  return out;
}
UiVariant decode_session_variant(const Packet &bytes) {
  Reader r{bytes};const auto length=r.get(2);
  if(length>1024)throw FormatError("Native session title exceeds its transport bounds");
  UiVariant out;out.name.clear();
  for(unsigned n=0;n<length;++n)out.name.push_back(char(r.get(1)));
  const auto engine=r.get(4),count=r.get(4);
  if(engine>std::numeric_limits<unsigned>::max() || count>4096)
    throw FormatError("Invalid native session configuration");
  out.engine=unsigned(engine);
  for(unsigned n=0;n<count;++n) {
    const auto parameter=unsigned(r.get(4));const auto value=r.integer();
    out.values.append(parameter,value);
  }
  r.end();return out;
}
LanAdmission::Endpoint admission_endpoint(const sockaddr_in &from){
  LanAdmission::Endpoint result;std::memcpy(result.data(),&from.sin_addr.s_addr,4);
  std::memcpy(result.data()+4,&from.sin_port,2);return result;
}
Packet admission_envelope(const Packet &request,const LanAdmission::Cookie &cookie){
  auto packet=header(admission_request);
  for(auto b:cookie)put(packet,b,1);
  packet.insert(packet.end(),request.begin(),request.end());return packet;
}
LanSessionDescription query_session(Udp &udp,sockaddr_in host,std::chrono::milliseconds timeout) {
  if(timeout<=std::chrono::milliseconds::zero() || timeout>std::chrono::seconds(30))
    throw FormatError("Invalid native session query timeout");
  const auto nonce=random_id();const auto deadline=Clock::now()+timeout;
  auto next=Clock::now();std::uint64_t fingerprint{};unsigned total{},count{},received{};
  std::vector<Packet> fragments;std::vector<bool> present;
  LanAdmission::Cookie admission{};
  while(Clock::now()<deadline) {
    if(Clock::now()>=next) {
      auto request=header(session_query);put(request,nonce,8);
      udp.send(udp.relay?request:admission_envelope(request,admission),host);
      next=Clock::now()+std::chrono::milliseconds(250);
    }
    Packet p;sockaddr_in from{};
    for(unsigned packets=0;packets<256 && udp.receive(p,from);++packets) {
      if(!same(from,host))continue;
      Reader r{p};Type type;
      try{type=r.type();if(r.get(8)!=nonce)continue;}catch(const FormatError &){continue;}
      if(type==admission_challenge&&!udp.relay){
        if(p.size()!=31)continue;
        for(auto &b:admission)b=r.get(1);
        next=Clock::now();continue;
      }
      if(type==reject) {
        std::string reason;while(r.at<p.size())reason.push_back(char(r.get(1)));
        throw FormatError("Native session query rejected: "+reason);
      }
      if(type!=session_info)continue;
      const auto identity=r.get(8);const auto index=unsigned(r.get(2));
      const auto parts=unsigned(r.get(2)),bytes=unsigned(r.get(4));
      if(!bytes || bytes>session_limit || parts!=(bytes+session_chunk-1)/session_chunk ||
          index>=parts || p.size()-r.at!=std::min(session_chunk,bytes-index*session_chunk))
        throw FormatError("Malformed native session fragment");
      if(fragments.empty()) {
        fingerprint=identity;total=bytes;count=parts;fragments.resize(parts);present.resize(parts);
      } else if(fingerprint!=identity || total!=bytes || count!=parts)
        throw FormatError("Native session configuration changed during its query");
      Packet payload(p.begin()+r.at,p.end());
      if(present[index]) {
        if(fragments[index]!=payload)throw FormatError("Conflicting native session fragments");
      }else {fragments[index]=std::move(payload);present[index]=true;++received;}
      if(received==count) {
        Packet body;body.reserve(total);
        for(const auto &fragment:fragments)body.insert(body.end(),fragment.begin(),fragment.end());
        return {fingerprint,decode_session_variant(body)};
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  throw FormatError("Timed out reading the host's native session configuration");
}
void encode_profile(Packet& packet,const std::optional<SourcePlayerConfiguration>& profile) {
  put(packet,profile.has_value(),1);
  if(profile)for(auto byte:*profile)put(packet,byte,1);
}
std::optional<SourcePlayerConfiguration> decode_profile(Reader& reader) {
  const auto present=reader.get(1);
  if(present>1)throw FormatError("Invalid native profile presence byte");
  if(!present)return std::nullopt;
  SourcePlayerConfiguration profile;for(auto& byte:profile)byte=reader.get(1);
  return profile;
}
// Native host-input delivery counters, not original game action state. Repeat
// them in every newer packet so a real short press survives packet loss and
// latest-continuous-input coalescing. Source action admission remains unchanged.
using PressSerials=std::array<std::uint32_t,10>;
using InteractionDurations=std::array<std::uint16_t,2>;
using VehicleIntent=std::optional<SourcePlayerActionTarget>;
void encode_vehicle_intent(Packet &p,const VehicleIntent &intent) {
  put(p,intent.has_value(),1);
  if(intent) {put(p,std::uint16_t(intent->action),2);put(p,std::uint16_t(intent->index),2);put(p,intent->object,4);}
}
VehicleIntent native_vehicle_intent(const std::optional<NativeVehicleIntent> &value) {
  if(!value)return std::nullopt;
  SourcePlayerActionTarget target;
  target.action=value->action;target.index=value->index;target.object=value->object;
  return target;
}
struct WireControl {Controls control;PressSerials presses{};InteractionDurations completed{};VehicleIntent completed_vehicle;};
void encode_control(Packet &p, const Controls &c,const PressSerials &presses,
    const InteractionDurations &completed,const VehicleIntent &completed_vehicle) {
  put_float(p, c.forward);
  put_float(p, c.right);
  put_float(p, c.yaw);
  put_float(p, c.pitch);
  put(p, c.jump | (c.crouch << 1) | (c.fire << 2) | (c.reload << 3) | (c.grenade << 4) | (c.pickup << 5) | (c.dual_wield << 6) |
      (c.switch_weapon << 7) | (c.fire_secondary << 8) | (c.melee << 9) | (c.switch_grenade << 10), 2);
  put(p,std::uint16_t(c.zoom),2);put(p,c.zoom_reset_serial,4);
  put(p,c.pickup_milliseconds,2);put(p,c.dual_wield_milliseconds,2);
  for(auto duration:completed)put(p,duration,2);
  for(auto serial:presses)put(p,serial,4);
  encode_vehicle_intent(p,c.vehicle_interaction_target);
  encode_vehicle_intent(p,completed_vehicle);
}
// Typed field adaptation only: the complete untrusted request was decoded in
// Rust. Native object legality and accepted sequence/seat ownership stay below.
WireControl native_control(const NativeInputControl &value) {
  WireControl wire;auto &c=wire.control;
  c.forward=value.forward;c.right=value.right;c.yaw=value.yaw;c.pitch=value.pitch;
  const auto flags=value.flags;
  c.jump = flags & 1;
  c.crouch = flags & 2;
  c.fire = flags & 4;
  c.reload = flags & 8;
  c.grenade = flags & 16;
  c.pickup=flags&32;c.dual_wield=flags&64;c.switch_weapon=flags&128;c.fire_secondary=flags&256;c.melee=flags&512;c.switch_grenade=flags&1024;
  c.zoom=value.zoom;c.zoom_reset_serial=value.zoom_reset_serial;
  c.pickup_milliseconds=value.pickup_milliseconds;c.dual_wield_milliseconds=value.dual_wield_milliseconds;
  wire.completed=value.completed;wire.presses=value.presses;
  c.vehicle_interaction_target=native_vehicle_intent(value.vehicle);
  wire.completed_vehicle=native_vehicle_intent(value.completed_vehicle);
  return wire;
}
void encode_fp_channel(Packet &p,const FirstPersonChannelState &s) {
  put(p,s.graph,4);put(p,s.animation_identifier,4);put(p,s.delay,1);put(p,s.flags,1);
  put(p,s.options,2);put(p,s.events,2);put(p,s.future_events,2);
  put_float(p,s.rate);put_float(p,s.frame);put(p,s.activity,4);put(p,std::uint16_t(s.fire_ticks),2);
}
FirstPersonChannelState decode_fp_channel(Reader &r) {
  FirstPersonChannelState s;
  s.graph=r.get(4);s.animation_identifier=r.get(4);s.delay=r.get(1);s.flags=r.get(1);
  s.options=r.get(2);s.events=r.get(2);s.future_events=r.get(2);
  s.rate=r.real();s.frame=r.real();s.activity=r.get(4);
  s.fire_ticks=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
  return s;
}
void encode_devices(Packet&p,const SourceDeviceWorldSnapshot&s,std::uint32_t tick){
  validate_source_device_world_snapshot(s);
  if(std::uint32_t(s.tick)!=tick)throw FormatError("native device bank belongs to a different snapshot tick");
  put(p,s.tick,8);put(p,s.groups.size(),4);
  const auto group=[&](const SourceDeviceGroup&g){put_float(p,g.value);put(p,g.flags,2);};
  for(const auto&g:s.groups)group(g);
  put(p,s.owners.size(),2);
  for(const auto&entry:s.owners){const auto&o=entry.state;
    put(p,entry.definition,4);put(p,o.handle,4);put(p,o.placement,4);put(p,o.created_tick,8);
    for(float f:{o.motion.position,o.motion.position_velocity,o.motion.power,o.motion.power_velocity})put_float(p,f);
    put(p,o.motion.flags,4);put(p,std::uint16_t(o.motion.delay_ticks),2);put(p,o.control_flags,4);
    put(p,o.door.flags,4);put(p,std::bit_cast<std::uint32_t>(o.door.open_ticks),4);
    group(o.private_power);group(o.private_position);
    put_float(p,o.root.scale);
    for(const auto&axis:o.root.basis)for(float f:{axis.x,axis.y,axis.z})put_float(p,f);
    for(float f:{o.root.position.x,o.root.position.y,o.root.position.z})put_float(p,f);
    put(p,unsigned(o.bsp_hidden)|(unsigned(o.visible)<<1),1);
  }
}
SourceDeviceWorldSnapshot decode_devices(Reader&r,std::uint32_t tick){
  SourceDeviceWorldSnapshot s;s.tick=r.get(8);
  if(std::uint32_t(s.tick)!=tick)throw FormatError("native device bank belongs to a different snapshot tick");
  const auto groups=r.get(4);if(groups>65536)throw FormatError("native device group capacity exceeded");
  const auto group=[&](){const auto value=r.real();return SourceDeviceGroup{value,std::uint16_t(r.get(2))};};
  for(unsigned i=0;i<groups;++i)s.groups.push_back(group());
  const auto owners=r.get(2);if(owners>2048)throw FormatError("native device owner capacity exceeded");
  for(unsigned i=0;i<owners;++i){
    SourceDeviceWorldSnapshot::Owner entry;auto&o=entry.state;
    entry.definition=r.get(4);o.handle=r.get(4);o.placement=r.get(4);o.created_tick=r.get(8);
    o.motion.position=r.real();o.motion.position_velocity=r.real();o.motion.power=r.real();o.motion.power_velocity=r.real();
    o.motion.flags=r.get(4);o.motion.delay_ticks=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));o.control_flags=r.get(4);
    o.door.flags=r.get(4);o.door.open_ticks=std::bit_cast<std::int32_t>(std::uint32_t(r.get(4)));
    o.private_power=group();o.private_position=group();o.root.scale=r.real();
    for(auto&axis:o.root.basis)axis={r.real(),r.real(),r.real()};
    o.root.position={r.real(),r.real(),r.real()};
    const auto flags=r.get(1);if(flags&~3u)throw FormatError("invalid native device visibility flags");
    o.bsp_hidden=flags&1u;o.visible=flags&2u;s.owners.push_back(entry);
  }
  validate_source_device_world_snapshot(s);return s;
}
std::vector<SourceWorldPoseOwner> body_pose_owners(const MultiplayerState &s) {
  std::vector<SourceWorldPoseOwner> owners;
  for(const auto &p:s.players)if(p.connected)owners.push_back({p.dead?p.source_previous_unit:p.source_unit,!p.dead});
  for(const auto &p:s.players)if(p.connected && !p.dead)
    for(const auto weapon:{p.source_weapon,p.secondary_weapon.datum})if(weapon!=null_datum)owners.push_back({weapon,false});
  for(const auto &corpse:s.corpses)owners.push_back({corpse.object,true});
  for(const auto &weapon:s.world_weapons)owners.push_back({weapon.handle,false});
  for(const auto &vehicle:s.world_vehicles)owners.push_back({vehicle.handle,true});
  for(const auto &root:s.source_scene_roots)owners.push_back({root.object,root.has_world_nodes});
  for(const auto &object:s.source_dynamic_objects)owners.push_back({object.object,object.has_world_nodes});
  return owners;
}
void validate_corpses(const MultiplayerState &s) {
  if(s.corpses.size()>2048)throw FormatError("Native corpse bank exceeds the source object pool");
  std::set<std::uint32_t> objects;
  for(const auto &corpse:s.corpses)
    if(corpse.object==null_datum || !(corpse.object>>16) || (corpse.object&65535u)>=2048 ||
        !objects.insert(corpse.object).second || corpse.definition==null_datum ||
        corpse.player==null_datum || !(corpse.player>>16) || (corpse.player&65535u)>=multiplayer_capacity ||
        std::any_of(s.players.begin(),s.players.end(),[&](const auto &owner) {
          return owner.connected && !owner.dead && owner.source_unit==corpse.object;
        }) || corpse.death_tick==null_datum || newer(corpse.death_tick,s.tick))
      throw FormatError("Invalid retained native corpse identity or death clock");
}
void validate_death_camera(const MultiplayerState::Player &player) {
  if(!player.source_death_camera)return;
  const auto &v=*player.source_death_camera;
  if(!player.dead || player.source_unit!=null_datum || !player.connected ||
      !(v.horizontal_fov>0 && v.horizontal_fov<3.14159265358979323846f) ||
      std::abs(length(v.forward)-1.f)>.01f || std::abs(length(v.up)-1.f)>.01f ||
      std::abs(dot(v.forward,v.up))>.01f)
    throw FormatError("Invalid native authoritative death camera owner or basis");
  for(float f:{v.position.x,v.position.y,v.position.z,v.forward.x,v.forward.y,v.forward.z,
      v.up.x,v.up.y,v.up.z,v.horizontal_fov,v.aim_displacement})
    if(!std::isfinite(f))throw FormatError("Non-finite native authoritative death camera");
}
void validate_vehicle_camera(const MultiplayerState::Player &player) {
  if(!player.source_vehicle_camera)return;
  const auto &v=*player.source_vehicle_camera;
  // The original observer continues through the authored exit handoff after
  // the unit has released its parent and seat. Its live player/unit still own
  // that camera; attachment is not a transport admission requirement.158790
  // switches to death mode when the current unit is released, so a dying unit
  // that still has its source player binding also retains this admission.
  if(!player.connected || player.source_unit==null_datum || player.source_player==null_datum ||
      (v.camera_mode!=0 && v.camera_mode!=1 && v.camera_mode!=3) ||
      !(v.horizontal_fov>0 && v.horizontal_fov<3.14159265358979323846f) ||
      std::abs(length(v.forward)-1.f)>.01f || std::abs(length(v.up)-1.f)>.01f ||
      std::abs(dot(v.forward,v.up))>.01f)
    throw FormatError("Invalid authoritative TU1.5 vehicle camera owner or basis");
  for(float value:{v.position.x,v.position.y,v.position.z,v.forward.x,v.forward.y,v.forward.z,
      v.up.x,v.up.y,v.up.z,v.horizontal_fov,v.aim_displacement})
    if(!std::isfinite(value))throw FormatError("Non-finite authoritative TU1.5 vehicle camera");
}
void encode_vehicle_hud(Packet &p,const VehicleWeaponHudStatus &hud) {
  put(p,unsigned(hud.ownership.seated)|(unsigned(hud.ownership.handheld_visible)<<1),1);
  put(p,hud.ownership.vehicle_weapon,4);put(p,hud.definition,4);
  put(p,hud.ownership.parent_hud,4);
  if(hud.ownership.vehicle_weapon==null_datum)return;
  const auto &w=hud.weapon;
  if(!hud.ownership.seated || !w.present || w.datum!=hud.ownership.vehicle_weapon || hud.definition==null_datum)
    throw FormatError("Invalid native vehicle weapon HUD ownership");
  for(auto n:{w.loaded,w.reserve,w.capacity,w.maximum_reserve})put(p,std::uint16_t(n),2);
  for(float f:{w.age,w.heat,w.charge,w.lock_progress,w.target_position.x,w.target_position.y,w.target_position.z})put_float(p,f);
  put(p,unsigned(bool(w.overheated))|(unsigned(bool(w.magazine_working))<<1)|
      (unsigned(bool(w.input_held))<<2)|(unsigned(bool(w.charging))<<3)|
      (unsigned(bool(w.target))<<4)|(unsigned(bool(w.locking))<<5)|(unsigned(bool(w.locked))<<6),1);
}
VehicleWeaponHudStatus decode_vehicle_hud(Reader &r) {
  VehicleWeaponHudStatus hud;const auto flags=r.get(1);
  if(flags>3u)throw FormatError("Invalid LAN vehicle HUD seat flags");
  hud.ownership.seated=flags&1u;hud.ownership.handheld_visible=flags&2u;
  hud.ownership.vehicle_weapon=r.get(4);hud.definition=r.get(4);
  hud.ownership.parent_hud=r.get(4);
  if(hud.ownership.vehicle_weapon==null_datum) {
    if(hud.definition!=null_datum || (!hud.ownership.seated && !hud.ownership.handheld_visible))
      throw FormatError("Invalid LAN empty vehicle HUD ownership");
    return hud;
  }
  if(!hud.ownership.seated || hud.definition==null_datum)throw FormatError("Invalid LAN vehicle weapon owner");
  auto &w=hud.weapon;w.datum=hud.ownership.vehicle_weapon;w.present=1;
  const auto short_word=[&] {return std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));};
  w.loaded=short_word();w.reserve=short_word();w.capacity=short_word();w.maximum_reserve=short_word();
  w.age=r.real();w.heat=r.real();w.charge=r.real();w.lock_progress=r.real();
  w.target_position={r.real(),r.real(),r.real()};const auto state=r.get(1);
  if(state>0x7fu)throw FormatError("Invalid LAN vehicle HUD weapon flags");
  w.overheated=bool(state&1u);w.magazine_working=bool(state&2u);w.input_held=bool(state&4u);
  w.charging=bool(state&8u);w.target=bool(state&0x10u);w.locking=bool(state&0x20u);w.locked=bool(state&0x40u);
  return hud;
}
enum class EngineSnapshotOwner:unsigned { bounded=0,original=1,defusal=2 };
bool original_engine_type(std::int32_t type) {
  return type==1 || type==2 || type==3 || type==4 || type==7 || type==8 || type==9;
}
EngineSnapshotOwner engine_snapshot_owner(const MultiplayerState& s) {
  if(!original_engine_type(s.source_engine) || (s.source_engine==4)!=s.king.has_value() ||
      s.defusal.has_value()!=s.defusal_rules.has_value() || (s.defusal && s.source_engine_entity))
    throw FormatError("Native snapshot has conflicting objective state owners");
  if(s.source_engine_entity) {
    const auto& entity=*s.source_engine_entity;
    if(s.source_game_type!=2 || entity.type!=s.source_engine ||
        entity.bytes.size()!=source_multiplayer_engine_entity_size_tu15(s.source_engine) ||
        (entity.type!=7 && entity.relationship_mask))
      throw FormatError("Native snapshot original engine entity has the wrong schema");
    return EngineSnapshotOwner::original;
  }
  if(s.defusal) {
    if(s.source_engine!=9)throw FormatError("Native custom Defusal snapshot has the wrong engine selector");
    return EngineSnapshotOwner::defusal;
  }
  if(s.source_engine!=2 && s.source_engine!=4)
    throw FormatError("Native snapshot requires its original engine entity owner");
  return EngineSnapshotOwner::bounded;
}
void encode_state(Packet &p, const MultiplayerState &s,std::span<const unsigned> local_ids) {
  put(p, s.tick, 4);
  if(s.source_game_type!=1 && s.source_game_type!=2)throw FormatError("Native snapshot has an invalid source world kind");
  put(p,unsigned(s.source_game_type),1);
  const auto engine_owner=engine_snapshot_owner(s);
  put(p,std::uint32_t(s.source_engine),4);
  put(p,unsigned(engine_owner),1);
  if(s.king){put(p,std::uint32_t(s.king->hill),4);put(p,s.king->occupants,2);}
  if(s.source_engine_entity) {
    put(p,s.source_engine_entity->relationship_mask,2);
    put(p,s.source_engine_entity->changed,4);
    p.insert(p.end(),s.source_engine_entity->bytes.begin(),s.source_engine_entity->bytes.end());
  }
  if(s.defusal) {
    const auto &d=*s.defusal;const auto &rules=*s.defusal_rules;validate_bomb_defusal_state(d);
    put(p,unsigned(d.phase),1);put(p,unsigned(d.outcome),1);put(p,d.round,1);
    put(p,d.remaining,4);put(p,d.progress,4);put(p,d.attacking_team,1);
    put(p,std::uint8_t(d.winner),1);put(p,std::uint8_t(d.match_winner),1);put(p,std::uint8_t(d.site),1);
    for(auto score:d.scores)put(p,score,2);
    put(p,d.carrier,4);put(p,d.actor,4);
    for(auto v:{d.bomb_position.x,d.bomb_position.y,d.bomb_position.z})put_float(p,v);
    for(auto h:d.roster)put(p,h,4);
    put(p,d.sites.size(),1);
    for(const auto &site:d.sites) {
      put(p,site.marker,4);for(auto v:{site.position.x,site.position.y,site.position.z,site.radius})put_float(p,v);
    }
    for(auto ticks:{rules.round_ticks,rules.fuse_ticks,rules.plant_ticks,rules.defuse_ticks,
        rules.freeze_ticks,rules.intermission_ticks,rules.half_rounds,rules.wins_needed})put(p,ticks,4);
  }
  if(s.results_finalized && !s.finished)throw FormatError("Native final results require an ended match");
  put(p, s.finished, 1);
  put(p, s.results_finalized, 1);
  if(s.game_over_countdown<0 || s.game_over_countdown>210 ||
      (!s.finished && (s.game_over_countdown || s.game_over_fade_started)))
    throw FormatError("Invalid native postgame countdown");
  put(p,std::uint32_t(s.game_over_countdown),4);put(p,s.game_over_fade_started,1);
  put(p, std::uint32_t(s.winner), 4);
  put(p, std::uint32_t(s.score_limit), 4);
  if(s.round_mode<0 || s.round_mode>6)throw FormatError("Invalid native round selector");
  put(p,std::uint32_t(s.round_mode),4);
  put(p,std::uint32_t(s.life_limit),4);
  put(p, s.time_ticks, 4);
  put(p, std::uint16_t(s.clock_seconds), 2);
  put(p, s.team_mode, 1);put(p, s.retained_team_mask, 1);
  put(p, std::uint8_t(s.winner_team), 1);
  for (auto score : s.team_scores) {
    if (score < -30000 || score > 30000)
      throw FormatError("Native LAN team statistic is outside the original short clamp");
    put(p, std::uint16_t(score), 2);
  }
  for(auto total:s.team_total_scores)put(p,std::uint16_t(total),2);
  for(auto wins:s.team_wins) {
    if(wins < -30000 || wins > 30000)throw FormatError("Native team wins outside original clamp");
    put(p,std::uint16_t(wins),2);
  }
  if (s.variant_name.size() > 32)
    throw FormatError("LAN variant name too long");
  for(const auto &team:s.team_statistics)for(const auto value:team)put(p,std::uint16_t(value),2);
  put(p, s.variant_name.size(), 1);
  for (unsigned char c : s.variant_name)
    put(p, c, 1);
  for (const auto &u : s.players) {
    put(p, u.connected | (u.dead << 1) | (u.reload_empty << 2), 1);
    if (!u.connected)
      continue;
    for (float v : {u.position.x, u.position.y, u.position.z, u.yaw, u.pitch,
                    u.crouch, u.health, u.shield})
      put_float(p, v);
    for(const auto &recent:{u.damage_feedback.shield,u.damage_feedback.body}) {
      put_float(p,recent.visible);put_float(p,recent.accumulated);
      put(p,std::uint8_t(recent.ticks),1);
    }
    put(p,u.shield_recharging,1);
    for (auto v : {u.weapon, std::uint32_t(u.loaded), std::uint32_t(u.reserve)})
      put(p, v, 4);
    for (int value : {u.score, u.kills, u.deaths, u.assists, u.suicides}) {
      if (value < -30000 || value > 30000)
        throw FormatError("Native LAN statistic is outside the original short clamp");
      put(p, std::uint16_t(value), 2);
    }
    if(u.wins < -30000 || u.wins > 30000)throw FormatError("Native player wins outside original clamp");
    put(p,std::uint16_t(u.total_score),2);put(p,std::uint16_t(u.wins),2);
    put(p,std::uint32_t(u.source_respawn_countdown),4);
    put(p,std::uint16_t(u.source_lives_remaining),2);
    put(p,std::uint16_t(u.source_king_inside_ticks),2);
    const auto &objectives=u.source_hud_objectives;
    if(objectives.territory_count>8)throw FormatError("Native objective HUD exceeds original meter bank");
    if(!std::isfinite(objectives.charge_fraction))throw FormatError("Nonfinite native objective HUD charge");
    put(p,objectives.charge_visible,1);put_float(p,objectives.charge_fraction);
    put(p,objectives.territory_count,1);
    for(unsigned n=0;n<objectives.territory_count;++n) {
      if(!std::isfinite(objectives.territories[n].fraction))throw FormatError("Nonfinite native objective HUD progress");
      put(p,objectives.territories[n].color,4);put_float(p,objectives.territories[n].fraction);
    }
    put(p, u.shot_tick, 4);
    put(p, u.reload_tick, 4);
    put_float(p, u.reload_seconds);
    put(p,u.weapon_shader.item_flags,4);put(p,u.weapon_shader.weapon_flags,4);
    put(p,std::bit_cast<std::uint32_t>(u.weapon_shader.power_fraction),4);
    put(p,std::bit_cast<std::uint32_t>(u.weapon_shader.heat),4);put(p,std::uint32_t(u.ammunition_available),4);
    put(p,u.weapon_trigger.input_flags,2);put(p,std::uint16_t(u.weapon_trigger.ticks),2);put(p,u.weapon_trigger.mode,1);put(p,u.weapon_trigger.charge,1);
    put(p,unsigned(u.weapon_trigger.target_acquired)|(u.weapon_trigger.target_position?2u:0u),1);
    if(u.weapon_trigger.target_position)for(float v:*u.weapon_trigger.target_position)put_float(p,v);
    for(float v:{u.velocity.x,u.velocity.y,u.velocity.z})put_float(p,v);
    put_float(p,u.native_forward);put_float(p,u.native_right);
    put(p,u.native_grounded|(u.native_crouching<<1),1);put(p,u.native_support_material,2);
    put(p,u.melee_action.activity,4);put(p,u.melee_action.started,4);
    put(p,u.melee_action.modifiers,1);put(p,u.melee_action.speed_modifier,1);
    for(auto v:{u.melee_action.progress,u.melee_action.impact,u.melee_action.recovery,u.melee_action.duration})put(p,std::uint8_t(v),1);
    put(p,std::uint16_t(u.landing.kind),2);put(p,std::uint8_t(u.landing.elapsed),1);put(p,std::uint8_t(u.landing.duration),1);
    if (u.grenades[0] > 2 || u.grenades[1] > 2)
      throw FormatError("Invalid native grenade counts");
    if (u.grenade_current < -1 || u.grenade_current > 1)
      throw FormatError("Invalid native grenade selection");
    put(p, u.grenades[0] | (u.grenades[1] << 2) | ((u.grenade_current + 1) << 4), 1);
    for(auto b:u.grenade_action.source)put(p,std::to_integer<unsigned>(b),1);
    put(p,u.grenade_action.graph,4);put(p,u.grenade_action.clip,4);put(p,u.grenade_action.activity,4);
    put_float(p,u.grenade_action.frame);put(p,u.grenade_action.animation_active,1);
    if(u.name.size()>65535)throw FormatError("Native LAN player name exceeds transport bounds");
    put(p,u.name.size(),2);
    for(unsigned char c:u.name)put(p,c,1);
    put(p,u.source_player,4);put(p,u.source_unit,4);put(p,u.source_weapon,4);
    put(p,u.source_previous_unit,4);put(p,u.source_death_tick,4);
    validate_death_camera(u);put(p,u.source_death_camera.has_value(),1);
    if(u.source_death_camera) {
      const auto &v=*u.source_death_camera;
      for(float f:{v.position.x,v.position.y,v.position.z,v.forward.x,v.forward.y,v.forward.z,
          v.up.x,v.up.y,v.up.z,v.horizontal_fov,v.aim_displacement})put_float(p,f);
    }
    put(p,u.source_flags,2);put(p,std::uint16_t(u.source_membership),2);put(p,u.source_ffa_identity,1);
    put(p,u.source_teleport_serial,4);
    put(p,u.source_teleport_sound,4);
    validate_source_hud_messages(u.source_messages);
    for(const auto &row:u.source_messages.rows)for(const auto value:row)put(p,std::to_integer<unsigned>(value),1);
    encode_profile(p,u.source_configuration);
    put(p,std::uint8_t(u.source_zoom.current),1);put(p,std::uint8_t(u.source_zoom.requested),1);
    put(p,u.source_zoom.motion,4);put(p,u.zoom_reset_serial,4);
    put(p,std::uint16_t(u.source_zoom_magazine_mode),2);
    put(p,u.source_parent,4);put(p,std::uint16_t(u.source_seat),2);put(p,u.source_action_word,4);put(p,u.source_unit_10a,1);
    validate_vehicle_camera(u);put(p,u.source_vehicle_camera.has_value(),1);
    if(u.source_vehicle_camera) {
      const auto &v=*u.source_vehicle_camera;
      for(float value:{v.position.x,v.position.y,v.position.z,v.forward.x,v.forward.y,v.forward.z,
          v.up.x,v.up.y,v.up.z,v.horizontal_fov,v.aim_displacement})put_float(p,value);
      put(p,v.camera_mode,1);
    }
    put(p,std::uint16_t(u.action_prompt.target.action),2);put(p,std::uint16_t(u.action_prompt.target.index),2);
    put(p,u.action_prompt.target.object,4);put(p,u.action_prompt.definition,4);put(p,u.action_prompt.message,4);
    put_float(p,u.source_camouflage.primary);put_float(p,u.source_camouflage.secondary);
    put_float(p,u.source_camouflage.fade_speed);
    put(p,u.source_hud_zoom_mask,2);
    put(p,u.source_hud_target_flags,4);
    put(p,u.source_hud_autoaimed,1);
    encode_vehicle_hud(p,u.source_vehicle_hud);
    put(p,u.motion_sensor_control,1);
    put(p,u.first_person_clock.present,1);
    if(u.first_person_clock.present) {
      if(u.source_unit==null_datum || u.source_weapon==null_datum)
        throw FormatError("Native LAN first-person clock has no current object");
      encode_fp_channel(p,u.first_person_clock.origin);encode_fp_channel(p,u.first_person_clock.current);
    }
    const auto &offhand=u.secondary_weapon;
    put(p,offhand.definition,4);put(p,offhand.datum,4);
    put(p,std::uint32_t(offhand.loaded),4);put(p,std::uint32_t(offhand.reserve),4);
    put(p,offhand.shot_tick,4);put(p,offhand.reload_tick,4);put_float(p,offhand.reload_seconds);
    put(p,offhand.reload_empty,1);put(p,offhand.clock.present,1);
    put(p,offhand.shader.item_flags,4);put(p,offhand.shader.weapon_flags,4);
    put(p,std::bit_cast<std::uint32_t>(offhand.shader.power_fraction),4);
    put(p,std::bit_cast<std::uint32_t>(offhand.shader.heat),4);put(p,std::uint32_t(offhand.ammunition_available),4);
    put(p,std::uint16_t(offhand.magazine_mode),2);
    put(p,offhand.trigger.input_flags,2);put(p,std::uint16_t(offhand.trigger.ticks),2);put(p,offhand.trigger.mode,1);put(p,offhand.trigger.charge,1);
    put(p,unsigned(offhand.trigger.target_acquired)|(offhand.trigger.target_position?2u:0u),1);
    if(offhand.trigger.target_position)for(float v:*offhand.trigger.target_position)put_float(p,v);
    if(offhand.clock.present){encode_fp_channel(p,offhand.clock.origin);encode_fp_channel(p,offhand.clock.current);}
    const auto &effect=u.weapon_effect;
    for(auto word:{effect.serial,effect.tick,effect.unit,effect.weapon,effect.effect,effect.a_bits,effect.b_bits})
      put(p,word,4);
  }
  // Validate the assignment independently of the complete roster identities.
  for(auto id:local_ids) {
    const auto &u=s.players.at(id);
    if(!u.connected || u.source_player==null_datum || (u.source_player&0xffffu)!=id ||
        (u.source_unit==null_datum && u.source_weapon!=null_datum))
      throw FormatError("Invalid assigned LAN binding identities");
  }
  if(s.world_weapons.size()>2048)throw FormatError("Native world weapon capacity exceeded");
  put(p,s.world_weapons.size(),2);
  for(const auto &w:s.world_weapons) {
    put(p,w.handle,4);put(p,w.definition,4);
    for(float f:{w.position.x,w.position.y,w.position.z,w.forward.x,w.forward.y,w.forward.z,w.up.x,w.up.y,w.up.z})put_float(p,f);
    put(p,std::uint32_t(w.loaded),4);put(p,std::uint32_t(w.reserve),4);
    put(p,w.shader.item_flags,4);put(p,w.shader.weapon_flags,4);
    put(p,std::bit_cast<std::uint32_t>(w.shader.power_fraction),4);
    put(p,std::bit_cast<std::uint32_t>(w.shader.heat),4);
  }
  if(s.world_vehicles.size()>2048)throw FormatError("Native vehicle capacity exceeded");
  put(p,s.world_vehicles.size(),2);
  for(const auto &v:s.world_vehicles){put(p,v.handle,4);put(p,v.placement,4);put_float(p,v.transform.scale);
    for(const auto point:{v.transform.position,v.transform.basis[0],v.transform.basis[1],v.transform.basis[2]})for(float f:{point.x,point.y,point.z})put_float(p,f);
    put(p,v.lighting_owner,4);put(p,v.lighting_definition,4);for(const auto point:{v.lighting_origin,v.lighting_velocity})for(float f:{point.x,point.y,point.z})put_float(p,f);
    put(p,v.lighting_flags,4);put(p,v.lighting_frozen,1);
    put(p,v.geometry_flags,4);put(p,v.geometry_control_flags,2);
    put(p,v.regions.has_value(),1);
    if(v.regions) {
      const auto &bank=*v.regions;const auto &selection=bank.selection;const auto count=selection.permutations.size();
      if(count>16 || selection.states.size()!=count || bank.looping_effects.size()!=count || !selection.resolved() || selection.variant_index < -1 || selection.variant_index>127)
        throw FormatError("Invalid native vehicle model region bank");
      put(p,std::uint32_t(selection.variant_index),4);put(p,count,1);
      for(unsigned i=0;i<count;++i){const auto &state=selection.states[i];
        put(p,selection.permutations[i],1);put(p,std::uint8_t(state.variant_permutation),1);
        put(p,state.damage,1);put(p,state.properties,1);put(p,bank.looping_effects[i],4);
      }
    }
  }
  // PROP_REMOVAL_BANK: persistent native authoritative scene lifetimes.
  if(s.weapon_projectiles.size()>1024)throw FormatError("Native projectile capacity exceeded");
  put(p,s.weapon_projectiles.size(),2);
  std::set<std::uint32_t> projectile_handles;
  for(const auto &row:s.weapon_projectiles) {
    if(row.datum==null_datum || row.definition==null_datum || !projectile_handles.insert(row.datum).second)
      throw FormatError("Invalid native projectile identity");
    put(p,row.datum,4);put(p,row.definition,4);
    for(auto v:{row.position,row.velocity,row.forward,row.up})
      for(float f:{v.x,v.y,v.z})put_float(p,f);
    put_float(p,row.traveled);
  }
  if(s.prop_removals.size()>2048)throw FormatError("Native LAN prop removal bank exceeds scene capacity");
  put(p,s.prop_removals.size(),2);
  std::set<std::uint32_t> prop_handles,prop_placements;
  for(const auto& row:s.prop_removals){
    if(row.handle==null_datum || !(row.handle>>16) || (row.handle&0xffff)>=2048 ||
        row.event_tick>s.tick ||
        !prop_handles.insert(row.handle).second || !prop_placements.insert(row.placement).second)
      throw FormatError("Invalid native LAN prop removal identity");
    put(p,row.handle,4);put(p,row.placement,4);put(p,row.definition,4);
    put(p,row.effect,4);put(p,row.event_tick,8);
    for(float f:{row.effect_position.x,row.effect_position.y,row.effect_position.z,
        row.effect_forward.x,row.effect_forward.y,row.effect_forward.z})put_float(p,f);
  }
  if(s.prop_regions.size()>2048)throw FormatError("Native prop region bank exceeds scene capacity");
  put(p,s.prop_regions.size(),2);std::set<std::uint32_t> region_handles,region_placements;
  for(const auto &row:s.prop_regions){const auto &bank=row.regions;const auto &selection=bank.selection;
    const auto count=selection.permutations.size();
    if(row.handle==null_datum || !(row.handle>>16) || (row.handle&0xffff)>=2048 || row.definition==null_datum ||
        !region_handles.insert(row.handle).second || !region_placements.insert(row.placement).second ||
        count>16 || selection.states.size()!=count || bank.looping_effects.size()!=count || !selection.resolved() ||
        selection.variant_index < -1 || selection.variant_index>127)
      throw FormatError("Invalid native prop region bank");
    put(p,row.handle,4);put(p,row.placement,4);put(p,row.definition,4);
    put(p,std::uint32_t(selection.variant_index),4);put(p,count,1);
    for(unsigned i=0;i<count;++i){const auto &state=selection.states[i];
      put(p,selection.permutations[i],1);put(p,std::uint8_t(state.variant_permutation),1);
      put(p,state.damage,1);put(p,state.properties,1);put(p,bank.looping_effects[i],4);
    }
  }
  encode_devices(p,s.devices,s.tick);
  validate_source_device_events(s.device_events,s.tick);
  put(p,s.device_events.size(),1);
  for(const auto &e:s.device_events){
    put(p,e.serial,8);put(p,e.tick,8);put(p,e.handle,4);put(p,e.tag,4);
  }
  validate_rumble_journal(s.rumble_events,s.tick);put(p,s.rumble_events.size(),1);
  for(const auto &row:s.rumble_events) {
    put(p,row.serial,8);put(p,row.tick,8);put(p,row.event.definition,4);put(p,std::uint8_t(row.event.owner+1),1);
    put(p,row.event.shielded,1);put_float(p,row.event.scale);
    put_float(p,row.event.flash_scale);put(p,row.event.seated,1);
  }
  if(s.grenade_projectiles.size()>1024)throw FormatError("Native LAN grenade bank exceeds capacity");
  put(p,s.grenade_projectiles.size(),2);
  for(const auto&g:s.grenade_projectiles){
    for(auto word:{g.datum,g.definition,g.parent,g.player,g.unit,g.flags})put(p,word,4);
    put(p,std::uint16_t(g.designator),2);put(p,std::uint16_t(g.type),2);put(p,std::uint32_t(g.native_owner),4);
    put(p,std::uint16_t(g.state),2);put(p,std::uint16_t(g.age),2);
    for(float f:{g.position.x,g.position.y,g.position.z,g.velocity.x,g.velocity.y,g.velocity.z,
        g.forward.x,g.forward.y,g.forward.z,g.up.x,g.up.y,g.up.z,g.timer,g.timer_step,g.arming,g.arming_step,g.traveled})put_float(p,f);
  }
  if(s.grenade_detonations.size()>64)throw FormatError("Native LAN detonation bank exceeds retention");
  put(p,s.grenade_detonations.size(),1);
  for(const auto&e:s.grenade_detonations){
    put(p,e.serial,8);put(p,e.tick,8);
    for(auto word:{e.projectile,e.definition,e.effect,e.player,e.unit})put(p,word,4);
    for(float f:{e.position.x,e.position.y,e.position.z,e.forward.x,e.forward.y,e.forward.z,e.up.x,e.up.y,e.up.z})put_float(p,f);
    if(e.source_locations.size()>32)throw FormatError("Source explosion location bank exceeds original capacity");
    put(p,e.colors.a,4);put(p,e.colors.b,4);
    put_float(p,e.controls.a);put_float(p,e.controls.b);
    put(p,e.source_locations.size(),1);
    for(const auto &location:e.source_locations) {
      put(p,location.name,4);const auto &t=location.frame;put_float(p,t.scale);
      for(float value:{t.position.x,t.position.y,t.position.z})put_float(p,value);
      for(const auto &axis:t.basis)for(float value:{axis.x,axis.y,axis.z})put_float(p,value);
    }
  }
  if(s.bullet_impacts.size()>64)throw FormatError("Native LAN impact bank exceeds retention");
  put(p,s.bullet_impacts.size(),1);
  for(const auto&e:s.bullet_impacts){
    put(p,e.serial,8);put(p,e.tick,8);put(p,e.projectile,4);put(p,e.weapon,4);
    put(p,std::uint16_t(e.material),2);put(p,e.terrain,1);
    for(float f:{e.position.x,e.position.y,e.position.z,e.normal.x,e.normal.y,e.normal.z,e.incoming.x,e.incoming.y,e.incoming.z})put_float(p,f);
    put(p,e.presentation.has_value(),1);
    if(e.presentation){
      put(p,std::uint16_t(e.presentation->response),2);put(p,e.presentation->admitted,1);
      put_float(p,e.presentation->scale_a);put_float(p,e.presentation->scale_b);
    }
  }
  if(s.melee_impacts.size()>64)throw FormatError("Native LAN melee impact bank exceeds retention");
  put(p,s.melee_impacts.size(),1);
  for(const auto &e:s.melee_impacts){
    put(p,e.serial,8);put(p,e.tick,8);put(p,e.attacker,4);put(p,e.weapon,4);put(p,e.damage,4);put(p,std::uint16_t(e.material),2);
    for(float f:{e.position.x,e.position.y,e.position.z,e.normal.x,e.normal.y,e.normal.z,
        e.forward.x,e.forward.y,e.forward.z,e.attacker_position.x,e.attacker_position.y,e.attacker_position.z})put_float(p,f);
  }
  validate_unit_damage_effect_events(s.unit_damage_effects,s.tick);
  put(p,s.unit_damage_effects.size(),1);
  for(const auto &e:s.unit_damage_effects) {
    put(p,e.serial,8);put(p,e.tick,8);put(p,e.target,4);
    put(p,unsigned(e.effect.kind),1);put(p,e.effect.datum,4);
    put(p,e.owner.player,4);put(p,e.owner.unit,4);put(p,std::uint16_t(e.owner.designator),2);
    put(p,e.colors.a,4);put(p,e.colors.b,4);
    put_float(p,e.controls.a);put_float(p,e.controls.b);
  }
  validate_unit_damage_voice_events(s.unit_damage_voices,s.tick);
  if(s.weapon_effects.size()>64)throw FormatError("Weapon effect journal exceeds retention");
  put(p,s.weapon_effects.size(),1);
  for(const auto &effect:s.weapon_effects) {
    for(auto word:{effect.serial,effect.tick,effect.unit,effect.weapon,effect.effect,effect.a_bits,effect.b_bits,
        effect.colors.a,effect.colors.b,effect.source_instance})put(p,word,4);
    put(p,unsigned(effect.source_presentation)|(unsigned(effect.source_scheduled)<<1)|
        (unsigned(effect.source_locations_observed)<<2),1);
    for(float f:{effect.velocity.x,effect.velocity.y,effect.velocity.z})put_float(p,f);
    if(effect.source_locations.size()>512 || (!effect.source_locations_observed && !effect.source_locations.empty()))
      throw FormatError("Invalid source firing location bank");
    put(p,effect.source_locations.size(),2);
    for(const auto &location:effect.source_locations) {
      put(p,location.name,4);const auto &t=location.frame;put_float(p,t.scale);
      for(float value:{t.position.x,t.position.y,t.position.z})put_float(p,value);
      for(const auto &axis:t.basis)for(float value:{axis.x,axis.y,axis.z})put_float(p,value);
    }
  }
  put(p,s.unit_damage_voices.size(),1);
  for(const auto &e:s.unit_damage_voices) {
    put(p,e.serial,8);put(p,e.tick,8);put(p,e.target,4);put(p,e.definition,4);put(p,e.report_flags,4);
    for(const auto f:{e.position.x,e.position.y,e.position.z,e.recent_body,e.recent_shield,e.visible_body})put_float(p,f);
  }
  validate_corpses(s);
  put(p,s.corpses.size(),2);
  for(const auto &corpse:s.corpses) {
    put(p,corpse.object,4);put(p,corpse.definition,4);put(p,corpse.player,4);put(p,corpse.death_tick,4);
    encode_profile(p,corpse.configuration);
    for(float value:{corpse.velocity.x,corpse.velocity.y,corpse.velocity.z})put_float(p,value);
    put(p,corpse.physical_state,1);put_float(p,corpse.health);put_float(p,corpse.shield);
    for(const auto &recent:{corpse.damage_feedback.shield,corpse.damage_feedback.body}) {
      put_float(p,recent.visible);put_float(p,recent.accumulated);put(p,std::uint8_t(recent.ticks),1);
    }
  }
  if(s.source_scene_roots.size()>2048 || (!s.source_scene_roots_authoritative && !s.source_scene_roots.empty()))
    throw FormatError("Invalid source scene authority or root count");
  put(p,s.source_scene_roots_authoritative,1);put(p,s.source_scene_roots.size(),2);
  for(const auto &root:s.source_scene_roots) {
    if(root.created_tick>s.tick)throw FormatError("Source scene creation follows its snapshot tick");
    if(!std::isfinite(root.velocity.x) || !std::isfinite(root.velocity.y) || !std::isfinite(root.velocity.z))
      throw FormatError("Non-finite source scene velocity");
    put(p,root.object,4);put(p,root.definition,4);put(p,root.placement,4);
    put(p,unsigned(root.visible)|(unsigned(root.has_world_nodes)<<1)|(unsigned(root.frozen)<<2),1);
    put_float(p,root.transform.scale);
    for(const auto &axis:root.transform.basis)for(float value:{axis.x,axis.y,axis.z})put_float(p,value);
    for(float value:{root.transform.position.x,root.transform.position.y,root.transform.position.z})put_float(p,value);
    put(p,root.runtime_flags,4);
    for(float value:{root.velocity.x,root.velocity.y,root.velocity.z})put_float(p,value);
    put(p,root.created_tick,8);
    if(root.render_clusters.size()>512)throw FormatError("Source scene cluster membership exceeds original BSP limit");
    put(p,root.render_clusters.size(),2);int previous_cluster=-1;
    for(const auto cluster:root.render_clusters) {
      if(cluster<=previous_cluster || cluster>=512)throw FormatError("Invalid source scene cluster membership");
      put(p,std::uint16_t(cluster),2);previous_cluster=cluster;
    }
    put(p,root.model_regions.has_value(),1);
    if(root.model_regions) {
      const auto &bank=*root.model_regions;const auto &selection=bank.selection;
      const auto count=selection.permutations.size();
      if(count>16 || selection.states.size()!=count || bank.looping_effects.size()!=count ||
          !selection.resolved() || selection.variant_index < -1 || selection.variant_index>127)
        throw FormatError("Invalid source scene model region bank");
      put(p,std::uint32_t(selection.variant_index),4);put(p,count,1);
      for(unsigned i=0;i<count;++i) {
        const auto &state=selection.states[i];
        put(p,selection.permutations[i],1);put(p,std::uint8_t(state.variant_permutation),1);
        put(p,state.damage,1);put(p,state.properties,1);put(p,bank.looping_effects[i],4);
      }
    }
    validate_source_attached_effects(root.attached_effects);
    put(p,root.attached_effects.size(),2);
    for(const auto &effect:root.attached_effects) {
      put(p,effect.slot,2);put(p,effect.flags,2);
      put(p,effect.instance,4);put(p,effect.definition,4);
      put_float(p,effect.scale_a);put_float(p,effect.scale_b);
      put(p,effect.colors[0],4);put(p,effect.colors[1],4);
      put(p,effect.locations.size(),2);
      for(const auto &location:effect.locations) {
        put(p,location.name,4);put(p,std::uint16_t(location.node),2);
        put_float(p,location.local.scale);
        for(const auto &axis:location.local.basis)for(float f:{axis.x,axis.y,axis.z})put_float(p,f);
        for(float f:{location.local.position.x,location.local.position.y,location.local.position.z})put_float(p,f);
      }
    }
  }
  validate_source_dynamic_objects(s.source_dynamic_objects);
  if(!s.source_scene_roots_authoritative && !s.source_dynamic_objects.empty())
    throw FormatError("Source dynamic objects require the completed source scene authority");
  put(p,s.source_dynamic_objects.size(),2);
  for(const auto &o:s.source_dynamic_objects) {
    put(p,o.object,4);put(p,o.definition,4);put(p,o.parent,4);put(p,o.type,1);
    put(p,unsigned(o.visible)|(unsigned(o.frozen)<<1)|(unsigned(o.has_world_nodes)<<2),1);
    put_float(p,o.transform.scale);
    for(const auto &v:o.transform.basis)for(float f:{v.x,v.y,v.z})put_float(p,f);
    for(const auto v:{o.transform.position,o.velocity,o.sphere.center})for(float f:{v.x,v.y,v.z})put_float(p,f);
    put_float(p,o.sphere.radius);put(p,o.runtime_flags,4);put(p,o.control_flags,2);
    put_float(p,o.camouflage);
    const auto &s=o.regions.selection;put(p,std::bit_cast<std::uint32_t>(s.variant_index),4);put(p,s.permutations.size(),1);
    for(unsigned i=0;i<s.permutations.size();++i) {
      put(p,s.permutations[i],1);put(p,std::uint8_t(s.states[i].variant_permutation),1);
      put(p,s.states[i].damage,1);put(p,s.states[i].properties,1);put(p,o.regions.looping_effects[i],4);
    }
    put(p,o.colors.size(),1);for(auto v:o.colors)for(float f:{v.x,v.y,v.z})put_float(p,f);
  }
  const auto poses=encode_source_world_poses(s.source_body_poses_authoritative,s.source_body_poses,body_pose_owners(s));
  p.insert(p.end(),poses.begin(),poses.end());
  validate_source_contrail_snapshot(s.source_contrails);
  put(p,s.source_contrails.authoritative,1);put(p,s.source_contrails.slices.size(),4);
  for(const auto &slice:s.source_contrails.slices) {
    put(p,slice.address,4);put(p,slice.bytes.size(),4);
    p.insert(p.end(),slice.bytes.begin(),slice.bytes.end());
  }
}
MultiplayerState decode_state(Reader &r,std::span<const unsigned> local_ids) {
  MultiplayerState s;
  s.tick = r.get(4);
  s.source_game_type=int(r.get(1));
  if(s.source_game_type!=1 && s.source_game_type!=2)throw FormatError("Native snapshot has an invalid source world kind");
  s.source_engine=r.integer();
  if(!original_engine_type(s.source_engine))
    throw FormatError("Native snapshot engine has no objective state owner");
  const auto engine_owner=r.get(1);
  if(engine_owner>unsigned(EngineSnapshotOwner::defusal))throw FormatError("Native snapshot has an unknown engine state owner");
  if(s.source_engine==4)s.king=MultiplayerState::KingObjective{r.integer(),std::uint16_t(r.get(2))};
  if(engine_owner==unsigned(EngineSnapshotOwner::original)) {
    auto& entity=s.source_engine_entity.emplace();entity.type=s.source_engine;
    entity.relationship_mask=std::uint16_t(r.get(2));entity.changed=std::uint32_t(r.get(4));
    entity.bytes.resize(source_multiplayer_engine_entity_size_tu15(s.source_engine));
    for(auto& byte:entity.bytes)byte=std::byte(r.get(1));
  }
  if(engine_owner==unsigned(EngineSnapshotOwner::defusal)) {
    auto &d=s.defusal.emplace();auto &rules=s.defusal_rules.emplace();
    d.phase=BombDefusalPhase(r.get(1));d.outcome=BombDefusalOutcome(r.get(1));d.round=unsigned(r.get(1));
    d.remaining=unsigned(r.get(4));d.progress=unsigned(r.get(4));d.attacking_team=int(r.get(1));
    d.winner=std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));
    d.match_winner=std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));
    d.site=std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));
    for(auto &score:d.scores)score=std::uint16_t(r.get(2));
    d.carrier=std::uint32_t(r.get(4));d.actor=std::uint32_t(r.get(4));
    d.bomb_position={r.real(),r.real(),r.real()};for(auto &h:d.roster)h=std::uint32_t(r.get(4));
    const auto count=r.get(1);if(count>16)throw FormatError("Too many replicated bomb sites");
    for(unsigned i=0;i<count;++i) {
      BombSite site;site.marker=std::uint32_t(r.get(4));site.position={r.real(),r.real(),r.real()};site.radius=r.real();d.sites.push_back(site);
    }
    for(auto *ticks:{&rules.round_ticks,&rules.fuse_ticks,&rules.plant_ticks,&rules.defuse_ticks,
        &rules.freeze_ticks,&rules.intermission_ticks,&rules.half_rounds,&rules.wins_needed})*ticks=unsigned(r.get(4));
    validate_bomb_defusal_state(d);
    if(rules.round_ticks<30 || rules.round_ticks>600*30 || !rules.plant_ticks || rules.plant_ticks>180*30 ||
        !rules.defuse_ticks || rules.defuse_ticks>180*30 || rules.fuse_ticks<=rules.defuse_ticks || rules.fuse_ticks>180*30 ||
        rules.freeze_ticks>30*30 || rules.intermission_ticks>30*30 || rules.half_rounds!=12 || rules.wins_needed!=13)
      throw FormatError("Invalid replicated Bomb Defusal rules");
  }
  if(unsigned(engine_snapshot_owner(s))!=engine_owner)throw FormatError("Native snapshot engine state owner differs");
  const auto finished = r.get(1);
  s.finished = finished;
  const auto finalized = r.get(1);s.results_finalized=finalized;
  if(finalized>1 || (finalized && !finished))throw FormatError("Invalid native LAN finalization state");
  s.game_over_countdown=r.integer();const auto fade_started=r.get(1);s.game_over_fade_started=fade_started;
  if(fade_started>1 || s.game_over_countdown<0 || s.game_over_countdown>210 ||
      (!finished && (s.game_over_countdown || fade_started)))
    throw FormatError("Invalid native LAN postgame countdown");
  s.winner = r.integer();
  s.score_limit = r.integer();
  s.round_mode=r.integer();
  if(s.round_mode<0 || s.round_mode>6)throw FormatError("Invalid native round selector");
  s.life_limit=r.integer();
  s.time_ticks = r.get(4);
  s.clock_seconds = std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
  const auto teams = r.get(1);s.team_mode = teams;
  s.retained_team_mask = r.get(1);
  s.winner_team = std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));
  for (auto &score : s.team_scores) {
    score = std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    if (score < -30000 || score > 30000) throw FormatError("Invalid native LAN team statistic");
  }
  for(auto& total:s.team_total_scores)total=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
  for(auto& wins:s.team_wins) {
    wins=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    if(wins < -30000 || wins > 30000)throw FormatError("Invalid native LAN team wins");
  }
  if (teams > 1 || s.winner_team < -1 || s.winner_team > 7 ||
      (s.winner_team != -1 && (!s.finished || !s.team_mode ||
          !(s.retained_team_mask & (1u << unsigned(s.winner_team))))) ||
      (s.team_mode && s.winner != -1) ||
      (!s.team_mode && (s.retained_team_mask || s.winner_team != -1 ||
          std::any_of(s.team_scores.begin(),s.team_scores.end(),[](auto score){return score != 0;}))))
    throw FormatError("Invalid native LAN team state");
  for(auto &team:s.team_statistics)for(auto &value:team)
    value=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
  const auto name_length = r.get(1);
  if (!name_length || name_length > 32 || s.score_limit < 0 ||
      s.score_limit > 32767 || s.life_limit<0 || s.life_limit>32767 || s.time_ticks > 86400 * 30)
    throw FormatError("Invalid LAN match rules");
  s.variant_name.clear();
  for (unsigned i = 0; i < name_length; ++i) {
    const auto c = r.get(1);
    if (c < 32)
      throw FormatError("Invalid LAN variant name");
    s.variant_name += char(c);
  }
  if (finished > 1 || s.winner < -1 || s.winner >= int(multiplayer_capacity))
    throw FormatError("Invalid LAN match state");
  for (auto &u : s.players) {
    const auto flags = r.get(1);
    if (flags > 7)
      throw FormatError("Invalid LAN player flags");
    u.connected = flags & 1;
    u.dead = flags & 2;
    u.reload_empty = flags & 4;
    if (!u.connected)
      continue;
    u.position = {r.real(), r.real(), r.real()};
    u.yaw = r.real();
    u.pitch = r.real();
    u.crouch = r.real();
    u.health = r.real();
    u.shield = r.real();
    for(auto *recent:{&u.damage_feedback.shield,&u.damage_feedback.body}) {
      recent->visible=r.real();recent->accumulated=r.real();
      recent->ticks=std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));
    }
    const auto recharging=r.get(1);if(recharging>1)throw FormatError("Invalid shield recharge state");
    u.shield_recharging=bool(recharging);
    u.weapon = r.get(4);
    u.loaded = r.integer();
    u.reserve = r.integer();
    auto statistic = [&] {
      const auto value = std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
      if (value < -30000 || value > 30000)
        throw FormatError("Invalid native LAN statistic");
      return int(value);
    };
    u.score = statistic(); u.kills = statistic(); u.deaths = statistic();
    u.assists = statistic(); u.suicides = statistic();
    u.total_score=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    u.wins=std::int16_t(statistic());u.source_respawn_countdown=r.integer();
    u.source_lives_remaining=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    u.source_king_inside_ticks=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    auto &objectives=u.source_hud_objectives;
    const auto charge_visible=r.get(1);
    if(charge_visible>1)throw FormatError("Invalid native objective HUD charge flag");
    objectives.charge_visible=charge_visible!=0;objectives.charge_fraction=r.real();
    objectives.territory_count=r.get(1);
    if(objectives.territory_count>8)throw FormatError("Invalid native objective HUD meter count");
    for(unsigned n=0;n<objectives.territory_count;++n)
      objectives.territories[n]={std::uint32_t(r.get(4)),r.real()};
    u.shot_tick = r.get(4);
    u.reload_tick = r.get(4);
    u.reload_seconds = r.real();
    u.weapon_shader={std::uint32_t(r.get(4)),std::uint32_t(r.get(4)),std::bit_cast<float>(std::uint32_t(r.get(4))),
        std::bit_cast<float>(std::uint32_t(r.get(4)))};u.ammunition_available=r.integer();
    u.weapon_trigger={std::uint16_t(r.get(2)),std::bit_cast<std::int16_t>(std::uint16_t(r.get(2))),std::uint8_t(r.get(1)),std::uint8_t(r.get(1))};
    const auto target_flags=r.get(1);
    if(target_flags>3u)throw FormatError("Invalid LAN weapon HUD target flags");
    u.weapon_trigger.target_acquired=target_flags&1u;
    if(target_flags&2u)u.weapon_trigger.target_position=std::array{r.real(),r.real(),r.real()};
    u.velocity={r.real(),r.real(),r.real()};
    u.native_forward=r.real();u.native_right=r.real();
    const auto support_flags=r.get(1);u.native_support_material=std::uint16_t(r.get(2));
    if(support_flags>3||std::abs(u.native_forward)>1||std::abs(u.native_right)>1)
      throw FormatError("Invalid native LAN presentation controls/support");
    u.native_grounded=support_flags&1;u.native_crouching=support_flags&2;
    u.melee_action.activity=r.get(4);u.melee_action.started=r.get(4);
    u.melee_action.modifiers=std::uint8_t(r.get(1));u.melee_action.speed_modifier=std::uint8_t(r.get(1));
    for(auto *v:{&u.melee_action.progress,&u.melee_action.impact,&u.melee_action.recovery,&u.melee_action.duration})
      *v=std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));
    if(u.melee_action.active() && (!valid_native_melee_activity(u.melee_action.activity) ||
        u.melee_action.progress<0 || u.melee_action.progress>=u.melee_action.duration || u.melee_action.started>s.tick))
      throw FormatError("Invalid native LAN melee animation");
    u.landing={std::bit_cast<std::int16_t>(std::uint16_t(r.get(2))),
        std::bit_cast<std::int8_t>(std::uint8_t(r.get(1))),std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)))};
    if(u.landing.kind< -1 || u.landing.kind>1 || u.landing.elapsed<0 || u.landing.duration<0 ||
       (u.landing.kind==-1?(u.landing.elapsed!=0 || u.landing.duration!=0):
                          (u.landing.duration==0 || u.landing.elapsed>=u.landing.duration)))
      throw FormatError("Invalid native LAN landing state");
    const auto grenades = r.get(1);
    if (grenades > 63 || (grenades & 3) > 2 || ((grenades >> 2) & 3) > 2 ||
        (grenades >> 4) > 2)
      throw FormatError("Invalid LAN grenade counts");
    u.grenades = {std::uint8_t(grenades & 3), std::uint8_t((grenades >> 2) & 3)};
    u.grenade_current = std::int8_t(int(grenades >> 4) - 1);
    for(auto&b:u.grenade_action.source)b=std::byte(r.get(1));
    u.grenade_action.graph=r.get(4);u.grenade_action.clip=r.get(4);u.grenade_action.activity=r.get(4);
    u.grenade_action.frame=r.real();const auto grenade_active=r.get(1);
    if(grenade_active>1 || u.grenade_action.phase()>3 || u.grenade_action.frame<0 ||
        (grenade_active&&(u.grenade_action.graph==null_datum||u.grenade_action.clip==null_datum||
          (u.grenade_action.activity!=0x0d000021 && u.grenade_action.activity!=0x1000006c))))
      throw FormatError("Invalid native LAN grenade action");
    u.grenade_action.animation_active=grenade_active;
    const auto name_size=r.get(2);
    for(unsigned i=0;i<name_size;++i)u.name+=char(r.get(1));
    u.source_player=r.get(4);u.source_unit=r.get(4);u.source_weapon=r.get(4);
    u.source_previous_unit=r.get(4);u.source_death_tick=r.get(4);
    const auto camera_present=r.get(1);
    if(camera_present>1)throw FormatError("Invalid native death camera presence");
    if(camera_present) {
      NativeDeathCameraView camera;
      camera.position={r.real(),r.real(),r.real()};camera.forward={r.real(),r.real(),r.real()};
      camera.up={r.real(),r.real(),r.real()};camera.horizontal_fov=r.real();camera.aim_displacement=r.real();u.source_death_camera=camera;
      if(std::find(local_ids.begin(),local_ids.end(),unsigned(&u-s.players.data()))==local_ids.end())
        throw FormatError("Native death camera belongs to another peer");
      validate_death_camera(u);
    }
    u.source_flags=r.get(2);
    u.source_membership=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    u.source_ffa_identity=r.get(1);
    u.source_teleport_serial=r.get(4);
    u.source_teleport_sound=r.get(4);
    for(auto &row:u.source_messages.rows)for(auto &value:row)value=std::byte(r.get(1));
    validate_source_hud_messages(u.source_messages);
    u.source_configuration=decode_profile(r);
    if ((s.team_mode && (u.source_membership < 0 || u.source_membership > 7 ||
          !(s.retained_team_mask & (1u << unsigned(u.source_membership))))) ||
        (!s.team_mode && (u.source_membership < 0 || u.source_membership >= int(multiplayer_capacity))))
      throw FormatError("Invalid native LAN source team membership");
    if(u.source_configuration && std::bit_cast<std::int8_t>((*u.source_configuration)[0x7c])!=u.source_membership)
      throw FormatError("Native profile and live membership differ");
    u.source_zoom.current=std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));
    u.source_zoom.requested=std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));
    u.source_zoom.motion=r.get(4);u.zoom_reset_serial=r.get(4);
    u.source_zoom_magazine_mode=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    u.source_parent=r.get(4);u.source_seat=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));u.source_action_word=r.get(4);u.source_unit_10a=r.get(1);
    if(r.get(1)) {
      NativeDeathCameraView view;view.position={r.real(),r.real(),r.real()};
      view.forward={r.real(),r.real(),r.real()};view.up={r.real(),r.real(),r.real()};
      view.horizontal_fov=r.real();view.aim_displacement=r.real();view.camera_mode=r.get(1);u.source_vehicle_camera=view;
    }
    validate_vehicle_camera(u);
    auto &prompt=u.action_prompt;
    prompt.target.action=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    prompt.target.index=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    prompt.target.object=r.get(4);prompt.definition=r.get(4);prompt.message=r.get(4);
    if((prompt.target.action<0 || prompt.target.action>8) ||
        (prompt.target.action==0 && (prompt.target.object!=null_datum || prompt.definition!=null_datum || prompt.message || prompt.target.index)) ||
        (prompt.target.action!=0 && (prompt.target.object==null_datum || prompt.definition==null_datum)) ||
        (prompt.target.action==3 && (!prompt.message || prompt.target.index)) ||
        ((prompt.target.action==2 || prompt.target.action==8) && (prompt.target.index || prompt.message)) ||
        (prompt.target.action>=4 && prompt.target.action<=6 && (prompt.target.index<0 || prompt.message)) ||
        ((prompt.target.action==1 || prompt.target.action==7) && (prompt.target.index<=0 || prompt.target.index>15 || prompt.message)))
      throw FormatError("Invalid native action prompt context");
    // Inactive disruption can leave negative primary until the next fade.
    // Preserve finite source values; rendering owns its separate clamp.
    u.source_camouflage.primary=r.real();u.source_camouflage.secondary=r.real();
    u.source_camouflage.fade_speed=r.real();
    u.source_hud_zoom_mask=r.get(2);
    u.source_hud_target_flags=r.get(4);
    const auto autoaimed=r.get(1);
    if(autoaimed>1u)throw FormatError("Invalid LAN source HUD autoaim input");
    u.source_hud_autoaimed=autoaimed;
    u.source_vehicle_hud=decode_vehicle_hud(r);
    u.motion_sensor_control=r.get(1);
    if(u.motion_sensor_control&~0x21u)throw FormatError("Invalid native radar control flags");
    const auto clock_present=r.get(1);
    if(clock_present>1 || (clock_present && (u.source_unit==null_datum || u.source_weapon==null_datum)))
      throw FormatError("Invalid native LAN first-person clock binding");
    u.first_person_clock.present=clock_present;
    if(clock_present) {
      u.first_person_clock.origin=decode_fp_channel(r);u.first_person_clock.current=decode_fp_channel(r);
    }
    auto &offhand=u.secondary_weapon;
    offhand.definition=r.get(4);offhand.datum=r.get(4);offhand.loaded=r.integer();offhand.reserve=r.integer();
    offhand.shot_tick=r.get(4);offhand.reload_tick=r.get(4);offhand.reload_seconds=r.real();
    const auto empty=r.get(1),present=r.get(1);
    offhand.shader={std::uint32_t(r.get(4)),std::uint32_t(r.get(4)),std::bit_cast<float>(std::uint32_t(r.get(4))),
        std::bit_cast<float>(std::uint32_t(r.get(4)))};offhand.ammunition_available=r.integer();
    offhand.magazine_mode=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    offhand.trigger={std::uint16_t(r.get(2)),std::bit_cast<std::int16_t>(std::uint16_t(r.get(2))),std::uint8_t(r.get(1)),std::uint8_t(r.get(1))};
    const auto secondary_target_flags=r.get(1);
    if(secondary_target_flags>3u)throw FormatError("Invalid LAN secondary HUD target flags");
    offhand.trigger.target_acquired=secondary_target_flags&1u;
    if(secondary_target_flags&2u)offhand.trigger.target_position=std::array{r.real(),r.real(),r.real()};
    if(empty>1 || present>1 || offhand.loaded<0 || offhand.reserve<0 || offhand.reload_seconds<0 ||
        ((offhand.datum==null_datum)!=(offhand.definition==null_datum)) ||
        (offhand.datum!=null_datum && (u.source_unit==null_datum || offhand.datum==u.source_weapon)) ||
        (present && offhand.datum==null_datum))throw FormatError("Invalid LAN secondary weapon");
    offhand.reload_empty=empty;offhand.clock.present=present;
    if(present){offhand.clock.origin=decode_fp_channel(r);offhand.clock.current=decode_fp_channel(r);}
    auto &effect=u.weapon_effect;
    effect.serial=r.get(4);effect.tick=r.get(4);effect.unit=r.get(4);effect.weapon=r.get(4);
    effect.effect=r.get(4);effect.a_bits=r.get(4);effect.b_bits=r.get(4);
    if(effect.serial && (effect.tick>s.tick || effect.unit==null_datum || effect.weapon==null_datum || effect.effect==null_datum))
      throw FormatError("Invalid native LAN weapon effect owner");
    if(u.source_hud_zoom_mask&~std::uint16_t(0x5c0))
      throw FormatError("Invalid LAN source zoom HUD mask");
    if(u.source_hud_target_flags&~0x1fu)throw FormatError("Invalid LAN source HUD aim flags");
    const auto slot=std::size_t(&u-s.players.data());
    if(u.source_player==null_datum || (u.source_player&0xffffu)!=slot ||
        (u.source_unit==null_datum && u.source_weapon!=null_datum))
      throw FormatError("Invalid LAN roster binding identities");
    if (u.loaded < 0 || u.reserve < 0 || u.kills < 0 || u.deaths < 0 ||
        u.assists < 0 || u.suicides < 0 ||
        std::abs(u.position.x) > 1e6 || std::abs(u.position.y) > 1e6 ||
        std::abs(u.position.z) > 1e6)
      throw FormatError("Invalid LAN player state");
  }
  for(auto id:local_ids) {
    auto &u=s.players.at(id);
    if(!u.connected || u.source_player==null_datum ||
        (u.source_unit==null_datum && u.source_weapon!=null_datum))
      throw FormatError("Invalid LAN current binding identities");
  }
  const auto world_count=r.get(2);if(world_count>2048)throw FormatError("Native world weapon capacity exceeded");
  std::set<std::uint32_t> world_handles;
  for(unsigned i=0;i<world_count;++i) {
    WorldWeaponPresentation w;w.handle=r.get(4);w.definition=r.get(4);
    w.position={r.real(),r.real(),r.real()};w.forward={r.real(),r.real(),r.real()};w.up={r.real(),r.real(),r.real()};
    w.loaded=r.integer();w.reserve=r.integer();
    w.shader={std::uint32_t(r.get(4)),std::uint32_t(r.get(4)),std::bit_cast<float>(std::uint32_t(r.get(4))),
        std::bit_cast<float>(std::uint32_t(r.get(4)))};
    if(w.handle==null_datum || w.definition==null_datum || w.loaded<0 || w.reserve<0 ||
        !world_handles.insert(w.handle).second)throw FormatError("Invalid LAN world weapon");
    s.world_weapons.push_back(w);
  }
  const auto vehicle_count=r.get(2);if(vehicle_count>2048)throw FormatError("Native vehicle capacity exceeded");
  std::set<std::uint32_t> vehicle_handles;
  for(unsigned i=0;i<vehicle_count;++i){WorldVehiclePresentation v;v.handle=r.get(4);v.placement=r.get(4);v.transform.scale=r.real();
    v.transform.position={r.real(),r.real(),r.real()};for(auto &b:v.transform.basis)b={r.real(),r.real(),r.real()};
    v.lighting_owner=r.get(4);v.lighting_definition=r.get(4);v.lighting_origin={r.real(),r.real(),r.real()};v.lighting_velocity={r.real(),r.real(),r.real()};
    v.lighting_flags=r.get(4);const auto frozen=r.get(1);if(frozen>1)throw FormatError("Invalid LAN vehicle lighting freeze flag");v.lighting_frozen=frozen!=0;
    v.geometry_flags=r.get(4);v.geometry_control_flags=r.get(2);
    const auto has_regions=r.get(1);if(has_regions>1)throw FormatError("Invalid native vehicle model region presence");
    if(has_regions) {
      v.regions.emplace();auto &bank=*v.regions;auto &selection=bank.selection;
      selection.variant_index=r.integer();const auto count=r.get(1);
      if(count>16 || selection.variant_index < -1 || selection.variant_index>127)throw FormatError("Invalid native vehicle model region bank");
      for(unsigned n=0;n<count;++n) {
        selection.permutations.push_back(r.get(1));SourceInitialModelRegionState state;
        state.variant_permutation=std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));state.damage=r.get(1);state.properties=r.get(1);
        selection.states.push_back(state);bank.looping_effects.push_back(r.get(4));
      }
    }
    if((v.lighting_owner==null_datum)!=(v.lighting_definition==null_datum))throw FormatError("Invalid LAN vehicle lighting owner");
    if(v.handle==null_datum || !vehicle_handles.insert(v.handle).second || !(v.transform.scale>0) || v.placement>=65536)throw FormatError("Invalid LAN vehicle identity");
    s.world_vehicles.push_back(v);}
  // PROP_REMOVAL_BANK: matching encoder block; later banks follow this one.
  const auto projectile_count=r.get(2);
  if(projectile_count>1024)throw FormatError("Native projectile capacity exceeded");
  std::set<std::uint32_t> projectile_handles;
  for(unsigned i=0;i<projectile_count;++i) {
    NativeWeaponProjectile row;row.datum=r.get(4);row.definition=r.get(4);
    row.position={r.real(),r.real(),r.real()};row.velocity={r.real(),r.real(),r.real()};
    row.forward={r.real(),r.real(),r.real()};row.up={r.real(),r.real(),r.real()};
    row.traveled=r.real();
    if(row.datum==null_datum || row.definition==null_datum || !projectile_handles.insert(row.datum).second)
      throw FormatError("Invalid native projectile identity");
    s.weapon_projectiles.push_back(row);
  }
  const auto prop_count=r.get(2);
  if(prop_count>2048)throw FormatError("Native LAN prop removal bank exceeds scene capacity");
  std::set<std::uint32_t> prop_handles,prop_placements;
  for(unsigned i=0;i<prop_count;++i){
    PropRemoval row{std::uint32_t(r.get(4)),std::uint32_t(r.get(4)),std::uint32_t(r.get(4))};
    row.effect=r.get(4);row.event_tick=r.get(8);
    row.effect_position={r.real(),r.real(),r.real()};row.effect_forward={r.real(),r.real(),r.real()};
    if(row.handle==null_datum || !(row.handle>>16) || (row.handle&0xffff)>=2048 ||
        row.event_tick>s.tick ||
        !prop_handles.insert(row.handle).second || !prop_placements.insert(row.placement).second)
      throw FormatError("Invalid native LAN prop removal identity");
    s.prop_removals.push_back(row);
  }
  const auto region_count=r.get(2);if(region_count>2048)throw FormatError("Native prop region bank exceeds scene capacity");
  std::set<std::uint32_t> region_handles,region_placements;
  for(unsigned n=0;n<region_count;++n){
    PropRegionSnapshot row{std::uint32_t(r.get(4)),std::uint32_t(r.get(4)),std::uint32_t(r.get(4))};
    auto &selection=row.regions.selection;selection.variant_index=std::bit_cast<std::int32_t>(std::uint32_t(r.get(4)));
    const auto count=r.get(1);
    if(row.handle==null_datum || !(row.handle>>16) || (row.handle&0xffff)>=2048 || row.definition==null_datum ||
        !region_handles.insert(row.handle).second || !region_placements.insert(row.placement).second ||
        count>16 || selection.variant_index < -1 || selection.variant_index>127)
      throw FormatError("Invalid native prop region bank");
    for(unsigned i=0;i<count;++i){selection.permutations.push_back(r.get(1));
      SourceInitialModelRegionState state;state.variant_permutation=std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));
      state.damage=r.get(1);state.properties=r.get(1);selection.states.push_back(state);
      row.regions.looping_effects.push_back(r.get(4));
    }
    s.prop_regions.push_back(std::move(row));
  }
  s.devices=decode_devices(r,s.tick);
  const auto device_events=r.get(1);
  if(device_events>64)throw FormatError("native device event journal exceeds retention");
  for(unsigned i=0;i<device_events;++i){
    SourceDeviceWorldEvent e;e.serial=r.get(8);e.tick=r.get(8);e.handle=r.get(4);e.tag=r.get(4);
    s.device_events.push_back(e);
  }
  validate_source_device_events(s.device_events,s.tick);
  const auto rumble_count=r.get(1);if(rumble_count>64)throw FormatError("Rumble journal exceeds retention");
  for(unsigned i=0;i<rumble_count;++i) {
    RumbleJournalEvent row;row.serial=r.get(8);row.tick=r.get(8);row.event.definition=r.get(4);row.event.owner=int(r.get(1))-1;
    const auto shielded=r.get(1);if(shielded>1)throw FormatError("Invalid rumble shield selector");row.event.shielded=shielded;row.event.scale=r.real();
    row.event.flash_scale=r.real();const auto seated=r.get(1);if(seated>1)throw FormatError("Invalid player effect seat selector");row.event.seated=seated;
    s.rumble_events.push_back(row);
  }
  validate_rumble_journal(s.rumble_events,s.tick);
  const auto grenade_count=r.get(2);if(grenade_count>1024)throw FormatError("Native LAN grenade bank exceeds capacity");
  std::set<std::uint32_t> grenade_datums;
  for(unsigned i=0;i<grenade_count;++i){
    NativeGrenadeProjectile g;
    g.datum=r.get(4);g.definition=r.get(4);g.parent=r.get(4);g.player=r.get(4);g.unit=r.get(4);g.flags=r.get(4);
    g.designator=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    g.type=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));g.native_owner=std::bit_cast<std::int32_t>(std::uint32_t(r.get(4)));
    g.state=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));g.age=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    g.position={r.real(),r.real(),r.real()};g.velocity={r.real(),r.real(),r.real()};
    g.forward={r.real(),r.real(),r.real()};g.up={r.real(),r.real(),r.real()};
    g.timer=r.real();g.timer_step=r.real();g.arming=r.real();g.arming_step=r.real();g.traveled=r.real();
    if(g.datum==null_datum||!(g.datum>>16)||(g.datum&0xffffu)>=2048||!grenade_datums.insert(g.datum).second||
        g.definition==null_datum||g.unit==null_datum||g.type<0||g.type>1||g.native_owner< -1||g.native_owner>14||
        g.state<0||g.state>2||g.timer<0||g.timer_step<0||g.arming<0||g.arming_step<0||g.traveled<0||
        std::abs(g.position.x)>1e6||std::abs(g.position.y)>1e6||std::abs(g.position.z)>1e6)
      throw FormatError("Invalid native LAN grenade projectile");
    s.grenade_projectiles.push_back(g);
  }
  const auto detonation_count=r.get(1);
  if(detonation_count>64)throw FormatError("Native LAN detonation bank exceeds retention");
  std::uint64_t previous_serial{};
  for(unsigned i=0;i<detonation_count;++i){
    NativeGrenadeDetonation e;e.serial=r.get(8);e.tick=r.get(8);
    // A remote presentation packet does not own this local native pool.
    e.source_effect_instance=null_datum;
    e.projectile=r.get(4);e.definition=r.get(4);e.effect=r.get(4);e.player=r.get(4);e.unit=r.get(4);
    e.position={r.real(),r.real(),r.real()};e.forward={r.real(),r.real(),r.real()};e.up={r.real(),r.real(),r.real()};
    e.colors={std::uint32_t(r.get(4)),std::uint32_t(r.get(4))};
    e.controls={r.real(),r.real()};
    const auto locations=r.get(1);
    if(locations>32)throw FormatError("Source explosion location bank exceeds original capacity");
    for(unsigned n=0;n<locations;++n) {
      SourceEffectLocation location;location.name=r.get(4);auto &t=location.frame;t.scale=r.real();
      t.position={r.real(),r.real(),r.real()};for(auto &axis:t.basis)axis={r.real(),r.real(),r.real()};
      if(location.name==null_datum || std::abs(t.scale)>1e6 ||
          std::abs(t.position.x)>1e6 || std::abs(t.position.y)>1e6 || std::abs(t.position.z)>1e6)
        throw FormatError("Invalid LAN source explosion location");
      for(const auto &axis:t.basis)if(std::abs(axis.x)>2.f || std::abs(axis.y)>2.f || std::abs(axis.z)>2.f)
        throw FormatError("Invalid LAN source explosion location basis");
      e.source_locations.push_back(location);
    }
    if(!e.serial||e.serial<=previous_serial||e.tick>s.tick||e.projectile==null_datum||!(e.projectile>>16)||
        (e.projectile&0xffffu)>=2048||e.definition==null_datum||e.unit==null_datum||
        std::abs(e.position.x)>1e6||std::abs(e.position.y)>1e6||std::abs(e.position.z)>1e6)
      throw FormatError("Invalid native LAN grenade detonation");
    previous_serial=e.serial;s.grenade_detonations.push_back(e);
  }
  const auto impact_count=r.get(1);
  if(impact_count>64)throw FormatError("Native LAN impact bank exceeds retention");
  previous_serial=0;
  for(unsigned i=0;i<impact_count;++i){
    BulletImpact e;e.serial=r.get(8);e.tick=r.get(8);e.projectile=r.get(4);e.weapon=r.get(4);
    e.material=std::int16_t(r.get(2));const auto terrain=r.get(1);e.terrain=terrain;
    e.position={r.real(),r.real(),r.real()};e.normal={r.real(),r.real(),r.real()};e.incoming={r.real(),r.real(),r.real()};
    const auto presentation=r.get(1);
    if(presentation>1)throw FormatError("Invalid native LAN impact presentation presence");
    if(presentation){
      const auto response=std::int16_t(r.get(2));const auto admitted=r.get(1);
      const auto scale_a=r.real(),scale_b=r.real();
      if(admitted>1 || scale_a<0 || scale_a>1 || scale_b<0 || scale_b>1)
        throw FormatError("Invalid native LAN impact presentation");
      e.presentation=SourceProjectileImpactPresentation{response,scale_a,scale_b,bool(admitted)};
    }
    const auto squared=[](Vec3 v){return v.x*v.x+v.y*v.y+v.z*v.z;};
    if(!e.serial || e.serial<=previous_serial || e.tick>s.tick || e.weapon==null_datum || terrain>1 ||
        e.material < -1 || squared(e.normal)<.5f || squared(e.normal)>1.5f ||
        squared(e.incoming)<.5f || squared(e.incoming)>1.5f ||
        std::abs(e.position.x)>1e6 || std::abs(e.position.y)>1e6 || std::abs(e.position.z)>1e6)
      throw FormatError("Invalid native LAN bullet impact");
    previous_serial=e.serial;s.bullet_impacts.push_back(e);
  }
  const auto melee_count=r.get(1);previous_serial=0;
  if(melee_count>64)throw FormatError("Native LAN melee impact bank exceeds retention");
  for(unsigned i=0;i<melee_count;++i){
    MeleeImpact e;e.serial=r.get(8);e.tick=r.get(8);e.attacker=r.get(4);e.weapon=r.get(4);e.damage=r.get(4);e.material=std::int16_t(r.get(2));
    e.position={r.real(),r.real(),r.real()};e.normal={r.real(),r.real(),r.real()};e.forward={r.real(),r.real(),r.real()};e.attacker_position={r.real(),r.real(),r.real()};
    const auto squared=[](Vec3 v){return v.x*v.x+v.y*v.y+v.z*v.z;};
    if(!e.serial || e.serial<=previous_serial || e.tick>s.tick || e.attacker==null_datum || e.weapon==null_datum || e.material<0 ||
        squared(e.normal)<.5f || squared(e.normal)>1.5f || squared(e.forward)<.5f || squared(e.forward)>1.5f ||
        std::abs(e.position.x)>1e6 || std::abs(e.position.y)>1e6 || std::abs(e.position.z)>1e6 ||
        std::abs(e.attacker_position.x)>1e6 || std::abs(e.attacker_position.y)>1e6 || std::abs(e.attacker_position.z)>1e6)
      throw FormatError("Invalid native LAN melee impact");
    previous_serial=e.serial;s.melee_impacts.push_back(e);
  }
  const auto damage_effect_count=r.get(1);
  if(damage_effect_count>64)throw FormatError("Unit damage effect journal exceeds retention");
  for(unsigned i=0;i<damage_effect_count;++i) {
    UnitDamageEffectEvent e;e.serial=r.get(8);e.tick=r.get(8);e.target=r.get(4);
    e.effect.kind=UnitDamageEffectKind(r.get(1));e.effect.datum=r.get(4);
    e.owner.player=r.get(4);e.owner.unit=r.get(4);e.owner.designator=std::int16_t(r.get(2));
    e.colors={std::uint32_t(r.get(4)),std::uint32_t(r.get(4))};
    e.controls={r.real(),r.real()};
    s.unit_damage_effects.push_back(e);
  }
  validate_unit_damage_effect_events(s.unit_damage_effects,s.tick);
  const auto weapon_effect_count=r.get(1);
  if(weapon_effect_count>64)throw FormatError("Weapon effect journal exceeds retention");
  std::uint32_t previous_effect{};
  for(unsigned i=0;i<weapon_effect_count;++i) {
    NativeWeaponEffectEvent e;e.serial=r.get(4);e.tick=r.get(4);e.unit=r.get(4);e.weapon=r.get(4);
    e.effect=r.get(4);e.a_bits=r.get(4);e.b_bits=r.get(4);e.colors.a=r.get(4);e.colors.b=r.get(4);
    e.source_instance=r.get(4);const auto observed=r.get(1);e.source_presentation=observed&1u;e.source_scheduled=(observed&2u)!=0;
    e.velocity={r.real(),r.real(),r.real()};e.source_locations_observed=(observed&4u)!=0;
    const auto locations=r.get(2);
    if(locations>512 || (!e.source_locations_observed && locations))throw FormatError("Invalid source firing location bank");
    for(unsigned n=0;n<locations;++n) {
      SourceEffectLocation location;location.name=r.get(4);auto &t=location.frame;t.scale=r.real();
      t.position={r.real(),r.real(),r.real()};for(auto &axis:t.basis)axis={r.real(),r.real(),r.real()};
      if(location.name==null_datum || std::abs(t.scale)>1e6 ||
          std::abs(t.position.x)>1e6 || std::abs(t.position.y)>1e6 || std::abs(t.position.z)>1e6)
        throw FormatError("Invalid LAN source firing location");
      for(const auto &axis:t.basis)if(std::abs(axis.x)>2.f || std::abs(axis.y)>2.f || std::abs(axis.z)>2.f)
        throw FormatError("Invalid LAN source firing location basis");
      e.source_locations.push_back(location);
    }
    if(!e.serial || (previous_effect && !newer(e.serial,previous_effect)) || newer(e.tick,s.tick) || observed>7 ||
        e.unit==null_datum || e.weapon==null_datum || e.effect==null_datum ||
        !std::isfinite(e.velocity.x) || !std::isfinite(e.velocity.y) || !std::isfinite(e.velocity.z))
      throw FormatError("Invalid retained weapon effect request");
    previous_effect=e.serial;s.weapon_effects.push_back(e);
    for(auto &player:s.players)if(player.weapon_effect.serial==e.serial)player.weapon_effect=e;
  }
  const auto voice_count=r.get(1);
  if(voice_count>64)throw FormatError("Unit damage voice journal exceeds retention");
  for(unsigned n=0;n<voice_count;++n) {
    UnitDamageVoiceEvent e;e.serial=r.get(8);e.tick=r.get(8);e.target=r.get(4);e.definition=r.get(4);e.report_flags=r.get(4);
    e.position={r.real(),r.real(),r.real()};e.recent_body=r.real();e.recent_shield=r.real();e.visible_body=r.real();
    s.unit_damage_voices.push_back(e);
  }
  validate_unit_damage_voice_events(s.unit_damage_voices,s.tick);
  const auto corpse_count=r.get(2);
  if(corpse_count>2048)throw FormatError("Native corpse bank exceeds the source object pool");
  for(unsigned n=0;n<corpse_count;++n) {
    MultiplayerState::Corpse corpse;
    corpse.object=r.get(4);corpse.definition=r.get(4);corpse.player=r.get(4);corpse.death_tick=r.get(4);
    corpse.configuration=decode_profile(r);corpse.velocity={r.real(),r.real(),r.real()};
    corpse.physical_state=r.get(1);corpse.health=r.real();corpse.shield=r.real();
    for(auto *recent:{&corpse.damage_feedback.shield,&corpse.damage_feedback.body}) {
      recent->visible=r.real();recent->accumulated=r.real();
      recent->ticks=std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));
    }
    s.corpses.push_back(std::move(corpse));
  }
  validate_corpses(s);
  const auto scene_authority=r.get(1),scene_count=r.get(2);
  if(scene_authority>1 || scene_count>2048 || (!scene_authority && scene_count))
    throw FormatError("Invalid source scene authority or root count");
  s.source_scene_roots_authoritative=scene_authority;
  std::set<std::uint32_t> scene_objects,scene_placements;
  for(unsigned n=0;n<scene_count;++n) {
    SourceSceneRoot root;root.object=r.get(4);root.definition=r.get(4);root.placement=r.get(4);
    const auto flags=r.get(1);root.visible=flags&1u;root.has_world_nodes=flags&2u;root.frozen=flags&4u;
    root.transform.scale=r.real();for(auto &axis:root.transform.basis)axis={r.real(),r.real(),r.real()};
    root.transform.position={r.real(),r.real(),r.real()};
    root.runtime_flags=r.get(4);root.velocity={r.real(),r.real(),r.real()};
    root.created_tick=r.get(8);
    const auto cluster_count=r.get(2);
    if(cluster_count>512)throw FormatError("Source scene cluster membership exceeds original BSP limit");
    int previous_cluster=-1;
    for(unsigned i=0;i<cluster_count;++i) {
      const auto cluster=r.get(2);
      if(cluster>=512 || int(cluster)<=previous_cluster)throw FormatError("Invalid source scene cluster membership");
      root.render_clusters.push_back(std::int16_t(cluster));previous_cluster=int(cluster);
    }
    const auto has_regions=r.get(1);
    if(has_regions>1)throw FormatError("Invalid source scene model region presence");
    if(has_regions) {
      auto &bank=root.model_regions.emplace();auto &selection=bank.selection;
      selection.variant_index=r.integer();const auto count=r.get(1);
      if(count>16 || selection.variant_index < -1 || selection.variant_index>127)
        throw FormatError("Invalid source scene model region bank");
      for(unsigned i=0;i<count;++i) {
        selection.permutations.push_back(r.get(1));SourceInitialModelRegionState state;
        state.variant_permutation=std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));
        state.damage=r.get(1);state.properties=r.get(1);
        selection.states.push_back(state);bank.looping_effects.push_back(r.get(4));
      }
    }
    const auto effect_count=r.get(2);
    if(effect_count>4095)throw FormatError("Source effect attachment bank exceeds signed body extent");
    for(unsigned i=0;i<effect_count;++i) {
      SourceAttachedEffect effect;effect.slot=r.get(2);effect.flags=r.get(2);
      effect.instance=r.get(4);effect.definition=r.get(4);
      effect.scale_a=r.real();effect.scale_b=r.real();
      effect.colors={std::uint32_t(r.get(4)),std::uint32_t(r.get(4))};
      const auto location_count=r.get(2);
      if(location_count>512)throw FormatError("Source effect locations exceed their original pool");
      for(unsigned j=0;j<location_count;++j) {
        SourceAttachedEffectLocation location;location.name=r.get(4);
        location.node=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
        location.local.scale=r.real();
        for(auto &axis:location.local.basis)axis={r.real(),r.real(),r.real()};
        location.local.position={r.real(),r.real(),r.real()};effect.locations.push_back(location);
      }
      root.attached_effects.push_back(effect);
    }
    validate_source_attached_effects(root.attached_effects);
    if(root.object==null_datum || !(root.object>>16) || (root.object&65535u)>=2048 ||
        root.definition==null_datum || root.placement>=65536 || flags>7 || !(root.transform.scale>0) || root.created_tick>s.tick ||
        !std::isfinite(root.velocity.x) || !std::isfinite(root.velocity.y) || !std::isfinite(root.velocity.z) ||
        !scene_objects.insert(root.object).second || !scene_placements.insert(root.placement).second)
      throw FormatError("Invalid source scene root identity or frame");
    s.source_scene_roots.push_back(root);
  }
  const auto dynamic_count=r.get(2);
  if(dynamic_count>2048 || (!s.source_scene_roots_authoritative && dynamic_count))
    throw FormatError("Invalid source dynamic object authority/count");
  for(unsigned n=0;n<dynamic_count;++n) {
    SourceDynamicObject o;o.object=r.get(4);o.definition=r.get(4);o.parent=r.get(4);o.type=r.get(1);
    const auto flags=r.get(1);if(flags>7)throw FormatError("Invalid source dynamic visibility flags");
    o.visible=flags&1;o.frozen=flags&2;o.has_world_nodes=flags&4;
    o.transform.scale=r.real();for(auto &v:o.transform.basis)v={r.real(),r.real(),r.real()};
    o.transform.position={r.real(),r.real(),r.real()};o.velocity={r.real(),r.real(),r.real()};
    o.sphere.center={r.real(),r.real(),r.real()};o.sphere.radius=r.real();o.runtime_flags=r.get(4);o.control_flags=r.get(2);
    o.camouflage=r.real();
    auto &model=o.regions.selection;model.variant_index=r.integer();const auto count=r.get(1);
    if(count>16)throw FormatError("Source dynamic region count exceeds original16 slots");
    for(unsigned i=0;i<count;++i) {
      model.permutations.push_back(r.get(1));const auto choice=std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));
      const auto damage=std::uint8_t(r.get(1)),properties=std::uint8_t(r.get(1));
      model.states.push_back({choice,damage,properties});o.regions.looping_effects.push_back(r.get(4));
    }
    const auto colors=r.get(1);if(colors>4)throw FormatError("Source dynamic color count exceeds original4 channels");
    for(unsigned i=0;i<colors;++i)o.colors.push_back({r.real(),r.real(),r.real()});
    s.source_dynamic_objects.push_back(std::move(o));
  }
  validate_source_dynamic_objects(s.source_dynamic_objects);
  auto poses=decode_source_world_poses(std::span(r.p).subspan(r.at),body_pose_owners(s));
  r.at+=poses.consumed;s.source_body_poses_authoritative=poses.authoritative;
  s.source_body_poses=std::move(poses.poses);
  const auto contrail_authoritative=r.get(1);if(contrail_authoritative>1)throw FormatError("Invalid contrail authority flag");
  s.source_contrails.authoritative=contrail_authoritative;
  const auto slices=r.get(4);if(slices>32768)throw FormatError("Contrail slice capacity exceeded");
  std::size_t contrail_bytes{};
  for(unsigned n=0;n<slices;++n) {
    SourceContrailSlice slice;slice.address=r.get(4);const auto size=r.get(4);
    contrail_bytes+=size;
    if(!size || contrail_bytes>2u*1024u*1024u || std::uint64_t(r.at)+size>r.p.size())throw FormatError("Contrail source input extent exceeds snapshot");
    slice.bytes.resize(size);std::memcpy(slice.bytes.data(),r.p.data()+r.at,size);r.at+=size;
    s.source_contrails.slices.push_back(std::move(slice));
  }
  validate_source_contrail_snapshot(s.source_contrails);
  return s;
}
sockaddr_in resolve(std::string address) {
  const auto colon = address.rfind(':');
  std::string port = "2002";
  if (colon != std::string::npos) {
    port = address.substr(colon + 1);
    address.resize(colon);
  }
  if (address.empty())
    throw FormatError("Missing LAN host address");
  unsigned long parsed{};
  std::size_t end{};
  try {
    parsed = std::stoul(port, &end);
  } catch (...) {
    throw FormatError("Invalid LAN host port");
  }
  if (!parsed || parsed > 65535 || end != port.size())
    throw FormatError("Invalid LAN host port");
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  addrinfo *result{};
  if (getaddrinfo(address.c_str(), port.c_str(), &hints, &result) != 0)
    throw FormatError("Could not resolve LAN host");
  const auto at = *reinterpret_cast<sockaddr_in *>(result->ai_addr);
  freeaddrinfo(result);
  return at;
}
} // namespace
LanSessionDescription query_lan_session(const std::string &address,std::chrono::milliseconds timeout) {
  Udp udp(0);return query_session(udp,resolve(address),timeout);
}
LanSessionDescription query_lan_session(std::shared_ptr<MultiplayerDatagrams> transport,
    unsigned host,std::chrono::milliseconds timeout) {
  if(!transport || host>=multiplayer_capacity)throw FormatError("Invalid native session transport or host");
  Udp udp(std::move(transport));sockaddr_in endpoint{};
  endpoint.sin_family=AF_INET;endpoint.sin_addr.s_addr=htonl(host+1);
  return query_session(udp,endpoint,timeout);
}
std::uint64_t multiplayer_fingerprint(const std::filesystem::path &map,
                                     const std::filesystem::path &shared_maps,
                                     std::uint32_t bsp,
                                     std::uint32_t starting_weapon,
                                     const std::filesystem::path &gameplay_xbe) {
  std::uint64_t hash = 14695981039346656037ull;
  const auto byte = [&](unsigned value) {
    hash = (hash ^ (value & 255)) * 1099511628211ull;
  };
  const auto integer = [&](std::uint64_t value) {
    for (unsigned i = 0; i < 8; ++i) byte(unsigned(value >> (8 * i)));
  };
  const auto text = [&](const std::string &value) {
    integer(value.size());
    for (unsigned char c : value) byte(c);
  };
  const auto file = [&](const std::string &role, const std::filesystem::path &path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f || f.tellg() < 0)
      throw FormatError("Missing LAN game data: " + path.string());
    text(role);
    integer(std::uint64_t(f.tellg()));
    f.seekg(0);
    char block[65536];
    while (f.read(block, sizeof(block)) || f.gcount()) {
      for (std::streamsize i = 0; i < f.gcount(); ++i)
        byte(static_cast<unsigned char>(block[i]));
    }
    if (!f.eof())
      throw FormatError("Failed reading LAN game data: " + path.string());
  };
  const auto mcc_cache = [](const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    std::array<unsigned char,8> header{};
    return input.read(reinterpret_cast<char*>(header.data()),header.size()) &&
        header==std::array<unsigned char,8>{'d','a','e','h',13,0,0,0};
  };
  const bool mcc=mcc_cache(map);
  if(mcc && !gameplay_xbe.empty())
    throw FormatError("MCC LAN gameplay uses native TU1.5 data, not an injected executable");
  text(mcc?"Halo2NativeMultiplayer-mcc-native-tu15-data-v1":"Halo2NativeMultiplayer-data-v1");
  integer(bsp);
  integer(starting_weapon);
  file("active-map", map);
  for (const auto name : {"mainmenu.map", "shared.map", "single_player_shared.map"}) {
    if(mcc && !mcc_cache(shared_maps/name))
      throw FormatError("MCC LAN shared cache is not version 13: "+(shared_maps/name).string());
    file(name, shared_maps / name);
  }
  if(mcc) {
    // These are the actual MCC bitmap/sound owners selected by mcc_cache.cpp.
    // Include their bytes without making an original executable a runtime input.
    for(const auto name:{"textures.dat","sounds_en.dat","sounds_neutral.dat","sounds_remastered.dat"})
      file(name,shared_maps/name);
  } else file("original-executable", shared_maps.parent_path() / "default.xbe");
  // Retain the legacy raw-data identity when no update was explicitly selected.
  // Updated and unprocessed sessions must reject one another rather than mix
  // damage definitions. File bytes, not machine-specific paths, are the identity.
  if (!gameplay_xbe.empty()) file("gameplay-update-executable", gameplay_xbe);
  const auto fonts = mcc?shared_maps.parent_path()/"h2_fonts":shared_maps/"fonts";
  if (!std::filesystem::is_regular_file(fonts / "font_table.txt"))
    throw FormatError("Missing LAN HUD font table: " + fonts.string());
  std::vector<std::filesystem::path> font_files;
  for (const auto &entry : std::filesystem::recursive_directory_iterator(fonts)) {
    if (entry.is_regular_file()) font_files.push_back(entry.path().lexically_relative(fonts));
  }
  std::sort(font_files.begin(), font_files.end());
  integer(font_files.size());
  for (const auto &path : font_files) file("fonts/" + path.generic_string(), fonts / path);
  return hash;
}
struct LanHost::Impl {
  struct Peer {
    sockaddr_in address;
    std::uint64_t nonce{}, cookie{};
    std::vector<unsigned> ids;
    std::vector<std::int8_t> selected_teams;
    std::vector<std::optional<SourcePlayerConfiguration>> profiles;
    std::uint32_t sequence{};
    std::uint32_t snapshot_sequence{};
    std::optional<MultiplayerSnapshotStamp> last_snapshot;
    Clock::time_point last_response{};
    bool seen{};
    Clock::time_point last = Clock::now();
    std::optional<Clock::time_point> last_accepted_input;
    std::uint64_t input_acceptances{};
    std::chrono::nanoseconds idle_offset{};
    auto age(Clock::time_point now)const {
      const auto elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(now-last);
      const auto maximum=std::chrono::nanoseconds::max();
      return elapsed>=maximum-idle_offset?maximum:elapsed+idle_offset;
    }
    void heard(){last=Clock::now();idle_offset={};}
    std::array<Controls, 4> controls{};
    std::array<PressSerials,4> received_presses{},delivered_presses{};
    std::array<InteractionDurations,4> completed{};
    std::array<VehicleIntent,4> completed_vehicle{};
    std::unique_ptr<NativeVehicleCamera> vehicle_camera;
    std::optional<std::uint32_t> vehicle_camera_tick;
    std::array<std::optional<NativeDeathCameraView>,4> vehicle_views;
    std::array<std::optional<NativeDeathCameraView>,4> death_views;
    std::array<std::uint32_t,4> camera_jump_serial{};
  };
  MultiplayerMatch &match;
  Udp udp;
  std::vector<OnlineAdmission> admissions;
  std::uint64_t fingerprint;
  Packet session_configuration;
  std::vector<Peer> peers;
  std::optional<PeerRemoval> last_removal;
  std::uint64_t removals{};
  Clock::time_point next_repeat_response{};
  const std::shared_ptr<const void> candidate_owner=std::make_shared<const unsigned char>(0);
  std::uint64_t candidate_revision{};
  struct PendingPacket {Packet bytes;sockaddr_in from;};
  std::mutex receive_mutex;
  std::deque<PendingPacket> pending_packets;
  // Declared last: stop/join before destroying the socket or queue.
  LanAdmission admission;
  std::jthread configuration_service;
  bool admit(Packet &packet,const sockaddr_in &from){
    if(udp.relay)return true;
    Reader outer{packet};const auto kind=outer.type();
    // Established input/leave packets already carry the per-connection cookie.
    if(kind==input||kind==leave)return true;
    if(kind!=admission_request||packet.size()<38||packet.size()>packet_limit)return false;
    LanAdmission::Cookie cookie;for(auto &b:cookie)b=outer.get(1);
    Packet request(packet.begin()+outer.at,packet.end());Reader body{request};
    const auto type=body.type();if(type!=hello&&type!=session_query)return false;
    const auto nonce=body.get(8);const auto endpoint=admission_endpoint(from);
    const auto now=LanAdmission::milliseconds();
    if(!admission.verify(endpoint,request,cookie,now)){
      auto reply=header(admission_challenge);put(reply,nonce,8);
      for(auto b:admission.issue(endpoint,request,now))put(reply,b,1);
      if(reply.size()<=packet.size()&&admission.reserve(endpoint,reply.size(),true,now))udp.send(reply,from);
      return false;
    }
    packet=std::move(request);return true;
  }
  void own()const {
    if(auto *world=match.source_object_runtime();world&&world->owns_physics())world->require_authority_executor();
  }
  bool receive_packet(Packet &p,sockaddr_in &from) {
    std::lock_guard lock(receive_mutex);
    if(pending_packets.empty())return udp.receive(p,from);
    p=std::move(pending_packets.front().bytes);from=pending_packets.front().from;
    pending_packets.pop_front();return true;
  }
  void start_configuration_service() {
    if(udp.relay || session_configuration.empty())return;
    configuration_service=std::jthread([this](std::stop_token stop) {
      while(!stop.stop_requested()) {
        Packet p;sockaddr_in from{};std::optional<std::uint64_t> query;bool consumed{};
        {
          std::lock_guard lock(receive_mutex);
          if(pending_packets.size()<256 && udp.receive(p,from)) {
            consumed=true;
            // Authenticate first flights before consuming the bounded pending
            // queue. Only the simulation owner may join an authenticated hello.
            try{
              Reader r{p};const auto kind=r.type();
              if(kind==admission_request){
                auto envelope=p;
                if(admit(p,from))if(auto request=decode_native_request(p,protocol_version)){
                  if(const auto *value=std::get_if<NativeSessionQuery>(&*request))query=value->nonce;
                  else if(std::holds_alternative<NativeHelloRequest>(*request))pending_packets.push_back({std::move(envelope),from});
                }
              }else if(kind==input||kind==leave)pending_packets.push_back({std::move(p),from});
            }catch(const FormatError &){}
          }
        }
        if(query)describe(from,*query);
        if(!consumed)std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
    });
  }
  static MultiplayerMatch& admitted(MultiplayerMatch& match) {
    if(original_engine_type(match.source_engine()) && match.source_engine_owned())return match;
    if(match.source_engine()!=2 && match.source_engine()!=4 &&
        !(match.source_engine()==9 && match.session_configuration() && bomb_defusal_variant(*match.session_configuration())))
      throw FormatError("Selected engine has no complete native objective replication binding");
    return match;
  }
  Impl(MultiplayerMatch &m, unsigned port, std::uint64_t f)
      : match(admitted(m)), udp(port), fingerprint(f) {}
  Impl(MultiplayerMatch &m, std::shared_ptr<MultiplayerDatagrams> relay, std::uint64_t f,
       std::vector<OnlineAdmission> roster)
      : match(admitted(m)), udp(std::move(relay)), admissions(std::move(roster)), fingerprint(f) {}
  void respond(Peer &peer,Packet *archive=nullptr) {
    peer.last_response=Clock::now();
    ++candidate_revision;
    Packet body;
    put(body, peer.ids.size(), 1);
    for (auto id : peer.ids)
      put(body, id, 1);
    auto state=match.state();
    if(auto *source=match.source_object_runtime();source && source->owns_physics()) {
      if(!peer.vehicle_camera) {
        if(!match.original_data())throw FormatError("Remote vehicle camera lost authenticated TU1.5 constants");
        peer.vehicle_camera=std::make_unique<NativeVehicleCamera>(*source,*match.original_data());
      }
      if(!peer.vehicle_camera_tick || *peer.vehicle_camera_tick!=state.tick) {
        const auto seconds=peer.vehicle_camera_tick?float(std::uint32_t(state.tick-*peer.vehicle_camera_tick))*tick_seconds:tick_seconds;
        for(unsigned view=0;view<peer.ids.size();++view) {
          const auto slot=peer.ids[view];const auto &player=state.players[slot];
          const bool cycle=newer(peer.received_presses[view][0],peer.camera_jump_serial[view]);
          peer.camera_jump_serial[view]=peer.received_presses[view][0];
          const auto camera=peer.vehicle_camera->update_player(view,player.source_player,player.source_flags,
              seconds,cycle);
          const bool death=peer.vehicle_camera->death_view(view);
          peer.vehicle_views[view]=death?std::nullopt:camera;
          peer.death_views[view]=death?camera:std::nullopt;
        }
        peer.vehicle_camera_tick=state.tick;
      }
      for(unsigned view=0;view<peer.ids.size();++view) {
        state.players[peer.ids[view]].source_vehicle_camera=peer.vehicle_views[view];
        state.players[peer.ids[view]].source_death_camera=peer.death_views[view];
      }
    }
    encode_state(body,state,peer.ids);
    if(body.empty() || body.size()>snapshot_limit)
      throw FormatError("Native LAN snapshot exceeds its transport bound");
    const unsigned count=unsigned((body.size()+snapshot_chunk-1)/snapshot_chunk);
    const auto sequence=++peer.snapshot_sequence;
    MultiplayerSnapshotStamp stamp{fingerprint,peer.nonce,peer.cookie,sequence,state.tick,std::uint32_t(body.size())};
    if(!SHA256(reinterpret_cast<const unsigned char*>(body.data()),body.size(),
        reinterpret_cast<unsigned char*>(stamp.digest.data())))throw FormatError("Cannot hash native snapshot body");
    peer.last_snapshot=stamp;
    if(archive){
      // Versioned archive of the ORIGINAL serialized packets, not another
      // snapshot codec. Delivery is reserved; native validation stays below.
      archive->reserve(8+body.size()+count*(snapshot_header+2));
      put(*archive,0x31534348u,4);put(*archive,1,2);put(*archive,count,2);
    }
    for(unsigned index=0;index<count;++index) {
      auto p=header(snapshot);
      put(p,peer.nonce,8);put(p,peer.cookie,8);put(p,sequence,4);
      put(p,index,2);put(p,count,2);put(p,body.size(),4);
      const auto begin=body.begin()+index*snapshot_chunk;
      const auto end=body.begin()+std::min(body.size(),std::size_t(index+1)*snapshot_chunk);
      p.insert(p.end(),begin,end);
      if(archive){put(*archive,p.size(),2);archive->insert(archive->end(),p.begin(),p.end());}
      udp.send(p,peer.address);
    }
  }
  void describe(sockaddr_in from,std::uint64_t nonce) {
    if(session_configuration.empty()) {
      rejection(from,nonce,"Session configuration is unavailable for this host");return;
    }
    const auto total=session_configuration.size();
    const unsigned count=unsigned((total+session_chunk-1)/session_chunk);
    if(!udp.relay&&!admission.reserve(admission_endpoint(from),total+count*session_header,false,LanAdmission::milliseconds()))return;
    for(unsigned index=0;index<count;++index) {
      auto p=header(session_info);put(p,nonce,8);put(p,fingerprint,8);
      put(p,index,2);put(p,count,2);put(p,total,4);
      const auto first=session_configuration.begin()+index*session_chunk;
      const auto last=session_configuration.begin()+std::min(total,std::size_t(index+1)*session_chunk);
      p.insert(p.end(),first,last);udp.send(p,from);
    }
  }
  void rejection(sockaddr_in from, std::uint64_t nonce, std::string reason) {
    auto p = header(reject);
    put(p, nonce, 8);
    for (char c : reason)
      p.push_back(std::byte(c));
    udp.send(p, from);
  }
  void remove(std::size_t i,PeerRemovalReason reason,Clock::time_point observed) {
    const auto &peer=peers[i];
    PeerRemoval notice;
    notice.reason=reason;notice.local_players=unsigned(peer.ids.size());
    notice.input_seen=peer.seen;notice.input_sequence=peer.sequence;
    notice.input_acceptances=peer.input_acceptances;
    const auto ns=[](auto duration){return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();};
    notice.observed_at_ns=ns(observed.time_since_epoch());notice.idle_age_ns=peer.age(observed).count();
    if(peer.last_accepted_input){
      notice.last_accepted_input_at_ns=ns(peer.last_accepted_input->time_since_epoch());
      notice.last_accepted_input_age_ns=ns(observed-*peer.last_accepted_input);
    }
    for(auto id:peer.ids)notice.slot_mask|=std::uint16_t(std::uint16_t(1)<<id);
    for (auto id : peers[i].ids)
      match.leave(id);
    peers.erase(peers.begin() + i);
    // Native leave can throw. Publish no successful-removal witness until all
    // original leave calls and the actual peer-table erase have completed.
    notice.source_tick=match.simulation().ticks();notice.ordinal=++removals;
    notice.removed_at_ns=ns(Clock::now().time_since_epoch());last_removal=notice;
  }
};
LanHost::LanHost(MultiplayerMatch &m, unsigned port, std::uint64_t f)
    : impl_(std::make_unique<Impl>(m, port, f)) {
  if (port > 65535)
    throw FormatError("Invalid LAN port");
  if(m.session_configuration())impl_->session_configuration=encode_session_variant(*m.session_configuration());
  impl_->start_configuration_service();
}
LanHost::~LanHost() = default;
LanHost::LanHost(MultiplayerMatch &m,unsigned port,std::uint64_t f,const UiVariant &variant)
    :impl_(std::make_unique<Impl>(m,port,f)) {
  impl_->session_configuration=encode_session_variant(variant);impl_->start_configuration_service();
}
LanHost::LanHost(MultiplayerMatch &m, std::shared_ptr<MultiplayerDatagrams> relay,
                 std::uint64_t f, std::vector<OnlineAdmission> roster)
    : impl_(std::make_unique<Impl>(m, std::move(relay), f, std::move(roster))) {
  if(m.session_configuration())impl_->session_configuration=encode_session_variant(*m.session_configuration());
  impl_->start_configuration_service();
}
LanHost::LanHost(MultiplayerMatch &m,std::shared_ptr<MultiplayerDatagrams> relay,
    std::uint64_t f,std::vector<OnlineAdmission> roster,const UiVariant &variant)
    :impl_(std::make_unique<Impl>(m,std::move(relay),f,std::move(roster))) {
  impl_->session_configuration=encode_session_variant(variant);
  impl_->start_configuration_service();
}
unsigned LanHost::port() const { return impl_->udp.port(); }
unsigned LanHost::peers() const { impl_->own();return unsigned(impl_->peers.size()); }
std::optional<LanHost::PeerRemoval> LanHost::last_peer_removal()const {impl_->own();return impl_->last_removal;}
MultiplayerMatch &LanHost::continuation_match()const {impl_->own();return impl_->match;}
std::uint64_t LanHost::continuation_allocation(const Continuation &cut){
  if(!cut.data_)return 0;const auto &s=*cut.data_;
  std::uint64_t bytes=sizeof(Continuation::Data)+s.configuration.size()+s.admissions.size()*sizeof(OnlineAdmission)+
      s.roster.size()*sizeof(SourcePlayerAdmission)+s.peers.size()*sizeof(Continuation::Data::Peer);
  for(const auto &p:s.admissions)bytes+=p.name.size();
  for(const auto &p:s.peers){bytes+=p.ids.size()*sizeof(unsigned)+p.selected_teams.size()*sizeof(std::int8_t)+
      p.profiles.size()*sizeof(std::optional<SourcePlayerConfiguration>);
    if(p.camera)bytes+=NativeVehicleCamera::checkpoint_allocation(*p.camera);}
  return bytes;
}
namespace {
std::vector<unsigned> allocated_host_slots(const MultiplayerMatch &match) {
  std::vector<unsigned> slots;
  for(unsigned slot=0;slot<multiplayer_capacity;++slot)
    if(match.player(slot).source_datum!=null_datum)slots.push_back(slot);
  return slots;
}
bool same_host_admissions(std::span<const OnlineAdmission> a,std::span<const OnlineAdmission> b) {
  if(a.size()!=b.size())return false;
  for(unsigned i=0;i<a.size();++i)
    if(a[i].slot!=b[i].slot||a[i].name!=b[i].name||a[i].team!=b[i].team||
        a[i].identity!=b[i].identity||online_admission_peer(a[i])!=online_admission_peer(b[i])||
        a[i].guest!=b[i].guest)return false;
  return true;
}
void host_continuation_boundary(MultiplayerObjectRuntime &world) {
  world.require_authority_executor(true);
  if(source_execution_inputs(world.memory()))
    throw FormatError("Host continuation requires a completed native frame");
}
std::array<std::uint32_t,4> host_local_players(MultiplayerObjectRuntime &world) {
  const auto &bank=world.memory();const auto census=Bytes(bank.view(0x4f54f0u,4)).u32(0);
  std::array<std::uint32_t,4> players{};
  bank.read(census+offsetof(SourceDepartureCensus,users),std::as_writable_bytes(std::span(players)));
  return players;
}
void host_loss_contexts(const AuthorityTransferContext &previous,const AuthorityTransferContext &next){
  if(previous.host>=16||next.host>=16||previous.host==next.host||
      !(previous.peers&(1u<<previous.host))||!(next.peers&(1u<<next.host))||
      next.session!=previous.session||previous.epoch==UINT32_MAX||next.epoch!=previous.epoch+1||
      next.membership_revision<=previous.membership_revision||
      next.peers!=std::uint16_t(previous.peers&~(1u<<previous.host))||
      std::ranges::all_of(previous.session,[](auto b){return b==std::byte{};}))
    throw FormatError("Host loss requires an independently agreed next generation and survivor membership");
}
}
LanHost::Continuation LanHost::checkpoint(MultiplayerMatch::AuthorityDigest binding)const {
  const auto &h=*impl_;auto *world=h.match.source_object_runtime();
  if(!h.udp.relay||!world||!world->owns_physics()||
      std::ranges::all_of(binding,[](auto b){return b==std::byte{};}))
    throw FormatError("Host continuation requires an admitted native cut and logical peer transport");
  host_continuation_boundary(*world);SourceMatchAuthority authority(h.match,*world);
  const auto before=authority.execution_boundary();
  auto saved=std::make_shared<Continuation::Data>();
  saved->binding=binding;saved->fingerprint=h.fingerprint;saved->tick=before.tick;
  saved->configuration=h.session_configuration;saved->admissions=h.admissions;
  saved->local_players=host_local_players(*world);
  saved->roster=authority.player_admissions(allocated_host_slots(h.match));
  const auto now=Clock::now();std::uint16_t endpoints{},slots{};
  for(const auto &peer:h.peers) {
    const auto endpoint=ntohl(peer.address.sin_addr.s_addr)-1;
    if(endpoint>=multiplayer_capacity||peer.address.sin_port||peer.address.sin_family!=AF_INET||
        (endpoints&(1u<<endpoint))||peer.ids.empty()||peer.ids.size()>4)
      throw FormatError("Host continuation has an invalid logical endpoint bank");
    endpoints|=std::uint16_t(1u<<endpoint);
    std::vector<unsigned> admitted;
    for(const auto &p:h.admissions)if(online_admission_peer(p)==endpoint)admitted.push_back(p.slot);
    if(admitted!=peer.ids||peer.selected_teams.size()!=peer.ids.size()||peer.profiles.size()!=peer.ids.size())
      throw FormatError("Host continuation differs from its accepted couch roster");
    for(auto slot:peer.ids) {
      if(slot>=multiplayer_capacity||(slots&(1u<<slot))||h.match.player(slot).source_datum==null_datum||
          (h.match.player(slot).source_flags&2u))
        throw FormatError("Host continuation has a missing, duplicate or departed remote player");
      slots|=std::uint16_t(1u<<slot);
    }
    auto &p=saved->peers.emplace_back();p.endpoint=endpoint;p.nonce=peer.nonce;p.cookie=peer.cookie;
    p.ids=peer.ids;p.selected_teams=peer.selected_teams;p.profiles=peer.profiles;
    p.sequence=peer.sequence;p.snapshot_sequence=peer.snapshot_sequence;p.seen=peer.seen;
    p.last_snapshot=peer.last_snapshot;
    p.idle=peer.age(now);p.controls=peer.controls;p.received_presses=peer.received_presses;
    p.delivered_presses=peer.delivered_presses;p.completed=peer.completed;p.completed_vehicle=peer.completed_vehicle;
    if(peer.vehicle_camera)p.camera=peer.vehicle_camera->checkpoint(binding);
    p.camera_tick=peer.vehicle_camera_tick;p.vehicle_views=peer.vehicle_views;p.death_views=peer.death_views;
    p.camera_jump_serial=peer.camera_jump_serial;
  }
  if(authority.execution_boundary()!=before)
    throw FormatError("Native world changed during host continuation capture");
  Continuation result;result.data_=std::move(saved);return result;
}
std::unique_ptr<LanHost> LanHost::resume(MultiplayerMatch &match,std::shared_ptr<MultiplayerDatagrams> transport,
    std::uint64_t fingerprint,std::vector<OnlineAdmission> admissions,const Continuation &cut,
    MultiplayerMatch::AuthorityDigest binding,std::span<const SourcePlayerAdmission> accepted_roster) {
  auto *world=match.source_object_runtime();
  if(!cut.data_||!transport||!world||!world->owns_physics())
    throw FormatError("Host continuation requires owning state, a native world and logical transport");
  host_continuation_boundary(*world);SourceMatchAuthority authority(match,*world);
  const auto before=authority.execution_boundary();const auto &saved=*cut.data_;
  const auto actual=authority.player_admissions(allocated_host_slots(match));
  std::vector<SourcePlayerAdmission> accepted(accepted_roster.begin(),accepted_roster.end());
  std::ranges::sort(accepted,{},&SourcePlayerAdmission::slot);
  if(saved.binding!=binding||saved.fingerprint!=fingerprint||saved.tick!=before.tick||
      !same_host_admissions(saved.admissions,admissions)||accepted!=saved.roster||actual!=accepted||
      saved.local_players!=host_local_players(*world))
    throw FormatError("Host continuation differs from its admitted cut, native identities or connection roster");
  for(const auto &peer:saved.peers)for(unsigned view=0;view<peer.ids.size();++view){
    const auto slot=peer.ids[view];const auto admitted=std::ranges::find(admissions,slot,&OnlineAdmission::slot);
    if(admitted==admissions.end()||peer.profiles[view]!=match.player(slot).source_profile_request||
        peer.selected_teams[view]!=(match.team_mode()?admitted->team:-1))
      throw FormatError("Host continuation has a different admitted profile request or join team");
  }
  auto host=std::make_unique<LanHost>(match,std::move(transport),fingerprint,std::move(admissions));
  auto &h=*host->impl_;
  if(h.session_configuration!=saved.configuration)
    throw FormatError("Host continuation has a different native session configuration");
  h.peers.reserve(saved.peers.size());const auto now=Clock::now();
  for(const auto &p:saved.peers) {
    Impl::Peer peer;peer.address={};peer.address.sin_family=AF_INET;
    peer.address.sin_addr.s_addr=htonl(p.endpoint+1);peer.nonce=p.nonce;peer.cookie=p.cookie;
    peer.ids=p.ids;peer.selected_teams=p.selected_teams;peer.profiles=p.profiles;
    peer.sequence=p.sequence;peer.snapshot_sequence=p.snapshot_sequence;peer.seen=p.seen;
    peer.last_snapshot=p.last_snapshot;
    // Retain the observed idle age. Time spent in the paused continuation is
    // neither fresh peer traffic nor time executing the resumed match.
    peer.last=now;peer.idle_offset=p.idle;peer.controls=p.controls;peer.received_presses=p.received_presses;
    peer.delivered_presses=p.delivered_presses;peer.completed=p.completed;peer.completed_vehicle=p.completed_vehicle;
    if(p.camera) {
      if(!match.original_data())throw FormatError("Host continuation lost native camera constants");
      peer.vehicle_camera=std::make_unique<NativeVehicleCamera>(*world,*match.original_data());
      auto prepared=peer.vehicle_camera->prepare(*p.camera,binding);
      if(!peer.vehicle_camera->commit(std::move(prepared)))
        throw FormatError("Host continuation could not restore its private remote camera");
    }
    peer.vehicle_camera_tick=p.camera_tick;peer.vehicle_views=p.vehicle_views;peer.death_views=p.death_views;
    peer.camera_jump_serial=p.camera_jump_serial;h.peers.push_back(std::move(peer));
  }
  if(authority.execution_boundary()!=before)
    throw FormatError("Native world changed during host continuation preparation");
  return host;
}
LanHost::HostLossResume LanHost::resume_after_host_loss(MultiplayerMatch &match,
    std::shared_ptr<MultiplayerDatagrams> transport,std::uint64_t fingerprint,std::vector<OnlineAdmission> admissions,
    const Continuation &cut,const AuthorityTransferContext &previous,const AuthorityTransferContext &next,
    std::span<const SourcePlayerAdmission> accepted){
  host_loss_contexts(previous,next);
  const auto before_binding=source_authority_binding(previous),after_binding=source_authority_binding(next);
  auto *world=match.source_object_runtime();
  if(!cut.data_||!world||!world->owns_physics()||cut.data_->binding!=before_binding)
    throw FormatError("Host loss has no owning native predecessor continuation");
  host_continuation_boundary(*world);SourceMatchAuthority authority(match,*world);const auto before=authority.execution_boundary();
  const auto &saved=*cut.data_;const auto promoted=std::ranges::find(saved.peers,next.host,&Continuation::Data::Peer::endpoint);
  if(promoted==saved.peers.end())throw FormatError("Replacement host has no admitted predecessor peer state");
  std::vector<unsigned> local_slots;
  for(const auto &p:admissions)if(online_admission_peer(p)==next.host)local_slots.push_back(p.slot);
  if(local_slots.empty()||local_slots!=promoted->ids||local_slots.size()>4)
    throw FormatError("Replacement host has a different admitted couch roster");
  if(promoted->profiles.size()!=local_slots.size()||promoted->selected_teams.size()!=local_slots.size())
    throw FormatError("Replacement host has an incomplete original profile request bank");
  std::array<std::uint32_t,4> local_players;local_players.fill(null_datum);
  for(unsigned i=0;i<local_slots.size();++i){const auto found=std::ranges::find(accepted,local_slots[i],&SourcePlayerAdmission::slot);
    if(found==accepted.end())throw FormatError("Replacement host omitted an independently admitted local player");
    const auto &player=match.player(local_slots[i]);const auto &admission=admissions.at(local_slots[i]);
    if(promoted->profiles[i]!=player.source_profile_request||
        promoted->selected_teams[i]!=(match.team_mode()?admission.team:-1))
      throw FormatError("Replacement host changed a local profile request or join team");
    local_players[i]=found->player;
  }
  if(host_local_players(*world)!=local_players)
    throw FormatError("Replacement native world has not rebound to its new local couch views");
  const auto rebind_camera=[&](const Continuation::Data::Peer &p){
    std::optional<NativeVehicleCamera::Checkpoint> camera;
    if(p.camera){std::vector<SourcePlayerAdmission> views;
      for(auto slot:p.ids){const auto found=std::ranges::find(accepted,slot,&SourcePlayerAdmission::slot);
        if(found==accepted.end())throw FormatError("Host loss omitted a camera's independent player admission");views.push_back(*found);}
      camera=NativeVehicleCamera::rebind_checkpoint(*p.camera,before_binding,after_binding,views);
    }return camera;
  };
  LocalContinuation local{promoted->ids,promoted->controls,promoted->received_presses,promoted->delivered_presses,
      promoted->completed,promoted->completed_vehicle,rebind_camera(*promoted),promoted->camera_tick,
      promoted->vehicle_views,promoted->death_views,promoted->camera_jump_serial};
  auto rebound=std::make_shared<Continuation::Data>(saved);rebound->binding=after_binding;rebound->local_players=local_players;
  std::erase_if(rebound->peers,[&](const auto &p){return p.endpoint==next.host;});
  for(auto &p:rebound->peers){p.camera=rebind_camera(p);p.last_snapshot.reset();}
  Continuation incoming;incoming.data_=std::move(rebound);
  auto host=resume(match,std::move(transport),fingerprint,std::move(admissions),incoming,after_binding,accepted);
  if(authority.execution_boundary()!=before)throw FormatError("Host role transition changed its native world");
  return {std::move(host),std::move(local)};
}
void LanHost::receive_local_continuation(LocalContinuation &saved,std::array<Controls,multiplayer_capacity> &input){
  if(saved.slots.empty()||saved.slots.size()>4)throw FormatError("Local continuation has no admitted couch seats");
  std::uint16_t seen{};for(auto slot:saved.slots){
    if(slot>=multiplayer_capacity||(seen&(1u<<slot)))throw FormatError("Local continuation repeats an invalid couch slot");
    seen|=std::uint16_t(1u<<slot);
  }
  for(unsigned j=0;j<saved.slots.size();++j){auto c=saved.controls[j];
    c.grenade=c.switch_weapon=c.melee=c.switch_grenade=false;
    std::array<bool *,10> actions{&c.jump,&c.fire,&c.reload,&c.grenade,&c.pickup,&c.dual_wield,&c.switch_weapon,&c.fire_secondary,&c.melee,&c.switch_grenade};
    for(unsigned action=0;action<actions.size();++action)
      if(newer(saved.received_presses[j][action],saved.delivered_presses[j][action])){
        *actions[action]=true;
        if(action==4||action==5){auto &duration=action==4?c.pickup_milliseconds:c.dual_wield_milliseconds;
          if(action==4){duration=saved.completed[j][0];c.vehicle_interaction_target=saved.completed_vehicle[j];}
          else duration=std::max(duration,saved.completed[j][1]);
          saved.delivered_presses[j][action]=saved.received_presses[j][action];
        }else ++saved.delivered_presses[j][action];
      }
    input[saved.slots[j]]=c;
  }
}
void LanHost::receive(std::array<Controls, multiplayer_capacity> &controls) {
  auto &h = *impl_;
  h.own();
  ++h.candidate_revision;
  Packet p;
  sockaddr_in from{};
  for (unsigned packets = 0; packets < 256 && h.receive_packet(p, from); ++packets)
    try {
      if(!h.admit(p,from))continue;
      auto request=decode_native_request(p,protocol_version);
      if(!request)throw FormatError("Invalid native host request");
      const auto nonce=std::visit([](const auto &value){return value.nonce;},*request);
      if(std::holds_alternative<NativeSessionQuery>(*request)) {
        h.describe(from,nonce);continue;
      }
      if (const auto *hello=std::get_if<NativeHelloRequest>(&*request)) {
        const auto fingerprint=hello->fingerprint;
        const auto count=hello->players.size();
        if (fingerprint != h.fingerprint) {
          h.rejection(from, nonce, "Map, shared game data, or starting configuration differs");
          continue;
        }
        std::vector<std::int8_t> selected_teams;
        std::vector<std::optional<SourcePlayerConfiguration>> profiles;
        static_assert(std::tuple_size_v<SourcePlayerConfiguration> == native_request_profile_bytes);
        for(const auto &player:hello->players) {
          selected_teams.push_back(player.team);profiles.push_back(player.profile);
        }
        std::vector<const OnlineAdmission *> admissions;
        if (h.udp.relay) {
          const auto slot = ntohl(from.sin_addr.s_addr)-1;
          for(const auto &a:h.admissions)if(online_admission_peer(a)==slot)admissions.push_back(&a);
          if (count != admissions.size() || admissions.empty() || admissions.front()->slot!=slot) { h.rejection(from,nonce,"Local players are not admitted to this match"); continue; }
          // The queue owner determines team and chosen display name. Profiles
          // remain appearance requests; they cannot rename a matched identity.
          for(unsigned i=0;i<count;++i)selected_teams[i] = h.match.team_mode() ? admissions[i]->team : -1;
        }
        if (std::any_of(selected_teams.begin(),selected_teams.end(),[&](auto team) {
              return team < (h.match.team_mode() ? 0 : -1) || team > 7;
            })) {
          h.rejection(from,nonce,"Team Slayer requires an explicitly selected source team 0..7");
          continue;
        }
        auto peer =
            std::find_if(h.peers.begin(), h.peers.end(),
                         [&](const auto &v) { return same(v.address, from); });
        if (peer != h.peers.end()) {
          if (peer->nonce == nonce && peer->selected_teams == selected_teams && peer->profiles==profiles) {
            const auto now=Clock::now();
            if(now-peer->last_response>=std::chrono::seconds(1) && now>=h.next_repeat_response){
              h.next_repeat_response=now+std::chrono::milliseconds(250);
              h.respond(*peer);
            }
          }
          continue;
        }
        // Native transport admission is one request for every local view.
        // Check capacity before creating any player: a rejected request must
        // not insert a never-joined team into the source sticky ranking mask.
        if (count > h.match.available_player_slots()) {
          h.rejection(from,nonce,"Multiplayer match is full");
          continue;
        }
        // Validate every local view before allocating any participant or
        // changing the original sticky team mask.
        try {
          for(unsigned i=0;i<count;++i)if(profiles[i]) {
            h.match.validate_player_configuration(*profiles[i],selected_teams[i]);
            native_player_configuration_name(*profiles[i]);
          }
        }catch(const FormatError& error) {
          h.rejection(from,nonce,error.what());continue;
        }
        Impl::Peer created;
        created.address = from;
        created.nonce = nonce;
        created.cookie = random_id();
        created.selected_teams = selected_teams;
        created.profiles=profiles;
        try {
          for (unsigned i = 0; i < count; ++i)
            created.ids.push_back(h.match.join(!admissions.empty() ? admissions[i]->name : profiles[i] ? native_player_configuration_name(*profiles[i])
                : "LAN Player",selected_teams[i],profiles[i], !admissions.empty() ? std::optional(admissions[i]->slot) : std::nullopt));
        } catch (const FormatError &e) {
          for (auto id : created.ids)
            h.match.cancel_join(id);
          h.rejection(from, nonce, e.what());
          continue;
        }
        for (unsigned i = 0; i < count; ++i)
          created.controls[i].yaw = h.match.unit(created.ids[i]).yaw;
        h.peers.push_back(std::move(created));
        h.respond(h.peers.back());
      } else {
        const auto *input=std::get_if<NativeInputRequest>(&*request);
        const auto cookie=input?input->cookie:std::get<NativeLeaveRequest>(*request).cookie;
        auto peer =
            std::find_if(h.peers.begin(), h.peers.end(), [&](const auto &v) {
              return same(v.address, from) && v.nonce == nonce &&
                     v.cookie == cookie;
            });
        if (peer == h.peers.end())
          continue;
        if (!input) {
          h.remove(std::size_t(peer - h.peers.begin()),PeerRemovalReason::authenticated_leave,Clock::now());
          continue;
        }
        const auto sequence=input->sequence;
        const auto count=input->controls.size();
        if (count != peer->ids.size())
          continue;
        std::array<WireControl, 4> decoded{};
        for (unsigned i = 0; i < count; ++i)
          decoded[i] = native_control(input->controls[i]);
        if (!peer->seen || newer(sequence, peer->sequence)) {
          bool serials_valid=true;
          for(unsigned view=0;view<count;++view)
            for(unsigned action=0;action<decoded[view].presses.size();++action) {
              const auto received=decoded[view].presses[action],prior=peer->received_presses[view][action];
              serials_valid&=received==prior || newer(received,prior);
            }
          if(!serials_valid)continue;
          for(unsigned view=0;view<count;++view) {
            peer->controls[view]=decoded[view].control;
            peer->received_presses[view]=decoded[view].presses;
            peer->completed[view]=decoded[view].completed;
            peer->completed_vehicle[view]=decoded[view].completed_vehicle;
          }
          peer->sequence = sequence;
          peer->seen = true;
          peer->heard();
          // Reuse the accepted input's actual heard clock. Duplicate Hello
          // refreshes ordinary liveness but cannot manufacture this witness.
          peer->last_accepted_input=peer->last;++peer->input_acceptances;
        }
      }
    } catch (const FormatError
                 &) { /* Malformed datagrams cannot mutate the match. */
    }
  for (std::size_t i = 0; i < h.peers.size();) {
    auto &peer = h.peers[i];
    const auto observed=Clock::now();
    const auto age = peer.age(observed);
    if (age > std::chrono::seconds(5)) {
      h.remove(i,PeerRemovalReason::idle_timeout,observed);
      continue;
    }
    for (unsigned j = 0; j < peer.ids.size(); ++j) {
      auto c = peer.controls[j];
      // Grenade is a consumed pulse, never a retained held input.
      c.grenade = c.switch_weapon = c.melee = c.switch_grenade = false;
      if (age > std::chrono::milliseconds(500)) {
        c.forward = c.right = 0;
        c.jump = c.fire = c.reload = c.fire_secondary = false;
        c.pickup=c.dual_wield=false;c.pickup_milliseconds=c.dual_wield_milliseconds=0;
        peer.delivered_presses[j]=peer.received_presses[j];
      } else {
        std::array<bool *,10> actions{&c.jump,&c.fire,&c.reload,&c.grenade,&c.pickup,&c.dual_wield,&c.switch_weapon,&c.fire_secondary,&c.melee,&c.switch_grenade};
        for(unsigned action=0;action<actions.size();++action)
          if(newer(peer.received_presses[j][action],peer.delivered_presses[j][action])) {
            *actions[action]=true;
            if(action==4 || action==5) {
              auto &duration=action==4?c.pickup_milliseconds:c.dual_wield_milliseconds;
              if(action==4) {
                duration=peer.completed[j][0];
                c.vehicle_interaction_target=peer.completed_vehicle[j];
              } else duration=std::max(duration,peer.completed[j][1]);
              // A completion serial owns its saved duration, independently of
              // the latest held input. Later neutral packets repeat both.
              peer.delivered_presses[j][action]=peer.received_presses[j][action];
            } else ++peer.delivered_presses[j][action];
          }
      }
      controls[peer.ids[j]] = c;
    }
    ++i;
  }
}
void LanHost::broadcast() {
  impl_->own();
  for (auto &peer : impl_->peers)
    impl_->respond(peer);
}
struct LanHost::CandidateEmission::Data {
  struct Peer {unsigned endpoint;std::vector<unsigned> ids;MultiplayerSnapshotStamp stamp;};
  std::weak_ptr<const void> owner;
  std::weak_ptr<PeerSnapshotCandidate> delivery;
  PeerSnapshotToken token;
  std::uint64_t revision{};
  std::uint32_t tick{};
  std::vector<Peer> peers;
};
const PeerSnapshotToken &LanHost::CandidateEmission::token()const noexcept{
  static const PeerSnapshotToken empty;return data_?data_->token:empty;
}
std::uint32_t LanHost::CandidateEmission::tick()const noexcept{return data_?data_->tick:0;}
LanHost::CandidateBroadcast LanHost::broadcast_candidate(
    std::shared_ptr<PeerSnapshotCandidate> delivery,const PeerSnapshotToken &token){
  auto &h=*impl_;h.own();
  if(!delivery||delivery->self()!=token.context.host||!h.udp.relay)return {};
  const auto prepared=delivery->state();
  if(prepared.pending!=token||prepared.context!=token.context||prepared.closing||prepared.published||
      prepared.armed!=token.recipients||prepared.recipients!=token.recipients)return {};
  auto emission=std::make_shared<CandidateEmission::Data>();
  emission->owner=h.candidate_owner;emission->delivery=delivery;emission->token=token;
  std::uint16_t endpoints{};
  for(const auto &peer:h.peers){const auto endpoint=ntohl(peer.address.sin_addr.s_addr)-1;
    if(endpoint>=16||peer.address.sin_port||peer.address.sin_family!=AF_INET||(endpoints&(1u<<endpoint)))return {};
    endpoints|=std::uint16_t(1u<<endpoint);
  }
  if(endpoints!=token.recipients)return {};
  std::vector<PeerSnapshotArchive> archives;archives.reserve(h.peers.size());emission->peers.reserve(h.peers.size());
  try{
    for(auto &peer:h.peers){const auto endpoint=ntohl(peer.address.sin_addr.s_addr)-1;
      PeerSnapshotArchive archive{endpoint,{}};h.respond(peer,&archive.bytes);
      if(!peer.last_snapshot)throw FormatError("Candidate broadcast lost its actual snapshot stamp");
      if(emission->peers.empty())emission->tick=peer.last_snapshot->tick;
      else if(emission->tick!=peer.last_snapshot->tick)throw FormatError("Candidate broadcast crossed native ticks");
      emission->peers.push_back({endpoint,peer.ids,*peer.last_snapshot});archives.push_back(std::move(archive));
    }
    emission->revision=h.candidate_revision;
    const auto now=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();
    if(!delivery->publish(token,std::move(archives),now)){delivery->close(token,PeerSnapshotClose::Abort);return {true,{}};}
  }catch(...){delivery->close(token,PeerSnapshotClose::Abort);throw;}
  CandidateEmission result;result.data_=std::move(emission);return {true,std::move(result)};
}
bool LanHost::candidate_current(const CandidateEmission &emission,const std::shared_ptr<PeerSnapshotCandidate> &expected)const noexcept{
  try{
    const auto &h=*impl_;h.own();if(!emission.data_)return false;const auto &e=*emission.data_;
    if(e.owner.lock()!=h.candidate_owner||e.revision!=h.candidate_revision||e.peers.size()!=h.peers.size())return false;
    const auto delivery=e.delivery.lock();if(!delivery||delivery!=expected)return false;const auto state=delivery->state();
    if(state.pending!=e.token||!state.published||state.closing)return false;
    for(const auto &p:e.peers){const auto peer=std::ranges::find_if(h.peers,[&](const auto &v){
        return v.address.sin_family==AF_INET&&!v.address.sin_port&&ntohl(v.address.sin_addr.s_addr)==p.endpoint+1;});
      if(peer==h.peers.end()||peer->ids!=p.ids||!peer->last_snapshot||*peer->last_snapshot!=p.stamp||
          peer->snapshot_sequence!=p.stamp.sequence)return false;
    }return true;
  }catch(...){return false;}
}
struct LanClient::SnapshotCandidate {
  std::weak_ptr<const void> live;
  std::shared_ptr<PeerSnapshotCandidate> delivery;
  PeerSnapshotToken token;
  const std::thread::id executor=std::this_thread::get_id();
  std::optional<PeerSnapshotCandidate::Delivery> packet;
  MultiplayerSnapshotOrigin origin;
  bool fenced=true,captured{},retired{};
};
struct LanClient::Impl {
  struct Assembly {
    std::uint64_t cookie{};
    unsigned total{}, count{}, received{};
    std::map<unsigned,Packet> fragments;
    Clock::time_point started=Clock::now();
  };
  Udp udp{0};
  sockaddr_in host;
  std::uint64_t fingerprint{}, nonce = random_id(), cookie{};
  LanAdmission::Cookie admission{};
  unsigned count{}, attempts{};
  Packet admission_hello;
  std::array<std::int8_t,4> selected_teams{-1,-1,-1,-1};
  std::array<std::optional<SourcePlayerConfiguration>,4> profiles;
  std::uint32_t sequence{};
  std::array<PressSerials,4> presses{};
  Packet latest_input;
  std::array<std::array<bool,6>,4> held{};
  std::array<InteractionDurations,4> active_interactions{},completed_interactions{};
  std::array<VehicleIntent,4> completed_vehicle{};
  std::uint32_t snapshot_sequence{};
  std::map<std::uint32_t,Assembly> assemblies;
  Clock::time_point assembly_window=Clock::now();
  unsigned assembly_creations{};
  bool connected{};
  MultiplayerState state;
  MultiplayerSnapshotOrigin state_origin;
  std::vector<MultiplayerState> received_states;
  std::vector<MultiplayerSnapshotOrigin> received_origins;
  std::shared_ptr<const int> snapshot_owner=std::make_shared<const int>(0);
  std::shared_ptr<const void> candidate_live=std::make_shared<const unsigned char>(0);
  std::shared_ptr<SnapshotCandidate> candidate;
  std::vector<unsigned> ids;
  std::string error;
  std::uint64_t rejected_packets{};
  std::string last_packet_rejection;
  Clock::time_point last = Clock::now();
  std::optional<Clock::time_point> started;
  std::optional<AuthorityTransferContext> continuity;
  std::uint64_t rebind_revision{};
  unsigned physical_self=16;
  std::optional<std::uint32_t> rebound_tick;
  Impl(std::string address, std::uint64_t f, unsigned n, std::span<const std::int8_t> teams,
      std::span<const SourcePlayerConfiguration> selected_profiles)
      : host(resolve(std::move(address))), fingerprint(f), count(n) {
    if (n < 1 || n > 4)
      throw FormatError("LAN client requires 1..4 local players");
    if (!teams.empty() && (teams.size() != n ||
        std::any_of(teams.begin(),teams.end(),[](auto team){return team < 0 || team > 7;})))
      throw FormatError("LAN team selections must provide one source team 0..7 per local player");
    std::copy(teams.begin(),teams.end(),selected_teams.begin());
    if(!selected_profiles.empty() && selected_profiles.size()!=n)
      throw FormatError("LAN profiles must provide one complete record per local player");
    for(unsigned i=0;i<selected_profiles.size();++i) {
      native_player_configuration_name(selected_profiles[i]);
      profiles[i]=selected_profiles[i];
    }
  }
  Impl(std::shared_ptr<MultiplayerDatagrams> relay, unsigned peer, std::uint64_t f,
       std::span<const std::int8_t> teams, std::span<const SourcePlayerConfiguration> selected_profiles)
      : udp(std::move(relay)), fingerprint(f), count(1) {
    host={}; host.sin_family=AF_INET; host.sin_addr.s_addr=htonl(peer+1);
    count=unsigned(std::max(teams.size(),selected_profiles.size()));
    if(!count)count=1;
    if(count>4 || (!teams.empty() && teams.size()!=count) || (!selected_profiles.empty() && selected_profiles.size()!=count))
      throw FormatError("ONLINE local profiles/teams must describe one to four admitted players");
    for(unsigned i=0;i<teams.size();++i)selected_teams[i]=teams[i];
    for(unsigned i=0;i<selected_profiles.size();++i)profiles[i]=selected_profiles[i];
  }
};
LanClient::LanClient(const std::string &a, std::uint64_t f, unsigned n, std::span<const std::int8_t> teams,
    std::span<const SourcePlayerConfiguration> profiles)
    : impl_(std::make_unique<Impl>(a, f, n, teams,profiles)) {}
LanClient::~LanClient() {
  reject_snapshot_candidate(impl_->candidate);
  impl_->candidate_live.reset();
  if (impl_->connected) {
    auto p = header(leave);
    put(p, impl_->nonce, 8);
    put(p, impl_->cookie, 8);
    impl_->udp.send(p, impl_->host);
  }
}
LanClient::LanClient(std::shared_ptr<MultiplayerDatagrams> relay, unsigned host,
                     std::uint64_t f, std::span<const std::int8_t> teams,
                     std::span<const SourcePlayerConfiguration> profiles)
    : impl_(std::make_unique<Impl>(std::move(relay),host,f,teams,profiles)) {}
std::shared_ptr<LanClient::SnapshotCandidate> LanClient::arm_snapshot_candidate(
    std::shared_ptr<PeerSnapshotCandidate> delivery,const PeerSnapshotToken &token){
  auto &v=*impl_;
  if(!delivery||delivery->self()!=v.physical_self||!v.udp.relay||!v.connected||!v.error.empty()||!v.continuity||*v.continuity!=token.context||
      v.rebound_tick||v.physical_self>=16||!(token.recipients&(1u<<v.physical_self))||
      (v.candidate&&!v.candidate->retired))throw FormatError("Candidate fence requires the current admitted native client");
  const auto state=delivery->state();
  if(state.pending!=token||state.closing||state.context!=token.context||state.recipients!=token.recipients)
    throw FormatError("Candidate preparation has a different session token");
  if(!v.candidate_live)v.candidate_live=std::make_shared<const unsigned char>(0);
  auto candidate=std::make_shared<SnapshotCandidate>();candidate->live=v.candidate_live;
  candidate->delivery=std::move(delivery);candidate->token=token;
  v.candidate=candidate;
  if(!candidate->delivery->acknowledge_arm(token)){v.candidate.reset();throw FormatError("Candidate preparation retired before its native fence");}
  return candidate;
}
bool LanClient::snapshot_candidate_live(const std::shared_ptr<SnapshotCandidate> &c)noexcept{
  return c&&c->executor==std::this_thread::get_id()&&!c->retired&&!c->live.expired();
}
bool LanClient::snapshot_candidate_matches(const std::shared_ptr<SnapshotCandidate> &c,
    const MultiplayerSnapshotOrigin &origin)noexcept{
  return snapshot_candidate_live(c)&&c->origin.same(origin);
}
bool LanClient::capture_snapshot_candidate(const std::shared_ptr<SnapshotCandidate> &c)noexcept{
  try{
    if(!snapshot_candidate_live(c))return false;
    if(c->captured)return true;
    if(!c->origin.tick()||!c->packet||!c->delivery->acknowledge_captured(std::move(*c->packet)))return false;
    c->packet.reset();c->captured=true;c->fenced=false;return true;
  }catch(...){return false;}
}
bool LanClient::can_retire_snapshot_candidate(const std::shared_ptr<SnapshotCandidate> &c)noexcept{
  try{
    return c&&c->executor==std::this_thread::get_id()&&
        (c->retired||c->delivery->can_acknowledge_retired(c->token,c->packet));
  }catch(...){return false;}
}
bool LanClient::retire_snapshot_candidate(const std::shared_ptr<SnapshotCandidate> &c)noexcept{
  try{
    if(!c||c->executor!=std::this_thread::get_id())return false;
    if(c->retired)return true;
    if(!can_retire_snapshot_candidate(c))return false;
    // Foreground retirement drops its own receipt/fence before the worker can
    // publish Retired. A refused final ACK keeps the real delivery for retry.
    c->origin={};c->fenced=false;
    if(!c->delivery->acknowledge_retired(c->token,std::move(c->packet))){c->fenced=true;return false;}
    c->packet.reset();c->origin={};c->fenced=false;c->retired=true;return true;
  }catch(...){return false;}
}
void LanClient::reject_snapshot_candidate(const std::shared_ptr<SnapshotCandidate> &c)noexcept{
  try{if(c&&c->executor==std::this_thread::get_id()&&!c->retired)c->delivery->reject(c->token);}catch(...){}
}
void LanClient::send(std::span<const Controls> c) {
  auto &v = *impl_;
  if (c.size() != v.count)
    throw FormatError("LAN input count differs from joined players");
  if (!v.error.empty())
    return;
  if (!v.connected) {
    if (v.attempts++ % 15)
      return;
    auto p = header(hello);
    put(p, v.nonce, 8);
    put(p, v.fingerprint, 8);
    put(p, v.count, 1);
    for (unsigned i = 0; i < v.count; ++i) put(p,std::uint8_t(v.selected_teams[i]),1);
    for(unsigned i=0;i<v.count;++i)encode_profile(p,v.profiles[i]);
    v.admission_hello=p;
    v.udp.send(v.udp.relay?p:admission_envelope(p,v.admission), v.host);
    // Graphics/resource preparation may outlive the connection timeout.
    // Begin measuring an unanswered request only when one has been sent.
    if (!v.started) v.started = Clock::now();
    return;
  }
  auto p = header(input);
  put(p, v.nonce, 8);
  put(p, v.cookie, 8);
  put(p, ++v.sequence, 4);
  put(p, v.count, 1);
  for (unsigned view=0;view<c.size();++view) {
    const std::array<bool,6> down{c[view].jump,c[view].fire,c[view].reload,c[view].fire_secondary,c[view].pickup,c[view].dual_wield};
    for(unsigned action=0;action<4;++action)
      if(down[action] && !v.held[view][action])++v.presses[view][action==3?7:action];
    const InteractionDurations durations{c[view].pickup_milliseconds,c[view].dual_wield_milliseconds};
    for(unsigned action=0;action<durations.size();++action) {
      auto &active=v.active_interactions[view][action];
      if(down[4+action]) {
        if(!v.held[view][4+action])active=0;
        active=std::max(active,durations[action]);
      } else if(v.held[view][4+action]) {
        v.completed_interactions[view][action]=active;
        if(action==0)v.completed_vehicle[view]=c[view].vehicle_interaction_target;
        ++v.presses[view][4+action];active=0;
      }
    }
    // Every supplied grenade pulse is a distinct consumption, including
    // pulses on consecutive simulation ticks. Repeated wire packets carry
    // the same serial and cannot retrigger the action at the authority.
    if(c[view].grenade)++v.presses[view][3];
    if(c[view].switch_weapon)++v.presses[view][6];
    if(c[view].melee)++v.presses[view][8];
    if(c[view].switch_grenade)++v.presses[view][9];
    v.held[view]=down;
    encode_control(p,c[view],v.presses[view],v.completed_interactions[view],v.completed_vehicle[view]);
  }
  v.latest_input=std::move(p);v.udp.send(v.latest_input, v.host);
}
void LanClient::receive() {
  auto &v = *impl_;
  v.received_states.clear();
  v.received_origins.clear();
  Packet p;
  sockaddr_in from{};
  std::vector<std::span<const std::byte>> reserved;
  const auto candidate=v.candidate;
  if(snapshot_candidate_live(candidate)&&candidate->fenced&&!candidate->origin.tick())try{
    if(!candidate->packet)candidate->packet=candidate->delivery->take_delivery();
    if(candidate->packet){
      if(candidate->packet->token()!=candidate->token||candidate->packet->recipient()!=v.physical_self)
        throw FormatError("Reserved candidate delivery has a different owner");
      reserved=candidate_packets(candidate->packet->bytes(),v.nonce,v.cookie,v.snapshot_sequence);
    }
  }catch(const FormatError &error){++v.rejected_packets;v.last_packet_rejection=error.what();reject_snapshot_candidate(candidate);}
  unsigned reserved_index{},packets{};
  while(true){
    const bool reserved_packet=reserved_index<reserved.size();
    if(reserved_packet){const auto raw=reserved[reserved_index++];p.assign(raw.begin(),raw.end());from=v.host;}
    else if(packets++>=256||!v.udp.receive(p,from))break;
    try {
      if (!same(from, v.host))
        continue;
      Reader r{p};
      auto type = r.type();
      if (r.get(8) != v.nonce)
        continue;
      if(type==admission_challenge&&!v.udp.relay&&!v.connected){
        if(p.size()!=31)continue;
        for(auto &b:v.admission)b=r.get(1);
        if(!v.admission_hello.empty())v.udp.send(admission_envelope(v.admission_hello,v.admission),v.host);
        continue;
      }
      if (type == reject && !v.connected) {
        v.error.assign(reinterpret_cast<const char *>(p.data() + r.at),
                       p.size() - r.at);
        continue;
      }
      if (type != snapshot)
        continue;
      const auto cookie = r.get(8);
      const auto sequence=std::uint32_t(r.get(4));
      const auto index=unsigned(r.get(2)), fragments=unsigned(r.get(2)), total=unsigned(r.get(4));
      if ((v.connected && (cookie != v.cookie || (!v.rebound_tick&&!newer(sequence,v.snapshot_sequence)))))
        continue;
      if(!total || total>snapshot_limit || !fragments ||
          fragments!=(total+snapshot_chunk-1)/snapshot_chunk || index>=fragments ||
          p.size()-r.at!=std::min(snapshot_chunk,total-index*snapshot_chunk))
        throw FormatError("Invalid native LAN snapshot fragment");
      if(snapshot_candidate_live(candidate)&&candidate->fenced&&!reserved_packet){
        // Genuine current-host traffic remains liveness evidence, while its
        // ordinary state/sequence cannot supersede the reserved future body.
        v.last=Clock::now();continue;
      }
      for(auto at=v.assemblies.begin();at!=v.assemblies.end();)
        if(Clock::now()-at->second.started>std::chrono::seconds(1) ||
            (v.connected && !v.rebound_tick && !newer(at->first,v.snapshot_sequence)))at=v.assemblies.erase(at);
        else ++at;
      auto at=v.assemblies.find(sequence);
      if(at==v.assemblies.end()) {
        const auto now=Clock::now();
        if(now-v.assembly_window>=std::chrono::seconds(1)){
          v.assembly_window=now;v.assembly_creations=0;
        }
        if(v.assembly_creations>=128)continue;
        ++v.assembly_creations;
        // A bounded set allows interleaved, reordered UDP snapshots. Eviction
        // discards an incomplete message; it never publishes partial state.
        if(v.assemblies.size()==4) {
          const auto oldest=std::min_element(v.assemblies.begin(),v.assemblies.end(),
              [](const auto &a,const auto &b){return a.second.started<b.second.started;});
          v.assemblies.erase(oldest);
        }
        Impl::Assembly created;
        created.cookie=cookie;created.total=total;created.count=fragments;
        at=v.assemblies.emplace(sequence,std::move(created)).first;
      }
      auto &assembly=at->second;
      if(assembly.cookie!=cookie || assembly.total!=total || assembly.count!=fragments) {
        v.assemblies.erase(at);
        throw FormatError("Conflicting native LAN snapshot fragments");
      }
      if(const auto found=assembly.fragments.find(index);found!=assembly.fragments.end()) {
        if(!std::equal(p.begin()+r.at,p.end(),found->second.begin(),found->second.end())) {
          v.assemblies.erase(at);
          throw FormatError("Conflicting duplicate LAN snapshot fragment");
        }
        continue;
      }
      assembly.fragments.emplace(index,Packet(p.begin()+r.at,p.end()));
      if(++assembly.received!=assembly.count)continue;
      // Allocate the declared body only after every byte has arrived. Tiny
      // tail fragments never cause a full snapshot allocation or zero-fill.
      Packet body;body.reserve(assembly.total);
      for(const auto &[part,bytes]:assembly.fragments)body.insert(body.end(),bytes.begin(),bytes.end());
      v.assemblies.erase(at);
      Reader complete{body};
      const auto count=complete.get(1);
      if(count!=v.count)throw FormatError("Invalid LAN local player count");
      std::vector<unsigned> ids;
      for (unsigned i = 0; i < count; ++i) {
        auto id = unsigned(complete.get(1));
        if (id >= multiplayer_capacity ||
            std::find(ids.begin(), ids.end(), id) != ids.end())
          throw FormatError("Invalid LAN slot assignment");
        ids.push_back(id);
      }
      auto state = decode_state(complete,ids);
      complete.end();
      for (auto id : ids)
        if (!state.players[id].connected)
          throw FormatError("Missing assigned LAN player");
      if(v.rebound_tick){
        if(ids!=v.ids||!(state.tick==*v.rebound_tick||newer(state.tick,*v.rebound_tick)))continue;
      }
      // A newer complete native snapshot can change roster/ownership between
      // world ticks. Its transport sequence already rejects stale fragments;
      // permit equal clocks without advancing the original simulation clock.
      if (v.rebound_tick || !v.connected || state.tick == v.state.tick || newer(state.tick, v.state.tick)) {
        MultiplayerSnapshotStamp stamp{v.fingerprint,v.nonce,cookie,sequence,state.tick,std::uint32_t(body.size())};
        if(!SHA256(reinterpret_cast<const unsigned char*>(body.data()),body.size(),
            reinterpret_cast<unsigned char*>(stamp.digest.data())))throw FormatError("Cannot hash accepted snapshot body");
        // Stage bound owning copies before publishing any accepted value.
        // Legacy clients keep their existing state-copy path and no payload.
        std::optional<MultiplayerState> adopted_state;
        std::unique_ptr<const MultiplayerSnapshotOrigin::Data::Decoded> decoded;
        if(v.continuity){
          adopted_state.emplace(state);
          decoded=std::make_unique<const MultiplayerSnapshotOrigin::Data::Decoded>(
              MultiplayerSnapshotOrigin::Data::Decoded{state,ids});
        }
        MultiplayerSnapshotOrigin origin;
        origin.data_=std::make_shared<MultiplayerSnapshotOrigin::Data>(MultiplayerSnapshotOrigin::Data{
            v.snapshot_owner,v.continuity,v.physical_self,v.rebind_revision,stamp,std::move(decoded)});
        const auto reserve_next=[](auto &rows){
          if(rows.size()!=rows.capacity())return;
          const auto maximum=rows.max_size(),capacity=rows.capacity();
          if(rows.size()==maximum)throw std::length_error("Accepted snapshot batch exceeds container limits");
          rows.reserve(capacity>maximum/2?maximum:std::max<std::size_t>(1,capacity*2));
        };
        reserve_next(v.received_states);reserve_next(v.received_origins);
        static_assert(std::is_nothrow_move_assignable_v<MultiplayerState> &&
            std::is_nothrow_move_constructible_v<MultiplayerState>);
        v.cookie = cookie;
        v.ids = std::move(ids);
        if(adopted_state)v.state = std::move(*adopted_state);
        else v.state = state;
        v.received_states.push_back(std::move(state));
        v.received_origins.push_back(origin);
        v.state_origin=std::move(origin);
        if(reserved_packet)candidate->origin=v.state_origin;
        v.connected = true;
        v.last = Clock::now();
        v.rebound_tick.reset();
      }
      v.snapshot_sequence=sequence;
    } catch (const FormatError &error) {
      ++v.rejected_packets;v.last_packet_rejection=error.what();
      if(reserved_packet)reject_snapshot_candidate(candidate);
    }
  }
  if(!reserved.empty()&&!candidate->origin.tick())reject_snapshot_candidate(candidate);
  if ((v.connected || v.started) && Clock::now() - (v.connected ? v.last : *v.started) >
          std::chrono::seconds(10) &&
      v.error.empty())
    v.error = v.connected ? "LAN host disconnected" : "LAN host did not answer";
}
bool LanClient::ready() const { return impl_->connected; }
std::uint64_t LanClient::rejected_packet_count()const{return impl_->rejected_packets;}
std::string LanClient::last_packet_rejection()const{return impl_->last_packet_rejection;}
std::size_t LanClient::snapshot_buffered_bytes()const{
  std::size_t bytes{};
  for(const auto &[sequence,assembly]:impl_->assemblies)
    for(const auto &[index,fragment]:assembly.fragments)bytes+=fragment.capacity();
  return bytes;
}
const MultiplayerState &LanClient::state() const { return impl_->state; }
MultiplayerSnapshotOrigin LanClient::state_origin() const { return impl_->state_origin; }
std::span<const MultiplayerState> LanClient::received_states() const { return impl_->received_states; }
std::span<const MultiplayerSnapshotOrigin> LanClient::received_origins() const { return impl_->received_origins; }
std::optional<std::uint32_t> MultiplayerSnapshotOrigin::tick()const noexcept {
  return data_?std::optional<std::uint32_t>(data_->stamp.tick):std::nullopt;
}
std::optional<std::uint32_t> MultiplayerSnapshotOrigin::sequence()const noexcept {
  return data_?std::optional<std::uint32_t>(data_->stamp.sequence):std::nullopt;
}
std::optional<AuthorityTransferContext> MultiplayerSnapshotOrigin::context()const noexcept {
  return data_?data_->context:std::nullopt;
}
bool MultiplayerSnapshotOrigin::same(const MultiplayerSnapshotOrigin &other)const noexcept {
  return data_ && data_==other.data_;
}
const MultiplayerState &MultiplayerSnapshotOrigin::accepted_state()const {
  if(!data_ || !data_->decoded)throw FormatError("Snapshot receipt has no continuity-bound decoded state");
  return data_->decoded->state;
}
std::span<const unsigned> MultiplayerSnapshotOrigin::local_slots()const noexcept {
  return data_ && data_->decoded?std::span<const unsigned>(data_->decoded->slots):std::span<const unsigned>{};
}
bool MultiplayerSnapshotOrigin::same_connection(const MultiplayerSnapshotOrigin &other)const noexcept {
  return data_ && other.data_ && data_->decoded && other.data_->decoded &&
      data_->owner==other.data_->owner && data_->context && data_->context==other.data_->context &&
      data_->self==other.data_->self && data_->revision==other.data_->revision;
}
const ValidatedAuthorityCut &MultiplayerSnapshotProof::cut()const {
  if(!data_)throw FormatError("Missing accepted snapshot proof");return data_->cut;
}
bool MultiplayerSnapshotProof::matches(const MultiplayerSnapshotOrigin &origin)const noexcept {
  return data_ && data_->origin.same(origin);
}
MultiplayerSnapshotProof LanClient::prove_snapshot(const LanHost::Continuation &cut,
    const ValidatedAuthorityCut &expected,const MultiplayerSnapshotOrigin &origin)const {
  const auto &v=*impl_;
  if(!cut.data_ || !origin.data_ || origin.data_->owner!=v.snapshot_owner || !v.udp.relay || !v.connected ||
      !expected.seal.id || !expected.seal.bytes || expected.peer>=16 ||
      !(expected.context.peers&(1u<<expected.peer)) ||
      std::ranges::all_of(expected.seal.digest,[](auto b){return b==std::byte{};}) ||
      !v.continuity || *v.continuity!=expected.context || origin.data_->context!=v.continuity ||
      origin.data_->self!=v.physical_self || origin.data_->revision!=v.rebind_revision ||
      v.physical_self>=16 || !(expected.context.peers&(1u<<v.physical_self)) ||
      ntohl(v.host.sin_addr.s_addr)!=expected.context.host+1 || v.host.sin_port ||
      cut.data_->binding!=source_authority_binding(expected.context) || cut.data_->tick!=expected.seal.tick ||
      cut.data_->fingerprint!=v.fingerprint)
    throw FormatError("Snapshot receipt has a different native connection or authority cut");
  const auto peer=std::ranges::find(cut.data_->peers,v.physical_self,&LanHost::Continuation::Data::Peer::endpoint);
  if(peer==cut.data_->peers.end() || peer->nonce!=v.nonce || peer->cookie!=v.cookie || peer->ids!=v.ids ||
      !peer->last_snapshot || *peer->last_snapshot!=origin.data_->stamp ||
      peer->last_snapshot->tick!=expected.seal.tick || peer->snapshot_sequence!=peer->last_snapshot->sequence)
    throw FormatError("Snapshot receipt is not the exact body/sequence retained by this native cut");
  MultiplayerSnapshotProof result;result.data_=std::make_shared<MultiplayerSnapshotProof::Data>(
      MultiplayerSnapshotProof::Data{origin,expected});return result;
}
std::span<const unsigned> LanClient::slots() const { return impl_->ids; }
std::string LanClient::error() const { return impl_->error; }
void LanClient::bind_continuity(const AuthorityTransferContext &context,unsigned self,std::span<const OnlineAdmission> roster){
  auto &v=*impl_;
  if(!v.udp.relay||!v.connected||!v.error.empty()||v.continuity||self>=16||context.host>=16||
      context.host==self||!(context.peers&(1u<<self))||!(context.peers&(1u<<context.host))||
      ntohl(v.host.sin_addr.s_addr)!=context.host+1||v.host.sin_port||
      std::ranges::all_of(context.session,[](auto b){return b==std::byte{};}))
    throw FormatError("Client continuity requires its independently admitted native peer context");
  if(roster.size()<2||roster.size()>16||self>=roster.size()||online_admission_peer(roster[self])!=self||roster[self].guest)
    throw FormatError("Client continuity omitted its physical connection owner");
  std::vector<unsigned> slots;std::uint16_t seen{},physical{};
  for(const auto &p:roster){if(p.slot>=roster.size()||(seen&(1u<<p.slot)))throw FormatError("Client continuity has a duplicate admitted slot");
    const auto owner=online_admission_peer(p);
    if(owner>=roster.size()||online_admission_peer(roster[owner])!=owner||roster[owner].guest||roster[p.slot].slot!=p.slot||
        std::ranges::count_if(roster,[&](const auto &a){return online_admission_peer(a)==owner;})>4)
      throw FormatError("Client continuity has a different physical couch-owner table");
    seen|=std::uint16_t(1u<<p.slot);physical|=std::uint16_t(1u<<owner);if(owner==self)slots.push_back(p.slot);}
  if(slots!=v.ids||(context.peers&~physical))throw FormatError("Client continuity differs from its accepted couch slots");
  v.continuity=context;v.physical_self=self;++v.rebind_revision;
  v.state_origin={};
}
void LanClient::merge_local_input(const LanHost::Continuation &cut,const AuthorityTransferContext &previous,
    LanHost::LocalContinuation &local)const{
  static_assert(std::is_nothrow_copy_assignable_v<Controls>&&std::is_nothrow_copy_assignable_v<VehicleIntent>);
  const auto &v=*impl_;
  if(!cut.data_||!v.udp.relay||!v.connected||!v.continuity||*v.continuity!=previous||
      v.physical_self>=16||v.physical_self==previous.host||!(previous.peers&(1u<<v.physical_self))||
      ntohl(v.host.sin_addr.s_addr)!=previous.host+1||cut.data_->binding!=source_authority_binding(previous)||
      cut.data_->fingerprint!=v.fingerprint)
    throw FormatError("Promoted input has a different native connection or authority context");
  const auto &peers=cut.data_->peers;
  const auto found=std::ranges::find(peers,v.physical_self,&LanHost::Continuation::Data::Peer::endpoint);
  if(found==peers.end()||found->nonce!=v.nonce||found->cookie!=v.cookie||found->ids!=v.ids||local.slots!=v.ids||
      v.ids.empty()||v.ids.size()>4||v.count!=v.ids.size()||found->profiles.size()!=v.count||
      found->selected_teams.size()!=v.count||!std::equal(found->profiles.begin(),found->profiles.end(),v.profiles.begin())||
      !std::equal(found->selected_teams.begin(),found->selected_teams.end(),v.selected_teams.begin()))
    throw FormatError("Promoted input changed its admitted couch owner, cookies or profile requests");
  const auto &prior=*found;
  for(unsigned view=0;view<v.count;++view)for(unsigned action=0;action<v.presses[view].size();++action)
    if(local.delivered_presses[view][action]!=prior.delivered_presses[view][action]||
        !(local.received_presses[view][action]==prior.received_presses[view][action]||
          newer(local.received_presses[view][action],prior.received_presses[view][action])))
      throw FormatError("Promoted input was consumed or rolled back before native activation");
  if(v.latest_input.empty()){
    if(prior.seen||v.sequence)throw FormatError("Promoted input lost its latest owning packet");
    return;
  }
  auto request=decode_native_request(v.latest_input,protocol_version);
  const auto *input=request?std::get_if<NativeInputRequest>(&*request):nullptr;
  if(!input||input->nonce!=v.nonce||input->cookie!=v.cookie)
    throw FormatError("Promoted input packet has a different native connection");
  const auto sequence=input->sequence;const auto count=input->controls.size();
  if(sequence!=v.sequence||count!=v.count||(prior.seen&&sequence!=prior.sequence&&!newer(sequence,prior.sequence)))
    throw FormatError("Promoted input packet predates its owning client or saved host cut");
  std::array<WireControl,4> decoded{};
  for(unsigned view=0;view<count;++view)decoded[view]=native_control(input->controls[view]);
  for(unsigned view=0;view<count;++view)for(unsigned action=0;action<decoded[view].presses.size();++action){
    const auto incoming=decoded[view].presses[action],received=local.received_presses[view][action];
    if(incoming!=v.presses[view][action]||!(incoming==received||newer(incoming,received))||
        (prior.seen&&sequence==prior.sequence&&incoming!=prior.received_presses[view][action]))
      throw FormatError("Promoted input packet has stale or inconsistent delivery serials");
  }
  if(prior.seen&&sequence==prior.sequence)return;
  // Parsing and every independent ownership/serial check precede mutation.
  // Keep delivered counters and native camera state at their accepted cut.
  for(unsigned view=0;view<count;++view){
    local.controls[view]=decoded[view].control;local.received_presses[view]=decoded[view].presses;
    local.completed[view]=decoded[view].completed;local.completed_vehicle[view]=decoded[view].completed_vehicle;
  }
}
struct LanClient::PreparedRebind::Impl {
  const LanClient::Impl *owner{};
  std::shared_ptr<const int> owner_identity;
  AuthorityTransferContext previous,next;
  AuthorityCutSeal cut;
  std::uint64_t revision{},nonce{},cookie{};
  std::array<unsigned,4> slots{};unsigned count{};
  bool consumed{};
  MultiplayerSnapshotProof proof;
  std::optional<MultiplayerState> state;
  // Swap the predecessor's owning storage here. Its destruction belongs to
  // the caller after session publication, never the locked native callback.
  std::map<std::uint32_t,LanClient::Impl::Assembly> retired_assemblies;
  std::vector<MultiplayerState> retired_states;
  std::vector<MultiplayerSnapshotOrigin> retired_origins;
  MultiplayerSnapshotOrigin retired_origin;
  std::string retired_error;
};
LanClient::PreparedRebind::PreparedRebind():impl_(std::make_unique<Impl>()){}
LanClient::PreparedRebind::~PreparedRebind()=default;
std::unique_ptr<LanClient::PreparedRebind> LanClient::prepare_rebind_host(
    const AuthorityTransferContext &previous,const AuthorityTransferContext &next,const AuthorityCutSeal &cut)const{
  auto &v=*impl_;host_loss_contexts(previous,next);
  if(!v.udp.relay||!v.connected||!v.continuity||*v.continuity!=previous||v.physical_self==next.host||
      !(next.peers&(1u<<v.physical_self))||v.ids.empty()||v.ids.size()>4||v.rebind_revision==UINT64_MAX||!cut.id||!cut.bytes||
      std::ranges::all_of(cut.digest,[](auto b){return b==std::byte{};}))
    throw FormatError("Client host rebind has a stale context, unadmitted role or missing accepted cut");
  auto prepared=std::unique_ptr<PreparedRebind>(new PreparedRebind);auto &p=*prepared->impl_;
  p.owner=&v;p.owner_identity=v.snapshot_owner;p.previous=previous;p.next=next;p.cut=cut;
  p.revision=v.rebind_revision;p.nonce=v.nonce;p.cookie=v.cookie;
  p.count=v.ids.size();std::copy(v.ids.begin(),v.ids.end(),p.slots.begin());return prepared;
}
std::unique_ptr<LanClient::PreparedRebind> LanClient::prepare_rebind_host(
    const AuthorityTransferContext &previous,const AuthorityTransferContext &next,const AuthorityCutSeal &cut,
    const MultiplayerSnapshotProof &proof)const{
  auto prepared=prepare_rebind_host(previous,next,cut);const auto &v=*impl_;
  if(!proof.data_)throw FormatError("Client replica rebind requires its exact authenticated snapshot proof");
  const auto &origin=proof.data_->origin;
  if(proof.data_->cut.context!=previous||proof.data_->cut.seal!=cut||!origin.data_||!origin.data_->decoded||
      origin.data_->owner!=v.snapshot_owner||origin.data_->context!=v.continuity||
      origin.data_->self!=v.physical_self||origin.data_->revision!=v.rebind_revision||
      origin.data_->stamp.fingerprint!=v.fingerprint||origin.data_->stamp.nonce!=v.nonce||
      origin.data_->stamp.cookie!=v.cookie||origin.data_->stamp.tick!=cut.tick||
      origin.data_->decoded->state.tick!=cut.tick||origin.data_->decoded->slots!=v.ids)
    throw FormatError("Client replica rebind proof has a different connection, decoded receipt or accepted native cut");
  prepared->impl_->proof=proof;prepared->impl_->state=origin.data_->decoded->state;
  return prepared;
}
bool LanClient::can_commit_rebind_host(const PreparedRebind &prepared)const noexcept{
  const auto &v=*impl_;const auto &p=*prepared.impl_;
  return (!v.candidate||v.candidate->retired)&&!p.consumed&&p.owner==&v&&p.owner_identity==v.snapshot_owner&&v.udp.relay&&v.connected&&v.continuity&&*v.continuity==p.previous&&
      v.rebind_revision==p.revision&&v.rebind_revision<UINT64_MAX&&v.nonce==p.nonce&&v.cookie==p.cookie&&
      v.ids.size()==p.count&&std::equal(v.ids.begin(),v.ids.end(),p.slots.begin());
}
bool LanClient::commit_rebind_host(std::unique_ptr<PreparedRebind> &&prepared)noexcept{
  static_assert(std::is_nothrow_swappable_v<MultiplayerState>);
  if(!prepared||!can_commit_rebind_host(*prepared))return false;
  auto &v=*impl_;auto &p=*prepared->impl_;
  reject_snapshot_candidate(v.candidate);v.candidate_live.reset();
  if(p.state)std::swap(v.state,*p.state);
  v.host.sin_addr.s_addr=htonl(p.next.host+1);v.continuity=p.next;
  v.assemblies.swap(p.retired_assemblies);v.received_states.swap(p.retired_states);
  v.received_origins.swap(p.retired_origins);std::swap(v.state_origin,p.retired_origin);
  v.rebound_tick=std::uint32_t(p.cut.tick);v.last=Clock::now();v.started.reset();v.attempts=0;v.error.swap(p.retired_error);
  ++v.rebind_revision;p.consumed=true;return true;
}
void LanClient::rebind_host(const AuthorityTransferContext &previous,const AuthorityTransferContext &next,const AuthorityCutSeal &cut){
  auto prepared=prepare_rebind_host(previous,next,cut);
  if(!commit_rebind_host(std::move(prepared)))throw FormatError("Prepared native client rebind became stale");
}
} // namespace halo2
