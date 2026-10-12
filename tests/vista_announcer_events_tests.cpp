#include "halo2/vista_announcer_events.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;using J=nlohmann::json;
namespace {
void check(bool v,const std::string& why){if(!v)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string& text){
  check(!(text.size()&1),"Invalid original hex bank");std::vector<std::byte> out(text.size()/2);
  for(unsigned i=0;i<out.size();++i)out[i]=std::byte(std::stoul(text.substr(i*2,2),nullptr,16));
  return out;
}
VistaAnnouncerEventState state(const J& j){
  VistaAnnouncerEventState out;const auto raw=bytes(j.at("queue"));const Bytes q(raw);
  for(unsigned i=0;i<5;++i){const auto at=i*12;out.queue.slots[i]={q.u32(at),q.u32(at+4),std::to_integer<std::uint8_t>(raw.at(at+8)),std::to_integer<std::uint8_t>(raw.at(at+9)),std::bit_cast<std::int16_t>(q.u16(at+10))};}
  out.queue.count=q.u32(60);const auto recent=bytes(j.at("recent"));const Bytes r(recent);
  for(unsigned i=0;i<4;++i){out.recent[i].hash=r.u32(i*24);out.recent[i].tick=r.u32(i*24+20);for(unsigned s=0;s<4;++s)out.recent[i].sounds[s]=r.u32(i*24+4+s*4);}
  return out;
}
VistaAnnouncerResponses responses(const J& j){
  VistaAnnouncerResponses out;
  for(const auto& name:{"general","slayer"})for(const auto& hex:j.at(name)){
    const auto raw=bytes(hex);check(raw.size()==0xa8,"Actual response stride");VistaAnnouncerResponse r;std::copy(raw.begin(),raw.end(),r.begin());(std::string(name)=="general"?out.general:out.slayer).push_back(r);
  }
  return out;
}
}
int main(int argc,char** argv){try{
  check(argc>=2 && argc<=4,"Supply original event fixture, optional actual Vista map and publication fixture");std::ifstream file(argv[1]);check(bool(file),"Open original event fixture");const auto data=J::parse(file);const auto banks=responses(data.at("responses"));unsigned count{},events{},updates{},completed{},requested{};
  for(const auto& r:data.at("cases")){
    ++count;auto s=state(r.at("before"));const auto& q=r.at("input");J calls=J::array();
    auto playback=[&](std::uint32_t sound,float scale){calls.push_back({r.at("kind")=="update"?0x4883b6:0x48836c,sound,std::bit_cast<std::uint32_t>(scale)});return q.value("voice",0x12340001u);};
    if(r.at("kind")=="event"){
      VistaAnnouncerEvent e;for(unsigned i=0;i<8;++i)e.words[i]=q.at("event").at(i);e.data=q.at("event").at(8);
      VistaAnnouncerLocalPlayer p{0x12340000u,q.value("flags",std::uint16_t(1)),q.value("member",std::int8_t(0)),q.value("user",std::int16_t(0))};
      VistaAnnouncerEventContext in{q.value("mode",std::uint8_t(1)),bool(q.value("over",0)),q.value("language",0u),q.value("tick",0u),r.at("rate")};
      const auto selected=vista_announcer_event_audio(s,banks,e,p,in,{r.at("cw"),r.at("mxcsr")},playback);
      check(bool(selected)==!r.at("selected").is_null(),"Original response admission at"+std::to_string(count));
      if(selected){const auto& bank=e.words[0]==0?banks.general:banks.slayer;check(std::vector<std::byte>(bank.at(selected->response_ordinal).begin(),bank.at(selected->response_ordinal).end())==bytes(r.at("selected").at("record")),"Original selected response identity");check(selected->sound==r.at("selected").at("sound"),"Original selected language sound");requested+=selected->requested;}
      ++events;
    }else if(r.at("kind")=="update"){vista_announcer_queue_update(s.queue,playback);++updates;}
    else {vista_announcer_queue_completed(s.queue,q.at("voice"));++completed;}
    const auto expected=state(r.at("after"));
    check(s.queue==expected.queue && s.recent==expected.recent,"Original complete announcer queue/recent bank at"+std::to_string(count));
    check(calls==r.at("calls"),"Original playback requests at"+std::to_string(count));
  }
  check(count==19348 && events && updates && completed && requested,"Original joined announcer coverage");
  VistaAnnouncerEventState initialized;vista_announcer_events_initialize(initialized,30);
  const auto original_initial=state(data.at("cases").at(0).at("before"));
  check(initialized.queue==original_initial.queue && initialized.recent==original_initial.recent,"Original queue and recent-hash initializer");
  if(argc>=3){Cache actual(argv[2]);const auto imported=load_vista_announcer_responses(actual);check(imported.general==banks.general && imported.slayer==banks.slayer,"Actual imported mulg response records differ from independent original parser");const auto owned=open_vista_announcer_assets(std::filesystem::path(argv[2]).parent_path(),actual.scenario());check(owned.cache && owned.cache->scenario()==actual.scenario() && owned.responses.general==banks.general && owned.responses.slayer==banks.slayer,"Actual scenario-qualified PC owner opening");std::cout<<"Actual Vista response owners imported; ";}
  if(argc==4){std::ifstream publication(argv[3]);check(bool(publication),"Open original end publication fixture");const auto rows=J::parse(publication);unsigned tested=0;
    for(const auto& r:rows){std::uint8_t marker=r.at("published");const auto event=vista_announcer_game_over_event(marker,bool(r.at("engine").get<unsigned>()),r.at("mode"),r.at("over"));
      check(marker==r.at("marker") && bool(event)==!r.at("events").empty(),"Actual end-publication guard/marker");
      if(event){const auto raw=bytes(r.at("events").at(0).at("bytes"));const Bytes original(raw);
        for(unsigned i=0;i<8;++i)check(event->words[i]==original.u32(i*4),"Actual end-event field");
        check(event->data==original.u16(32) && r.at("events").at(0).at("marker")==1,"End marker was not stored before delivery");}
      ++tested;}
    check(tested==2688,"Original end-publication corpus size");std::cout<<tested<<" original end-publication operations; ";}
  std::cout<<count<<" original event/queue operations, "<<requested<<" admitted requests; passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
