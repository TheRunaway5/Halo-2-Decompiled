#include "halo2/unit_grenade.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool value,const std::string &message) {if(!value)throw std::runtime_error(message);}
std::uint32_t hex(const std::string &s) {return std::stoul(s,nullptr,16);}
std::vector<std::byte> bytes(const std::string &s) {
  std::vector<std::byte> result;
  for(std::size_t n=0;n<s.size();n+=2)result.push_back(std::byte(hex(s.substr(n,2))));
  return result;
}
halo2::SourceGrenadeAction action(const std::string &s) {
  const auto raw=bytes(s);check(raw.size()==0x44,"Action fixture size");
  halo2::SourceGrenadeAction result;std::memcpy(result.data(),raw.data(),raw.size());return result;
}
std::array<std::uint8_t,2> counts(const std::string &s) {
  const auto comma=s.find(',');return {std::uint8_t(std::stoi(s.substr(0,comma))),std::uint8_t(std::stoi(s.substr(comma+1)))};
}
std::array<float,3> vector(const std::string &s) {
  const auto raw=bytes(s);std::array<float,3> result;check(raw.size()==12,"Vector fixture size");std::memcpy(result.data(),raw.data(),12);return result;
}
}
int main(int argc,char **argv) {
 try {
  check(argc==2,"Supply original grenade callback fixture export");std::ifstream input(argv[1]);if(!input)return 77;
  unsigned initial=0,begin=0,states=0,descriptors=0;
  for(std::string line;std::getline(input,line);) {
   std::istringstream in(line);std::vector<std::string> r;for(std::string s;in>>s;)r.push_back(s);if(r.empty())continue;
   const std::string name="Original grenade differs: "+r.at(1);
   if(r[0]=="I") {
    halo2::SourceGrenadeAction actual;halo2::initialize_source_grenade_action(actual);check(actual==action(r[2]),name);++initial;
   }else if(r[0]=="B") {
    auto actual=action(r[2]);halo2::source_grenade_begin_channel_prefix(actual,std::stoi(r[4]),std::stoi(r[5]));check(actual==action(r[3]),name);++begin;
   }else if(r[0]=="S") {
    auto actual=action(r[3]);auto inventory=counts(r[6]);std::optional<bool> returned;
    halo2::GrenadeCallbackStep step{halo2::GrenadeCallback::complete,std::nullopt,0};
    if(r[2]=="000e99b0")step=halo2::source_grenade_update(actual);
    if(r[9]=="P") {
     check(r[2]=="000e8d60" || step.callback==halo2::GrenadeCallback::prepare_projectile,name+" prepare request");
     const auto prep=halo2::source_grenade_prepare_prefix(actual,{hex(r[8]),std::int8_t(std::stoi(r[5])),inventory,true});
     const bool source_stopped=r[11]=="None";
     check((prep.stop==halo2::GrenadePreparationStop::marker_request)==source_stopped,name+" constructor boundary");
     if(source_stopped)check(prep.marker_identifier==0x09000100,name+" original marker request");
     if(prep.count_debited)check(prep.changed_flags==0x400000,name+" inventory notification before marker");
    }else if(r[9]=="R") {
     check(step.callback==halo2::GrenadeCallback::release_projectile,name+" release request");
     check(halo2::source_grenade_release_empty(actual),name+" original held NONE path");returned=true;
    }else if(r[9]=="A") {
     check(step.callback==halo2::GrenadeCallback::animation_query,name+" original animation query");
     std::istringstream replies(r[10]);for(std::string reply;std::getline(replies,reply,',');) {
      const auto colon=reply.find(':');const auto id=hex(reply.substr(0,colon));check(step.animation_identifier==id,name+" ordered animation identifiers");
      step=halo2::source_grenade_animation_reply(actual,id,bool(std::stoi(reply.substr(colon+1))));
     }
     returned=step.returned;
    }else {check(step.callback==halo2::GrenadeCallback::complete,name+" completed update");returned=step.returned;}
    check(actual==action(r[4]) && inventory==counts(r[7]),name+" all44 action bytes and counts");
    // E8D60 is void; its incidental AL is not a return contract.
    if(r[2]=="000e99b0" && r[11]!="None")check(returned && unsigned(*returned)==std::stoul(r[11]),name+" defined AL");
    ++states;
   }else if(r[0]=="D") {
    auto raw=bytes(r[2]);const auto expected=bytes(r[3]);check(raw.size()==0xc4,"Descriptor fixture size");
    halo2::patch_source_grenade_creation_request(std::span<std::byte,0xc4>(raw.data(),0xc4),vector(r[4]),vector(r[5]),
      {std::bit_cast<float>(hex(r[6])),std::bit_cast<float>(hex(r[7]))});
    check(raw==expected,name+" entire source constructor request including retained external bytes");++descriptors;
   }else throw std::runtime_error("Unknown source fixture row");
  }
  check(initial==1 && begin==1 && states==15 && descriptors==3,"Complete measured grenade fixture coverage");
  std::cout<<states<<" original callback states, "<<descriptors<<" complete constructor requests, constructor and partial begin matched; unavailable action/world owners remain explicit\n";
 }catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
