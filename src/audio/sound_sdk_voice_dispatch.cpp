#include "halo2/sound_sdk_voice_dispatch.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 { namespace {
struct Retained {
  const SoundSdkDeviceMemory &memory;const SoundDriverStartupObserver &observer;
  std::span<std::byte> bytes(std::uint32_t address,unsigned size) const {
    if(!memory)throw std::runtime_error("Original SDK voice dispatch memory unavailable");
    auto raw=memory(address,size);
    if(raw.size()!=size)throw std::runtime_error("Original SDK voice dispatch extent differs");
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
bool failed(std::uint32_t value){return std::bit_cast<std::int32_t>(value)<0;}
std::uint32_t channels(std::uint32_t count){
  // Original DEC/SAR/INC AL, including zero's byte wrap.
  return ((std::bit_cast<std::int32_t>(count-1u)>>1)+1u)&0xffu;
}
template<class Callback> const Callback &need(const Callback &callback,const char *message){
  if(!callback)throw std::runtime_error(message);
  return callback;
}
template<class Body> void locked(const SoundSdkDeviceServices &s,Body body){
  bool invoked=false;
  need(s.under_device_lock,"Original SDK voice format lock unavailable")([&]{
    if(invoked)throw std::runtime_error("Original SDK voice format lock body repeated");
    invoked=true;body();
  });
  if(!invoked)throw std::runtime_error("Original SDK voice format lock body skipped");
}
} // namespace

std::uint32_t source_sound_sdk_voice_format_apply(std::uint32_t voice,
    const SoundSdkDeviceServices &s,const SoundDriverStartupObserver &observer){
  std::uint32_t result=0;
  locked(s,[&]{
    const Retained r{s.memory,observer};const auto data=r.read(voice+0x70u);
    const auto count=channels(r.read(data+0xeu,1));
    if(count!=r.read(voice+0x64u,1)){
      if(r.read(voice+0x12u,1)&1u){result=0x88780032u;return;}
      r.write(0x380982u,voice+0x64u,1,count);
    }
    const auto tag=r.read(data+0xcu,2);
    if(tag==0x69u)r.write(0x38099eu,voice+0x14u,4,(r.read(voice+0x14u)&0x0fffffffu)|0xa0000000u);
    else if(tag==1u){
      switch(r.read(data+0xfu,1)){
        case 8:r.write(0x3809dau,voice+0x17u,1,r.read(voice+0x17u,1)&0x0fu);break;
        case 16:r.write(0x38099eu,voice+0x14u,4,(r.read(voice+0x14u)&0x0fffffffu)|0x50000000u);break;
        case 24:r.write(0x38099eu,voice+0x14u,4,(r.read(voice+0x14u)&0x0fffffffu)|0xe0000000u);break;
        case 32:r.write(0x3809b6u,voice+0x17u,1,r.read(voice+0x17u,1)|0xf0u);break;
      }
    }
    const auto delta=(((r.read(data+0xeu,1)-1u)<<16u)^r.read(voice+0x14u))&0x001f0000u;
    r.write(0x3809eeu,voice+0x14u,4,r.read(voice+0x14u)^delta);
    const bool mono=r.read(data+0xeu,1)==1u;
    auto format=r.read(voice+0x14u);format=mono?format&0xf7ffffffu:format|0x08000000u;
    // The active predicate precedes this store and its observer/callback.
    const bool active=(r.read(voice+0x12u,1)&1u)!=0;
    r.write(0x380a0au,voice+0x14u,4,format);
    if(!active)return;
    auto count_now=r.read(voice+0x64u,1);
    const auto fifo=need(s.fifo_available,"Original SDK voice format FIFO unavailable");
    while((fifo()>>2u)<count_now*2u){}
    std::uint32_t index=0;
    while(index<count_now){
      need(s.device_write,"Original SDK voice format device owner unavailable")(
          0xfe8202f8u,r.read(voice+0xcu+index*2u,2));
      const auto current=r.read(voice+0x14u);
      s.device_write(0xfe820304u,index?current|0x400000u:current);
      count_now=r.read(voice+0x64u,1);++index;
    }
  });
  return result;
}

std::uint32_t source_sound_sdk_voice_prepare(std::uint32_t voice,
    const SoundSdkHardwareVoiceBindings &bindings,const SoundSdkVoiceDispatchServices &s,
    const SoundDriverStartupObserver &observer){
  const Retained r{s.hardware.device.memory,observer};
  auto result=source_sound_sdk_activate_hardware_voice(voice,bindings,s.hardware,observer);
  if(failed(result))return result;
  const auto data=r.read(voice+0x80u),flags=r.read(data+8u);
  if(flags&0x2000u)return result;
  if(flags&0x180000u)
    need(s.attach_stream,"Original SDK voice stream attachment owner unavailable")(voice);
  else if(r.read(data+0xbcu)&&r.read(data+0xc0u))
    result=need(s.attach_buffer,"Original SDK voice buffer attachment owner unavailable")(voice);
  return result;
}

std::uint32_t source_sound_sdk_voice_reconfigure(std::uint32_t voice,
    const SoundSdkVoiceDispatchBindings &,const SoundSdkHardwareVoiceBindings &bindings,
    const SoundSdkVoiceDispatchServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.hardware.device.memory,observer};const bool active=(r.read(voice+0x12u,1)&1u)!=0;
  if(active){
    const auto result=need(s.stop,"Original SDK voice stop owner unavailable")(voice,0);
    if(failed(result))return result;
    need(s.wait_pending,"Original SDK voice drain owner unavailable")(voice);
    const auto data=r.read(voice+0x80u);
    if(channels(r.read(data+0xeu,1))!=r.read(voice+0x64u,1))
      need(s.release_channels,"Original SDK voice channel release owner unavailable")(voice);
  }
  const auto result=source_sound_sdk_voice_format_apply(voice,s.hardware.device,observer);
  if(failed(result)||!active)return result;
  return source_sound_sdk_voice_prepare(voice,bindings,s,observer);
}

