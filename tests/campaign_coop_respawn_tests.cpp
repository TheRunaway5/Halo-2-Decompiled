#include "halo2/source_campaign_coop.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>

namespace {
void check(bool value,const std::string&message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string&s) {
  check(s.size()%2==0,"Complete original bytes");std::vector<std::byte> result;
  for(std::size_t i=0;i<s.size();i+=2)result.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
  return result;
}
struct Memory:halo2::SourcePhysicsContactMemory {
  std::map<std::uint32_t,std::byte> data;
  void read(std::uint32_t address,std::span<std::byte> out)const override {
    for(auto&b:out) {
      const auto i=data.find(address++);
      check(i!=data.end(),"Missing declared cooperative source input "+std::to_string(address-1));b=i->second;
    }
  }
  void write(std::uint32_t address,std::span<const std::byte> in)override {
    for(auto b:in)data.at(address++)=b;
  }
};
}
int main(int argc,char**argv)try {
  check(argc==2,"Supply original cooperative respawn fixture");std::ifstream input(argv[1]);if(!input)return 77;
  const auto fixture=nlohmann::json::parse(input);
  check(fixture.at("source_sha256")=="aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377",
        "Wrong original cooperative source image");
  const auto&cases=fixture.at("cases");check(cases.size()==45,"All original cooperative respawn cases");
  std::size_t stores=0,calls=0;
  for(const auto&c:cases) {
    const auto&spec=c.at("spec");const auto name=spec.at("name").get<std::string>();Memory m;
    for(const auto&row:c.at("initial")) {
      auto address=row.at("address").get<std::uint32_t>();
      for(auto b:bytes(row.at("bytes")))check(m.data.emplace(address++,b).second,"Unique original input");
    }
    const auto pointers=c.at("context").get<std::array<std::uint32_t,6>>();
    const halo2::SourceCampaignCoopContext context{pointers[0],pointers[1],pointers[2],pointers[3],pointers[4],pointers[5]};
    const auto fp=c.at("fp").get<std::array<std::uint32_t,2>>();
    std::size_t next_store=0,next_call=0;
    const auto observe=[&](const halo2::SourcePhysicsContactRecordStore&s) {
      check(next_store<c.at("writes").size(),name+" extra source write");
      const auto&expected=c.at("writes")[next_store++];
      check(s.instruction==expected.at("instruction") && s.address==expected.at("address") &&
            s.before==bytes(expected.at("before")) && s.written==bytes(expected.at("written")),
            name+" original ordered store "+std::to_string(next_store));
    };
    // Only these named safety boundaries are substituted in the native
    // scheduler comparison. The oracle executes their complete originals.
    const auto service=[&](std::uint32_t entry,std::optional<std::uint32_t> unit={}) {
      check(next_call<c.at("calls").size(),name+" extra safety owner");
      const auto&expected=c.at("calls")[next_call++];
      check(expected.at("entry")==entry,name+" safety-owner order");
      if(unit)check(expected.at("unit")==*unit,name+" actual biped identity");
      return expected.at("result").get<bool>();
    };
    halo2::SourceCampaignCoopRespawnServices services;
    services.action_unsafe=[&]{return service(0xcd5c0);};
    services.combat_unsafe=[&]{return service(0x1ce060);};
    services.biped_unsafe=[&](auto unit){return service(0xe5680,unit);};
    switch(spec.at("entry").get<std::uint32_t>()) {
    case 0x13a010:
      check(halo2::source_campaign_coop_respawn_allowed(spec.at("game_type"),spec.at("difficulty"),
          spec.at("override").get<unsigned>()!=0)==c.at("result"),name+" original respawn permission");break;
    case 0x14e4c0:
      halo2::source_campaign_coop_request_respawn(m,context,std::bit_cast<float>(c.at("delay_bits").get<std::uint32_t>()),
          {std::uint16_t(fp[0]),fp[1]},observe);break;
    case 0x14e510:
      halo2::source_campaign_coop_update_respawn(m,context,services,observe);break;
    default:check(false,"Original cooperative entry");
    }
    check(next_store==c.at("writes").size() && next_call==c.at("calls").size(),name+" complete source stores/services");
    stores+=next_store;calls+=next_call;
  }
  std::cout<<cases.size()<<" original cooperative respawn cases, "<<stores<<" ordered stores and "<<calls<<" declared safety calls match\n";
  std::cout<<"LIMIT player birth, safe placement, session, checkpoint and transport owners are not supplied by this component test\n";
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
