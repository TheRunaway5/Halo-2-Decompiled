#include "halo2/sound_sdk_voice_retirement.hpp"
#include <stdexcept>

namespace halo2 { namespace {
struct Retained {
  const SoundSdkDeviceMemory &memory;const SoundDriverStartupObserver &observer;
  std::span<std::byte> bytes(std::uint32_t address,unsigned size) const {
    if(!memory)throw std::runtime_error("Original SDK retirement memory unavailable");
    auto raw=memory(address,size);if(raw.size()!=size)throw std::runtime_error("Original SDK retirement extent differs");return raw;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4) const {
    const auto raw=bytes(address,size);std::uint32_t value=0;for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(i*8u);return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,unsigned size,std::uint32_t value) const {
    if(size<4)value&=(1u<<(size*8u))-1u;
    const auto before=read(address,size);const auto raw=bytes(address,size);for(unsigned i=0;i<size;++i)raw[i]=std::byte(value>>(i*8u));
    if(observer)observer({pc,address,size,before,value});
  }
};
template<class Callback> const Callback &need(const Callback &callback,const char *message){if(!callback)throw std::runtime_error(message);return callback;}
template<class Body> void locked(const SoundSdkVoiceRetirementServices &s,Body body){
  bool called=false;need(s.notifications.device.under_device_lock,"Original SDK retirement lock unavailable")([&]{
    if(called)throw std::runtime_error("Original SDK retirement lock body repeated");
    called=true;body();
  });if(!called)throw std::runtime_error("Original SDK retirement lock body skipped");
}
std::uint32_t device_read(const SoundSdkVoiceRetirementServices &s,std::uint32_t address,unsigned size){
  return need(s.device_read,"Original SDK retirement device read unavailable")(address,size);
}
void device_write(const SoundSdkVoiceRetirementServices &s,std::uint32_t address,std::uint32_t value){
  need(s.notifications.device.device_write,"Original SDK retirement device write unavailable")(address,value);
}
void wait_fifo(const SoundSdkVoiceRetirementServices &s){
  const auto &read=need(s.notifications.device.fifo_available,"Original SDK retirement FIFO unavailable");while((read()&0xfffffffcu)<4u){}
}
void detach(const Retained &r,std::uint32_t entry){
  const auto next=r.read(entry);if(next==entry)return;const auto previous=r.read(entry+4u);
  r.write(0x379c6du,next+4u,4,previous);const auto previous_now=r.read(entry+4u),next_now=r.read(entry);
  r.write(0x379c75u,previous_now,4,next_now);r.write(0x379c77u,entry+4u,4,entry);r.write(0x379c7au,entry,4,entry);
}
void mapping_reset(const Retained &r,std::uint32_t mapping){r.write(0x3808f0u,mapping+0xfu,1,0x80u);}
std::uint32_t register_at(const Retained &r,const SoundSdkVoiceRetirementBindings &b,std::uint32_t group,unsigned slot){
  return r.read(b.group_register_table+group*12u+slot*4u)+0xfe800000u;
}
void require_method(const Retained &r,std::uint32_t voice,const SoundSdkVoiceRetirementBindings &b,unsigned slot,std::uint32_t method){
  const auto table=r.read(voice),actual=r.read(table+slot);
  if(table!=b.voice_vtable||actual!=method)throw std::runtime_error("Original SDK retirement virtual owner unavailable");
}
} // namespace

std::uint32_t source_sound_sdk_gate_voice_device(std::uint32_t enable,
    const SoundSdkVoiceRetirementBindings &b,const SoundSdkVoiceRetirementServices &s,const SoundDriverStartupObserver &observer){
  std::uint32_t result=enable;
  locked(s,[&]{
    const Retained r{s.notifications.device.memory,observer};
    if(enable){
      const auto blocked=device_read(s,0xfe801100u,1)&0xe0u;result=0;
      if(blocked||!(device_read(s,0xfe802000u,4)&0x18u))return;
      result=1;wait_fifo(s);device_write(s,0xfe82010cu,0);
      const auto mapping=r.read(b.voice_mapping_pointer_address);
      if(!mapping){need(s.stall_execution,"Original SDK retirement stall unavailable")(0x29bu);return;}
      mapping_reset(r,mapping);device_write(s,0xfe820104u,2);std::uint32_t counter=0x29bu;
      while(r.read(mapping+0xfu,1)!=1u){
        need(s.stall_execution,"Original SDK retirement stall unavailable")(1);
        const auto previous=counter--;if(!previous)break;
      }
      mapping_reset(r,mapping);
    }else{wait_fifo(s);device_write(s,0xfe82010cu,1);}
  });return result;
}
void source_sound_sdk_unlink_active_voice(std::uint32_t voice,std::uint32_t queue_idle,
    const SoundSdkVoiceRetirementBindings &b,const SoundSdkVoiceRetirementServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.notifications.device.memory,observer};const auto group=r.read(voice+0x65u,1);
  const auto first_register=register_at(r,b,group,0);auto first=device_read(s,first_register,4);
  const auto current_register=register_at(r,b,group,1),last_register=register_at(r,b,group,2);
  const auto current=device_read(s,current_register,4),last=device_read(s,last_register,4);
  const auto head=r.read(voice+8u)+group*8u+0x488u;
  const auto previous_link=r.read(voice+0x50u);const auto previous_voice=previous_link==head?0u:previous_link-0x4cu;
  const auto previous_slot=previous_voice?r.read(previous_voice+r.read(previous_voice+0x64u,1)*2u+0xau,2):0xffffu;
  const auto next_link=r.read(voice+0x4cu);const auto next_voice=next_link==head?0u:next_link-0x4cu;
  auto next_slot=next_voice?r.read(next_voice+0xcu,2):0xffffu;
  if(previous_voice){
    const auto at=r.read(b.slot_mapping_pointer_address)+(previous_slot<<7u)+0x7cu;
    r.write(0x380f7fu,at,4,(r.read(at)&0xffff0000u)|next_slot);
  }else{first=next_slot;device_write(s,first_register,next_slot);}
  bool has_current=false,has_last=false;std::uint32_t index=0;
  while(index<r.read(voice+0x64u,1)){
    const auto at=voice+0xcu+index*2u;const auto slot=r.read(at,2);
    if(current==slot)has_current=true;
    if(last==slot)has_last=true;
    const auto reloaded_slot=r.read(at,2),mapping=r.read(b.slot_mapping_pointer_address)+(reloaded_slot<<7u)+0x7cu;
    const auto value=(r.read(mapping)&0xffff0000u)|reloaded_slot;++index;r.write(0x380fdcu,mapping,4,value);
  }
  if(has_current){
    std::uint32_t next_current=0xffffu;
    if(next_voice){
      next_current=next_slot;const auto mapping=r.read(b.slot_mapping_pointer_address);next_slot=r.read(mapping+(next_slot<<7u)+0x7cu)&0xffffu;
      if(next_slot==0xffffu)next_slot=first;
    }else next_slot=first;
    device_write(s,register_at(r,b,r.read(voice+0x65u,1),1),next_current);
    device_write(s,register_at(r,b,r.read(voice+0x65u,1),2),next_slot);
  }else if(has_last){
    if(!next_voice)next_slot=first;
    device_write(s,register_at(r,b,r.read(voice+0x65u,1),2),next_slot);
  }
  detach(r,voice+0x4cu);const bool pending=(r.read(voice+0x12u,2)&0x8000u)!=0;
  r.write(0x381061u,voice+0x65u,1,0xffu);
  if(pending)r.write(0x3824eau,voice+0x12u,2,r.read(voice+0x12u,2)&0x7fffu);
  else{
    const auto mapping=r.read(voice+0x68u);r.write(0x38107bu,mapping+0x3fu,1,1);
    if(queue_idle){
      const auto entry=voice+0x54u,head=r.read(voice+8u)+0x4a0u;r.write(0x38108du,entry,4,head);
      const auto previous=r.read(head+4u);r.write(0x381092u,entry+4u,4,previous);r.write(0x381095u,previous,4,entry);
      r.write(0x381099u,r.read(entry)+4u,4,entry);
      const auto device=r.read(voice+8u);r.write(0x38109fu,device+0x4b8u,4,r.read(device+0x4b8u)|0x40u);
      need(s.insert_queue_dpc,"Original SDK retirement DPC insertion unavailable")(r.read(voice+8u)+0x4c8u,0,0);
    }
  }
}
std::uint32_t source_sound_sdk_voice_mapping_status(std::uint32_t voice,
    const SoundSdkVoiceRetirementBindings &b,const SoundSdkVoiceRetirementServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.notifications.device.memory,observer};
  const auto result=source_sound_sdk_mapping_completed(voice+0x68u,3,s.notifications.device.memory);
  if(result){
    r.write(0x382a14u,r.read(voice+0x68u)+0x3fu,1,0x80u);
    need(s.stop_voice,"Original SDK retirement common stop unavailable")(voice,0);
    if(r.read(r.read(voice+0x80u)+0xau,1)&4u){
      require_method(r,voice,b,0x1c,b.arm_method);source_sound_sdk_voice_arm_notification(voice,1,0,0,0,s.notifications,observer);
    }
  }return result;
}
void source_sound_sdk_force_pending_voice(std::uint32_t voice,
    const SoundSdkVoiceRetirementBindings &b,const SoundSdkVoiceRetirementServices &s,const SoundDriverStartupObserver &observer){
  locked(s,[&]{
    const Retained r{s.notifications.device.memory,observer};r.write(0x3810d2u,b.force_counter_address,4,r.read(b.force_counter_address)+1u);
    const auto gated=source_sound_sdk_gate_voice_device(1,b,s,observer);
    if((r.read(voice+0x12u,2)&0x8001u)==0x8001u)source_sound_sdk_unlink_active_voice(voice,0,b,s,observer);
    if(gated)source_sound_sdk_gate_voice_device(0,b,s,observer);
    require_method(r,voice,b,0x18,b.status_method);source_sound_sdk_voice_mapping_status(voice,b,s,observer);
  });
}
} // namespace halo2
