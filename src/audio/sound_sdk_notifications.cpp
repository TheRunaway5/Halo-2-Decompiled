#include "halo2/sound_sdk_notifications.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 { namespace {
struct Retained {
  const SoundSdkDeviceMemory &memory;const SoundDriverStartupObserver &observer;
  std::span<std::byte> bytes(std::uint32_t address,unsigned size) const {
    if(!memory)throw std::runtime_error("Original SDK notification memory unavailable");
    auto raw=memory(address,size);
    if(raw.size()!=size)throw std::runtime_error("Original SDK notification extent differs");
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
template<class Body> void locked(const SoundSdkNotificationServices &s,Body body){
  bool invoked=false;
  need(s.device.under_device_lock,"Original SDK notification lock unavailable")([&]{
    if(invoked)throw std::runtime_error("Original SDK notification lock body repeated");
    invoked=true;body();
  });
  if(!invoked)throw std::runtime_error("Original SDK notification lock body skipped");
}
std::int64_t signed_time(std::uint64_t value){return std::bit_cast<std::int64_t>(value);}
std::uint64_t combine(std::uint32_t low,std::uint32_t high){return (std::uint64_t(high)<<32u)|low;}
std::uint64_t time_at(const Retained &r,std::uint32_t at){const auto low=r.read(at);return combine(low,r.read(at+4u));}
void detach(const Retained &r,std::uint32_t entry){
  const auto next=r.read(entry);if(next==entry)return;
  const auto previous=r.read(entry+4u);r.write(0x379c6du,next+4u,4,previous);
  const auto previous_now=r.read(entry+4u),next_now=r.read(entry);
  r.write(0x379c75u,previous_now,4,next_now);
  r.write(0x379c77u,entry+4u,4,entry);r.write(0x379c7au,entry,4,entry);
}
void set_timer(const Retained &r,std::uint32_t device,std::uint32_t notification,
    const SoundSdkNotificationServices &s){
  const auto high=r.read(notification+0x1cu),low=r.read(notification+0x18u);
  need(s.set_timer,"Original SDK notification timer owner unavailable")(device+0x4e8u,combine(low,high),device+0x510u);
}
void cancel_timed(std::uint32_t device,std::uint32_t notification,
    const SoundSdkNotificationServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.device.memory,observer};const auto head=device+0x4a8u;
  const bool first=notification==r.read(head);detach(r,notification);const auto next=r.read(head);
  if(next==head){
    need(s.cancel_timer,"Original SDK notification timer cancellation unavailable")(device+0x4e8u);
    need(s.remove_queue_dpc,"Original SDK notification DPC removal unavailable")(device+0x510u);
  }else if(first)set_timer(r,device,next,s);
}
std::uint32_t arm_immediate(const Retained &r,std::uint32_t device,std::uint32_t notification){
  if(!(r.read(notification+8u,1)&2u)){
    const auto head=device+0x4b0u;r.write(0x37eaf2u,notification,4,head);
    const auto last=r.read(head+4u);r.write(0x37eaf7u,notification+4u,4,last);
    r.write(0x37eafau,last,4,notification);r.write(0x37eafeu,r.read(notification)+4u,4,notification);
  }return 1;
}
std::uint32_t arm_timed(std::uint32_t device,std::uint32_t notification,
    const SoundSdkNotificationServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.device.memory,observer};
  const auto initial_low=r.read(notification+0x18u),initial_high=r.read(notification+0x1cu);
  if(!(initial_low|initial_high))return 0;
  const auto now=need(s.query_system_time,"Original SDK notification clock unavailable")();
  const auto high=r.read(notification+0x1cu),low=r.read(notification+0x18u);
  const auto requested=combine(low,high);
  if(signed_time(requested)<=0){
    const auto when=now-requested;r.write(0x37f15au,notification+0x18u,4,std::uint32_t(when));
    r.write(0x37f15du,notification+0x1cu,4,std::uint32_t(when>>32u));
  }
  // Original compares the high word signed, then the low word unsigned.
  const auto when_high=r.read(notification+0x1cu),when_low=r.read(notification+0x18u);
  if(signed_time(combine(when_low,when_high))<=signed_time(now))return 0;
  if(r.read(notification+8u,1)&2u)source_sound_sdk_cancel_notification(device,notification,s,observer);
  const auto head=device+0x4a8u;auto current=r.read(head);
  if(current!=head){
    const auto new_low=r.read(notification+0x18u),new_high=r.read(notification+0x1cu);
    do{
      const auto current_high=r.read(current+0x1cu);
      if(std::bit_cast<std::int32_t>(current_high)>std::bit_cast<std::int32_t>(new_high))break;
      if(current_high==new_high&&r.read(current+0x18u)>new_low)break;
      current=r.read(current);
    }while(current!=head);
  }
  r.write(0x37f1b4u,notification,4,current);const auto previous=r.read(current+4u);
  r.write(0x37f1b9u,notification+4u,4,previous);r.write(0x37f1bcu,previous,4,notification);
  r.write(0x37f1c0u,r.read(notification)+4u,4,notification);
  if(notification==r.read(head))set_timer(r,device,notification,s);
  return 1;
}
} // namespace

