#include "halo2/source_result_capture.hpp"
#include <nlohmann/json.hpp>
#include <cstring>
#include <bit>
#include <fstream>
#include <iostream>

using namespace halo2;using nlohmann::json;
namespace {
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::vector<std::uint8_t> hex(const std::string& s) {
  check(s.size()%2==0,"Odd raw record");std::vector<std::uint8_t> r;
  for(unsigned i=0;i<s.size();i+=2)r.push_back(std::uint8_t(std::stoul(s.substr(i,2),nullptr,16)));return r;
}
SourceResultStatistics seed() {
  SourceResultStatistics r;auto* raw=reinterpret_cast<std::uint8_t*>(&r);
  for(unsigned i=0;i<0x2020;++i){const auto v=std::uint16_t(i*73+0x3141);raw[i*2]=std::uint8_t(v);raw[i*2+1]=std::uint8_t(v>>8);}return r;
}
SourceGameEndStats bank(const json& r) {
  const auto raw=hex(r);check(raw.size()==sizeof(SourceGameEndStats),"Match bank size differs");SourceGameEndStats b;
  std::memcpy(&b,raw.data(),raw.size());return b;
}
void retained(const SourceResultStatistics& actual,const json& updates) {
  auto wanted=seed();auto* raw=reinterpret_cast<std::uint8_t*>(&wanted);
  for(const auto& r:updates) {
    const unsigned at=r.at("word");const auto old=r.at("old").get<unsigned>(),value=r.at("new").get<unsigned>();
    check(at<0x2020 && (unsigned(raw[at*2])|(unsigned(raw[at*2+1])<<8))==old,"Original poisoned input differs");
    raw[at*2]=std::uint8_t(value);raw[at*2+1]=std::uint8_t(value>>8);
  }
  check(std::memcmp(&wanted,&actual,sizeof(actual))==0,"Whole retained bank differs from unchanged original owner");
}
SourceResultStatDescriptors general(const json& v) {
  SourceResultStatDescriptors r;
  for(unsigned i=0;i<45;++i) {
    r[i].minimum=v.at("descriptors")[i].at("minimum");r[i].maximum=v.at("descriptors")[i].at("maximum");
    const auto tail=hex(v.at("descriptors")[i].at("remaining_hex"));std::copy(tail.begin(),tail.end(),r[i].remaining.begin());
  }return r;
}
SourceResultCaptureConstants constants(const json& v) {
  SourceResultCaptureConstants c;const auto bytes=hex(v.at("weapon_descriptors_hex"));check(bytes.size()==112,"Weapon descriptor size differs");
  std::memcpy(c.weapon_descriptors.data(),bytes.data(),bytes.size());
  for(unsigned i=0;i<64;++i)c.cause_weapons[i]=v.at("cause_mapping")[i];
  const auto bits=hex(v.at("multikill_seconds_bits"));std::memcpy(&c.multikill_seconds,bits.data(),4);return c;
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=3)throw std::runtime_error("Supply original capture and general-stat fixtures");
    std::ifstream a(argv[1]),b(argv[2]);json capture,stats;a>>capture;b>>stats;unsigned cases=0;
    check(capture.at("instruction_patches").empty() && capture.at("supplied_returns").empty(),"Capture oracle supplied a dependency");
    for(const auto& [name,v]:capture.at("versions").items()) {
      const auto d=general(stats.at("versions").at(name));const auto c=constants(v);
      for(unsigned cause=0;cause<256;++cause) {check(c.cause_weapons[cause&63]==v.at("cause_mapping")[cause],"Original reporting-type dispatch differs");++cases;}
      for(const auto& kind:{"kills","deaths","lifetimes"})for(const auto& r:v.at(kind)) {
        auto actual=seed();const SourceResultRecording recording{r.at("flags")[0],r.at("flags")[1]};SourceResultPublication p{actual,recording,d};
        SourceResultCapture writer{p,c};auto match=bank(r.at("before_bank_hex"));const unsigned slot=r.at("slot");
        if(std::string_view(kind)=="kills")source_result_credited_kill(match,slot,r.at("tick"),r.at("cutoff"),r.at("cause").get<std::uint8_t>(),&writer,r.at("team").get<bool>()?7:-1);
        else if(std::string_view(kind)=="deaths") {
          const int credit=r.at("credited");const bool team=r.at("team");
          source_result_victim_death(match,slot,credit,r.at("self"),r.at("betrayal"),r.at("cause").get<std::uint8_t>(),&writer,
              team?7:-1,team && credit!=-1 ? (unsigned(credit)==slot?7:0) : -1);
        } else source_result_lifetime_tick(p,slot,{r.at("tick"),r.at("rate"),r.at("mode"),r.at("current"),r.at("present"),r.at("lifecycle")==1,r.at("status")==1});
        const auto wanted=bank(r.at("after_bank_hex"));check(std::memcmp(&wanted,&match,sizeof(match))==0,"Complete match bank differs from original capture producer");
        retained(actual,r.at("retained_updates"));++cases;
      }
      for(const auto& r:v.at("weapons")) {
        auto actual=seed();const SourceResultRecording recording{r.at("flags")[0],r.at("flags")[1]};SourceResultPublication p{actual,recording,d};SourceResultCapture writer{p,c};
        source_result_weapon_add(writer,r.at("slot"),r.at("kind"),r.at("weapon"),r.at("delta"));retained(actual,r.at("retained_updates"));++cases;
      }
      for(const auto& r:v.at("cutoffs")) {
        check(source_result_multikill_cutoff(r.at("tick"),r.at("rate"),c)==r.at("cutoff"),
            "Executed original damage-owner cutoff differs");++cases;
      }
    }
    std::cout<<"PASS "<<cases<<" original cause/weapon, credited-kill, victim-death and full-player lifetime calls; complete banks preserved\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
