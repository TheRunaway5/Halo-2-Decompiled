#include "halo2/sound_sdk_notifications.hpp"
#include "halo2/native_sound_storage.hpp"
#include "halo2/native_synchronization.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::uint32_t number(const nlohmann::json &v){return std::stoul(v.get<std::string>(),nullptr,16);}
std::string hex(std::uint64_t value,unsigned width=8){char text[17];std::snprintf(text,sizeof(text),"%0*llx",width,static_cast<unsigned long long>(value));return text;}
std::vector<std::byte> unhex(const nlohmann::json &v){
  const auto text=v.get<std::string>();check(text.size()%2==0,"Invalid notification fixture bytes");
  std::vector<std::byte> raw;for(std::size_t i=0;i<text.size();i+=2)raw.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return raw;
}
struct Harness {
  const nlohmann::json &row;
  std::map<std::uint32_t,std::vector<std::byte>> retained;
  std::map<std::string,std::uint32_t> addresses;
  SoundSdkDeviceMemory native_memory;
  std::function<void(const std::function<void()> &)> native_guard;
  std::vector<SoundDriverStartupStore> stores;
  std::vector<bool> done;
  nlohmann::json calls=nlohmann::json::array();
  unsigned clock=0,guard_index=0,depth=0;
  explicit Harness(const nlohmann::json &r):row(r),done(r.at("actions").size(),false){
    for(const auto &x:r.at("regions")){
      const auto at=number(x.at("address"));retained.emplace(at,unhex(x.at("before_hex")));addresses.emplace(x.at("name").get<std::string>(),at);
    }
  }
  std::span<std::byte> memory(std::uint32_t address,std::size_t size){
    if(native_memory)return native_memory(address,size);
    const auto after=retained.upper_bound(address);check(after!=retained.begin(),"Unknown notification identity");
    const auto found=std::prev(after);const auto offset=std::uint64_t(address)-found->first;
    check(offset+size<=found->second.size(),"Notification extent differs");return std::span<std::byte>(found->second).subspan(offset,size);
  }
  void effects(const nlohmann::json &values){
    for(const auto &e:values){
      const auto before=unhex(e.at("before_hex")),after=unhex(e.at("after_hex"));check(before.size()==after.size(),"External notification extent differs");
      auto raw=memory(addresses.at(e.at("region").get<std::string>()),before.size());
      check(std::ranges::equal(raw,before),"External notification input state differs");
      for(unsigned i=0;i<before.size();++i)if(before[i]!=after[i])raw[i]=after[i];
    }
  }
  void apply(const char *event,const nlohmann::json &value){
    for(unsigned i=0;i<done.size();++i){const auto &a=row.at("actions").at(i);
      if(!done[i]&&a.contains(event)&&a.at(event)==value){const auto raw=unhex(a.at("hex"));std::ranges::copy(raw,memory(a.at("address"),raw.size()).begin());done[i]=true;}
    }
  }
  void observe(const SoundDriverStartupStore &store){stores.push_back(store);apply("pc",hex(store.instruction));}
  void guard(std::uint32_t owner){
    check(guard_index<row.at("guards").size(),"Undeclared notification guard");const auto &g=row.at("guards").at(guard_index++);
    check(number(g.at("owner"))==owner,"Original SDK notification guard order differs");effects(g.at("effects"));apply("guard",guard_index);
  }
  void record(const char *name,nlohmann::json arguments,nlohmann::json time=nullptr){
    check(calls.size()<row.at("calls").size(),"Undeclared notification platform call");const auto &expected=row.at("calls").at(calls.size());
    check(expected.at("name")==name,"Original SDK notification platform order differs");effects(expected.at("effects"));
    calls.push_back({{"name",name},{"arguments",arguments},{"time",time},{"effects",expected.at("effects")}});apply("callback",name);
  }
  SoundSdkNotificationServices services(){
    SoundSdkNotificationServices s;s.device.memory=[this](auto a,auto n){return memory(a,n);};
    s.device.under_device_lock=[this](const auto &body){
      const auto invoke=[&]{guard(0x379c90);++depth;try{body();}catch(...){--depth;throw;}--depth;guard(0x379cb2);};
      if(native_guard)native_guard(invoke);else invoke();
    };
    s.query_system_time=[this]{
      check(clock<row.at("times").size(),"Undeclared notification clock");const auto value=std::stoull(row.at("times").at(clock++).get<std::string>(),nullptr,16);
      record("query_system_time",nlohmann::json::array(),hex(value,16));apply("clock",clock);return value;
    };
    s.cancel_timer=[this](auto timer){check(depth,"Notification timer cancellation outside guard");record("cancel_timer",nlohmann::json::array({hex(timer)}));};
    s.remove_queue_dpc=[this](auto dpc){check(depth,"Notification DPC removal outside guard");record("remove_queue_dpc",nlohmann::json::array({hex(dpc)}));};
    s.set_timer=[this](auto timer,auto when,auto dpc){check(depth,"Notification timer arm outside guard");record("set_timer",nlohmann::json::array({hex(timer),hex(std::uint32_t(when)),hex(std::uint32_t(when>>32u)),hex(dpc)}));};
    // The timeout callee is a declared boundary. This sink never claims to run
    // the still-unported3810BD device/DSP forced-stop implementation.
    s.force_pending_voice=[this](auto voice){record("force_pending_voice",nlohmann::json::array({hex(voice)}));};return s;
  }
};
std::uint32_t invoke(const nlohmann::json &row,Harness &h,const SoundSdkNotificationServices &s,SoundSdkNotificationBindings bindings){
  const auto observer=[&](const auto &v){h.observe(v);};const auto kind=row.at("kind").get<std::string>();
  std::vector<std::uint32_t> args;for(const auto &v:row.at("arguments"))args.push_back(number(v));
  constexpr std::uint32_t voice=0x1001000,device=0x1002000,descriptor=0x1004000;
  if(kind=="arm")return source_sound_sdk_arm_notification(device,args.at(0),s,observer);
  if(kind=="cancel")source_sound_sdk_cancel_notification(device,args.at(0),s,observer);
  else if(kind=="voice_arm"||kind=="arm_immediate")source_sound_sdk_voice_arm_notification(voice,args.at(0),args.at(1),args.at(2),args.at(3),s,observer);
  else if(kind=="voice_cancel")source_sound_sdk_voice_cancel_notification(voice,args.at(0),s,observer);
  else if(kind=="pending")source_sound_sdk_poll_pending_voice(voice,bindings,s,observer);
  else if(kind=="drain")source_sound_sdk_wait_pending_voice(voice,bindings,s,observer);
  else if(kind=="mapping")return source_sound_sdk_mapping_completed(descriptor,args.at(0),s.device.memory);
  else check(false,"Unknown notification kind");
  return 0;
}
void compare(const nlohmann::json &row,Harness &h,std::uint32_t result){
  for(const auto &region:row.at("regions")){
    const auto expected=unhex(region.at("after_hex"));check(std::ranges::equal(h.memory(number(region.at("address")),expected.size()),expected),"Original SDK notification output differs");
  }
  check(h.stores.size()==row.at("stores").size(),"Original SDK notification store count differs");
  for(unsigned i=0;i<h.stores.size();++i){const auto &v=h.stores[i];const auto &e=row.at("stores").at(i);
    check(v.instruction==number(e.at("pc"))&&v.address==number(e.at("address"))&&v.bytes==e.at("size")&&v.before==number(e.at("before"))&&v.after==number(e.at("after")),"Original SDK notification store order differs");
  }
  check(h.calls==row.at("calls")&&h.clock==row.at("times").size(),"Original SDK notification platform calls differ");
  check(h.guard_index==row.at("guards").size()&&!h.depth,"Original SDK notification guards differ");
  check(std::ranges::all_of(h.done,[](bool v){return v;}),"Original SDK notification action not reached");
  if(row.at("kind")=="arm"||row.at("kind")=="mapping")check(result==number(row.at("result")),"Original SDK notification result differs");
}
void execute(const nlohmann::json &row,SoundSdkNotificationBindings bindings,bool native){
  Harness h(row);NativeSoundStorage storage({0xa1100001,0x2000000,0x2100000});
  NativeSynchronization synchronization({0x3001000,0x3001100});const auto sync=synchronization.services();
  if(native){
    std::map<std::uint32_t,bool> pages;
    for(const auto &r:row.at("regions")){const auto at=number(r.at("address"));const auto raw=unhex(r.at("before_hex"));
      for(std::uint64_t page=at&~0xfffu;page<std::uint64_t(at)+raw.size();page+=0x1000)pages.emplace(page,true);
    }
    for(auto i=pages.begin();i!=pages.end();){const auto start=i->first;auto end=start+0x1000u;++i;while(i!=pages.end()&&i->first==end){end+=0x1000u;++i;}storage.map_zeroed(start,end-start);}
    for(const auto &r:row.at("regions")){const auto raw=unhex(r.at("before_hex"));std::ranges::copy(raw,storage.memory(number(r.at("address")),raw.size()).begin());}
    h.native_memory=[&](auto a,auto n){return storage.memory(a,n);};const auto mutex=sync.create_mutex(false);check(mutex.handle&&!mutex.error,"Native notification mutex unavailable");
    h.native_guard=[&,handle=mutex.handle](const auto &body){check(sync.wait(handle,0xffffffffu).state==NativeSynchronizationWaitState::Acquired,"Native notification lock failed");
      try{body();}catch(...){check(!sync.release_mutex(handle),"Native notification unlock failed");throw;}check(!sync.release_mutex(handle),"Native notification unlock failed");
    };
  }
  const auto result=invoke(row,h,h.services(),bindings);compare(row,h,result);
}
} // namespace
int main(int argc,char **argv){
  try{
    check(argc==2,"Supply original notification fixture");nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("schema")=="ORIGINAL_SOUND_SDK_NOTIFICATIONS_V1","Wrong notification schema");
    const auto &b=fixture.at("bindings");const SoundSdkNotificationBindings bindings{number(b.at("voice_vtable")),number(b.at("cancel_method")),number(b.at("arm_method"))};
    unsigned stores=0,calls=0;
    for(const auto &row:fixture.at("cases")){
      try{execute(row,bindings,false);execute(row,bindings,true);}catch(const std::exception &e){throw std::runtime_error(row.at("label").get<std::string>()+": "+e.what());}
      stores+=row.at("stores").size();calls+=row.at("calls").size();
    }
    const auto select=[&](const char *label)->const nlohmann::json &{for(const auto &row:fixture.at("cases"))if(row.at("label")==label)return row;throw std::runtime_error("Missing notification owner-control case");};
    unsigned stopped=0;
    for(unsigned failure=0;failure<12;++failure){
      const char *label=failure==6||failure==7?"cancel_flags00000002_peers()":failure==9?"pending_flags8001_previous0000000000000000_delta0":failure>=10?"pending_flags0000_previous0000000000000000_delta0":"arm_flags00000000_when0000000000000065_now0000000000000064";
      const auto &row=select(label);Harness h(row);auto s=h.services();auto selected=bindings;
      switch(failure){
        case 0:s.device.memory={};break;case 1:s.device.memory=[&](auto a,auto n){auto raw=h.memory(a,n);return raw.first(raw.size()-1);};break;
        case 2:s.device.under_device_lock={};break;case 3:s.device.under_device_lock=[](const auto &){};break;
        case 4:s.device.under_device_lock=[&](const auto &body){++h.depth;try{body();body();}catch(...){--h.depth;throw;}--h.depth;};break;
        case 5:s.query_system_time={};break;case 6:s.cancel_timer={};break;case 7:s.remove_queue_dpc={};break;
        case 8:s.set_timer={};break;case 9:s.force_pending_voice={};break;
        case 10:selected.voice_vtable=0x417254;break;case 11:selected.cancel_method=0x32131e;break;
      }
      bool failed=false;try{invoke(row,h,s,selected);}catch(const std::exception &){failed=true;}
      check(failed&&!h.depth,"Notification missing owner silently succeeded");++stopped;
    }
    std::cout<<"PASS "<<fixture.at("cases").size()<<" original notification cases, "<<stores<<" ordered stores, "<<calls
      <<" platform calls; all repeated with OS storage/recursive locks, "<<stopped<<" owner stops; clock/timer/DPC/forced-stop boundaries remain declared, no live DSP/output claim\n";return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
