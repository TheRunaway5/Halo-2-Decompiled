#include "halo2/sound_sdk_device.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
struct Retained {
  const SoundSdkDeviceMemory &provider;
  std::span<std::byte> memory(std::uint32_t address,std::size_t size)const{
    if(!provider)throw std::runtime_error("Original SDK device retained memory unavailable");
    auto value=provider(address,size);
    if(value.size()!=size)throw std::runtime_error("Original SDK device retained backing extent differs");
    return value;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4)const{
    const auto raw=memory(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(i*8);
    return value;
  }
  void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,
      const SoundDriverStartupObserver &observer)const{
    const auto old=read(address);auto raw=memory(address,4);
    for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(i*8));
    if(observer)observer({pc,address,4,old,value});
  }
};
std::uint32_t clamp_volume(std::uint32_t word){
  const auto value=std::bit_cast<std::int32_t>(word);
  return value < -10000 ? std::bit_cast<std::uint32_t>(std::int32_t(-10000)) : value > 0 ? 0u : word;
}
template<class Body> void locked(const SoundSdkDeviceServices &services,Body body){
  if(!services.under_device_lock)throw std::runtime_error("Original SDK device lock unavailable");
  unsigned calls=0;
  services.under_device_lock([&]{
    if(calls++)throw std::runtime_error("Original SDK device lock callback repeated");
    body();
  });
  if(calls!=1)throw std::runtime_error("Original SDK device lock callback not executed");
}
void wait_fifo(const SoundSdkDeviceServices &services,std::uint32_t required){
  if(!services.fifo_available)throw std::runtime_error("Original SDK device FIFO unavailable");
  while((services.fifo_available()>>2)<required){}
}
void command(const SoundSdkDeviceServices &services,std::uint32_t address,std::uint32_t value){
  if(!services.device_write)throw std::runtime_error("Original SDK device command provider unavailable");
  services.device_write(address,value);
}
} // namespace

void source_sound_sdk_gain_commands(std::uint32_t voice,std::span<std::uint32_t,9> output,
    const SoundSdkDeviceMemory &memory){
  const Retained r{memory};const auto data=r.read(voice+0x70u);
  const auto parameters=r.read(data+0xb8u);
  const bool spatial=parameters&&(r.read(data+0xbu,1)&1u);
  std::array<std::uint32_t,5> volumes{};
  if(spatial){
    volumes[0]=clamp_volume(r.read(parameters+8u)+r.read(parameters+4u));
    volumes[1]=clamp_volume(r.read(parameters+0x1cu)+r.read(parameters+0x14u)+r.read(parameters+0xcu)+volumes[0]);
    volumes[2]=clamp_volume(r.read(parameters+0x1cu)+r.read(parameters+0x10u)+volumes[0]);
    volumes[3]=clamp_volume(r.read(parameters+0x1cu)+r.read(parameters+0x18u)+r.read(parameters+0xcu)+volumes[0]);
    volumes[4]=clamp_volume(r.read(parameters+0x20u)+volumes[0]);
  }
  const auto count=r.read(voice+0x64u,1);
  if(!count||count>3u)throw std::runtime_error("Original SDK gain caller scratch/count unavailable");
  const auto per_voice=r.read(data+0x24u)/count;
  for(std::uint32_t index=0;index<count;++index){
    std::array<std::uint32_t,8> gain{};
    for(std::uint32_t bin=0;bin<8u;++bin){
      gain[bin]=0xfffu;
      if(bin<index*per_voice||bin>=(index+1u)*per_voice)continue;
      const auto selected=r.read(data+0x28u+bin,1);
      auto attenuation=0u-(r.read(data+0x30u+selected*4u)+r.read(data+0x1cu));
      if(spatial){
        const auto group=selected==2u?3u:selected<=5u?0u:selected<=7u?1u:
                         selected<=9u?2u:selected==10u?4u:0u;
        attenuation-=volumes[group];
      }
      const auto packed=(attenuation<<6)/100u;
      gain[bin]=packed>0xfffu?0xfffu:packed;
    }
    // Original380D47..380DC4 divides the last two twelve-bit gains into
    // nibbles across three words. Every operation uses 32-bit wrap semantics.
    auto first=(gain[7]&0xfu)|(gain[1]<<4);
    first=(first<<12)|(gain[0]&0xfffu);
    output[index]=(first<<4)|(gain[6]&0xfu);
    auto second=(gain[7]&0xf0u)|(gain[3]<<8);
    second=(second<<8)|(gain[2]&0xfffu);
    output[index+3u]=(second<<4)|((gain[6]>>4)&0xfu);
    auto third=(gain[7]&0xf00u)|(gain[5]<<12);
    third=(third<<4)|(gain[4]&0xfffu);
    output[index+6u]=(third<<4)|((gain[6]>>8)&0xfu);
  }
}

