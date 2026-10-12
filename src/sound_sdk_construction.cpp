#include "halo2/sound_sdk_construction.hpp"
#include <stdexcept>

namespace halo2 { namespace {
struct Retained {
  const SoundSdkDeviceMemory &memory;
  std::span<std::byte> bytes(std::uint32_t address,unsigned size) const {
    if(!memory)throw std::runtime_error("Original SDK construction memory unavailable");
    auto raw=memory(address,size);
    if(raw.size()!=size)throw std::runtime_error("Original SDK construction extent differs");
    return raw;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4) const {
    const auto raw=bytes(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(i*8);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,unsigned size,std::uint32_t value,
      const SoundDriverStartupObserver &observer) const {
    const auto before=read(address,size);const auto raw=bytes(address,size);
    for(unsigned i=0;i<size;++i)raw[i]=std::byte(value>>(i*8));
    if(observer)observer({pc,address,size,before,value});
  }
};
struct StateAccess {
  const SoundSdkConstructionServices &services;bool acquired=false;
  explicit StateAccess(const SoundSdkConstructionServices &s):services(s){
    if(!s.acquire_state_access||!s.release_state_access)
      throw std::runtime_error("Original SDK reference state access unavailable");
    acquired=s.acquire_state_access();
  }
  ~StateAccess(){if(acquired)services.release_state_access();}
};
} // namespace

std::uint32_t source_sound_sdk_add_reference(std::uint32_t identity,
    const SoundSdkConstructionData &data,const SoundSdkConstructionServices &services,
    const SoundDriverStartupObserver &observer){
  const Retained r{services.memory};const auto table=r.read(identity);
  const SoundSdkReferenceBinding *binding=nullptr;
  for(const auto &candidate:data.references)if(candidate.vtable_identity==table){
    if(binding)throw std::runtime_error("Original SDK reference table binding ambiguous");
    binding=&candidate;
  }
  if(!binding)throw std::runtime_error("Original SDK reference table owner unavailable");
  if(binding->method_identity==data.plain_reference_method){
    r.write(0x379d10u,identity+4u,4,r.read(identity+4u)+1u,observer);
    return r.read(identity+4u);
  }
  if(binding->method_identity!=data.synchronized_reference_method)
    throw std::runtime_error("Original SDK reference method owner unavailable");
  StateAccess access(services);
  if(r.read(data.busy_address))return 0x80004005u;
  r.write(0x37a17au,identity+4u,4,r.read(identity+4u)+1u,observer);
  return r.read(identity+4u);
}

std::uint32_t source_sound_sdk_retain(std::uint32_t identity,const SoundSdkConstructionData &data,
    const SoundSdkConstructionServices &services,const SoundDriverStartupObserver &observer){
  if(identity)(void)source_sound_sdk_add_reference(identity,data,services,observer);
  return identity;
}

void source_sound_sdk_initialize_pair(std::uint32_t identity,std::uint32_t payload,
    const SoundSdkConstructionServices &services,const SoundDriverStartupObserver &observer){
  const Retained r{services.memory};
  r.write(payload?0x37a11au:0x37a12cu,identity+8u,4,payload?r.read(payload):0xffffffffu,observer);
  r.write(payload?0x37a120u:0x37a12du,identity+0xcu,4,payload?r.read(payload+4u):0xffffffffu,observer);
}

std::uint32_t source_sound_sdk_construct_device(std::uint32_t identity,
    const SoundSdkConstructionData &data,const SoundSdkConstructionServices &services,
    const SoundDriverStartupObserver &observer){
  const Retained r{services.memory};const auto head=identity+0x10u;
  r.write(0x37a137u,identity+4u,4,2,observer);
  r.write(0x37a13eu,identity,4,data.device_vtable,observer);
  r.write(0x37a144u,data.singleton_address,4,identity,observer);
  r.write(0x37a149u,identity+0x14u,4,head,observer);
  r.write(0x37a14cu,head,4,head,observer);
  return identity;
}

std::uint32_t source_sound_sdk_construct_device_data(std::uint32_t identity,
    const SoundSdkConstructionData &data,const SoundSdkConstructionServices &services,
    const SoundDriverStartupObserver &observer){
  const Retained r{services.memory};
  r.write(0x37cc76u,identity+4u,4,1,observer);
  r.write(0x37cc7du,identity,4,data.device_data_vtable,observer);
  for(unsigned i=0;i<15;++i)r.write(0x37cc8bu,identity+0x38u+i*4u,4,data.listener_defaults[i],observer);
  r.write(0x37cc9au,identity+0x30u,4,0x3f,observer);
  r.write(0x37cc9du,identity+0x80u,4,0xfff,observer);
  for(unsigned i=0;i<12;++i)r.write(0x37ccabu,identity+0x84u+i*4u,4,data.environment_defaults[i],observer);
  r.write(0x37ccadu,identity+0x30u,4,0x3f,observer);
  r.write(0x37ccb0u,identity+0x80u,4,0xfff,observer);
  auto setting=r.read(data.speaker_override_address);
  if((setting&0xffffu)>2u||(setting&0xfffc0000u)){
    if(!services.saved_audio_setting)throw std::runtime_error("Original SDK saved audio setting owner unavailable");
    setting=services.saved_audio_setting();
  }
  r.write(0x37ccd0u,data.speaker_config_address,4,setting,observer);
  for(unsigned i=0;i<7;++i)r.write(0x37cce0u,identity+0x10u+i*4u,4,0x01010101u,observer);
  r.write(0x37cce2u,identity+0x2cu,2,0x0101,observer);
  r.write(0x37cce4u,identity+0x2eu,1,1,observer);
  r.write(0x37cce9u,identity+0x2fu,1,0,observer);
  source_sound_sdk_initialize_pair(identity,0,services,observer);
  return identity;
}

std::uint32_t source_sound_sdk_construct_buffer(std::uint32_t identity,std::uint32_t device,
    const SoundSdkConstructionData &data,const SoundSdkConstructionServices &services,
    const SoundDriverStartupObserver &observer){
  const Retained r{services.memory};const auto head=identity+0x14u;
  r.write(0x37be31u,identity+4u,4,1,observer);
  r.write(0x37be38u,identity,4,data.buffer_base_vtable,observer);
  r.write(0x37be3eu,identity+0x18u,4,head,observer);
  r.write(0x37be41u,head,4,head,observer);
  const auto retained=source_sound_sdk_retain(device,data,services,observer);
  r.write(0x37be48u,identity+8u,4,retained,observer);
  r.write(0x37c1e3u,identity,4,data.buffer_vtable,observer);
  return identity;
}

std::uint32_t source_sound_sdk_construct_voice(std::uint32_t identity,std::uint32_t hardware_device,
    std::uint32_t voice_data,const SoundSdkConstructionData &data,
    const SoundSdkConstructionServices &services,const SoundDriverStartupObserver &observer){
  const Retained r{services.memory};
  r.write(0x381e9bu,identity+4u,4,1,observer);
  r.write(0x381ea2u,identity,4,data.voice_base_vtable,observer);
  r.write(0x381ea8u,identity+0x68u,4,0,observer);
  r.write(0x381eacu,identity+0x6cu,4,0,observer);
  auto retained=source_sound_sdk_retain(hardware_device,data,services,observer);
  r.write(0x381eb9u,identity+8u,4,retained,observer);
  retained=source_sound_sdk_retain(voice_data,data,services,observer);
  r.write(0x381ec1u,identity+0x70u,4,retained,observer);
  constexpr std::array<std::array<std::uint32_t,3>,4> heads{{
      {0x4c,0x381ec7,0x381eca},{0x54,0x381ecf,0x381ed2},
      {0x5c,0x381ed7,0x381eda},{0x44,0x381edf,0x381ee2}}};
  for(const auto &head:heads){
    r.write(head[1],identity+head[0]+4u,4,identity+head[0],observer);
    r.write(head[2],identity+head[0],4,identity+head[0],observer);
  }
  r.write(0x381ee4u,identity+0x65u,1,0xff,observer);
  r.write(0x381eeeu,identity+0xcu,4,0xffffffffu,observer);
  r.write(0x381eefu,identity+0x10u,2,0xffff,observer);
  r.write(0x382b36u,identity,4,data.voice_vtable,observer);
  retained=source_sound_sdk_retain(voice_data,data,services,observer);
  r.write(0x382b41u,identity+0x80u,4,retained,observer);
  for(unsigned i=0;i<6;++i){
    const auto node=identity+0x88u+i*0x20u;
    r.write(0x382b4fu,node+0x10u,4,i,observer);
    r.write(0x382b52u,node+4u,4,node,observer);
    r.write(0x382b55u,node,4,node,observer);
    r.write(0x382b57u,node+0xcu,4,identity,observer);
  }
  for(unsigned i=0;i<3;++i){
    const auto address=identity+0x90u+i*0x20u;
    r.write(0x382b63u+i*7u,address,4,r.read(address)|5u,observer);
  }
  return identity;
}

void source_sound_sdk_bind_buffer(std::uint32_t identity,std::uint32_t voice,
    std::uint32_t voice_data,const SoundSdkConstructionData &data,
    const SoundSdkConstructionServices &services,const SoundDriverStartupObserver &observer){
  const Retained r{services.memory};
  auto retained=source_sound_sdk_retain(voice,data,services,observer);
  r.write(0x37be7eu,identity+0xcu,4,retained,observer);
  retained=source_sound_sdk_retain(voice_data,data,services,observer);
  r.write(0x37be86u,identity+0x10u,4,retained,observer);
  if(!(r.read(retained+8u,1)&0x10u))return;
  const auto node=identity+0x14u,head=r.read(identity+8u)+0x10u;
  r.write(0x37be98u,node,4,head,observer);
  auto tail=r.read(head+4u);
  r.write(0x37be9du,node+4u,4,tail,observer);
  r.write(0x37bea0u,tail,4,node,observer);
  r.write(0x37bea4u,r.read(node)+4u,4,node,observer);
  const auto setting=r.read(data.speaker_config_address);
  const auto source=[&]{return r.read(r.read(identity+0x10u)+0xb4u);};
  if(((setting&0xffffu)==2u||(setting&0x10000u))&&!(setting&0x80000000u)){
    auto s=source();r.write(0x37bed2u,s+0x58u,4,r.read(s+0x58u)|1u,observer);
    s=source();r.write(0x37bedfu,s,4,r.read(s)|0x04000000u,observer);
  }
  if(r.read(r.read(identity+0x10u)+0xau,1)&2u){
    auto s=source();r.write(0x37bef0u,s+0x58u,4,r.read(s+0x58u)|4u,observer);
    s=source();r.write(0x37befdu,s,4,r.read(s)|0x04000000u,observer);
  }
}
} // namespace halo2
