#include "halo2/online_peer_transport.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <thread>
using namespace halo2;
using namespace std::chrono_literals;
int main(){
  // Live relay qualification is explicit: ordinary offline CTest must not
  // create users or depend on external credentials/network availability.
  if(!std::getenv("H2_EOS_LIVE_TEST"))return 77;
  try{
    std::cerr<<"EOS_TEST starting\n";
    const auto *socket=std::getenv("H2_EOS_TEST_SOCKET");
    const auto *key=std::getenv("H2_EOS_TEST_KEY");
    const auto *signals=std::getenv("H2_EOS_TEST_SIGNALS");
    const auto *own=std::getenv("H2_EOS_TEST_SLOT");
    if(!socket||!key||!signals||!own)throw std::runtime_error("Missing EOS test admission");
    const unsigned self=std::stoul(own),peer=self==0?4:0;
    const bool force=!std::getenv("H2_EOS_TEST_DIRECT");
    OnlineEosOptions options{socket,{{peer,key}},force};
    if(std::getenv("H2_EOS_TEST_SESSIONS")){
      options.sessions=true;options.playlist="halo2-validation";options.revision="eos-v1";options.map="lockout";options.variant="Slayer";
    }
    OnlinePeerTransport transport(self,options);
    std::cerr<<"EOS_TEST constructed\n";
    const auto started=std::chrono::steady_clock::now(),deadline=started+60s;
    bool signaled=false,received=false,sent=false;
    std::vector<std::byte> payload(1404);
    for(unsigned i=0;i<payload.size();++i)payload[i]=std::byte(i%251);
    while(std::chrono::steady_clock::now()<deadline){
      for(auto &signal:transport.signals()){
        std::ofstream file(std::filesystem::path(signals)/std::to_string(self));file<<signal.payload<<'\n';file.close();
        signaled=true;
        std::cerr<<"EOS_TEST local identity ready\n";
      }
      // Force the WAN ordering: the initiator's SDK request can arrive before
      // this endpoint receives its authenticated roster identity.
      if(signaled&&(self!=4||std::chrono::steady_clock::now()-started>=5s)){std::ifstream file(std::filesystem::path(signals)/std::to_string(peer));std::string user;
        if(std::getline(file,user)&&user.size()==32)transport.signal({peer,"eos",user,{}});}
      if(transport.ready(peer)){
        if(force&&!transport.relayed(peer))throw std::runtime_error("EOS force-relay did not establish a relay route");
        sent=transport.send(payload,peer)||sent;
      }
      std::vector<std::byte> packet;unsigned source{};
      while(transport.receive(packet,source))if(source==peer&&packet==payload)received=true;
      if(received&&sent){std::cout<<"EOS_RELAY_PASS bytes=1404"<<std::endl;std::this_thread::sleep_for(500ms);return 0;}
      std::this_thread::sleep_for(2ms);
    }
    throw std::runtime_error("EOS relay qualification timed out");
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
