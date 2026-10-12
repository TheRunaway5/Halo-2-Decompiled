#include "halo2/sound_sdk_voice_initialization.hpp"
#include "halo2/sound_capabilities.hpp"
#include "halo2/native_sound_storage.hpp"
#include "halo2/native_synchronization.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
using namespace halo2;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::uint32_t number(const nlohmann::json &v){return std::stoul(v.get<std::string>(),nullptr,16);}
std::vector<std::byte> unhex(const nlohmann::json &v){
  const auto text=v.get<std::string>();std::vector<std::byte> out;
  check(text.size()%2==0,"Invalid original voice initializer bytes");
  for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return out;
}
struct FloatingPointState {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  unsigned short control;unsigned mxcsr;
  FloatingPointState():mxcsr(_mm_getcsr()){asm volatile("fnstcw %0":"=m"(control));}
#endif
  friend bool operator==(const FloatingPointState &,const FloatingPointState &)=default;
};
SoundSdkVoiceInitializationData import(const nlohmann::json &j){
  SoundSdkVoiceInitializationData d{};
  for(unsigned i=0;i<18;++i)d.source_defaults[i]=j.at("source_defaults").at(i);
  for(unsigned i=0;i<9;++i)d.environment_defaults[i]=j.at("environment_defaults").at(i);
  for(unsigned i=0;i<4;++i){d.pcm_guid[i]=j.at("pcm_guid").at(i);d.adpcm_guid[i]=j.at("adpcm_guid").at(i);}
  d.default_format_identity=number(j.at("default_format_identity"));d.spatial_bins_identity=number(j.at("spatial_bins_identity"));
  d.parent_bins_identity=number(j.at("parent_bins_identity"));d.channel_bins_table_identity=number(j.at("channel_bins_table_identity"));
  d.unsigned_correction_bits=number(j.at("unsigned_correction_bits"));d.frequency_reciprocal_bits=number(j.at("frequency_reciprocal_bits"));
  for(const auto &region:j.at("immutable"))d.immutable.push_back({number(region.at("address")),unhex(region.at("hex"))});
  return d;
}
struct Harness {
  const nlohmann::json &row;
  std::map<std::uint32_t,std::vector<std::byte>> retained;
  std::vector<SoundDriverStartupStore> stores;
  std::vector<std::uint32_t> frees;
  unsigned allocations=0,state_guards=0,irql_guards=0,depth=0;
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> native_memory;
  explicit Harness(const nlohmann::json &r):row(r){
    for(const auto &region:r.at("regions"))retained.emplace(number(region.at("address")),unhex(region.at("before_hex")));
  }
  std::span<std::byte> memory(std::uint32_t address,std::size_t size){
    if(native_memory)return native_memory(address,size);
    const auto after=retained.upper_bound(address);check(after!=retained.begin(),"Unknown voice initialization identity");
    const auto found=std::prev(after);const auto offset=std::uint64_t(address)-found->first;
    check(offset+size<=found->second.size(),"Voice initialization extent differs");
    return std::span<std::byte>(found->second).subspan(offset,size);
  }
  void observe(const SoundDriverStartupStore &store){
    stores.push_back(store);
    for(const auto &action:row.at("actions"))if(number(action.at("pc"))==store.instruction){
      const auto raw=unhex(action.at("hex"));const auto target=memory(action.at("address"),raw.size());
      std::copy(raw.begin(),raw.end(),target.begin());
    }
  }
  SoundSdkVoiceInitializationServices services(){
    SoundSdkVoiceInitializationServices s;s.memory=[this](auto a,auto n){return memory(a,n);};
    s.allocate=[this](auto tag,auto bytes,auto zero){
      check(allocations<row.at("allocations").size(),"Unexpected voice allocation");
      const auto &call=row.at("allocations").at(allocations++);
      check(tag==number(call.at("arguments").at(0))&&bytes==number(call.at("arguments").at(1))&&
          zero==number(call.at("arguments").at(2)),"Original voice allocation request differs");
      return number(call.at("identity"));
    };
    s.release_allocation=[this](auto a){frees.push_back(a);};
    s.under_state_lock=[this](const auto &body){++state_guards;++depth;try{body();}catch(...){--depth;throw;}--depth;};
    s.under_irql_guard=[this](const auto &body){++irql_guards;++depth;try{body();}catch(...){--depth;throw;}--depth;};
    return s;
  }
};
std::uint32_t apply(const nlohmann::json &row,const SoundSdkVoiceInitializationData &d,
    const SoundSdkVoiceInitializationServices &s,const SoundDriverStartupObserver &observer){
  constexpr std::uint32_t identity=0x1001000;
  const auto kind=row.at("kind").get<std::string>();std::vector<std::uint32_t> a;
  for(const auto &v:row.at("arguments"))a.push_back(number(v));
  const SoundDriverFloatingPointContext fp{row.at("context").at(0).get<std::uint16_t>(),row.at("context").at(1).get<std::uint32_t>()};
  if(kind=="initialize")return source_sound_sdk_initialize_voice_data(identity,a.at(0),d,fp,s,observer);
  if(kind=="format")return source_sound_sdk_voice_format(identity,a.at(0),a.at(1),d,fp,s,observer);
  if(kind=="bins"){source_sound_sdk_voice_mix_bins(identity,a.at(0),d,s,observer);return 0;}
  if(kind=="buffer")return source_sound_sdk_voice_buffer(identity,a.at(0),a.at(1),d,s,observer);
  throw std::runtime_error("Unknown voice initializer fixture kind");
}
void compare(const nlohmann::json &row,Harness &h,std::uint32_t result){
  for(const auto &region:row.at("regions")){
    const auto expected=unhex(region.at("after_hex"));
    check(std::ranges::equal(h.memory(number(region.at("address")),expected.size()),expected),"Original SDK voice initialization output differs");
  }
  check(h.stores.size()==row.at("stores").size(),"Original SDK voice initialization store count differs");
  for(unsigned i=0;i<h.stores.size();++i){const auto &a=h.stores[i];const auto &e=row.at("stores").at(i);
    check(a.instruction==number(e.at("pc"))&&a.address==number(e.at("address"))&&a.bytes==e.at("size")&&
        a.before==number(e.at("before"))&&a.after==number(e.at("after")),"Original SDK voice initialization store order differs");
  }
  if(row.at("kind")!="bins")check(result==number(row.at("result")),"Original SDK voice initialization result differs");
  check(h.allocations==row.at("allocations").size()&&h.frees.size()==row.at("frees").size(),"Original SDK voice allocation count differs");
  for(unsigned i=0;i<h.frees.size();++i)check(h.frees[i]==number(row.at("frees").at(i)),"Original SDK voice release identity differs");
  unsigned state=0,irql=0;
  for(const auto &call:row.at("guards")){const auto pc=number(call);state+=pc==0x379cc9;irql+=pc==0x379c90;}
  check(h.state_guards==state&&h.irql_guards==irql&&!h.depth,"Original SDK voice guard count differs");
}
void native_heap(const nlohmann::json &row,const SoundSdkVoiceInitializationData &data){
  // Real host storage and locks, independent of the declared fixed-return
  // allocator used by fault controls. This is raw CPU composition, not boot.
  Harness h(row);constexpr std::uint32_t allocator=0xa1100001u;
  NativeSoundStorage storage({allocator,0x1008000,0x100f000});check(storage.page_size()==4096,"Original fixture alias granularity differs");
  for(const auto &region:row.at("regions")){
    const auto name=region.at("name").get<std::string>();if(name.starts_with("allocation_"))continue;
    const auto address=number(region.at("address"));const auto raw=unhex(region.at("before_hex"));
    storage.map_zeroed(address&~0xfffu,raw.size()+(address&0xfffu));std::ranges::copy(raw,storage.memory(address,raw.size()).begin());
  }
  constexpr std::uint32_t globals=0x2000000,statistics=globals+0x10;
  storage.map_zeroed(globals,4096);
  auto put=[&](auto address,auto value){const auto raw=storage.memory(address,4);for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));};
  put(globals+4,statistics);
  h.native_memory=[&](auto address,auto size){return storage.memory(address,size);};
  SoundCapabilitiesBindings bindings{};bindings.private_pool_pointer=globals;bindings.statistics_pointer=globals+4;
  SoundCapabilitiesServices allocation;allocation.memory=h.native_memory;
  const auto raw=storage.cache_services();std::map<std::uint32_t,std::uint32_t> capacities;
  allocation.raw_pool_allocate=[&](auto bytes,auto){
    check(bytes<=storage.page_size(),"Unrepresented native heap extent");const auto result=raw.raw_allocate(allocator,bytes);
    if(result)capacities.emplace(result,storage.memory(result,storage.page_size()).size());
    return result;
  };
  allocation.raw_pool_capacity=[&](auto identity){return capacities.at(identity);};
  auto services=h.services();services.allocate=[&](auto tag,auto bytes,auto zero){
    const auto identity=source_sound_sdk_allocate(tag,bytes,zero,bindings,allocation);
    check(h.allocations<row.at("allocations").size(),"Unexpected real voice allocation");
    const auto &expected=row.at("allocations").at(h.allocations++);
    check(identity==number(expected.at("identity"))&&tag==number(expected.at("arguments").at(0))&&
        bytes==number(expected.at("arguments").at(1))&&zero==number(expected.at("arguments").at(2)),"Real source voice allocation differs");
    return identity;
  };
  NativeSynchronization synchronization({0x3001000,0x3001100});const auto sync=synchronization.services();
  const auto mutex=sync.create_mutex(false);check(mutex.handle&&!mutex.error,"Native voice mutex unavailable");
  const auto guard=[&](const auto &body,bool irql){
    check(sync.wait(mutex.handle,0xffffffffu).state==NativeSynchronizationWaitState::Acquired,"Native voice lock failed");
    if(irql)++h.irql_guards;else ++h.state_guards;
    ++h.depth;try{body();}catch(...){--h.depth;check(!sync.release_mutex(mutex.handle),"Native voice unlock failed");throw;}
    --h.depth;check(!sync.release_mutex(mutex.handle),"Native voice unlock failed");
  };
  services.under_state_lock=[&](const auto &body){guard(body,false);};
  services.under_irql_guard=[&](const auto &body){guard(body,true);};
  const auto before=FloatingPointState{};const auto result=apply(row,data,services,[&](const auto &s){h.observe(s);});
  check(FloatingPointState{}==before,"Real voice initialization changed caller FP state");compare(row,h,result);
  check(capacities.size()==row.at("allocations").size(),"Actual OS voice allocation count differs");
}
} // namespace
int main(int argc,char **argv){
  try{
    check(argc==2,"Supply original SDK voice initialization fixture");nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("schema")=="ORIGINAL_SOUND_SDK_VOICE_INITIALIZATION_CPU_V1","Wrong voice initializer fixture schema");
    const auto data=import(fixture.at("data"));unsigned cases=0,stores=0,real=0,stops=0;
    for(const auto &row:fixture.at("cases"))try{
      Harness h(row);const auto services=h.services();const auto before=FloatingPointState{};
      const auto result=apply(row,data,services,[&](const auto &s){h.observe(s);});
      check(FloatingPointState{}==before,"Voice initialization changed caller FP state");compare(row,h,result);
      if(row.at("genuine").get<bool>()){native_heap(row,data);++real;}
      ++cases;stores+=h.stores.size();
    }catch(const std::exception &e){throw std::runtime_error(row.at("label").get<std::string>()+": "+e.what());}
    const auto find=[&](const char *label)->const nlohmann::json &{for(const auto &r:fixture.at("cases"))if(r.at("label")==label)return r;throw std::runtime_error("Missing initializer control");};
    const auto reject=[&](const char *label,const auto &change){
      const auto &row=find(label);Harness h(row);auto d=data;auto s=h.services();change(h,d,s);bool stopped=false;
      try{(void)apply(row,d,s,{});}catch(const std::runtime_error &){stopped=true;}
      check(stopped,"Unavailable voice initialization owner admitted");check(!h.depth,"Voice failure left guard held");++stops;
    };
    reject("spatial",[](auto &,auto &,auto &s){s.memory={};});
    reject("spatial",[](auto &,auto &,auto &s){s.allocate={};});
    reject("spatial",[](auto &,auto &,auto &s){s.under_state_lock={};});
    reject("spatial",[](auto &,auto &,auto &s){s.under_state_lock=[](const auto &){};});
    reject("spatial",[](auto &,auto &,auto &s){s.under_state_lock=[](const auto &body){body();body();};});
    reject("spatial",[](auto &h,auto &,auto &){h.retained.erase(0x1008004);});
    reject("spatial",[](auto &,auto &d,auto &){d.immutable.clear();});
    reject("parent_channels_2",[](auto &,auto &,auto &s){s.under_irql_guard={};});
    reject("buffer_flags00000000_size0_borrowed00000000",[](auto &,auto &,auto &s){s.release_allocation={};});
    reject("buffer_flags00000000_size32_borrowed00000000",[](auto &,auto &,auto &s){s.allocate={};});
    std::cout<<"PASS "<<cases<<" original voice initialization cases, "<<stores<<" ordered stores, "<<real
             <<" genuine cases with real OS heap/locks, "<<stops<<" owner stops; no live playback or DSP claim\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
