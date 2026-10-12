#include "halo2/sound_sdk_voice_stop.hpp"
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
  const auto text=v.get<std::string>();check(text.size()%2==0,"Invalid voice stop fixture bytes");
  std::vector<std::byte> raw;for(std::size_t i=0;i<text.size();i+=2)raw.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return raw;
}
struct Harness {
  const nlohmann::json &row;
  std::map<std::uint32_t,std::vector<std::byte>> retained;
  SoundSdkDeviceMemory native_memory;
  std::function<void(const std::function<void()> &)> native_guard;
  std::vector<SoundDriverStartupStore> stores;
  std::vector<bool> done;
  nlohmann::json calls=nlohmann::json::array(),reads=nlohmann::json::array(),writes=nlohmann::json::array();
  unsigned clock=0,guard_index=0,depth=0,stalls=0;
  explicit Harness(const nlohmann::json &r):row(r),done(r.at("actions").size(),false){
    for(const auto &x:r.at("regions"))retained.emplace(number(x.at("address")),unhex(x.at("before_hex")));
  }
  std::span<std::byte> memory(std::uint32_t address,std::size_t size){
    if(native_memory)return native_memory(address,size);
    const auto after=retained.upper_bound(address);check(after!=retained.begin(),"Unknown voice stop identity");
    const auto found=std::prev(after);const auto offset=std::uint64_t(address)-found->first;
    check(offset+size<=found->second.size(),"Retirement extent differs");return std::span<std::byte>(found->second).subspan(offset,size);
  }
  void apply(const char *event,const nlohmann::json &value){
    for(unsigned i=0;i<done.size();++i){const auto &a=row.at("actions").at(i);
      if(!done[i]&&a.contains(event)&&a.at(event)==value){const auto raw=unhex(a.at("hex"));std::ranges::copy(raw,memory(a.at("address"),raw.size()).begin());done[i]=true;}
    }
  }
  void observe(const SoundDriverStartupStore &store){stores.push_back(store);apply("pc",hex(store.instruction));}
  void guard(std::uint32_t owner){
    check(guard_index<row.at("locks").size(),"Undeclared voice stop guard");
    check(number(row.at("locks").at(guard_index++))==owner,"Original voice stop guard order differs");apply("guard",guard_index);
  }
  void record(const char *name,nlohmann::json arguments,nlohmann::json time=nullptr){
    calls.push_back({{"name",name},{"arguments",arguments},{"time",time}});apply("callback",name);
  }
  std::uint32_t read(std::uint32_t address,unsigned size){
    const auto raw=memory(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<unsigned>(raw[i])<<(i*8u);
    reads.push_back({{"address",hex(address)},{"size",size},{"value",hex(value)}});apply("read",reads.size());return value;
  }
  SoundSdkVoiceStopServices services(SoundSdkNotificationBindings bindings){
    SoundSdkVoiceStopServices s;
    s.notifications.device.memory=[this](auto a,auto n){return memory(a,n);};
    s.notifications.device.under_device_lock=[this](const auto &body){
      const auto invoke=[&]{guard(0x379c90);++depth;try{body();}catch(...){--depth;throw;}--depth;guard(0x379cb2);};
      if(native_guard)native_guard(invoke);else invoke();
    };
    s.notifications.device.fifo_available=[this]{return read(0xfe820010u,4);};
    s.notifications.device.device_write=[this](auto address,auto value){
      auto raw=memory(address,4);for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(i*8u));
      writes.push_back({{"address",hex(address)},{"value",hex(value)}});apply("write",writes.size());
    };
    s.stop_buffer=[this](auto owner,auto wait){record("stop_buffer",nlohmann::json::array({hex(owner),hex(wait)}));return 0u;};
    s.notifications.query_system_time=[this]{
      check(clock<row.at("times").size(),"Undeclared voice stop clock");const auto value=std::stoull(row.at("times").at(clock++).get<std::string>(),nullptr,16);
      record("query_system_time",nlohmann::json::array(),hex(value,16));apply("clock",clock);return value;
    };
    s.notifications.cancel_timer=[this](auto timer){record("cancel_timer",nlohmann::json::array({hex(timer)}));};
    s.notifications.remove_queue_dpc=[this](auto dpc){record("remove_queue_dpc",nlohmann::json::array({hex(dpc)}));};
    s.notifications.set_timer=[this](auto timer,auto when,auto dpc){record("set_timer",nlohmann::json::array({hex(timer),hex(std::uint32_t(when)),hex(std::uint32_t(when>>32u)),hex(dpc)}));};
    s.notifications.force_pending_voice=[this](auto voice){record("force_pending_voice",nlohmann::json::array({hex(voice)}));};return s;
  }
};
std::uint32_t invoke(const nlohmann::json &row,Harness &h,const SoundSdkVoiceStopServices &s,SoundSdkNotificationBindings bindings){
  const auto observer=[&](const auto &v){h.observe(v);};const auto kind=row.at("kind").get<std::string>();
  std::vector<std::uint32_t> args;for(const auto &v:row.at("arguments"))args.push_back(number(v));
  constexpr std::uint32_t voice=0x1001000;
  if(kind=="pause")return source_sound_sdk_voice_pause_flags(voice,args.at(0),s,observer);
  if(kind=="envelope")source_sound_sdk_voice_stop_envelope(voice,s,observer);
  else if(kind=="stop")source_sound_sdk_common_voice_stop(voice,args.at(0),bindings,s,observer);
  else check(false,"Unknown voice stop kind");
  return 0;
}
void compare(const nlohmann::json &row,Harness &h,std::uint32_t result){
  for(const auto &region:row.at("regions")){
    const auto expected=unhex(region.at("after_hex"));check(std::ranges::equal(h.memory(number(region.at("address")),expected.size()),expected),"Original voice stop output differs");
  }
  check(h.stores.size()==row.at("stores").size(),"Original voice stop store count differs");
  for(unsigned i=0;i<h.stores.size();++i){const auto &v=h.stores[i];const auto &e=row.at("stores").at(i);
    check(v.instruction==number(e.at("pc"))&&v.address==number(e.at("address"))&&v.bytes==e.at("size")&&v.before==number(e.at("before"))&&v.after==number(e.at("after")),"Original voice stop store order differs");
  }
  auto expected_reads=row.at("reads"),expected_writes=row.at("writes");
  for(auto &v:expected_reads)v.erase("pc");
  for(auto &v:expected_writes)v.erase("pc");
  check(h.reads==expected_reads,"Original voice stop device reads differ");check(h.writes==expected_writes,"Original voice stop device writes differ");
  check(h.calls==row.at("calls")&&h.clock==row.at("times").size(),"Original voice stop platform calls differ");
  check(h.guard_index==row.at("locks").size()&&!h.depth,"Original voice stop guards differ");
  check(std::ranges::all_of(h.done,[](bool v){return v;}),"Original voice stop action not reached");
  if(row.at("kind")=="pause")check(result==number(row.at("result")),"Original voice stop result differs");
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
    h.native_memory=[&](auto a,auto n){return storage.memory(a,n);};const auto mutex=sync.create_mutex(false);check(mutex.handle&&!mutex.error,"Native voice stop mutex unavailable");
    h.native_guard=[&,handle=mutex.handle](const auto &body){check(sync.wait(handle,0xffffffffu).state==NativeSynchronizationWaitState::Acquired,"Native voice stop lock failed");
      try{body();}catch(...){check(!sync.release_mutex(handle),"Native voice stop unlock failed");throw;}check(!sync.release_mutex(handle),"Native voice stop unlock failed");
    };
  }
  const auto result=invoke(row,h,h.services(bindings),bindings);compare(row,h,result);
}
} // namespace
int main(int argc,char **argv){
  try{
    check(argc==2,"Supply original voice stop fixture");nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("schema")=="ORIGINAL_SOUND_SDK_VOICE_STOP_V1","Wrong voice stop schema");
    const auto &b=fixture.at("bindings");const SoundSdkNotificationBindings bindings{
      number(b.at("voice_vtable")),number(b.at("cancel_method")),number(b.at("arm_method"))};
    unsigned stores=0,calls=0;
    for(const auto &row:fixture.at("cases")){
      try{execute(row,bindings,false);execute(row,bindings,true);}catch(const std::exception &e){throw std::runtime_error(row.at("label").get<std::string>()+": "+e.what());}
      stores+=row.at("stores").size();calls+=row.at("calls").size();
    }
    const auto select=[&](const char *label)->const nlohmann::json &{for(const auto &r:fixture.at("cases"))if(r.at("label")==label)return r;throw std::runtime_error("Missing voice stop control");};
    unsigned stopped=0;
    for(unsigned failure=0;failure<15;++failure){
      const auto &row=select(failure==9?"stop_flags0001_group0_count1":failure==10?"stop_parent_eligible":failure==11||failure==12?"stop_cancels_pending_timer":failure==13?"stop_wait_forced_retirement":"stop_flags0003_group0_count1");
      Harness h(row);auto s=h.services(bindings);auto selected=bindings;
      switch(failure){
        case 0:s.notifications.device.memory={};break;
        case 1:s.notifications.device.memory=[&](auto a,auto n){return h.memory(a,n).first(n-1);};break;
        case 2:s.notifications.device.under_device_lock={};break;
        case 3:s.notifications.device.under_device_lock=[](const auto &){};break;
        case 4:s.notifications.device.under_device_lock=[](const auto &body){body();body();};break;
        case 5:s.notifications.device.fifo_available={};break;
        case 6:s.notifications.device.device_write={};break;
        case 7:s.notifications.query_system_time={};break;
        case 8:selected.arm_method=0x32131e;break;
        case 9:selected.cancel_method=0x32131e;break;
        case 10:s.stop_buffer={};break;
        case 11:s.notifications.cancel_timer={};break;
        case 12:s.notifications.remove_queue_dpc={};break;
        case 13:s.notifications.force_pending_voice={};break;
        case 14:selected.voice_vtable=0x417254;break;
      }
      bool failed=false;try{invoke(row,h,s,selected);}catch(const std::exception &){failed=true;}
      check(failed&&!h.depth,"Voice stop missing owner silently succeeded");++stopped;
    }
    std::cout<<"PASS "<<fixture.at("cases").size()<<" original voice stop cases, "<<stores<<" ordered stores, "<<calls
      <<" platform calls, "<<stopped<<" required-owner stops; all repeated with OS storage/recursive locks; device readiness, parent buffer stop, forced pending retirement and kernel calls remain declared boundaries\n";return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
