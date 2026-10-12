#include "halo2/sound_sdk_voice_attachment.hpp"
#include <stdexcept>

namespace halo2 { namespace {
struct Retained {
  const SoundSdkDeviceMemory &memory;const SoundDriverStartupObserver &observer;
  std::span<std::byte> bytes(std::uint32_t address,unsigned size) const {
    if(!memory)throw std::runtime_error("Original SDK voice attachment memory unavailable");
    auto raw=memory(address,size);
    if(raw.size()!=size)throw std::runtime_error("Original SDK voice attachment extent differs");
    return raw;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4) const {
    const auto raw=bytes(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(i*8u);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,unsigned size,std::uint32_t value) const {
    if(size<4)value&=(1u<<(size*8u))-1u;
    const auto before=read(address,size);const auto raw=bytes(address,size);
    for(unsigned i=0;i<size;++i)raw[i]=std::byte(value>>(i*8u));
    if(observer)observer({pc,address,size,before,value});
  }
};
template<class Callback> const Callback &need(const Callback &callback,const char *message){
  if(!callback)throw std::runtime_error(message);
  return callback;
}
template<class Body> void locked(const SoundSdkDeviceServices &s,Body body){
  bool invoked=false;
  need(s.under_device_lock,"Original SDK voice cursor lock unavailable")([&]{
    if(invoked)throw std::runtime_error("Original SDK voice cursor lock body repeated");
    invoked=true;body();
  });
  if(!invoked)throw std::runtime_error("Original SDK voice cursor lock body skipped");
}
} // namespace

std::uint32_t source_sound_sdk_voice_sample_bytes(std::uint32_t voice,std::uint32_t samples,
    const SoundSdkDeviceMemory &memory){
  const Retained r{memory,{}};const auto data=r.read(voice+0x70u);
  if(r.read(data+0xcu,2)==0x69u)samples>>=6u;
  return r.read(data+0x14u)*samples;
}
void source_sound_sdk_voice_cursor(std::uint32_t voice,std::uint32_t cursor,
    const SoundSdkDeviceServices &s,const SoundDriverStartupObserver &observer){
  locked(s,[&]{
    const Retained r{s.memory,observer};const auto data=r.read(voice+0x80u);
    std::uint32_t offset,page;
    if(r.read(data+0xau,1)&0x18u){offset=r.read(data+0xbcu)&0xfffu;page=0x7ffu;}
    else{
      const auto position=(r.read(data+0xbcu)&0xfffu)+cursor;
      const auto entry=r.read(voice+0x84u);page=r.read(entry+8u,2)+(position>>12u);offset=position&0xfffu;
    }
    r.write(0x382550u,voice+0x154u,4,(page<<12u)|offset);
  });
}
void source_sound_sdk_detach_voice_samples(std::uint32_t voice,
    const SoundSdkVoiceAttachmentServices &s,const SoundDriverStartupObserver &observer){
  need(s.stop,"Original SDK voice detach stop owner unavailable")(voice,0);
  need(s.wait_pending,"Original SDK voice detach drain owner unavailable")(voice);
  const Retained r{s.pages.device.memory,observer};
  if(!(r.read(voice+0x12u,2)&0x100u))return;
  r.write(0x3824eau,voice+0x12u,2,r.read(voice+0x12u,2)&0xfeffu);
  if(const auto entry=r.read(voice+0x84u)){
    const auto manager=r.read(voice+8u)+0x60u;
    source_sound_sdk_release_sample_pages(manager,entry,s.pages,observer);
    r.write(0x38259du,voice+0x84u,4,0);
  }
}
std::uint32_t source_sound_sdk_attach_voice_buffer(std::uint32_t voice,
    const SoundSdkVoiceAttachmentServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.pages.device.memory,observer};if(r.read(voice+0x12u,2)&0x100u)return 0;
  r.write(0x3808beu,voice+0x12u,2,r.read(voice+0x12u,2)|0x100u);
  const auto data=r.read(voice+0x80u),size=r.read(data+0xc0u);
  const auto manager=r.read(voice+8u)+0x60u,buffer=r.read(data+0xbcu);
  const auto entry=source_sound_sdk_acquire_sample_pages(manager,buffer,size,s.pages,observer);
  r.write(0x382bb5u,voice+0x84u,4,entry);
  if(!entry){source_sound_sdk_detach_voice_samples(voice,s,observer);return 0x8007000eu;}
  const auto current_data=r.read(voice+0x80u);
  source_sound_sdk_voice_cursor(voice,r.read(current_data+0xc4u),s.pages.device,observer);return 0;
}
void source_sound_sdk_attach_voice_stream(std::uint32_t voice,
    const SoundSdkVoiceAttachmentBindings &bindings,const SoundSdkVoiceInitializationData &initialization,
    const SoundSdkVoiceAttachmentServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.pages.device.memory,observer};if(r.read(voice+0x12u,2)&0x100u)return;
  r.write(0x3808beu,voice+0x12u,2,r.read(voice+0x12u,2)|0x100u);
  const auto size=source_sound_sdk_voice_sample_bytes(voice,32,s.pages.device.memory);
  const auto data=r.read(voice+0x80u),index=r.read(data+0xd4u)-0xbu;
  const auto buffer=index*size+r.read(bindings.stream_samples_pointer_address);
  source_sound_sdk_voice_buffer(data,buffer,size,initialization,s.initialization,observer);
  const auto current_data=r.read(voice+0x80u);
  source_sound_sdk_voice_cursor(voice,r.read(current_data+0xc4u),s.pages.device,observer);
}
} // namespace halo2
