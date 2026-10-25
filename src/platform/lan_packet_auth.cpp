#include "halo2/lan_packet_auth.hpp"
#include "halo2/cache.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
namespace halo2 {
namespace {
auto slice(std::span<const std::byte> p){return rust::Slice<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p.data()),p.size());}
std::vector<std::byte> copy(const rust::Vec<std::uint8_t> &p){
  if(p.empty())return {};
  const auto *first=reinterpret_cast<const std::byte*>(p.data());return {first,first+p.size()};
}
}
struct LanPacketAuth::Impl {
  rust::Box<online_protocol_bridge::LanEndpoint> endpoint=online_protocol_bridge::new_lan_endpoint();
  Key public_key=online_protocol_bridge::lan_public(*endpoint);
};
struct LanPacketAuth::Session::Impl {rust::Box<online_protocol_bridge::LanSession> session=online_protocol_bridge::new_lan_session();};
LanPacketAuth::Session::Session():inner(std::make_unique<Impl>()){}
LanPacketAuth::Session::~Session()=default;
LanPacketAuth::Session::Session(Session &&)noexcept=default;
LanPacketAuth::Session &LanPacketAuth::Session::operator=(Session &&)noexcept=default;
LanPacketAuth::LanPacketAuth():inner_(std::make_unique<Impl>()){}
LanPacketAuth::~LanPacketAuth()=default;
const LanPacketAuth::Key &LanPacketAuth::public_key()const{return inner_->public_key;}
bool LanPacketAuth::packet(std::span<const std::byte> p){return online_protocol_bridge::lan_is_packet(slice(p));}
void LanPacketAuth::bind(Session &s,const Key &peer)const{
  try{online_protocol_bridge::lan_bind(*inner_->endpoint,*s.inner->session,peer);s.peer=peer;}
  catch(const rust::Error &){throw FormatError("Invalid LAN admission key");}
}
std::vector<std::byte> LanPacketAuth::seal(std::span<const std::byte> p,Session &s,bool host)const{
  try{return copy(online_protocol_bridge::lan_seal(*inner_->endpoint,*s.inner->session,host,slice(p)));}
  catch(const rust::Error &e){throw FormatError("Authenticated LAN sealing failed (body "+std::to_string(p.size())+
    ", admitted key "+std::to_string(s.peer.has_value())+"): "+e.what());}
}
bool LanPacketAuth::open(std::vector<std::byte> &p,Session &s,bool host)const{
  try{
    const auto result=online_protocol_bridge::lan_open(*inner_->endpoint,*s.inner->session,host,slice(p));
    // Only full LNA1 frames carry the sender key; compact LNA2 frames are
    // accepted solely on a session already pinned to that key.
    if(std::to_integer<char>(p[3])=='1'){
      Key peer;for(unsigned i=0;i<32;++i)peer[i]=std::to_integer<unsigned char>(p[13+i]);
      s.peer=peer;
    }
    p=copy(result);return true;
  }catch(const rust::Error &){return false;}
}
}
