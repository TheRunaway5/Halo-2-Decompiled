#include "halo2/sound_sdk_voice_stop.hpp"
#include <stdexcept>

namespace halo2 { namespace {
struct Retained {
  const SoundSdkDeviceMemory &memory;const SoundDriverStartupObserver &observer;
  std::span<std::byte> bytes(std::uint32_t address,unsigned size) const {
    if(!memory)throw std::runtime_error("Original SDK voice stop memory unavailable");
    auto raw=memory(address,size);
    if(raw.size()!=size)throw std::runtime_error("Original SDK voice stop extent differs");return raw;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4) const {
    const auto raw=bytes(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(i*8u);return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,unsigned size,std::uint32_t value) const {
    if(size<4)value&=(1u<<(size*8u))-1u;
    const auto before=read(address,size);const auto raw=bytes(address,size);
    for(unsigned i=0;i<size;++i)raw[i]=std::byte(value>>(i*8u));
    if(observer)observer({pc,address,size,before,value});
  }
};
template<class Callback> const Callback &need(const Callback &callback,const char *message){
  if(!callback)throw std::runtime_error(message);return callback;
}
template<class Body> void locked(const SoundSdkVoiceStopServices &s,Body body){
  bool called=false;
  need(s.notifications.device.under_device_lock,"Original SDK voice stop lock unavailable")([&]{
    if(called)throw std::runtime_error("Original SDK voice stop lock body repeated");called=true;body();
  });if(!called)throw std::runtime_error("Original SDK voice stop lock body skipped");
}
void fifo(const SoundSdkVoiceStopServices &s,std::uint32_t words){
  const auto &read=need(s.notifications.device.fifo_available,"Original SDK voice stop FIFO unavailable");
  while((read()>>2u)<words){}
}
void command(const SoundSdkVoiceStopServices &s,std::uint32_t address,std::uint32_t value){
  need(s.notifications.device.device_write,"Original SDK voice stop device write unavailable")(address,value);
}
void require_method(const Retained &r,std::uint32_t voice,
    const SoundSdkNotificationBindings &b,unsigned slot,std::uint32_t method){
  const auto table=r.read(voice),actual=r.read(table+slot);
  if(table!=b.voice_vtable||actual!=method)
    throw std::runtime_error("Original SDK voice stop virtual owner unavailable");
}
void slot_stop(const Retained &r,std::uint32_t voice,std::uint32_t initial_count,
    std::uint32_t method,const SoundSdkVoiceStopServices &s){
  fifo(s,initial_count*3u);
  if(!initial_count)return;
  std::uint32_t index=0;
  do {
    const auto at=voice+0xcu+index*2u;
    command(s,0xfe8202f8u,r.read(at,2));
    const auto format=r.read(voice+0x14u);
    command(s,0xfe820304u,format|(index?0x400000u:0u));
    command(s,method,r.read(at,2));++index;
  }while(index<r.read(voice+0x64u,1));
}
} // namespace

std::uint32_t source_sound_sdk_voice_pause_flags(std::uint32_t voice,
    std::uint32_t flags,const SoundSdkVoiceStopServices &s,const SoundDriverStartupObserver &observer){
  std::uint32_t result=0;
  locked(s,[&]{
    const Retained r{s.notifications.device.memory,observer};const auto requested=flags&0x444u;
    if((r.read(voice+0x12u,2)&0x444u)==requested)return;
    if((requested&0x40u)!=(r.read(voice+0x12u,1)&0x40u)){
      const auto device=r.read(voice+8u);
      if(requested&0x40u){
        const auto count=r.read(voice+0x64u,1)+r.read(device+0x53cu);
        if(count<0x1eu)r.write(0x37ed31u,device+0x53cu,4,count);else result=0x88780032u;
      }else r.write(0x37ed3eu,device+0x53cu,4,r.read(device+0x53cu)-r.read(voice+0x64u,1));
    }
    if(bool(r.read(voice+0x12u,2)&0x444u)!=bool(requested) &&
        (r.read(voice+0x12u,1)&3u)==3u){
      const auto count=r.read(voice+0x64u,1);fifo(s,count);
      if(count){std::uint32_t index=0;
        do {command(s,0xfe820140u,r.read(voice+0xcu+index*2u,2)|(requested?0x40000u:0u));++index;}
        while(index<r.read(voice+0x64u,1));
      }
    }
    r.write(0x3824eau,voice+0x12u,2,r.read(voice+0x12u,2)&0xfbbbu);
    r.write(0x3808beu,voice+0x12u,2,r.read(voice+0x12u,2)|requested);
  });return result;
}
void source_sound_sdk_voice_stop_envelope(std::uint32_t voice,
    const SoundSdkVoiceStopServices &s,const SoundDriverStartupObserver &observer){
  locked(s,[&]{
    const Retained r{s.notifications.device.memory,observer};
    if((r.read(voice+0x12u,1)&3u)!=3u)return;
    r.write(0x3808beu,voice+0x12u,2,r.read(voice+0x12u,2)|8u);
    const auto count=r.read(voice+0x64u,1);
    r.write(0x38114eu,voice+0x16u,1,r.read(voice+0x16u,1)&0x7fu);
    slot_stop(r,voice,count,0xfe82012cu,s);
  });
}
void source_sound_sdk_common_voice_stop(std::uint32_t voice,std::uint32_t wait,
    const SoundSdkNotificationBindings &b,const SoundSdkVoiceStopServices &s,
    const SoundDriverStartupObserver &observer){
  const Retained r{s.notifications.device.memory,observer};
  source_sound_sdk_voice_pause_flags(voice,r.read(voice+0x12u,2)&0xffbfu,s,observer);
  if((r.read(voice+0x12u,1)&3u)==3u){
    locked(s,[&]{
      const auto device=r.read(voice+8u);r.write(0x381f2du,device+0x84u,4,r.read(device+0x84u)+1u);
      r.write(0x3824eau,voice+0x12u,2,r.read(voice+0x12u,2)&0x3111u);
      r.write(0x381f3fu,voice+0x16u,2,r.read(voice+0x16u,2)&0xfd7fu);
      if(r.read(voice+0x65u,1)<3u){
        r.write(0x3808beu,voice+0x12u,2,r.read(voice+0x12u,2)|0x8000u);
        slot_stop(r,voice,r.read(voice+0x64u,1),0xfe820128u,s);
      }
      const auto current_device=r.read(voice+8u);
      r.write(0x381fa4u,current_device+0x84u,4,r.read(current_device+0x84u)-1u);
      const auto now=need(s.notifications.query_system_time,"Original SDK voice stop clock unavailable")();
      // KeQuerySystemTime writes this retained argument. These are platform
      // output bytes, not attributed to an invented source store instruction.
      const auto time=r.bytes(voice+0x78u,8);
      for(unsigned i=0;i<8;++i)time[i]=std::byte(now>>(i*8u));
      if(!wait){require_method(r,voice,b,0x1cu,b.arm_method);
        source_sound_sdk_voice_arm_notification(voice,0,0,0,0,s.notifications,observer);}
      const auto parent=r.read(r.read(voice+0x70u)+0xb0u);
      if(parent){const auto owner=r.read(parent+0x20u);
        if(owner && (r.read(r.read(owner+0x80u)+8u)&0x82000u)){
          const auto head=owner+0x44u;auto entry=r.read(head);
          while(entry!=head){if((r.read(entry-0x4au,1)&3u)==3u)return;entry=r.read(entry);}
          need(s.stop_buffer,"Original SDK voice stop parent buffer owner unavailable")(owner,0);
        }
      }
    });
  }else if(!(r.read(voice+0x12u,2)&0x8000u)){
    require_method(r,voice,b,0x20u,b.cancel_method);
    source_sound_sdk_voice_cancel_notification(voice,0,s.notifications,observer);
  }
  if(wait)source_sound_sdk_wait_pending_voice(voice,b,s.notifications,observer);
}
} // namespace halo2
