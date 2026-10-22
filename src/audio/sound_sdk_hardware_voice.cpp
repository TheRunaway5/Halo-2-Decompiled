#include "halo2/sound_sdk_hardware_voice.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 { namespace {
struct Retained {
  const SoundSdkDeviceMemory &memory;
  const SoundDriverStartupObserver &observer;
  std::span<std::byte> bytes(std::uint32_t address,unsigned size) const {
    if(!memory)throw std::runtime_error("Original SDK hardware voice memory unavailable");
    auto raw=memory(address,size);
    if(raw.size()!=size)throw std::runtime_error("Original SDK hardware voice extent differs");
    return raw;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4) const {
    const auto raw=bytes(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(i*8);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,unsigned size,std::uint32_t value) const {
    const auto before=read(address,size);const auto raw=bytes(address,size);
    for(unsigned i=0;i<size;++i)raw[i]=std::byte(value>>(i*8));
    if(observer)observer({pc,address,size,before,value});
  }
};
template<class Body> void locked(const SoundSdkDeviceServices &s,Body body){
  if(!s.under_device_lock)throw std::runtime_error("Original SDK hardware voice lock unavailable");
  bool invoked=false;
  s.under_device_lock([&]{
    if(invoked)throw std::runtime_error("Original SDK hardware voice lock body repeated");
    invoked=true;body();
  });
  if(!invoked)throw std::runtime_error("Original SDK hardware voice lock body skipped");
}
void wait_fifo(const SoundSdkDeviceServices &s){
  if(!s.fifo_available)throw std::runtime_error("Original SDK hardware voice FIFO unavailable");
  while((s.fifo_available()&0xfffffffcu)<0x48u){}
}
void command(const SoundSdkDeviceServices &s,std::uint32_t address,std::uint32_t value){
  if(!s.device_write)throw std::runtime_error("Original SDK hardware voice device owner unavailable");
  s.device_write(address,value);
}
} // namespace

std::uint32_t source_sound_sdk_allocate_voice_slots(std::uint32_t device,std::uint32_t voice,
    const SoundSdkDeviceServices &s,const SoundDriverStartupObserver &observer){
  std::uint32_t result=0;
  locked(s,[&]{
    const Retained r{s.memory,observer};
    r.write(0x37e74eu,device+0x84u,4,r.read(device+0x84u)+1u);
    const auto data=r.read(voice+0x70u);
    const bool spatial=(r.read(data+8u)&0x200010u)!=0;
    const auto count_address=r.read(device+(spatial?0x5cu:0x58u));
    const auto needed=r.read(voice+0x64u,1),available=r.read(count_address);
    if(needed>available)result=0x88780032u;
    else {
      r.write(0x37e77bu,count_address,4,available-needed);
      std::uint32_t slot=spatial?0u:0x40u,index=0;
      while(index<r.read(voice+0x64u,1)){
        const auto entry=device+0x88u+(slot&0xffffu)*4u;
        if(!r.read(entry)){
          r.write(0x37e798u,voice+0xcu+(index&0xffu)*2u,2,slot&0xffffu);
          r.write(0x37e79du,entry,4,voice);
          index=(index+1u)&0xffu;
        }
        ++slot;
      }
    }
    r.write(0x37e7b0u,device+0x84u,4,r.read(device+0x84u)-1u);
  });
  return result;
}

