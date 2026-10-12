#include "halo2/sound_driver_startup.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace halo2 {
namespace {
template<class F> const F &need(const F &f,const char *message){if(!f)throw std::runtime_error(message);return f;}
class Owner {
 SoundDriverStartupState &s;const SoundDriverStartupConstants &c;const SoundDriverStartupServices &p;const SoundDriverStartupObserver &observe;
 std::span<std::byte> memory(std::uint32_t a,std::size_t n)const{auto b=need(p.memory,"Source driver storage unavailable")(a,n);if(b.size()!=n)throw std::runtime_error("Source driver storage extent unavailable");return b;}
 std::uint32_t read(std::uint32_t a,unsigned n=4)const{auto b=memory(a,n);std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;}
 std::int32_t signed_short(std::uint32_t a)const{return std::bit_cast<std::int16_t>(std::uint16_t(read(a,2)));}
 void write(std::uint32_t pc,std::uint32_t a,std::uint32_t v,unsigned n=4){const auto before=read(a,n);auto b=memory(a,n);for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(8*i));if(observe)observe({pc,a,n,before,v&(n==4?0xffffffffu:((1u<<(8*n))-1))});}
 void global(std::uint32_t pc,std::uint32_t a,std::uint32_t &field,std::uint32_t v){const auto before=field;field=v;if(observe)observe({pc,a,4,before,v});}
 std::uint32_t allocate(std::uint32_t extent,std::uint32_t alignment,std::uint32_t pc){
  const auto position=std::uint64_t(s.arena_base)+s.arena_used,aligned=(position+alignment-1)&~std::uint64_t(alignment-1),amount=aligned-position+extent;
  if(aligned>std::numeric_limits<std::uint32_t>::max()||std::uint64_t(s.arena_used)+amount>std::numeric_limits<std::uint32_t>::max())throw std::runtime_error("Source driver raw address space unavailable");
  memory(std::uint32_t(aligned),extent);global(pc,0x517ff4,s.arena_used,s.arena_used+std::uint32_t(amount));
  auto crc=s.arena_crc;for(unsigned i=0;i<4;++i){auto table=(crc^std::uint32_t(amount>>(8*i)))&255;for(unsigned bit=0;bit<8;++bit)table=(table>>1)^((table&1)?0xedb88320u:0u);crc=(crc>>8)^table;}
  global(0x1656ed,0x517ff8,s.arena_crc,crc);return std::uint32_t(aligned);
 }
 std::uint32_t device()const{return read(s.driver_address+0x2ab0);}
 std::int32_t device_operation(SoundDriverDeviceOperation op,std::uint32_t a,std::uint32_t b){return need(p.native_device_operation,"Native source device operation unavailable")({op,op==SoundDriverDeviceOperation::LightHrtf4Channel?0:device(),a,b});}
 void buffer_operation(SoundDriverBufferOperation op,std::uint32_t identity,std::array<std::uint32_t,3> words={},std::span<const std::uint32_t> bins={}){need(p.native_buffer_operation,"Native source buffer operation unavailable")({op,identity,words,bins});}
 bool buffer(std::uint32_t output,std::uint32_t flags,std::uint32_t input){return need(p.native_buffer_create,"Native source buffer constructor unavailable")({device(),output,0,{0x18,flags,0,0,0,input}})>=0;}
 void cleanup(){
  need(p.source_flush_streams,"Original2213A0 stream flush owner unavailable")(s);
  auto driver=s.driver_address;
  if(driver+0xc){for(std::int32_t i=0;i<signed_short(driver+4);++i){const auto slot=driver+std::uint32_t(i)*0x34+0x30,identity=read(slot);if(identity){need(p.native_stream_release,"Native source stream release unavailable")(identity);driver=s.driver_address;write(0x220c28,slot,0);}}write(0x220c36,driver+4,0,2);}
  if(driver+0x1a18){for(std::int32_t i=0;i<signed_short(driver+6);++i){const auto record=driver+std::uint32_t(i)*0x3c+0x1a18;for(unsigned j=0;j<2;++j){const auto slot=record+0x34+4*j,identity=read(slot);if(identity){need(p.native_buffer_release,"Native source buffer release unavailable")(identity);driver=s.driver_address;write(j?0x220c82:0x220c6d,slot,0);}}}}
  if(const auto identity=read(driver+0x2ab0)){need(p.native_device_release,"Native source device release unavailable")(identity);write(0x220ca7,s.driver_address+0x2ab0,0);}
  need(p.source_hardware_queue_shutdown,"Original2B4470 hardware queue shutdown owner unavailable")(s);
  global(0x220cbf,0x51842c,s.hardware_queue_address,0);write(0x220cc5,s.driver_address,0,1);
 }
 bool failed(){cleanup();return false;}
public:
 Owner(SoundDriverStartupState &state,const SoundDriverStartupConstants &constants,const SoundDriverStartupServices &services,const SoundDriverStartupObserver &observer):s(state),c(constants),p(services),observe(observer){}
 bool run(std::span<const std::byte,16> policy){
  const auto driver=allocate(0x2ad8,4,0x2205fd);for(unsigned i=0;i<0xab6;++i)write(0x220611,driver+4*i,0);
  global(0x220615,0x526474,s.driver_address,driver);write(0x22061b,driver,0,1);
  if(need(p.native_device_create,"Native source device constructor unavailable")({0,driver+0x2ab0,0})<0)return failed();
  {auto b=memory(s.driver_address+0x2aa0,16);if(need(p.native_device_capabilities,"Actual source-compatible device capabilities unavailable")({device(),s.driver_address+0x2aa0,std::span<std::byte,16>(b.data(),16)})<0)return failed();}
  if(device_operation(SoundDriverDeviceOperation::DistanceFactor,0x4043126f,0)<0)return failed();
  if(device_operation(SoundDriverDeviceOperation::RolloffFactor,0,0)<0)return failed();
  const auto saved=need(p.saved_audio_setting,"Actual saved audio setting owner unavailable")({device()});
  if(saved.result<0)return failed();
  if(!saved.value)throw std::runtime_error("Actual saved audio setting value unavailable");
  const auto setting=*saved.value;
  write(0x2206d4,s.driver_address+1,((setting&0xffffu)==2&&(setting&0x10000u))?1:0,1);
  for(unsigned i=0;i<32;++i)device_operation(SoundDriverDeviceOperation::MixBinHeadroom,i,0);
  device_operation(SoundDriverDeviceOperation::DopplerFactor,0,1);device_operation(SoundDriverDeviceOperation::LightHrtf4Channel,0,0);
  if(!need(p.source_environment_construct,"Original192D30 DSP/environment owner unavailable")(s,device()))return failed();
  std::array<std::byte,56> listener{};std::copy(c.basis_forward.begin(),c.basis_forward.end(),listener.begin()+12);std::copy(c.basis_up.begin(),c.basis_up.end(),listener.begin()+24);
  need(p.source_listener_install,"Original2228A0 listener install owner unavailable")(s,listener);
  const auto queue=allocate(0xcc,16,0x12d7e4);global(0x12d7f0,0x51842c,s.hardware_queue_address,queue);const auto old=s.hardware_queue_flag;s.hardware_queue_flag=0;if(observe)observe({0x12d7f6,0x518430,1,old,0});
  global(0x2207a0,0x4865dc,s.queue_4865dc,0);global(0x2207a6,0x4865ec,s.queue_4865ec,0);global(0x2207ac,0x4865e8,s.queue_budget,0x10000);
  auto count=[&](unsigned at){return std::bit_cast<std::int16_t>(std::uint16_t(std::to_integer<unsigned>(policy[at])|(std::to_integer<unsigned>(policy[at+1])<<8)));};
  bool complete=true;
  for(unsigned category=0;category<3;++category){auto current=s.driver_address;const auto first=signed_short(current+4);write(0x2207c9,current+0x1a0c+2*category,std::uint32_t(first),2);write(0x2207d0,current+4,read(current+4,2)+std::uint16_t(count(2*category)),2);
   for(std::int32_t i=0;complete&&i<count(2*category);++i)complete=need(p.source_stream_construct,"Original2215F0 stream CPU owner unavailable")(s,std::uint32_t(first+i),category);
   write(0x22082b,s.driver_address+0x1a12+2*category,read(s.driver_address+4,2)-1,2);
  }
  write(0x220840,s.driver_address+6,std::uint16_t(count(12)),2);write(0x220844,s.driver_address+8,std::uint16_t(count(12)),2);
  for(std::int32_t i=0;complete&&i<count(12);++i)complete=need(p.source_static_buffer_construct,"Original222B80 static buffer CPU owner unavailable")(s,std::uint32_t(i));
  need(p.source_effect_buffer_mapping,"Original192E50 effect mapping owner unavailable")(s,device());
  const auto params=s.driver_address;write(0x22089d,params+0x2a0c,0);write(0x2208a3,params+0x2a10,0xc0c0a8c1);write(0x2208a9,params+0x2a14,0);write(0x2208af,params+0x2a18,0xc0c0a8c1);write(0x2208b5,params+0x2a1c,0xc0c0a8c1);write(0x2208bb,params+0x2a20,0xc0400000);write(0x2208c5,params+0x2a24,0xc0c0a8c1);write(0x2208cb,params+0x2a38,0xc0c0a8c1);
  for(unsigned i=0;i<3;++i)write(0x2208d1+10*i,params+0x2a4c+4*i,0x17+i);
  for(unsigned i=0;i<3;++i){const auto current=s.driver_address,bin=read(current+0x2a4c+4*i),first=current+0x2a58+4*i,second=current+0x2a64+4*i;
   if(buffer(first,0x100000,bin))buffer_operation(SoundDriverBufferOperation::Play,read(first),{0,0,0});
   if(buffer(second,0x100010,bin)){auto identity=read(second);buffer_operation(SoundDriverBufferOperation::MaxDistance,identity,{0x7f7fffff,1,0});buffer_operation(SoundDriverBufferOperation::MinDistance,read(second),{0x7f7fffff,1,0});buffer_operation(SoundDriverBufferOperation::Play,read(second),{0,0,0});}
  }
  if(auto identity=read(s.driver_address+0x2ab4))need(p.native_plugin_release,"Native source routing plugin release unavailable")(identity);
  const auto current=s.driver_address;const auto output_policy=std::uint8_t(read(current+1,1));write(0x2209fe,current+0x2ab4,c.routing_plugin_identity);
  {auto b=memory(current+0x2a0c,64);need(p.native_plugin_initialize,"Native source routing plugin constructor unavailable")({c.routing_plugin_identity,current+0x2a0c,current+0x2a4c,output_policy,std::span<const std::byte,64>(b.data(),64)});}
  {const auto identity=read(s.driver_address+0x2ab4);const auto policy_byte=std::uint8_t(read(s.driver_address+1,1));auto b=memory(s.runtime_environment_parameters_address,44);need(p.native_plugin_environment,"Native source environment plugin operation unavailable")({identity,s.runtime_environment_parameters_address,policy_byte,std::span<const std::byte,44>(b.data(),44)});}
  for(unsigned i=0;i<8;++i){const auto slot=s.driver_address+0x2a70+4*i;if(buffer(slot,0x100000,0xf+i)){auto identity=read(slot);buffer_operation(SoundDriverBufferOperation::Volume,identity,{0,0,0});const std::array<std::uint32_t,2> bins{6+i%4,0};buffer_operation(SoundDriverBufferOperation::MixBins,read(slot),{1,0,0},bins);buffer_operation(SoundDriverBufferOperation::Play,read(slot),{0,0,0});}}
  for(unsigned i=0;i<2;++i){need(p.source_feedback_buffer_construct,"Original2235A0 feedback buffer CPU owner unavailable")(s,s.driver_address+0x2a90+4*i);write(0x220b6f,s.driver_address+0x2a98+4*i,0xffffffff);}
  auto final_driver=s.driver_address;for(unsigned i=0;i<4;++i)write(0x220b8f+8*i,final_driver+0x2abc+4*i,c.one_bits);write(0x220bb2,final_driver+0x2acc,8000);write(0x220bbc,final_driver+0x2ad0,0);
  if(!complete)return failed();
  write(0x220bc9,final_driver,1,1);return true;
 }
};
}
bool source_sound_driver_startup(SoundDriverStartupState &s,std::span<const std::byte,16> policy,const SoundDriverStartupConstants &c,const SoundDriverStartupServices &p,const SoundDriverStartupObserver &observe){return Owner(s,c,p,observe).run(policy);}
} // namespace halo2