void source_sound_sdk_cancel_notification(std::uint32_t device,std::uint32_t notification,
    const SoundSdkNotificationServices &s,const SoundDriverStartupObserver &observer){
  locked(s,[&]{
    const Retained r{s.device.memory,observer};const auto flags=r.read(notification+8u);
    if(flags&2u){if(flags&1u)detach(r,notification);else cancel_timed(device,notification,s,observer);}
    r.write(0x37f09au,notification+8u,4,r.read(notification+8u)&0xfffffffdu);
  });
}
std::uint32_t source_sound_sdk_arm_notification(std::uint32_t device,std::uint32_t notification,
    const SoundSdkNotificationServices &s,const SoundDriverStartupObserver &observer){
  std::uint32_t result=0;
  locked(s,[&]{
    const Retained r{s.device.memory,observer};
    result=r.read(notification+8u,1)&1u?arm_immediate(r,device,notification):arm_timed(device,notification,s,observer);
    if(result)r.write(0x37f278u,notification+8u,4,r.read(notification+8u)|2u);
  });return result;
}
void source_sound_sdk_voice_cancel_notification(std::uint32_t voice,std::uint32_t index,
    const SoundSdkNotificationServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.device.memory,observer};const auto notification=voice+0x88u+(index<<5u);
  source_sound_sdk_cancel_notification(r.read(voice+8u),notification,s,observer);
}
void source_sound_sdk_voice_arm_notification(std::uint32_t voice,std::uint32_t index,
    std::uint32_t low,std::uint32_t high,std::uint32_t context,
    const SoundSdkNotificationServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.device.memory,observer};const auto notification=voice+0x88u+(index<<5u);
  r.write(0x382aebu,notification+0x18u,4,low);r.write(0x382af2u,notification+0x1cu,4,high);
  r.write(0x382af9u,notification+0x14u,4,context);
  source_sound_sdk_arm_notification(r.read(voice+8u),notification,s,observer);
}
void source_sound_sdk_poll_pending_voice(std::uint32_t voice,const SoundSdkNotificationBindings &bindings,
    const SoundSdkNotificationServices &s,const SoundDriverStartupObserver &observer){
  const auto now=need(s.query_system_time,"Original SDK pending voice clock unavailable")();
  const Retained r{s.device.memory,observer};
  if((r.read(voice+0x12u,2)&0x8001u)==0x8001u){
    const auto limit=time_at(r,voice+0x78u)+0x4c4b40u;
    if(signed_time(now)>=signed_time(limit))need(s.force_pending_voice,"Original SDK pending force owner unavailable")(voice);
  }else{
    const auto table=r.read(voice),method=r.read(table+0x20u);
    if(table!=bindings.voice_vtable||method!=bindings.cancel_method)
      throw std::runtime_error("Original SDK pending virtual notification owner unavailable");
    source_sound_sdk_voice_cancel_notification(voice,0,s,observer);
  }
}
void source_sound_sdk_wait_pending_voice(std::uint32_t voice,const SoundSdkNotificationBindings &bindings,
    const SoundSdkNotificationServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.device.memory,observer};if(!(r.read(voice+0x12u,1)&1u))return;
  while(r.read(voice+0x12u,2)&0x8000u)source_sound_sdk_poll_pending_voice(voice,bindings,s,observer);
}
std::uint32_t source_sound_sdk_mapping_completed(std::uint32_t descriptor,std::uint32_t index,
    const SoundSdkDeviceMemory &memory){
  const Retained r{memory,{}};return r.read(r.read(descriptor)+(index<<4u)+0xfu,1)==1u?1u:0u;
}
} // namespace halo2
