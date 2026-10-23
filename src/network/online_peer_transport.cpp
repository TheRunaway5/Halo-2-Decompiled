#include "halo2/online_peer_transport.hpp"
#include "halo2/peer_packet_queue.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <rtc/rtc.hpp>
#include <algorithm>
#include <array>
#include <deque>
#include <cstdio>
#include <mutex>
#include <optional>
#include <stdexcept>

namespace halo2 {
namespace {
void require_peer(bool ok) { if(!ok)throw std::runtime_error("Unable to establish a direct player connection."); }
constexpr std::size_t maximum_packet=1404;
constexpr const char *channel_name="halo2-native-datagrams-v1";
struct State {
  std::mutex mutex;
  std::deque<OnlinePeerSignal> signals;
  PeerPacketQueue packets;
  std::array<bool,16> open{},failed{};
};
struct RuntimeOwners {std::mutex mutex;std::size_t count{};};
RuntimeOwners &runtime_owners(){static RuntimeOwners owners;return owners;}
// The pinned RTC initializer retains a global token and detached cleanup
// future. Drain it while its crypto providers are still alive, after the last
// wrapper's channels/connections have released their own tokens. Serialize a
// new room against final cleanup; closing one of several rooms never unloads
// the runtime used by the others. RTC callbacks retain only weak State values
// and never construct or destroy this lease.
class RuntimeLease {
public:
  RuntimeLease(){auto &owners=runtime_owners();std::lock_guard lock(owners.mutex);++owners.count;}
  ~RuntimeLease()noexcept{
    auto &owners=runtime_owners();std::lock_guard lock(owners.mutex);
    if(--owners.count)return;
    try{rtc::Cleanup().get();}
    catch(...){std::fputs("Direct peer runtime cleanup failed.\n",stderr);}
  }
  RuntimeLease(const RuntimeLease &)=delete;
  RuntimeLease &operator=(const RuntimeLease &)=delete;
};
}
struct OnlinePeerTransport::Impl {
  RuntimeLease runtime; // Destroyed last, including failed-constructor unwinding.
  struct Link {
    std::shared_ptr<rtc::PeerConnection> connection;
    std::shared_ptr<rtc::DataChannel> channel;
    std::vector<rtc::Candidate> pending;
    bool description{};
  };
  unsigned self;
  bool account_login{};
  std::shared_ptr<State> state=std::make_shared<State>();
  std::array<Link,16> links;
  std::mutex eos_mutex;
  std::optional<rust::Box<online_protocol_bridge::EosTransport>> eos;
  Impl(unsigned slot,const OnlineEosOptions &options):self(slot),account_login(options.account_login) {
    require_peer(slot<16);
    rust::Vec<online_protocol_bridge::EosPeer> peers;
    for(const auto &peer:options.peers){require_peer(peer.slot<16);peers.push_back({std::uint8_t(peer.slot),peer.key});}
    online_protocol_bridge::EosSession session{options.sessions,options.playlist,options.revision,options.map,options.variant};
    if(options.account_login)
      eos.emplace(online_protocol_bridge::new_eos_account_transport(std::uint8_t(slot),std::move(peers),options.socket,options.force_relay,std::move(session)));
    else
      eos.emplace(online_protocol_bridge::new_eos_transport(std::uint8_t(slot),std::move(peers),options.socket,options.force_relay,std::move(session)));
  }
  Impl(unsigned slot,std::vector<unsigned> peers,const std::vector<std::string> &servers):self(slot) {
    require_peer(slot<16 && !peers.empty() && peers.size()<=16 && std::count(peers.begin(),peers.end(),slot)==1);
    std::sort(peers.begin(),peers.end());
    require_peer(peers.back()<16 && std::adjacent_find(peers.begin(),peers.end())==peers.end() && servers.size()<=4);
    rtc::Configuration config;
    config.enableIceTcp=false;
    config.disableAutoNegotiation=true;
    config.maxMessageSize=maximum_packet;
    config.mtu=1200;
    for(const auto &url:servers) {
      require_peer(url.starts_with("stun:") && url.size()<=256 && url.find_first_of("@?#\\\r\n\t ")==std::string::npos);
      rtc::IceServer server(url);require_peer(server.type==rtc::IceServer::Type::Stun);
      config.iceServers.push_back(std::move(server));
    }
    const std::weak_ptr<State> weak=state;
    for(auto peer:peers)if(peer!=self) {
      auto &link=links[peer];
      link.connection=std::make_shared<rtc::PeerConnection>(config);
      auto pc=link.connection;
      pc->onLocalDescription([weak,peer](rtc::Description description){
        if(auto s=weak.lock()) {std::lock_guard lock(s->mutex);
          if(s->signals.size()<1024)s->signals.push_back({peer,description.typeString(),std::string(description),{}});
          else s->failed[peer]=true;
        }
      });
      pc->onLocalCandidate([weak,peer](rtc::Candidate candidate){
        if(candidate.transportType()!=rtc::Candidate::TransportType::Udp || candidate.type()==rtc::Candidate::Type::Relayed)return;
        if(auto s=weak.lock()) {std::lock_guard lock(s->mutex);
          if(s->signals.size()<1024)s->signals.push_back({peer,"candidate",candidate.candidate(),candidate.mid()});
          else s->failed[peer]=true;
        }
      });
      pc->onStateChange([weak,peer](rtc::PeerConnection::State value){
        if(auto s=weak.lock()) {std::lock_guard lock(s->mutex);
          if(value==rtc::PeerConnection::State::Failed || value==rtc::PeerConnection::State::Closed) {
            s->open[peer]=false;s->failed[peer]=true;
          }
        }
      });
      // Both authenticated endpoints create the same channel, avoiding a
      // remote-created stream with different reliability/ordering settings.
      rtc::DataChannelInit init;init.negotiated=true;init.id=0;
      init.reliability.unordered=true;init.reliability.maxRetransmits=0;
      link.channel=pc->createDataChannel(channel_name,init);
      auto dc=link.channel;const std::weak_ptr<rtc::PeerConnection> weak_pc=pc;
      dc->onOpen([weak,weak_pc,peer]{
        auto pc=weak_pc.lock();rtc::Candidate local,remote;
        const bool direct=pc && pc->getSelectedCandidatePair(&local,&remote) &&
          local.transportType()==rtc::Candidate::TransportType::Udp && remote.transportType()==rtc::Candidate::TransportType::Udp &&
          local.type()!=rtc::Candidate::Type::Relayed && remote.type()!=rtc::Candidate::Type::Relayed;
        if(auto s=weak.lock()) {std::lock_guard lock(s->mutex);s->open[peer]=direct;s->failed[peer]=!direct;}
      });
      dc->onClosed([weak,peer]{if(auto s=weak.lock()){std::lock_guard lock(s->mutex);s->open[peer]=false;s->failed[peer]=true;}});
      dc->onError([weak,peer](std::string){if(auto s=weak.lock()){std::lock_guard lock(s->mutex);s->open[peer]=false;s->failed[peer]=true;}});
      dc->onMessage([weak,peer](rtc::message_variant message){
        auto bytes=std::get_if<rtc::binary>(&message);
        if(!bytes || bytes->empty() || bytes->size()>maximum_packet)return;
        if(auto s=weak.lock()) {std::lock_guard lock(s->mutex);
          if(s->open[peer])s->packets.push(peer,*bytes);
        }
      });
      if(self<peer)pc->setLocalDescription(rtc::Description::Type::Offer);
    }
  }
  ~Impl() {
    // Callbacks own only weak references. Retiring them before closing prevents
    // an async ICE/DTLS teardown from borrowing this room or a destroyed link.
    for(auto &link:links) {
      if(link.channel){link.channel->resetCallbacks();link.channel->close();}
      if(link.connection){link.connection->resetCallbacks();link.connection->close();}
    }
  }
};
OnlinePeerTransport::OnlinePeerTransport(unsigned self,std::vector<unsigned> peers,const std::vector<std::string> &servers)
  :impl_(std::make_unique<Impl>(self,std::move(peers),servers)) {}
OnlinePeerTransport::~OnlinePeerTransport()=default;
OnlinePeerTransport::OnlinePeerTransport(unsigned self,const OnlineEosOptions &options)
  :impl_(std::make_unique<Impl>(self,options)){}
void OnlinePeerTransport::signal(const OnlinePeerSignal &signal) {
  if(impl_->eos){
    std::lock_guard lock(impl_->eos_mutex);
    require_peer(signal.peer<16&&signal.type=="eos"&&signal.mid.empty());
    online_protocol_bridge::eos_signal(**impl_->eos,std::uint8_t(signal.peer),signal.payload);return;
  }
  require_peer(signal.peer<16 && signal.peer!=impl_->self);
  auto &link=impl_->links[signal.peer];require_peer(bool(link.connection));
  require_peer(signal.payload.size()<=16384 && !signal.payload.empty());
  if(signal.type=="candidate") {
    require_peer(signal.payload.size()<=1024 && signal.mid.size()<=32);
    rtc::Candidate candidate(signal.payload,signal.mid);
    require_peer(candidate.transportType()==rtc::Candidate::TransportType::Udp && candidate.type()!=rtc::Candidate::Type::Relayed);
    if(link.description)link.connection->addRemoteCandidate(std::move(candidate));
    else {require_peer(link.pending.size()<64);link.pending.push_back(std::move(candidate));}
  } else {
    require_peer(!link.description && signal.type==(impl_->self<signal.peer?"answer":"offer") && signal.mid.empty());
    // Fingerprints remain verified by DTLS; the coordinator authenticates which
    // physical roster endpoint supplied this description.
    link.connection->setRemoteDescription(rtc::Description(signal.payload,signal.type));
    link.description=true;
    for(auto &candidate:link.pending)link.connection->addRemoteCandidate(std::move(candidate));
    link.pending.clear();
    if(signal.type=="offer")link.connection->setLocalDescription(rtc::Description::Type::Answer);
  }
}
std::vector<OnlinePeerSignal> OnlinePeerTransport::signals() {
  if(impl_->eos){
    std::lock_guard lock(impl_->eos_mutex);online_protocol_bridge::eos_tick(**impl_->eos);
    std::vector<OnlinePeerSignal> result;
    for(auto &signal:online_protocol_bridge::eos_signals(**impl_->eos))result.push_back({signal.peer,"eos",std::string(signal.user),{}});
    return result;
  }
  std::lock_guard lock(impl_->state->mutex);auto &queue=impl_->state->signals;
  std::vector<OnlinePeerSignal> result(queue.begin(),queue.end());queue.clear();return result;
}
OnlineEosAuthRequest OnlinePeerTransport::authentication_request() {
  if(!impl_->eos)return {};
  std::lock_guard lock(impl_->eos_mutex);
  const auto request=online_protocol_bridge::eos_auth_request(**impl_->eos);
  return {request.kind,request.ticket,std::string(request.token),std::string(request.user)};
}
bool OnlinePeerTransport::complete_authentication(const OnlineEosAuthResponse &value) {
  if(!impl_->eos)return false;
  online_protocol_bridge::EosAuthResponse response{};
  response.kind=value.kind;response.ticket=value.ticket;response.ok=value.ok;
  response.token=value.token;response.user=value.user;
  response.expires_in=value.expires_in;response.status=value.status;
  std::lock_guard lock(impl_->eos_mutex);
  return online_protocol_bridge::eos_auth_complete(**impl_->eos,response);
}
bool OnlinePeerTransport::authentication_ready() const {
  if(!impl_->account_login)return true;
  std::lock_guard lock(impl_->eos_mutex);
  return impl_->eos&&online_protocol_bridge::eos_auth_bound(**impl_->eos);
}
bool OnlinePeerTransport::ready(unsigned peer) const {
  if(impl_->eos){std::lock_guard lock(impl_->eos_mutex);return peer<16&&online_protocol_bridge::eos_ready(**impl_->eos,std::uint8_t(peer));}
  std::lock_guard lock(impl_->state->mutex);return peer<16 && impl_->state->open[peer];
}
bool OnlinePeerTransport::failed(unsigned peer) const {
  if(impl_->eos){std::lock_guard lock(impl_->eos_mutex);return peer>=16||online_protocol_bridge::eos_failed(**impl_->eos,std::uint8_t(peer));}
  std::lock_guard lock(impl_->state->mutex);return peer>=16 || impl_->state->failed[peer];
}
unsigned OnlinePeerTransport::round_trip_ms(unsigned peer) const {
  if(impl_->eos)return 60000; // Session QoS probes measure the actual EOS route.
  require_peer(peer<16 && bool(impl_->links[peer].connection));
  const auto value=impl_->links[peer].connection->rtt();return value?unsigned(std::clamp<std::int64_t>(value->count(),0,60000)):60000;
}
bool OnlinePeerTransport::send(std::span<const std::byte> packet,unsigned peer) {
  if(impl_->eos){std::lock_guard lock(impl_->eos_mutex);return peer<16&&online_protocol_bridge::eos_send(**impl_->eos,std::uint8_t(peer),
      {reinterpret_cast<const std::uint8_t*>(packet.data()),packet.size()});}
  if(peer>=16 || packet.empty() || packet.size()>maximum_packet || !ready(peer))return false;
  auto dc=impl_->links[peer].channel;
  // Late audio/snapshots are discarded instead of growing a TCP-style queue.
  if(!dc || dc->bufferedAmount()>65536)return false;
  // Closing the other endpoint can race the ready check. A lost datagram is
  // handled by its native protocol; it must not tear down healthy room peers.
  try {dc->send(packet.data(),packet.size());return true;}
  catch(const std::exception &){return false;}
}
bool OnlinePeerTransport::receive(std::vector<std::byte> &packet,unsigned &peer) {
  if(impl_->eos){std::lock_guard lock(impl_->eos_mutex);auto result=online_protocol_bridge::eos_receive(**impl_->eos);
    if(!result.present)return false;
    peer=result.peer;packet.resize(result.data.size());
    std::transform(result.data.begin(),result.data.end(),packet.begin(),[](auto byte){return std::byte(byte);});return true;}
  std::lock_guard lock(impl_->state->mutex);auto &queue=impl_->state->packets;
  return queue.receive(packet,peer);
}
bool OnlinePeerTransport::relayed(unsigned peer)const{
  if(!impl_->eos)return false;
  std::lock_guard lock(impl_->eos_mutex);return peer<16&&online_protocol_bridge::eos_relayed(**impl_->eos,std::uint8_t(peer));
}
} // namespace halo2
