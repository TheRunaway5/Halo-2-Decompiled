#include "halo2/sound_sdk_voice_attachment.hpp"
#include "halo2/sound_sdk_voice_dispatch.hpp"
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
  check(text.size()%2==0,"Invalid voice attachment fixture bytes");
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
  unsigned depth=0,locks=0,fifo_reads=0,physical_index=0;
  std::vector<bool> done;
  nlohmann::json calls=nlohmann::json::array();
  explicit Harness(const nlohmann::json &r):row(r),done(r.at("actions").size(),false){
    for(const auto &x:r.at("regions"))retained.emplace(number(x.at("address")),unhex(x.at("before_hex")));
  }
  std::span<std::byte> memory(std::uint32_t address,std::size_t size){
    if(native_memory)return native_memory(address,size);
    const auto after=retained.upper_bound(address);check(after!=retained.begin(),"Unknown voice attachment identity");
    const auto found=std::prev(after);const auto offset=std::uint64_t(address)-found->first;
    check(offset+size<=found->second.size(),"Sample-page extent differs");
    return std::span<std::byte>(found->second).subspan(offset,size);
  }
  void action(const nlohmann::json &a){
    const auto raw=unhex(a.at("hex"));std::ranges::copy(raw,memory(a.at("address"),raw.size()).begin());
  }
  void apply(const char *event,const nlohmann::json &value){
    for(unsigned i=0;i<done.size();++i){const auto &a=row.at("actions").at(i);
      if(!done[i]&&a.contains(event)&&a.at(event)==value){action(a);done[i]=true;}
    }
  }
  void observe(const SoundDriverStartupStore &store){stores.push_back(store);apply("pc",hex(store.instruction));}
  static std::string hex(std::uint32_t value){char text[9];std::snprintf(text,sizeof(text),"%08x",value);return text;}
  void record(const char *name,nlohmann::json arguments,nlohmann::json result=nullptr){
    calls.push_back({{"name",name},{"arguments",arguments},{"result",result}});
  }
  SoundSdkVoiceAttachmentServices services(){
    SoundSdkVoiceAttachmentServices s;
    s.pages.device.memory=[this](auto a,auto n){return memory(a,n);};
    s.pages.device.under_device_lock=[this](const auto &body){
      const auto invoke=[&]{++locks;apply("guard",locks);++depth;try{body();}catch(...){--depth;throw;}--depth;};
      if(native_guard)native_guard(invoke);else invoke();
    };
    // Explicit FIFO/MMIO boundaries. Genuine CPU cases also compose real OS
    // storage and locks; these sinks do not claim native DSP or PCM output.
    s.pages.device.fifo_available=[this]{check(depth,"Sample-page FIFO outside lock");++fifo_reads;return fifo_reads==1?0u:0x100u;};
    s.pages.device.device_write=[this](auto a,auto value){check(depth&&fifo_reads>=2,"Sample-page device write before FIFO credits");writes.emplace_back(a,value);};
    s.pages.lock_pages=[this](auto buffer,auto bytes,bool unlock){
      check(!depth,"Sample-page kernel pin/unlock still holds device lock");
      record("lock_pages",nlohmann::json::array({hex(buffer),hex(bytes),hex(unlock)}));
    };
    s.pages.physical_address=[this](auto buffer){
      check(!depth&&physical_index<row.at("physical").size(),"Undeclared physical-address input");
      const auto value=row.at("physical").at(physical_index++).get<std::uint32_t>();
      record("physical_address",nlohmann::json::array({hex(buffer)}),value);return value;
    };
    s.initialization.memory=s.pages.device.memory;
    s.initialization.release_allocation=[this](auto address){
      check(!row.at("genuine").get<bool>(),"Undeclared genuine free boundary");
      record("release_allocation",nlohmann::json::array({hex(address)}));apply("callback","release_allocation");
    };
    s.initialization.allocate=[this](auto tag,auto bytes,auto flags){
      check(!row.at("genuine").get<bool>()&&!row.at("allocation").is_null(),"Undeclared allocator input");
      const auto value=row.at("allocation").get<std::uint32_t>();
      record("allocate",nlohmann::json::array({hex(tag),hex(bytes),hex(flags)}),value);apply("callback","allocate");return value;
    };
    s.stop=[this](auto voice,auto flags){
      check(!row.at("genuine").get<bool>(),"Undeclared genuine stop boundary");
      const auto value=row.at("stop_status").get<std::uint32_t>();
      record("stop",nlohmann::json::array({hex(voice),hex(flags)}),value);apply("callback","stop");return value;
    };
    s.wait_pending=[this](auto voice){
      check(!row.at("genuine").get<bool>(),"Undeclared genuine drain boundary");
      record("wait_pending",nlohmann::json::array({hex(voice)}));apply("callback","wait_pending");
    };
    return s;
  }

};
void compare(const nlohmann::json &row,Harness &h,std::uint32_t result){
  for(const auto &region:row.at("regions")){
    const auto expected=unhex(region.at("after_hex"));
    check(std::ranges::equal(h.memory(number(region.at("address")),expected.size()),expected),"Original SDK voice attachment output differs");
  }
  check(h.stores.size()==row.at("stores").size(),"Original SDK voice attachment store count differs");
  for(unsigned i=0;i<h.stores.size();++i){const auto &v=h.stores[i];const auto &e=row.at("stores").at(i);
    check(v.instruction==number(e.at("pc"))&&v.address==number(e.at("address"))&&v.bytes==e.at("size")&&
        v.before==number(e.at("before"))&&v.after==number(e.at("after")),"Original SDK voice attachment store order differs");
  }
  check(h.writes.size()==row.at("writes").size(),"Original SDK voice attachment command count differs");
  for(unsigned i=0;i<h.writes.size();++i){const auto &e=row.at("writes").at(i);
    check(h.writes[i]==std::pair{number(e.at("address")),number(e.at("value"))},"Original SDK voice attachment command differs");}
  check(h.calls==row.at("calls")&&h.physical_index==row.at("physical").size(),"Original SDK voice attachment platform calls differ");
  if(row.at("kind")=="buffer"||row.at("kind")=="sample_bytes"||row.at("kind")=="prepare_buffer"||row.at("kind")=="prepare_stream")check(result==number(row.at("result")),"Original SDK voice attachment result differs");
  unsigned locks=0;for(const auto &x:row.at("guards"))locks+=number(x)==0x379c90;
  check(h.locks==locks&&!h.depth,"Original SDK voice attachment lock count differs");
  check(std::ranges::all_of(h.done,[](bool v){return v;}),"Original SDK voice attachment action not reached");
}
std::uint32_t invoke(const nlohmann::json &row,Harness &h,const SoundSdkVoiceAttachmentServices &s){
  const auto observer=[&](const auto &v){h.observe(v);};const auto kind=row.at("kind").get<std::string>();
  std::vector<std::uint32_t> args;for(const auto &v:row.at("arguments"))args.push_back(number(v));
  constexpr std::uint32_t voice=0x1001000;const SoundSdkVoiceInitializationData initialization{};
  if(kind=="sample_bytes")return source_sound_sdk_voice_sample_bytes(voice,args.at(0),s.pages.device.memory);
  if(kind=="cursor"){source_sound_sdk_voice_cursor(voice,args.at(0),s.pages.device,observer);return 0;}
  if(kind=="detach"){source_sound_sdk_detach_voice_samples(voice,s,observer);return 0;}
  if(kind=="buffer")return source_sound_sdk_attach_voice_buffer(voice,s,observer);
  if(kind=="stream"){source_sound_sdk_attach_voice_stream(voice,{0x387330},initialization,s,observer);return 0;}
  check(kind=="prepare_buffer"||kind=="prepare_stream","Unknown attachment kind");
  SoundSdkVoiceDispatchServices dispatch;dispatch.hardware.device=s.pages.device;
  dispatch.attach_buffer=[&](auto address){return source_sound_sdk_attach_voice_buffer(address,s,observer);};
  dispatch.attach_stream=[&](auto address){source_sound_sdk_attach_voice_stream(address,{0x387330},initialization,s,observer);};
  return source_sound_sdk_voice_prepare(voice,{},dispatch,observer);
}
void execute(const nlohmann::json &row,bool native){
  Harness h(row);
  NativeSoundStorage storage({0xa1100001,0x2000000,0x2100000});
  NativeSynchronization synchronization({0x3001000,0x3001100});const auto sync=synchronization.services();
  std::uint32_t mutex=0;
  if(native){
    std::map<std::uint32_t,bool> pages;
    for(const auto &r:row.at("regions")){
      const auto address=number(r.at("address"));const auto raw=unhex(r.at("before_hex"));
      for(std::uint64_t page=address&~0xfffu;page<std::uint64_t(address)+raw.size();page+=0x1000)pages.emplace(page,true);
    }
    for(auto i=pages.begin();i!=pages.end();){const auto start=i->first;auto end=start+0x1000u;++i;
      while(i!=pages.end()&&i->first==end){end+=0x1000u;++i;}storage.map_zeroed(start,end-start);
    }
    for(const auto &r:row.at("regions")){
      const auto address=number(r.at("address"));const auto raw=unhex(r.at("before_hex"));
      std::ranges::copy(raw,storage.memory(address,raw.size()).begin());
    }
    h.native_memory=[&](auto a,auto n){return storage.memory(a,n);};
    const auto created=sync.create_mutex(false);check(created.handle&&!created.error,"Native voice attachment mutex unavailable");mutex=created.handle;
    h.native_guard=[&](const auto &body){
      check(sync.wait(mutex,0xffffffffu).state==NativeSynchronizationWaitState::Acquired,"Native voice attachment lock failed");
      try{body();}catch(...){check(!sync.release_mutex(mutex),"Native voice attachment unlock failed");throw;}
      check(!sync.release_mutex(mutex),"Native voice attachment unlock failed");
    };
  }
  const FloatingPointState before;
  const auto result=invoke(row,h,h.services());
  check(FloatingPointState{}==before,"Original SDK voice attachment leaked floating-point state");compare(row,h,result);
}
} // namespace
int main(int argc,char **argv){
  try{
    check(argc==2,"Supply original voice attachment fixture");nlohmann::json fixture;
    std::ifstream(argv[1])>>fixture;
    check(fixture.at("schema")=="ORIGINAL_SOUND_SDK_VOICE_ATTACHMENT_V1","Wrong voice attachment schema");
    unsigned genuine=0,stores=0,words=0;
    for(const auto &row:fixture.at("cases")){
      try{execute(row,false);if(row.at("genuine")){execute(row,true);++genuine;}}
      catch(const std::exception &e){throw std::runtime_error(row.at("label").get<std::string>()+": "+e.what());}
      stores+=row.at("stores").size();words+=row.at("writes").size();
    }
    const auto select=[&](std::string_view label)->const nlohmann::json &{
      for(const auto &row:fixture.at("cases"))if(row.at("label")==label)return row;
      throw std::runtime_error("Missing voice attachment owner-control case");
    };
    unsigned stopped=0;
    for(unsigned failure=0;failure<14;++failure){
      const char *label=failure==8||failure==9?"buffer_failure_stop00000000":failure==11||failure==12?"stream_tag0001_flags00100000_stride00000004_index0000000d":failure==13?"stream_allocate_00000000":"genuine_allocated";
      const auto &row=select(label);Harness h(row);auto s=h.services();
      switch(failure){
        case 0:s.pages.device.memory={};break;case 1:s.pages.device.under_device_lock={};break;
        case 2:s.pages.device.under_device_lock=[](const auto &){};break;
        case 3:s.pages.device.under_device_lock=[&](const auto &body){++h.depth;try{body();body();}catch(...){--h.depth;throw;}--h.depth;};break;
        case 4:s.pages.device.fifo_available={};break;case 5:s.pages.device.device_write={};break;
        case 6:s.pages.lock_pages={};break;case 7:s.pages.physical_address={};break;
        case 8:s.stop={};break;case 9:s.wait_pending={};break;
        case 10:s.pages.device.memory=[&](auto a,auto n){auto raw=h.memory(a,n);return raw.first(raw.size()-1);};break;
        case 11:s.initialization.memory={};break;case 12:s.initialization.release_allocation={};break;
        case 13:s.initialization.allocate={};break;
      }
      bool failed=false;try{invoke(row,h,s);}catch(const std::exception &){failed=true;}
      check(failed&&!h.depth,"Voice attachment missing owner silently succeeded");++stopped;
    }
    check(genuine==3,"Missing genuine voice attachment cases");
    std::cout<<"PASS "<<fixture.at("cases").size()<<" original voice attachment cases, "<<stores<<" ordered stores, "<<words
      <<" device words, "<<genuine<<" genuine cases with real OS storage/locks, "<<stopped<<" owner stops; no live output or DSP claim\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
