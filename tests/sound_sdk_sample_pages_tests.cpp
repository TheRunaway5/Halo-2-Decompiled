#include "halo2/sound_sdk_sample_pages.hpp"
#include "halo2/native_sound_storage.hpp"
#include "halo2/native_synchronization.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <cstdio>
#include <string_view>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
using namespace halo2;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::uint32_t number(const nlohmann::json &v){return std::stoul(v.get<std::string>(),nullptr,16);}
std::vector<std::byte> unhex(const nlohmann::json &v){
  const auto text=v.get<std::string>();std::vector<std::byte> raw;
  check(text.size()%2==0,"Invalid sample-page fixture bytes");
  for(std::size_t i=0;i<text.size();i+=2)raw.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return raw;
}
struct FloatingPointState {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  unsigned short control;unsigned mxcsr;
  FloatingPointState():mxcsr(_mm_getcsr()){asm volatile("fnstcw %0":"=m"(control));}
#endif
  friend bool operator==(const FloatingPointState &,const FloatingPointState &)=default;
};
struct Harness {
  const nlohmann::json &row;
  std::map<std::uint32_t,std::vector<std::byte>> retained;
  SoundSdkDeviceMemory native_memory;
  std::function<void(const std::function<void()> &)> native_guard;
  std::vector<SoundDriverStartupStore> stores;
  std::vector<std::pair<std::uint32_t,std::uint32_t>> writes;
  unsigned depth=0,locks=0,fifo_reads=0,physical_index=0,unlocks=0;
  nlohmann::json calls=nlohmann::json::array();
  explicit Harness(const nlohmann::json &r):row(r){
    for(const auto &x:r.at("regions"))retained.emplace(number(x.at("address")),unhex(x.at("before_hex")));
  }
  std::span<std::byte> memory(std::uint32_t address,std::size_t size){
    if(native_memory)return native_memory(address,size);
    const auto after=retained.upper_bound(address);check(after!=retained.begin(),"Unknown sample-page identity");
    const auto found=std::prev(after);const auto offset=std::uint64_t(address)-found->first;
    check(offset+size<=found->second.size(),"Sample-page extent differs");
    return std::span<std::byte>(found->second).subspan(offset,size);
  }
  void action(const nlohmann::json &a){
    const auto raw=unhex(a.at("hex"));std::ranges::copy(raw,memory(a.at("address"),raw.size()).begin());
  }
  void observe(const SoundDriverStartupStore &store){
    stores.push_back(store);
    for(const auto &a:row.at("actions"))if(a.contains("pc")&&number(a.at("pc"))==store.instruction)action(a);
  }
  static std::string hex(std::uint32_t value){char text[9];std::snprintf(text,sizeof(text),"%08x",value);return text;}
  void record(const char *name,nlohmann::json arguments,nlohmann::json result=nullptr){
    calls.push_back({{"name",name},{"arguments",arguments},{"result",result}});
  }
  SoundSdkSamplePageServices services(){
    SoundSdkSamplePageServices s;
    s.device.memory=[this](auto a,auto n){return memory(a,n);};
    s.device.under_device_lock=[this](const auto &body){
      const auto invoke=[&]{++locks;++depth;try{body();}catch(...){--depth;throw;}--depth;++unlocks;
        for(const auto &a:row.at("actions"))if(a.contains("after_unlock")&&a.at("after_unlock")==unlocks)action(a);
      };
      if(native_guard)native_guard(invoke);else invoke();
    };
    // Explicit FIFO/MMIO boundaries. Genuine CPU cases also compose real OS
    // storage and locks; these sinks do not claim native DSP or PCM output.
    s.device.fifo_available=[this]{check(depth,"Sample-page FIFO outside lock");++fifo_reads;return fifo_reads==1?0u:0x100u;};
    s.device.device_write=[this](auto a,auto value){check(depth&&fifo_reads>=2,"Sample-page device write before FIFO credits");writes.emplace_back(a,value);};
    s.lock_pages=[this](auto buffer,auto bytes,bool unlock){
      check(!depth,"Sample-page kernel pin/unlock still holds device lock");
      record("lock_pages",nlohmann::json::array({hex(buffer),hex(bytes),hex(unlock)}));
    };
    s.physical_address=[this](auto buffer){
      check(!depth&&physical_index<row.at("physical").size(),"Undeclared physical-address input");
      const auto value=row.at("physical").at(physical_index++).get<std::uint32_t>();
      record("physical_address",nlohmann::json::array({hex(buffer)}),value);return value;
    };
    s.allocate=[this](auto tag,auto bytes,auto flags){
      check(!row.at("genuine").get<bool>()&&!row.at("allocation").is_null(),"Undeclared allocator input");
      const auto value=row.at("allocation").get<std::uint32_t>();
      record("allocate",nlohmann::json::array({hex(tag),hex(bytes),hex(flags)}),value);return value;
    };return s;
  }

};
void compare(const nlohmann::json &row,Harness &h,std::uint32_t result){
  for(const auto &region:row.at("regions")){
    const auto expected=unhex(region.at("after_hex"));
    check(std::ranges::equal(h.memory(number(region.at("address")),expected.size()),expected),"Original SDK sample-page output differs");
  }
  check(h.stores.size()==row.at("stores").size(),"Original SDK sample-page store count differs");
  for(unsigned i=0;i<h.stores.size();++i){const auto &v=h.stores[i];const auto &e=row.at("stores").at(i);
    check(v.instruction==number(e.at("pc"))&&v.address==number(e.at("address"))&&v.bytes==e.at("size")&&
        v.before==number(e.at("before"))&&v.after==number(e.at("after")),"Original SDK sample-page store order differs");
  }
  check(h.writes.size()==row.at("writes").size(),"Original SDK sample-page command count differs");
  for(unsigned i=0;i<h.writes.size();++i){const auto &e=row.at("writes").at(i);
    check(h.writes[i]==std::pair{number(e.at("address")),number(e.at("value"))},"Original SDK sample-page command differs");}
  check(h.calls==row.at("calls")&&h.physical_index==row.at("physical").size(),"Original SDK sample-page platform calls differ");
  if(row.at("kind")!="map"&&row.at("kind")!="release")check(result==number(row.at("result")),"Original SDK sample-page result differs");
  unsigned locks=0;for(const auto &x:row.at("guards"))locks+=number(x)==0x379c90;
  check(h.locks==locks&&!h.depth,"Original SDK sample-page lock count differs");
}
std::uint32_t invoke(const nlohmann::json &row,Harness &h,const SoundSdkSamplePageServices &s){
  const auto observer=[&](const auto &v){h.observe(v);};const auto kind=row.at("kind").get<std::string>();
  std::vector<std::uint32_t> args;for(const auto &v:row.at("arguments"))args.push_back(number(v));
  if(kind=="construct")return source_sound_sdk_construct_sample_pages(0x1001000,{0x417240,0x386af8},s.device.memory,observer);
  if(kind=="initialize")return source_sound_sdk_initialize_sample_pages(0x1001000,args.at(0),s,observer);
  if(kind=="acquire")return source_sound_sdk_acquire_sample_pages(0x1001000,args.at(0),args.at(1),s,observer);
  if(kind=="map"){source_sound_sdk_map_sample_pages(args.at(0),args.at(1),args.at(2),s);return 0;}
  check(kind=="release","Unknown sample-page kind");source_sound_sdk_release_sample_pages(0x1001000,args.at(0),s,observer);return 0;
}
void execute(const nlohmann::json &row,bool native){
  Harness h(row);
  NativeSoundStorage storage({0xa1100001,0x2000000,0x2100000});
  NativeSynchronization synchronization({0x3001000,0x3001100});const auto sync=synchronization.services();
  std::uint32_t mutex=0;
  if(native){
    for(const auto &r:row.at("regions")){
      const auto address=number(r.at("address"));const auto raw=unhex(r.at("before_hex"));
      storage.map_zeroed(address&~0xfffu,raw.size()+(address&0xfffu));std::ranges::copy(raw,storage.memory(address,raw.size()).begin());
    }
    h.native_memory=[&](auto a,auto n){return storage.memory(a,n);};
    const auto created=sync.create_mutex(false);check(created.handle&&!created.error,"Native sample-page mutex unavailable");mutex=created.handle;
    h.native_guard=[&](const auto &body){
      check(sync.wait(mutex,0xffffffffu).state==NativeSynchronizationWaitState::Acquired,"Native sample-page lock failed");
      try{body();}catch(...){check(!sync.release_mutex(mutex),"Native sample-page unlock failed");throw;}
      check(!sync.release_mutex(mutex),"Native sample-page unlock failed");
    };
  }
  const FloatingPointState before;
  const auto result=invoke(row,h,h.services());
  check(FloatingPointState{}==before,"Original SDK sample-page leaked floating-point state");compare(row,h,result);
}
} // namespace
int main(int argc,char **argv){
  try{
    check(argc==2,"Supply original sample-page fixture");nlohmann::json fixture;
    std::ifstream(argv[1])>>fixture;
    check(fixture.at("schema")=="ORIGINAL_SOUND_SDK_SAMPLE_PAGES_V1","Wrong sample-page schema");
    unsigned genuine=0,stores=0,words=0;
    for(const auto &row:fixture.at("cases")){
      try{execute(row,false);if(row.at("genuine")){execute(row,true);++genuine;}}
      catch(const std::exception &e){throw std::runtime_error(row.at("label").get<std::string>()+": "+e.what());}
      stores+=row.at("stores").size();words+=row.at("writes").size();
    }
    const auto select=[&](std::string_view label)->const nlohmann::json &{
      for(const auto &row:fixture.at("cases"))if(row.at("label")==label)return row;
      throw std::runtime_error("Missing sample-page owner-control case");
    };
    unsigned stopped=0;
    for(unsigned failure=0;failure<11;++failure){
      const auto &row=select(failure==8?"initialize_count3_success1":failure==10?"release_counts(8,)_index0_pattern0":"genuine_allocated");
      Harness h(row);auto s=h.services();
      switch(failure){
        case 0:s.device.memory={};break;case 1:s.device.under_device_lock={};break;
        case 2:s.device.under_device_lock=[](const auto &){};break;
        case 3:s.device.under_device_lock=[&](const auto &body){++h.depth;try{body();body();}catch(...){--h.depth;throw;}--h.depth;};break;
        case 4:s.device.fifo_available={};break;case 5:s.device.device_write={};break;
        case 6:s.lock_pages={};break;case 7:s.physical_address={};break;case 8:s.allocate={};break;
        case 9:s.device.memory=[&](auto a,auto n){auto raw=h.memory(a,n);return raw.first(raw.size()-1);};break;
        case 10:s.lock_pages={};break;
      }
      bool failed=false;try{invoke(row,h,s);}catch(const std::exception &){failed=true;}
      check(failed&&!h.depth,"Sample-page missing owner silently succeeded");++stopped;
    }
    check(genuine==2,"Missing genuine sample-page cases");
    std::cout<<"PASS "<<fixture.at("cases").size()<<" original sample-page cases, "<<stores<<" ordered stores, "<<words
      <<" device words, "<<genuine<<" genuine cases with real OS storage/locks, "<<stopped<<" owner stops; no live output or DSP claim\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