std::uint32_t source_sound_sdk_pitch_command(std::uint32_t voice,const SoundSdkDeviceMemory &memory){
  const Retained r{memory};const auto data=r.read(voice+0x70u),flags=r.read(data+8u);
  if(flags&0x182000u)return 0;
  auto value=r.read(data+0x18u);const auto parameters=r.read(data+0xb8u);
  if(parameters&&(flags&0x1000000u))value+=r.read(parameters+0x24u);
  const auto parent=r.read(data+0xb0u);
  if(parent){
    const auto owner=r.read(parent+0x20u);
    if(owner){
      const auto retained=r.read(owner+0x80u);
      value+=r.read(retained+0x18u);
      const auto extra=r.read(retained+0xb8u);
      if(extra)value+=r.read(extra+0x24u);
    }
  }
  const auto signed_value=std::bit_cast<std::int32_t>(value);
  if(signed_value < -32767)value=std::bit_cast<std::uint32_t>(std::int32_t(-32767));
  else if(signed_value>8191)value=8191u;
  return value<<16;
}

std::uint32_t source_sound_sdk_gain_apply(std::uint32_t voice,const SoundSdkDeviceServices &services){
  locked(services,[&]{
    const Retained r{services.memory};
    if(!(r.read(voice+0x12u,1)&1u))return;
    std::array<std::uint32_t,9> words{};
    source_sound_sdk_gain_commands(voice,words,services.memory);
    const auto count=r.read(voice+0x64u,1);
    wait_fifo(services,count*6u);
    if(!count)return;
    for(std::uint32_t i=0;;){
      if(i>=3u)throw std::runtime_error("Original SDK gain command scratch extent exceeded");
      command(services,0xfe8202f8u,r.read(voice+0xcu+i*2u,2));
      command(services,0xfe8202fcu,1);
      command(services,0xfe820360u,words[i]);
      command(services,0xfe820364u,words[i+3u]);
      command(services,0xfe820368u,words[i+6u]);
      command(services,0xfe8202fcu,0);
      const auto next_count=r.read(voice+0x64u,1);
      if(++i>=next_count)break;
    }
  });
  return 0;
}

std::uint32_t source_sound_sdk_pitch_apply(std::uint32_t voice,const SoundSdkDeviceServices &services){
  locked(services,[&]{
    const Retained r{services.memory};const auto data=r.read(voice+0x70u);
    if(r.read(data+8u)&0x182000u){
      const auto sentinel=voice+0x44u;auto link=r.read(sentinel);
      while(link!=sentinel){
        source_sound_sdk_pitch_apply(link-0x5cu,services);
        link=r.read(link);
      }
    }else if(r.read(voice+0x12u,1)&1u){
      const auto pitch=source_sound_sdk_pitch_command(voice,services.memory);
      const auto count=r.read(voice+0x64u,1);
      wait_fifo(services,count*2u);
      if(!count)return;
      for(std::uint32_t i=0;;){
        command(services,0xfe8202f8u,r.read(voice+0xcu+i*2u,2));
        command(services,0xfe82037cu,pitch);
        const auto next_count=r.read(voice+0x64u,1);
        if(++i>=next_count)break;
      }
    }
  });
  return 0;
}

std::uint32_t source_sound_sdk_filter_apply(std::uint32_t voice,std::uint32_t filter_override,
    const SoundSdkDeviceServices &services,const SoundDriverStartupObserver &observer){
  locked(services,[&]{
    const Retained r{services.memory};std::array<std::uint32_t,6> filter{};
    if(filter_override){
      for(unsigned i=0;i<6;++i)filter[i]=r.read(filter_override+i*4u);
    }else{
      filter={r.read(voice+0x1au,1)&3u,(r.read(voice+0x18u)>>18)&7u,
        r.read(voice+0x38u)&0xffffu,r.read(voice+0x3au,2),
        r.read(voice+0x3cu)&0xffffu,r.read(voice+0x3eu,2)};
    }
    const auto data=r.read(voice+0x70u);
    if((r.read(data+8u)&0x200010u)&&filter[0]==0)filter[0]=3;
    const auto selected=(((filter[1]&7u)<<2)|(filter[0]&3u))<<16;
    const auto packed=selected|(r.read(voice+0x18u)&0xffe0ffffu);
    r.store(0x3817abu,voice+0x18u,packed,observer);
    r.store(0x3817b4u,voice+0x38u,(filter[3]<<16)|(filter[2]&0xffffu),observer);
    const auto parameters=r.read(data+0xb8u);std::uint32_t low,high;
    if(parameters&&(r.read(data+0xbu,1)&1u)){
      low=r.read(parameters+0x30u)&0xffffu;
      r.store(0x3817c9u,voice+0x3cu,low,observer);
      high=r.read(parameters+0x34u);
    }else{low=filter[4]&0xffffu;high=filter[5];}
    r.store(0x3817e6u,voice+0x3cu,(high<<16)|low,observer);
    if(!(r.read(voice+0x12u,1)&1u))return;
    const auto count=r.read(voice+0x64u,1);
    wait_fifo(services,count*6u);
    if(!count)return;
    for(std::uint32_t i=0;;){
      command(services,0xfe8202f8u,r.read(voice+0xcu+i*2u,2));
      command(services,0xfe8202fcu,1);
      command(services,0xfe820318u,r.read(voice+0x18u));
      command(services,0xfe820374u,r.read(voice+0x38u));
      command(services,0xfe820378u,r.read(voice+0x3cu));
      command(services,0xfe8202fcu,0);
      const auto next_count=r.read(voice+0x64u,1);
      if(++i>=next_count)break;
    }
  });
  return 0;
}
} // namespace halo2
