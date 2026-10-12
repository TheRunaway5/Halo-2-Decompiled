#include "halo2/sound_sdk_commit.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
struct Retained {
  const SoundSdkDeviceMemory &memory;
  std::span<std::byte> bytes(std::uint32_t address,std::size_t size)const{
    if(!memory)throw std::runtime_error("Original SDK commit retained memory unavailable");
    auto raw=memory(address,size);
    if(raw.size()!=size)throw std::runtime_error("Original SDK commit retained extent differs");
    return raw;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4)const{
    const auto raw=bytes(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(i*8);
    return value;
  }
  void clear(std::uint32_t pc,std::uint32_t address,const SoundDriverStartupObserver &observer)const{
    const auto before=read(address);const auto raw=bytes(address,4);
    for(auto &byte:raw)byte=std::byte(0);
    if(observer)observer({pc,address,4,before,0});
  }
};
} // namespace

std::uint32_t source_sound_sdk_dispatch(std::uint32_t voice,const SoundSdkDispatchServices &services,
    const SoundDriverStartupObserver &observer){
  const Retained r{services.memory};
  const auto parameters=[&]{return r.read(r.read(voice+0x70u)+0xb8u);};
  if(r.read(parameters(),1)&0x1fu){
    if(!services.gain)throw std::runtime_error("Original SDK gain CPU owner unavailable");
    services.gain(voice);
  }
  if(r.read(parameters(),1)&0x20u){
    if(!services.pitch)throw std::runtime_error("Original SDK pitch CPU owner unavailable");
    services.pitch(voice);
  }
  if(r.read(parameters(),1)&0x40u){
    if(!services.hrtf)throw std::runtime_error("Original SDK HRTF CPU owner unavailable");
    services.hrtf(voice,0,0);
  }
  if(r.read(parameters(),1)&0x80u){
    if(!services.filter)throw std::runtime_error("Original SDK filter CPU owner unavailable");
    services.filter(voice,0);
  }
  const auto selected=parameters();r.clear(0x381e8eu,selected,observer);
  return selected;
}

void source_sound_sdk_update(std::uint32_t voice,const SoundSdkUpdateServices &services,
    const SoundDriverStartupObserver &observer){
  if(!services.under_state_lock)throw std::runtime_error("Original SDK commit state lock unavailable");
  unsigned calls=0;
  services.under_state_lock([&]{
    if(calls++)throw std::runtime_error("Original SDK commit state lock callback repeated");
    const Retained r{services.memory};
    if((r.read(voice+0x12u,1)&3u)!=3u)return;
    if(!services.calculate)throw std::runtime_error("Original SDK calculation CPU owner unavailable");
    auto source=r.read(r.read(voice+0x70u)+0xb4u);
    auto retained=r.read(r.read(voice+8u)+0xcu);
    services.calculate(retained+0x30u,source);
    const auto parameters=r.read(r.read(voice+0x70u)+0xb8u);
    source=r.read(r.read(voice+0x70u)+0xb4u);
    retained=r.read(r.read(voice+8u)+0xcu);
    if(!services.voice_data)throw std::runtime_error("Original SDK voice-data CPU owner unavailable");
    services.voice_data(retained+0x30u,source,retained+0x80u,source+0x7cu,parameters);
    source=r.read(r.read(voice+0x70u)+0xb4u);
    r.clear(0x382095u,source,observer);
    source=r.read(r.read(voice+0x70u)+0xb4u);
    r.clear(0x3820a1u,source+0x7cu,observer);
    if(!services.dispatch)throw std::runtime_error("Original SDK dispatcher CPU owner unavailable");
    services.dispatch(voice);
  });
  if(calls!=1)throw std::runtime_error("Original SDK commit state lock callback not executed");
}

void source_sound_sdk_spatial_commit(std::uint32_t voice,const SoundSdkSpatialConstants &spatial,
    const SoundSdkVoiceConstants &constants,const SoundSdkHrtfBindings &bindings,
    const SoundDriverFloatingPointContext &fp,const SoundSdkPipelineServices &services,
    const SoundDriverStartupObserver &observer){
  auto calculate=services.calculate;calculate.memory=services.memory;
  calculate.under_state_lock=services.under_state_lock;
  auto voice_data=services.voice_data;voice_data.memory=services.memory;
  voice_data.under_state_lock=services.under_state_lock;
  auto device=services.device;device.memory=services.memory;
  auto hrtf=services.hrtf;hrtf.memory=services.memory;
  hrtf.under_device_lock=services.device.under_device_lock;
  hrtf.fifo_available=services.device.fifo_available;hrtf.device_write=services.device.device_write;
  SoundSdkDispatchServices dispatch;dispatch.memory=services.memory;
  dispatch.gain=[&](std::uint32_t selected){source_sound_sdk_gain_apply(selected,device);};
  dispatch.pitch=[&](std::uint32_t selected){source_sound_sdk_pitch_apply(selected,device);};
  dispatch.hrtf=[&](std::uint32_t selected,std::uint32_t source,std::uint32_t parameters){
    source_sound_sdk_hrtf_apply(selected,source,parameters,bindings,spatial,fp,hrtf,observer);
  };
  dispatch.filter=[&](std::uint32_t selected,std::uint32_t filter){
    source_sound_sdk_filter_apply(selected,filter,device,observer);
  };
  SoundSdkUpdateServices update;update.memory=services.memory;
  update.under_state_lock=services.under_state_lock;
  update.calculate=[&](std::uint32_t listener,std::uint32_t source){
    source_sound_sdk_calculate(listener,source,fp,calculate,observer);
  };
  update.voice_data=[&](std::uint32_t listener,std::uint32_t source,std::uint32_t listener_environment,
      std::uint32_t source_environment,std::uint32_t parameters){
    source_sound_sdk_voice_data(listener,source,listener_environment,source_environment,parameters,
        constants,fp,voice_data,observer);
  };
  update.dispatch=[&](std::uint32_t selected){source_sound_sdk_dispatch(selected,dispatch,observer);};
  source_sound_sdk_update(voice,update,observer);
}
} // namespace halo2
