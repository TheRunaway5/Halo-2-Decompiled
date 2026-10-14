#include "halo2/online_peer_transport.hpp"
#include <rtc/rtc.hpp>
#include <algorithm>
#include <array>
#include <deque>
#include <mutex>
#include <stdexcept>

namespace halo2 {
namespace {
void require_peer(bool ok) { if(!ok)throw std::runtime_error("Unable to establish a direct player connection."); }
constexpr std::size_t maximum_packet=1404;
constexpr const char *channel_name="halo2-native-datagrams-v1";
struct Packet {unsigned peer;std::vector<std::byte> bytes;};
struct State {
  std::mutex mutex;
  std::deque<OnlinePeerSignal> signals;
  std::deque<Packet> packets;
  std::array<bool,16> open{},failed{};
};
}
struct OnlinePeerTransport::Impl {
  struct Link {
    std::shared_ptr<rtc::PeerConnection> connection;
    std::shared_ptr<rtc::DataChannel> channel;
    std::vector<rtc::Candidate> pending;
    bool description{};
  };
  unsigned self;
  std::shared_ptr<State> state=std::make_shared<State>();
  std::array<Link,16> links;
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
          if(s->open[peer] && s->packets.size()<512)s->packets.push_back({peer,{bytes->begin(),bytes->end()}});
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
void OnlinePeerTransport::signal(const OnlinePeerSignal &signal) {
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
  std::lock_guard lock(impl_->state->mutex);auto &queue=impl_->state->signals;
  std::vector<OnlinePeerSignal> result(queue.begin(),queue.end());queue.clear();return result;
}
bool OnlinePeerTransport::ready(unsigned peer) const {
  std::lock_guard lock(impl_->state->mutex);return peer<16 && impl_->state->open[peer];
}
bool OnlinePeerTransport::failed(unsigned peer) const {
  std::lock_guard lock(impl_->state->mutex);return peer>=16 || impl_->state->failed[peer];
}
unsigned OnlinePeerTransport::round_trip_ms(unsigned peer) const {
  require_peer(peer<16 && bool(impl_->links[peer].connection));
  const auto value=impl_->links[peer].connection->rtt();return value?unsigned(std::clamp<std::int64_t>(value->count(),0,60000)):60000;
}
bool OnlinePeerTransport::send(std::span<const std::byte> packet,unsigned peer) {
  if(peer>=16 || packet.empty() || packet.size()>maximum_packet || !ready(peer))return false;
  auto dc=impl_->links[peer].channel;
  // Late audio/snapshots are discarded instead of growing a TCP-style queue.
  if(!dc || dc->bufferedAmount()>65536)return false;
  dc->send(packet.data(),packet.size());return true;
}
bool OnlinePeerTransport::receive(std::vector<std::byte> &packet,unsigned &peer) {
  std::lock_guard lock(impl_->state->mutex);auto &queue=impl_->state->packets;
  if(queue.empty())return false;
  auto next=std::move(queue.front());queue.pop_front();peer=next.peer;packet=std::move(next.bytes);return true;
}
} // namespace halo2
