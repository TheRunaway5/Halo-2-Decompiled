#include "halo2/lan_admission.hpp"
#include "halo2/lan_packet_auth.hpp"
#include "halo2/voice_origin.hpp"
#include "halo2/online_packet_role.hpp"
#include "halo2/peer_packet_queue.hpp"
#include <iostream>
#include <stdexcept>
using namespace halo2;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
VoiceOrigin::Key public_key(const VoiceOrigin::Key &seed){
  auto *key=EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519,nullptr,seed.data(),seed.size());
  check(key,"Create fixture key");VoiceOrigin::Key result;std::size_t size=32;
  check(EVP_PKEY_get_raw_public_key(key,result.data(),&size)==1&&size==32,"Read fixture public key");
  EVP_PKEY_free(key);return result;
}
int main(){try{
  check(online_packet_role(false,false,6),"Direct voice between nonhosts was rejected");
  check(!online_packet_role(false,false,3)&&!online_packet_role(false,false,1),"Direct voice relaxed simulation roles");
  check(online_packet_role(true,false,18)&&online_packet_role(false,true,18),
      "Native shadow control or payload lost its authenticated host/client direction");
  check(!online_packet_role(false,false,18),
      "A nonhost direct peer gained native shadow admission authority");
  check(online_packet_role(true,false,19) && online_packet_role(false,true,20),"Team switching lost its authenticated direction");
  check(!online_packet_role(false,false,19) && !online_packet_role(false,false,20) &&
      !online_packet_role(false,true,19) && !online_packet_role(true,false,20),"A team request or result can spoof the authority");
  LanPacketAuth host_auth,client_auth;LanPacketAuth::Session host_session,client_session;
  LanPacketAuth::Context context{};context[0]=107;
  host_auth.bind(host_session,client_auth.public_key(),true,context);
  client_auth.bind(client_session,host_auth.public_key(),false,context);
  auto original=host_auth.seal(std::vector<std::byte>{std::byte{3}},host_session,true);
  auto changed=original;changed.back()^=std::byte{1};
  check(!client_auth.open(changed,client_session,true),"Invalid MAC pinned a key");
  LanPacketAuth attacker;LanPacketAuth::Session attacker_session,unbound;
  attacker.bind(attacker_session,client_auth.public_key(),true,context);
  auto forged=attacker.seal(std::vector<std::byte>{std::byte{3}},attacker_session,true);
  auto first_forgery=forged;
  check(!client_auth.open(first_forgery,unbound,true)&&!unbound.peer,"Unbound client accepted an attacker's self-declared host key");
  check(!client_auth.open(forged,client_session,true)&&client_session.peer==host_auth.public_key(),"Attacker's first frame replaced independently selected host key");
  auto packet=original;check(client_auth.open(packet,client_session,true),"Rust/C++ authenticated snapshot rejected");
  packet=original;check(!client_auth.open(packet,client_session,true),"Rust/C++ replay accepted");
  PeerPacketQueue queue;std::vector<std::byte> payload(1404),received;unsigned source{};
  for(unsigned i=0;i<512;++i)check(queue.push(0,payload),"Peer lost its reserved queue capacity");
  for(unsigned i=0;i<10000;++i)check(!queue.push(0,payload),"Flood bypassed per-peer capacity");
  check(queue.push(15,payload),"Flood consumed a healthy peer's capacity");
  check(queue.receive(received,source)&&source==0,"First peer missing");
  check(queue.push(0,payload),"Flooding peer could not refill its own slot");
  check(queue.receive(received,source)&&source==15,"Busy peer starved another peer");
  LanAdmission gate;LanAdmission::Endpoint a{127,0,0,1,10,20},b=a;
  std::vector<std::byte> request(38,std::byte{1});
  const auto cookie=gate.issue(a,request,30000);
  check(gate.verify(a,request,cookie,59999),"Current LAN cookie rejected");
  check(gate.verify(a,request,cookie,60000),"Previous time-window LAN cookie rejected");
  check(!gate.verify(a,request,cookie,90000),"Expired LAN cookie accepted");
  b[5]^=1;check(!gate.verify(b,request,cookie,30000),"Cookie moved to another UDP port");
  b=a;b[3]^=1;check(!gate.verify(b,request,cookie,30000),"Cookie moved to another IP");
  request.back()^=std::byte{1};check(!gate.verify(a,request,cookie,30000),"Cookie authorized another request");
  unsigned replies{};while(gate.reserve(a,31,true,1000))++replies;
  check(replies&&replies*31<=8192,"Unverified challenge byte budget bypassed");
  check(gate.reserve(a,31,true,2000),"Challenge rate never recovered");
  unsigned descriptions{};while(gate.reserve(a,34000,false,2000))++descriptions;
  check(descriptions&&descriptions*34000<=256*1024,"Verified query response byte budget bypassed");
  std::size_t flood_bytes{};
  for(unsigned i=0;i<10000;++i){auto endpoint=a;endpoint[0]=i;endpoint[1]=i>>8;
    if(gate.reserve(endpoint,31,true,3000))flood_bytes+=31;}
  check(flood_bytes<=128*1024,"Changing spoofed IPs bypassed global budget");
  check(gate.reserve(a,31,true,10000),"Legitimate endpoint could not recover after a spoof flood");
  VoiceOrigin::Key one{},two{};one[0]=1;two[0]=2;
  const std::vector<VoiceOrigin::Key> keys{public_key(one),public_key(two)};
  VoiceOrigin sender("00000000-0000-0000-0000-000000000001"),receiver("00000000-0000-0000-0000-000000000001"),other_room("00000000-0000-0000-0000-000000000002");
  check(sender.configure(one,keys,0)&&receiver.configure(two,keys,1)&&other_room.configure(two,keys,1),"Configure room keys");
  std::vector<std::byte> body(23);body[0]=std::byte{'H'};body[8]=std::byte{0};body[9]=std::byte{1};body[22]=std::byte{7};
  const auto addressed=sender.sign(body,3,1);
  check(addressed && receiver.verify(body,*addressed,0,3,1) && !receiver.verify(body,*addressed,0,3,0),"Voice recipient binding failed");
  const auto signature=sender.sign(body,3,1);check(signature.has_value(),"Sign voice");
  // Fixed seed/body signed by the coordinator's Python voice_message/Ed25519
  // implementation. This locks the C++/Python domain and byte layout together.
  constexpr char expected[]="bb9d508873314e0f6cabda744d00c703ca04b0de0ac25efe49f18908618a388509927686aa21da57f23c69489670c439a5a0b816338376320cd29100484dd600";
  constexpr char digits[]="0123456789abcdef";std::string actual;
  for(auto b:*signature){const auto n=std::to_integer<unsigned>(b);actual+=digits[n>>4];actual+=digits[n&15];}
  check(actual==expected,"Native and coordinator voice proof formats differ");
  check(receiver.verify(body,*signature,0,3,1),"Valid signed relay rejected");
  check(!receiver.verify(body,*signature,1,3,1),"Host signed as a different microphone owner");
  check(!receiver.verify(body,*signature,0,4,1),"Voice replayed across authority generations");
  check(!other_room.verify(body,*signature,0,3,1),"Voice replayed across rooms");
  body[8]=std::byte{1};check(!receiver.verify(body,*signature,0,3,1),"Relabeled speaker accepted");body[8]=std::byte{0};
  body[22]^=std::byte{1};check(!receiver.verify(body,*signature,0,3,1),"Forged codec bytes accepted");body[22]^=std::byte{1};
  body[14]=std::byte{1};check(!receiver.verify(body,*signature,0,3,1),"Modified voice gains accepted");
  check(!receiver.configure(one,keys,1),"Reconnect replaced microphone identity");
  std::cout<<"LAN endpoint/request/expiry cookies and bounded replies; room/epoch/speaker/codec voice authentication passed\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
