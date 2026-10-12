#include "halo2/sound_sdk_sample_pages.hpp"
#include <stdexcept>

namespace halo2 { namespace {
struct Retained {
  const SoundSdkDeviceMemory &memory;const SoundDriverStartupObserver &observer;
  std::span<std::byte> bytes(std::uint32_t address,unsigned count) const {
    if(!memory)throw std::runtime_error("Original SDK sample-page memory unavailable");
    auto raw=memory(address,count);
    if(raw.size()!=count)throw std::runtime_error("Original SDK sample-page extent differs");
    return raw;
  }
  std::uint32_t read(std::uint32_t address,unsigned count=4) const {
    const auto raw=bytes(address,count);std::uint32_t value=0;
    for(unsigned i=0;i<count;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(i*8u);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,unsigned count,std::uint32_t value) const {
    if(count<4)value&=(1u<<(count*8u))-1u;
    const auto before=read(address,count);const auto raw=bytes(address,count);
    for(unsigned i=0;i<count;++i)raw[i]=std::byte(value>>(i*8u));
    if(observer)observer({pc,address,count,before,value});
  }
};
template<class Callback> const Callback &need(const Callback &callback,const char *message){
  if(!callback)throw std::runtime_error(message);
  return callback;
}
template<class Body> void locked(const SoundSdkDeviceServices &s,Body body){
  bool invoked=false;
  need(s.under_device_lock,"Original SDK sample-page lock unavailable")([&]{
    if(invoked)throw std::runtime_error("Original SDK sample-page lock body repeated");
    invoked=true;body();
  });
  if(!invoked)throw std::runtime_error("Original SDK sample-page lock body skipped");
}
void initialize_entry(const Retained &r,std::uint32_t entry,std::uint32_t index,
    std::uint32_t count,std::uint32_t after){
  const auto next=r.read(after);
  r.write(0x380207,entry+4u,4,after);r.write(0x38020c,entry,4,next);
  r.write(0x38020e,next+4u,4,entry);r.write(0x380214,r.read(entry+4u),4,entry);
  r.write(0x38021b,entry+0xcu,4,0);r.write(0x38021f,entry+0x10u,4,0);
  r.write(0x380223,entry+8u,2,index);r.write(0x38022c,entry+0xau,2,count);
}
void detach_entry(const Retained &r,std::uint32_t entry){
  const auto next=r.read(entry);if(next==entry)return;
  const auto previous=r.read(entry+4u);r.write(0x379c6d,next+4u,4,previous);
  const auto previous_now=r.read(entry+4u),next_now=r.read(entry);
  r.write(0x379c75,previous_now,4,next_now);
  r.write(0x379c77,entry+4u,4,entry);r.write(0x379c7a,entry,4,entry);
}
std::uint32_t allocate_entry(const Retained &r,std::uint32_t manager,
    std::uint32_t base,std::uint32_t requested){
  const auto largest=r.read(manager+0x18u),needed=requested&0xffffu;
  if(!largest||r.read(largest+0xau,2)<needed)return 0;
  const auto head=manager+4u;auto current=r.read(head);if(current==head)return 0;
  std::uint32_t selected=0;
  do{
    if(!r.read(current+0xcu)){
      const auto count=r.read(current+0xau,2);
      if(count>=needed&&(!selected||count<r.read(selected+0xau,2)))selected=current;
    }
    current=r.read(current);
  }while(current!=head);
  if(!selected)return 0;
  if(needed<r.read(selected+0xau,2)){
    const auto spare=r.read(manager+0xcu);detach_entry(r,spare);
    const auto remainder=(r.read(selected+0xau,2)-needed)&0xffffu;
    const auto index=(r.read(selected+8u,2)+needed)&0xffffu;
    initialize_entry(r,spare,index,remainder,selected);
    r.write(0x3803a5,selected+0xau,2,needed);
  }
  const auto count=r.read(selected+0xau,2);
  r.write(0x3803b1,selected+0xcu,4,1);r.write(0x3803b8,selected+0x10u,4,base);
  const auto available=r.read(manager+0x1cu);
  r.write(0x3803be,available,4,r.read(available)-count);
  if(selected==r.read(manager+0x18u)){
    r.write(0x3803c5,manager+0x18u,4,0);current=r.read(head);
    while(current!=head){
      if(!r.read(current+0xcu)){
        const auto old=r.read(manager+0x18u);
        if(!old||r.read(old+0xau,2)<r.read(current+0xau,2))r.write(0x3803e7,manager+0x18u,4,current);
      }
      current=r.read(current);
    }
  }
  return selected;
}
void move_to_pool(const Retained &r,std::uint32_t head,std::uint32_t entry){
  const auto next=r.read(entry),previous=r.read(entry+4u);
  r.write(0x37e61b,next+4u,4,previous);
  const auto previous_now=r.read(entry+4u),next_now=r.read(entry);
  r.write(0x37e623,previous_now,4,next_now);r.write(0x37e629,entry,4,head);
  const auto last=r.read(head+4u);r.write(0x37e62e,entry+4u,4,last);
  r.write(0x37e631,head+4u,4,entry);r.write(0x37e637,r.read(entry+4u),4,entry);
}
std::uint32_t merge_entries(const Retained &r,std::uint32_t manager,
    std::uint32_t first,std::uint32_t second){
  const auto count=r.read(second+0xau,2);
  r.write(0x380405,first+0xau,2,r.read(first+0xau,2)+count);
  move_to_pool(r,manager+0xcu,second);return first;
}
void free_entry(const Retained &r,std::uint32_t manager,std::uint32_t entry){
  const auto count=r.read(entry+0xau,2),available=r.read(manager+0x1cu);
  r.write(0x38075b,available,4,r.read(available)+count);
  const auto head=manager+4u;
  while(true){const auto next=r.read(entry);if(next==head||r.read(next+0xcu))break;
    entry=merge_entries(r,manager,entry,next);
  }
  while(true){const auto previous=r.read(entry+4u);if(previous==head||r.read(previous+0xcu))break;
    entry=merge_entries(r,manager,previous,entry);
  }
  const auto largest=r.read(manager+0x18u);
  if(!largest||r.read(largest+0xau,2)<r.read(entry+0xau,2))r.write(0x3807a1,manager+0x18u,4,entry);
}
} // namespace

std::uint32_t source_sound_sdk_construct_sample_pages(std::uint32_t manager,
    const SoundSdkSamplePageBindings &bindings,const SoundSdkDeviceMemory &memory,
    const SoundDriverStartupObserver &observer){
  const Retained r{memory,observer};r.write(0x38025f,manager+8u,4,manager+4u);
  r.write(0x380262,manager+4u,4,manager+4u);r.write(0x380267,manager,4,bindings.vtable_identity);
  r.write(0x38026d,manager+0x1cu,4,bindings.available_count_address);
  r.write(0x380274,manager+0x10u,4,manager+0xcu);r.write(0x380277,manager+0xcu,4,manager+0xcu);
  return manager;
}
std::uint32_t source_sound_sdk_initialize_sample_pages(std::uint32_t manager,
    std::uint32_t pages,const SoundSdkSamplePageServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.device.memory,observer};const auto count=pages&0xffffu;
  r.write(0x3802ab,r.read(manager+0x1cu),4,count);
  const auto pool=need(s.allocate,"Original SDK sample-page allocator unavailable")(0x61645344u,(count+1u)*20u,1u);
  r.write(0x3802d3,manager+0x14u,4,pool);if(!pool)return 0x8007000eu;
  initialize_entry(r,pool,0,pages,manager+4u);r.write(0x3802e9,manager+0x18u,4,pool);
  const auto head=manager+0xcu;
  for(std::uint32_t index=0;index<count;++index){
    const auto entry=r.read(manager+0x14u)+(index+1u)*20u;
    r.write(0x38030c,entry,4,head);const auto previous=r.read(head+4u);
    r.write(0x380311,entry+4u,4,previous);r.write(0x380314,head+4u,4,entry);
    r.write(0x38031a,r.read(entry+4u),4,entry);
  }
  return 0;
}
void source_sound_sdk_map_sample_pages(std::uint32_t first_index,std::uint32_t buffer,
    std::uint32_t pages,const SoundSdkSamplePageServices &s){
  need(s.lock_pages,"Original SDK sample-page pin owner unavailable")(buffer,pages<<12u,false);
  while(pages){
    const auto physical=need(s.physical_address,"Original SDK physical-address owner unavailable")(buffer);
    locked(s.device,[&]{
      const auto &fifo=need(s.device.fifo_available,"Original SDK sample-page FIFO unavailable");
      while((fifo()&0xfffffffcu)<8u){}
      need(s.device.device_write,"Original SDK sample-page device owner unavailable")(0xfe820804u,first_index);
      s.device.device_write(0xfe820808u,physical);
    });
    buffer+=0x1000u;++first_index;--pages;
  }
}
std::uint32_t source_sound_sdk_acquire_sample_pages(std::uint32_t manager,std::uint32_t buffer,
    std::uint32_t bytes,const SoundSdkSamplePageServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.device.memory,observer};const auto pages=((buffer&0xfffu)+bytes+0xfffu)>>12u;
  const auto base=buffer&0xfffff000u;std::uint32_t result=0;bool allocated=false;
  locked(s.device,[&]{
    const auto head=manager+4u;auto current=r.read(head);
    while(current!=head){
      if(r.read(current+0xcu)&&base==r.read(current+0x10u)&&
          (pages&0xffffu)==r.read(current+0xau,2)){
        r.write(0x380746,current+0xcu,4,r.read(current+0xcu)+1u);result=current;return;
      }
      current=r.read(current);
    }
    result=allocate_entry(r,manager,base,pages);allocated=result!=0;
  });
  if(allocated)source_sound_sdk_map_sample_pages(r.read(result+8u,2),base,pages&0xffffu,s);
  return result;
}
void source_sound_sdk_release_sample_pages(std::uint32_t manager,std::uint32_t entry,
    const SoundSdkSamplePageServices &s,const SoundDriverStartupObserver &observer){
  const Retained r{s.device.memory,observer};std::uint32_t buffer=0,pages=0;bool released=false;
  locked(s.device,[&]{
    const auto references=r.read(entry+0xcu)-1u;r.write(0x380874,entry+0xcu,4,references);
    if(references)return;
    buffer=r.read(entry+0x10u);pages=r.read(entry+0xau,2);free_entry(r,manager,entry);released=true;
  });
  if(released)need(s.lock_pages,"Original SDK sample-page unlock owner unavailable")(buffer,pages<<12u,true);
}
} // namespace halo2
