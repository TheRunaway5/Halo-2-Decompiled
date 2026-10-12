#include "halo2/vista_announcer_events.hpp"
#include "halo2/source_king.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace halo2;
using J=nlohmann::json;
namespace {
void check(bool value,const std::string& why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string& hex){
  check(!(hex.size()&1),"Invalid original byte bank");std::vector<std::byte> out(hex.size()/2);
  for(unsigned i=0;i<out.size();++i)out[i]=std::byte(std::stoul(hex.substr(i*2,2),nullptr,16));
  return out;
}
VistaAnnouncerEventState state(const J& row){
  VistaAnnouncerEventState out;const auto raw=bytes(row.at("queue"));const Bytes q(raw);
  for(unsigned i=0;i<5;++i){const auto at=i*12;out.queue.slots[i]={q.u32(at),q.u32(at+4),
      std::to_integer<std::uint8_t>(raw.at(at+8)),std::to_integer<std::uint8_t>(raw.at(at+9)),
      std::bit_cast<std::int16_t>(q.u16(at+10))};}
  out.queue.count=q.u32(60);const auto recent=bytes(row.at("recent"));const Bytes r(recent);
  for(unsigned i=0;i<4;++i){out.recent[i].hash=r.u32(i*24);out.recent[i].tick=r.u32(i*24+20);
    for(unsigned j=0;j<4;++j)out.recent[i].sounds[j]=r.u32(i*24+4+j*4);}
  return out;
}
}
int main(int argc,char** argv){try{
  check(argc==3,"Supply original King presentation fixture and start-producer report");std::ifstream file(argv[1]);
  check(bool(file),"Open original King fixture");const auto data=J::parse(file);
  VistaAnnouncerResponses banks;
  for(const auto& name:{"general","slayer","king"})for(const auto& hex:data.at("responses").at(name)){
    const auto raw=bytes(hex);check(raw.size()==0xa8,"Original response stride");
    VistaAnnouncerResponse response;std::copy(raw.begin(),raw.end(),response.begin());
    (std::string(name)=="general"?banks.general:std::string(name)=="slayer"?banks.slayer:banks.king).push_back(response);
  }
  check(banks.king.size()==8,"Original King bank coverage");std::set<unsigned> ordinals;
  unsigned count=0,missing=0,bridges=0;
  for(const auto& row:data.at("cases")){
    ++count;auto actual=state(row.at("before"));const auto& input=row.at("input");J calls=J::array();
    if(row.at("reset").get<bool>()){
      VistaAnnouncerEventState initialized;vista_announcer_events_initialize(initialized,row.at("rate"));
      check(initialized.queue==actual.queue && initialized.recent==actual.recent,"Original King initializer");
    }
    const auto playback=[&](std::uint32_t sound,float scale){
      calls.push_back({row.at("kind")=="update"?0x4883b6:0x48836c,sound,std::bit_cast<std::uint32_t>(scale)});
      return input.value("voice",0x12340001u);
    };
    if(row.at("kind")=="event"){
      VistaAnnouncerEvent event;for(unsigned i=0;i<8;++i)event.words[i]=input.at("event").at(i);
      event.data=input.at("event").at(8);
      VistaAnnouncerEventContext context{input.value("mode",std::uint8_t(1)),bool(input.value("over",0)),
          input.value("language",0u),input.value("tick",0u),row.at("rate")};
      std::optional<VistaAnnouncerAudioSelection> selected;
      for(const auto& binding:row.at("recipient_bindings")){
        const VistaAnnouncerLocalPlayer player{binding.at("datum"),binding.at("flags"),
            binding.at("membership"),binding.at("output_user")};
        if(const auto result=vista_announcer_event_audio(actual,banks,event,player,context,
            {row.at("cw"),row.at("mxcsr")},playback))selected=result;
      }
      check(bool(selected)==!row.at("selected").is_null(),"Original King response admission at"+std::to_string(count));
      if(selected){
        const auto& bank=event.words[0]==0?banks.general:banks.king;
        check(bytes(row.at("selected").at("record"))==std::vector<std::byte>(bank.at(selected->response_ordinal).begin(),bank.at(selected->response_ordinal).end()),"Original King response identity");
        check(selected->sound==row.at("selected").at("sound"),"Original King language selection");
        const Bytes response(bank.at(selected->response_ordinal));
        check(selected->caption_key==response.u32(0xc) && selected->display_key==response.u32(0x14),"Authored King message keys");
        if(event.words[0]==6)ordinals.insert(selected->response_ordinal);
        if(selected->sound==0xffffffffu){++missing;check(!selected->requested,"Missing authored cue became a playback request");}
      }
      if(event.words[0]==6 && event.words[1]!=0 && event.words[2]==0xffffffffu &&
          event.words[5]==0xffffffffu && event.words[6]==0xffffffffu){
        const auto converted=vista_announcer_king_notice({std::bit_cast<std::int32_t>(event.words[1]),
            event.words[3],std::bit_cast<std::int32_t>(event.words[4])});
        check(converted.words==event.words && converted.data==event.data,"Original King notice prefix mapping");++bridges;
      }
    }else if(row.at("kind")=="update")vista_announcer_queue_update(actual.queue,playback);
    else {check(row.at("kind")=="complete","Unknown original King operation");vista_announcer_queue_completed(actual.queue,input.at("voice"));}
    const auto expected=state(row.at("after"));
    check(actual.queue==expected.queue && actual.recent==expected.recent,"Original complete King queue/recent state at"+std::to_string(count));
    check(calls==row.at("calls"),"Original King playback requests at"+std::to_string(count));
  }
  check(count==3584 && ordinals.size()==8 && missing && bridges,"Original King presentation coverage");
  std::ifstream producer_file(argv[2]);check(bool(producer_file),"Open original King start report");
  const auto producers=J::parse(producer_file);unsigned starts=0;
  for(const auto& version:producers.at("versions"))for(const auto& row:version.at("results")){
    std::array<std::byte,0x24> storage;storage.fill(std::byte{0xa5});
    const auto published=source_king_activation_publication(row.at("input").at("mode"),row.at("input").at("recipient"),storage);
    check(published==!row.at("events").empty(),"Original King start admission");
    if(published){const auto expected=bytes(row.at("events").at(0).at("bytes"));
      check(expected.size()==34 && std::equal(expected.begin(),expected.end(),storage.begin()),"Original King start defined prefix");
    }else for(auto value:storage)check(value==std::byte{0xa5},"Suppressed King start mutated caller bytes");
    check(storage[34]==std::byte{0xa5} && storage[35]==std::byte{0xa5},"King start defined stack padding");++starts;
  }
  check(starts==54,"Original King start coverage");
  std::cout<<"PASS "<<count<<" original King presentation operations, all8 responses, "<<missing<<" absent cues, "<<bridges<<" event bridges\n";
  std::cout<<"PASS "<<starts<<" original King start publications and opaque padding\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
