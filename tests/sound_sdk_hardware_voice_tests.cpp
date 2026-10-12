#include "halo2/sound_sdk_hardware_voice.hpp"
#include "halo2/sound_sdk_hrtf.hpp"
#include "halo2/sound_sdk_spatial_data.hpp"
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
  const auto text=v.get<std::string>();std::vector<std::byte> raw;
  check(text.size()%2==0,"Invalid hardware voice fixture bytes");
  for(std::size_t i=0;i<text.size();i+=2)raw.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return raw;
}
std::uint32_t word(std::span<const std::byte> raw,std::size_t at=0){
  check(at+4<=raw.size(),"Hardware voice word exceeds backing");std::uint32_t value=0;
  for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[at+i])<<(i*8);
  return value;
}
struct FloatingPointState {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  unsigned short control;unsigned mxcsr;
  FloatingPointState():mxcsr(_mm_getcsr()){asm volatile("fnstcw %0":"=m"(control));}
#endif
  friend bool operator==(const FloatingPointState &,const FloatingPointState &)=default;
};
SoundSdkSpatialData immutable_data(const nlohmann::json &fixture){
  const auto &ranges=fixture.at("ranges");SoundSdkSpatialData d{};auto &c=d.constants;
  const auto get=[&](unsigned i){return word(unhex(ranges.at(i).at("hex")));};
  const std::array<std::uint32_t *,10> fields{&c.zero_bits,&c.one_bits,&c.half_bits,&c.position_sentinel_bits,
    &c.angle_round_bias_bits,&c.half_turn_bits,&c.azimuth_scale_bits,&c.octant_bits,&c.quarter_turn_bits,&c.volume_scale_bits};
  for(unsigned i=0;i<fields.size();++i)*fields[i]=get(i);
  for(unsigned i=0;i<3;++i){c.mode_one_side[i]=word(unhex(ranges.at(10).at("hex")),i*4);
    c.mode_one_forward[i]=word(unhex(ranges.at(11).at("hex")),i*4);}
  c.kernel_table_a=number(ranges.at(12).at("base_address"));c.kernel_table_b=number(ranges.at(13).at("base_address"));
  d.null_kernel_identity=number(ranges.at(14).at("base_address"));
  const auto kernels=unhex(ranges.at(12).at("hex")),null=unhex(ranges.at(14).at("hex"));
  check(kernels.size()==d.kernels.size()&&null.size()==d.null_kernel.size(),"Hardware voice immutable extent differs");
  std::ranges::copy(kernels,d.kernels.begin());std::ranges::copy(null,d.null_kernel.begin());return d;
}
struct Harness {
  const nlohmann::json &row;
  std::map<std::uint32_t,std::vector<std::byte>> retained;
  SoundSdkDeviceMemory native_memory;
  std::function<void(const std::function<void()> &)> native_guard;
  std::vector<SoundDriverStartupStore> stores;
  std::vector<std::pair<std::uint32_t,std::uint32_t>> writes;
  unsigned depth=0,locks=0,fifo_reads=0,hrtf_calls=0;
  explicit Harness(const nlohmann::json &r):row(r){
    for(const auto &x:r.at("regions"))retained.emplace(number(x.at("address")),unhex(x.at("before_hex")));
  }
  std::span<std::byte> memory(std::uint32_t address,std::size_t size){
    if(native_memory)return native_memory(address,size);
    const auto after=retained.upper_bound(address);check(after!=retained.begin(),"Unknown hardware voice identity");
    const auto found=std::prev(after);const auto offset=std::uint64_t(address)-found->first;
    check(offset+size<=found->second.size(),"Hardware voice extent differs");
    return std::span<std::byte>(found->second).subspan(offset,size);
  }
  void action(const nlohmann::json &a){
    const auto raw=unhex(a.at("hex"));std::ranges::copy(raw,memory(a.at("address"),raw.size()).begin());
  }
  void observe(const SoundDriverStartupStore &store){
    stores.push_back(store);
    for(const auto &a:row.at("actions"))if(!a.contains("after_device_words")&&number(a.at("pc"))==store.instruction)action(a);
  }
  SoundSdkHardwareVoiceServices services(const SoundSdkSpatialData &data,const SoundDriverFloatingPointContext &fp){
    SoundSdkHardwareVoiceServices s;
    s.device.memory=[this](auto a,auto n){return memory(a,n);};
    s.device.under_device_lock=[this](const auto &body){
      const auto invoke=[&]{++locks;++depth;try{body();}catch(...){--depth;throw;}--depth;};
      if(native_guard)native_guard(invoke);else invoke();
    };
    // Declared FIFO/command boundary. Real native locks/storage are separately
    // composed below; this provider is not a native MCPX device or PCM output.
    s.device.fifo_available=[this]{check(depth,"FIFO outside hardware voice lock");++fifo_reads;return fifo_reads==1?0u:0x100u;};
    s.device.device_write=[this](auto a,auto value){
      check(depth&&fifo_reads>=2,"Hardware command before declared FIFO credits");writes.emplace_back(a,value);
      for(const auto &x:row.at("actions"))if(x.contains("after_device_words")&&x.at("after_device_words")==writes.size())action(x);
    };
    SoundSdkHrtfServices h;h.memory=s.device.memory;h.under_device_lock=s.device.under_device_lock;
    h.fifo_available=s.device.fifo_available;h.device_write=s.device.device_write;
    h.select_kernel=[this,&data,fp](auto azimuth,auto,auto fold){check(depth,"HRTF selection outside lock");return source_sound_sdk_kernel(azimuth,fold,data.constants,fp);};
    h.kernel_bytes=[this,&data](auto identity){check(depth,"HRTF bytes outside lock");return data.kernel(identity);};
    s.apply_hrtf=[this,&data,fp,h](auto voice){
      ++hrtf_calls;source_sound_sdk_hrtf_apply(voice,0,0,{0x386b04,data.null_kernel_identity},data.constants,fp,h,[this](const auto &v){observe(v);});
    };
    return s;
  }
};
void compare(const nlohmann::json &row,Harness &h,std::uint32_t result){
  for(const auto &region:row.at("regions")){
    const auto expected=unhex(region.at("after_hex"));
    check(std::ranges::equal(h.memory(number(region.at("address")),expected.size()),expected),"Original SDK hardware voice output differs");
  }
  check(h.stores.size()==row.at("stores").size(),"Original SDK hardware voice store count differs");
  for(unsigned i=0;i<h.stores.size();++i){const auto &v=h.stores[i];const auto &e=row.at("stores").at(i);
    check(v.instruction==number(e.at("pc"))&&v.address==number(e.at("address"))&&v.bytes==e.at("size")&&
        v.before==number(e.at("before"))&&v.after==number(e.at("after")),"Original SDK hardware voice store order differs");
  }
  check(h.writes.size()==row.at("writes").size(),"Original SDK hardware voice command count differs");
  for(unsigned i=0;i<h.writes.size();++i){const auto &e=row.at("writes").at(i);
    check(h.writes[i]==std::pair{number(e.at("address")),number(e.at("value"))},"Original SDK hardware voice command differs");}
  check(result==number(row.at("result")),"Original SDK hardware voice result differs");
  unsigned locks=0;for(const auto &x:row.at("locks"))locks+=number(x)==0x379c90;
  check(h.locks==locks&&!h.depth,"Original SDK hardware voice lock count differs");
}
void execute(const nlohmann::json &row,const SoundSdkSpatialData &data,bool native){
  Harness h(row);const SoundDriverFloatingPointContext fp{row.at("context").at(0).get<std::uint16_t>(),row.at("context").at(1).get<std::uint32_t>()};
  NativeSoundStorage storage({0xa1100001,0x2000000,0x2100000});
  NativeSynchronization synchronization({0x3001000,0x3001100});const auto sync=synchronization.services();
  std::uint32_t mutex=0;
  if(native){
    for(const auto &r:row.at("regions")){
      const auto address=number(r.at("address"));const auto raw=unhex(r.at("before_hex"));
      storage.map_zeroed(address&~0xfffu,raw.size()+(address&0xfffu));std::ranges::copy(raw,storage.memory(address,raw.size()).begin());
    }
    h.native_memory=[&](auto a,auto n){return storage.memory(a,n);};
    const auto created=sync.create_mutex(false);check(created.handle&&!created.error,"Native hardware voice mutex unavailable");mutex=created.handle;
    h.native_guard=[&](const auto &body){
      check(sync.wait(mutex,0xffffffffu).state==NativeSynchronizationWaitState::Acquired,"Native hardware voice lock failed");
      try{body();}catch(...){check(!sync.release_mutex(mutex),"Native hardware voice unlock failed");throw;}
      check(!sync.release_mutex(mutex),"Native hardware voice unlock failed");
    };
  }
  const FloatingPointState before;
  const auto result=source_sound_sdk_activate_hardware_voice(0x1001000,{0x3872c0},h.services(data,fp),[&](const auto &v){h.observe(v);});
  check(FloatingPointState{}==before,"Original SDK hardware voice leaked floating-point state");compare(row,h,result);
}
} // namespace
int main(int argc,char **argv){
  try{
    check(argc==3,"Supply original hardware voice and immutable fixtures");nlohmann::json fixture,immutable;
    std::ifstream(argv[1])>>fixture;std::ifstream(argv[2])>>immutable;
    check(fixture.at("schema")=="ORIGINAL_SOUND_SDK_HARDWARE_VOICE_V1","Wrong hardware voice schema");
    const auto data=immutable_data(immutable);unsigned genuine=0,stores=0,words=0;
    for(const auto &row:fixture.at("cases")){
      try{execute(row,data,false);if(row.at("genuine")){execute(row,data,true);++genuine;}}
      catch(const std::exception &e){throw std::runtime_error(row.at("label").get<std::string>()+": "+e.what());}
      stores+=row.at("stores").size();words+=row.at("writes").size();
    }
    const auto &row=fixture.at("cases").at(1);unsigned stopped=0;
    for(unsigned failure=0;failure<8;++failure){
      Harness h(row);auto s=h.services(data,{0x23f,0x1fa0});
      switch(failure){
        case 0:s.device.memory={};break;case 1:s.device.under_device_lock={};break;
        case 2:s.device.under_device_lock=[](const auto &){};break;
        case 3:s.device.under_device_lock=[&](const auto &body){
          ++h.depth;try{body();body();}catch(...){--h.depth;throw;}--h.depth;
        };break;
        case 4:s.device.fifo_available={};break;case 5:s.device.device_write={};break;
        case 6:s.apply_hrtf={};break;
        case 7:s.device.memory=[&](auto a,auto n){auto raw=h.memory(a,n);return raw.first(raw.size()-1);};break;
      }
      bool failed=false;try{source_sound_sdk_activate_hardware_voice(0x1001000,{0x3872c0},s);}
      catch(const std::exception &){failed=true;}check(failed&&!h.depth,"Hardware voice missing owner silently succeeded");++stopped;
    }
    check(genuine==2,"Missing genuine hardware voice cases");
    std::cout<<"PASS "<<fixture.at("cases").size()<<" original hardware activation cases, "<<stores<<" ordered stores, "<<words
      <<" device words, "<<genuine<<" genuine cases with real OS storage/locks, "<<stopped<<" owner stops; no live output or DSP claim\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