std::uint32_t source_sound_sdk_voice_mark_format(std::uint32_t voice,std::uint32_t changed,
    const SoundSdkVoiceDispatchBindings &bindings,const SoundSdkHardwareVoiceBindings &hardware,
    const SoundSdkVoiceDispatchServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.hardware.device.memory,observer};
  if(changed)r.write(0x380908u,voice+0x17u,1,r.read(voice+0x17u,1)|1u);
  const auto data=r.read(voice+0x70u);
  if(r.read(data+8u)&0x2000u)r.write(0x38091au,voice+0x16u,2,r.read(voice+0x16u,2)|0x43fu);
  if(changed)r.write(0x380926u,voice+0x1au,1,r.read(voice+0x1au,1)|0x80u);
  if(!(r.read(data+8u)&0x182000u))r.write(0x380933u,voice+0x15u,1,r.read(voice+0x15u,1)|0xe0u);
  if(r.read(data+8u)&0x2000u){r.write(0x38093du,voice+0x64u,1,1);return 0;}
  const auto table=r.read(voice);const SoundSdkReferenceBinding *method=nullptr;
  for(const auto &candidate:bindings.format_methods)if(candidate.vtable_identity==table){
    if(method)throw std::runtime_error("Original SDK voice format table ambiguous");
    method=&candidate;
  }
  if(!method)throw std::runtime_error("Original SDK voice format table unavailable");
  if(method->method_identity==bindings.base_format_method)
    return source_sound_sdk_voice_format_apply(voice,s.hardware.device,observer);
  if(method->method_identity==bindings.reconfigure_method)
    return source_sound_sdk_voice_reconfigure(voice,bindings,hardware,s,observer);
  throw std::runtime_error("Original SDK voice format method unavailable");
}

std::uint32_t source_sound_sdk_voice_initialize(std::uint32_t voice,
    const SoundSdkVoiceDispatchBindings &bindings,const SoundSdkHardwareVoiceBindings &hardware,
    const SoundSdkVoiceDispatchServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.hardware.device.memory,observer};
  const auto result=source_sound_sdk_voice_mark_format(voice,0,bindings,hardware,s,observer);
  if(failed(result))return result;
  if(r.read(r.read(voice+0x80u)+0xau,1)&4u)return result;
  return source_sound_sdk_voice_prepare(voice,hardware,s,observer);
}
} // namespace halo2