void source_sound_sdk_hardware_bin_commands(std::uint32_t voice,std::span<std::uint32_t,1> first,
    std::uint32_t second,const SoundSdkDeviceMemory &memory,const SoundDriverStartupObserver &observer){
  const Retained r{memory,observer};const auto data=r.read(voice+0x70u);
  const auto count=r.read(data+0x24u);
  if(count>8u)throw std::runtime_error("Original SDK hardware bin scratch extent exceeded");
  std::array<std::uint32_t,8> bins{};
  for(std::uint32_t i=0;i<count;++i)bins[i]=r.read(data+0x28u+i,1);
  auto length=count;
  for(std::uint32_t candidate=0;length<8u;++candidate){
    bool found=false;
    for(std::uint32_t i=0;i<length;++i)if(bins[i]==candidate){found=true;break;}
    if(!found)bins[length++]=candidate;
  }
  first[0]=0;std::uint32_t packed=0;
  for(unsigned i=0;i<3;++i)packed|=bins[i]<<(i*5u);
  first[0]=packed;
  for(unsigned i=3;i<6;++i)packed|=bins[i]<<(i*5u+1u);
  first[0]=packed;
  r.write(0x380b6cu,second,2,r.read(second,2)&0xfc00u);
  auto tail=r.read(second);
  tail|=bins[6];tail|=bins[7]<<5u;
  r.write(0x380b8fu,second,4,tail);
}

std::uint32_t source_sound_sdk_activate_hardware_voice(std::uint32_t voice,
    const SoundSdkHardwareVoiceBindings &bindings,const SoundSdkHardwareVoiceServices &s,
    const SoundDriverStartupObserver &observer){
  std::uint32_t result=0;
  locked(s.device,[&]{
    const Retained r{s.device.memory,observer};
    if(r.read(voice+0x12u,1)&1u)return;
    result=source_sound_sdk_allocate_voice_slots(r.read(voice+8u),voice,s.device,observer);
    if(std::bit_cast<std::int32_t>(result)<0)return;
    const auto first_slot=r.read(voice+0xcu,2);
    const auto mapping=r.read(bindings.voice_mapping_pointer_address)+((first_slot*4u+2u)<<4u);
    r.write(0x381a81u,voice+0x68u,4,mapping);
    r.write(0x381a87u,voice+0x6cu,4,4);
    std::uint32_t index=0,offset=0;
    while(index<r.read(voice+0x6cu)){
      r.write(0x3808f0u,r.read(voice+0x68u)+offset+0xfu,1,0x80u);
      ++index;offset+=0x10u;
    }
    r.write(0x3808beu,voice+0x12u,2,r.read(voice+0x12u,2)|1u);
    std::array<std::uint32_t,1> bins{};
    source_sound_sdk_hardware_bin_commands(voice,bins,voice+0x14u,s.device.memory,observer);
    std::array<std::uint32_t,9> gain{};
    source_sound_sdk_gain_commands(voice,gain,s.device.memory);
    const auto pitch=source_sound_sdk_pitch_command(voice,s.device.memory);
    for(std::uint32_t i=0;i<r.read(voice+0x64u,1);++i){
      if(i>=3u)throw std::runtime_error("Original SDK hardware voice command scratch extent exceeded");
      wait_fifo(s.device);
      command(s.device,0xfe8202f8u,r.read(voice+0xcu+i*2u,2));
      const auto format=r.read(voice+0x14u);
      command(s.device,0xfe820304u,i?format|0x400000u:format);
      constexpr std::array<std::array<std::uint32_t,2>,10> registers{{
          {0xfe820318,0x18},{0xfe820308,0x1c},{0xfe82030c,0x20},{0xfe820310,0x24},
          {0xfe820314,0x28},{0xfe820350,0x2c},{0xfe82036c,0x30},{0xfe820370,0x34},
          {0xfe820374,0x38},{0xfe820378,0x3c}}};
      for(const auto &entry:registers)command(s.device,entry[0],r.read(voice+entry[1]));
      command(s.device,0xfe820300u,bins[0]);
      command(s.device,0xfe820360u,gain[i]);
      command(s.device,0xfe820364u,gain[i+3u]);
      command(s.device,0xfe820368u,gain[i+6u]);
      command(s.device,0xfe82037cu,pitch);
      command(s.device,0xfe82031cu,0xffffu);
    }
    const auto data=r.read(voice+0x70u);
    if(r.read(data+8u)&0x200010u){
      if(!s.apply_hrtf)throw std::runtime_error("Original SDK hardware voice HRTF owner unavailable");
      s.apply_hrtf(voice);
    }
  });
  return result;
}
} // namespace halo2
