#include "halo2/sound_driver_owners.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
template<class F> const F &need(const F &f,const char *message){if(!f)throw std::runtime_error(message);return f;}
class Owner {
 const SoundDriverOwnersState &s;const SoundDriverOwnersServices &p;const SoundDriverOwnersObserver &observe;
 std::span<std::byte> memory(std::uint32_t a,std::size_t n)const{auto b=need(p.memory,"Source driver owner storage unavailable")(a,n);if(b.size()!=n)throw std::runtime_error("Source driver owner storage extent unavailable");return b;}
 std::uint32_t read(std::uint32_t a,unsigned n=4)const{auto b=memory(a,n);std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;}
 void write(std::uint32_t pc,std::uint32_t a,std::uint32_t v,unsigned n=4){auto b=memory(a,n);const auto before=read(a,n);for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(8*i));if(observe)observe({pc,a,n,before,v&(n==4?0xffffffffu:((1u<<(8*n))-1))});}
 static std::int32_t pending(std::uint32_t flags){const auto shifted=std::uint8_t(flags<<5);return std::int32_t(std::bit_cast<std::int8_t>(shifted))>>5;}
 void virtual_call(std::uint32_t identity,std::uint32_t slot,SoundDriverVirtualThis abi,std::span<std::byte> output={}){
  const auto vtable=read(identity),method=read(vtable+slot);
  need(p.native_virtual_call,"Retained source virtual method unavailable")({identity,vtable,slot,method,abi,output});
 }
 void unpin(std::uint32_t packet,std::uint32_t pc){const auto datum=read(packet+8),array=read(s.cache_address+0x44),address=array+(datum&0xffffu)*0x10+4;write(pc,address,read(address,1)-1,1);}
 void buffer_operation(SoundDriverStaticBufferOperation op,std::uint32_t identity,std::uint32_t a,std::uint32_t b,std::span<const std::uint32_t> bins={}){
  need(p.native_buffer_operation,"Retained source static buffer operation unavailable")({op,identity,a,b,bins});
 }
public:
 Owner(const SoundDriverOwnersState &state,const SoundDriverOwnersServices &services,const SoundDriverOwnersObserver &observer):s(state),p(services),observe(observer){}
 void release_packets(std::uint32_t record){
  while(pending(read(record+2,1))>0){
   const auto count=pending(read(record+2,1));const auto packet=read(record+0x18+std::uint32_t(count)*4);unpin(packet,0x2b4c21);
   const auto flags=read(record+2,1);write(0x2b4c3f,record+2,(flags&0xf8u)|((std::uint32_t(pending(flags))-1)&7),1);
  }
 }
 void complete_packet(std::uint32_t record,std::uint32_t packet){
  const auto flags=read(record+2,1);
  const auto middle=((flags>>3)-1)&7;
  const auto new_flags=(flags&0xc0u)|(middle<<3)|((flags-1)&7);
  write(0x2b48c7,record+2,new_flags,1);unpin(packet,0x2b48e6);
  // Exact memmove arguments and following source REP stores. The original
  // zero-length operation performs no access; wrapped negative extents must
  // still fail real backing, rather than being clamped to zero.
  const auto move_bytes=std::uint32_t(pending(read(record+2,1)))*4;
  if(move_bytes){auto from=memory(record+0x20,move_bytes),to=memory(record+0x1c,move_bytes);std::memmove(to.data(),from.data(),move_bytes);}
  // Native memmove represents the exact byte output, not fabricated
  // original per-instruction stores. Current controls stop before it.
  const auto count=pending(read(record+2,1));auto at=record+0x1c+std::uint32_t(count)*4;
  const auto clear_bytes=(2u-std::uint32_t(count))*4;
  if(clear_bytes)memory(at,clear_bytes);
  for(std::uint32_t i=0;i<(clear_bytes>>2);++i)write(0x2b492f,at+4*i,0);
  for(std::uint32_t i=0;i<(clear_bytes&3);++i)write(0x2b4936,at+(clear_bytes&~3u)+i,0,1);
  const auto final=pending(read(record+2,1));
  if(final==0)write(0x2b4966,record+1,(read(record+1,1)&0xf9u)|1,1);
  else if(final==1)write(0x2b4959,record+1,(read(record+1,1)&0xfau)|2,1);
 }
 void flush(){
  std::int32_t index=0;std::uint32_t offset=0;
  while(index<std::bit_cast<std::int16_t>(std::uint16_t(read(s.driver_address+4,2)))){
   const auto record=s.driver_address+offset+0xc;
   write(0x2213c4,record+3,read(record+3,1)|4,1);
   std::array<std::byte,4> status;
   virtual_call(read(record+0x24),0xc,SoundDriverVirtualThis::Stack,status);
   virtual_call(read(record+0x24),0x18,SoundDriverVirtualThis::Stack);
   virtual_call(read(record+0x30),0x1c,SoundDriverVirtualThis::Ecx);
   write(0x2213e7,record+1,read(record+1,1)&0xf8u,1);release_packets(record);
   write(0x2213f1,record+3,read(record+3,1)&0xfbu,1);
   const bool again=(read(record+1,1)&7)==1;
   write(0x2213fc,record+0xc,0xc2800000);
   if(again){
    std::array<std::byte,4> second_status;
    virtual_call(read(record+0x24),0xc,SoundDriverVirtualThis::Stack,second_status);
    virtual_call(read(record+0x24),0x18,SoundDriverVirtualThis::Stack);
    virtual_call(read(record+0x30),0x1c,SoundDriverVirtualThis::Ecx);
    write(0x221424,record+1,read(record+1,1)&0xf8u,1);release_packets(record);
    write(0x22142e,record+3,read(record+3,1)&0xfbu,1);
   }
   if(read(record+2,1)&0x38)write(0x22143b,record+2,read(record+2,1)&0xc7u,1);
   if(read(record+2,1)&7)write(0x221447,record+2,read(record+2,1)&0xf8u,1);
   ++index;offset+=0x34;
  }
 }
 void shutdown(){
  const auto queue=s.static_queue_address,voice=read(queue+8);
  if(!voice)return;
  virtual_call(voice,4,SoundDriverVirtualThis::Stack);
  const auto record=read(queue+4);write(0x2b4480,queue+8,0);
  const auto flags=read(record+2,1);write(0x2b4499,record+2,(flags&0xc7u)|((flags&7)<<3),1);
  while(pending(read(read(queue+4)+2,1))>0){const auto current=read(queue+4);complete_packet(current,read(current+0x1c));}
 }
 bool static_buffer(std::uint32_t index){
  const auto record=s.driver_address+index*0x3c+0x1a18,slot=record+0x34;
  const auto result=need(p.native_buffer_create,"Real source static buffer constructor unavailable")({read(s.driver_address+0x2ab0),slot,0,{0x18,0x2010,0,0,0,0}});
  if(result<0){write(0x222c8a,record+0x38,0);return false;}
  const std::array<std::uint32_t,10> bins{6,0,8,0,7,0,9,0,10,0};
  buffer_operation(SoundDriverStaticBufferOperation::MixBins,read(slot),5,0,bins);
  write(0x222c43,record,0x1f);
  buffer_operation(SoundDriverStaticBufferOperation::MaxDistance,read(slot),0x7f7fffff,1);
  buffer_operation(SoundDriverStaticBufferOperation::MinDistance,read(slot),0x7f7fffff,1);
  buffer_operation(SoundDriverStaticBufferOperation::RolloffFactor,read(slot),0,1);
  buffer_operation(SoundDriverStaticBufferOperation::DopplerFactor,read(slot),0,1);
  buffer_operation(SoundDriverStaticBufferOperation::ConeOutsideVolume,read(slot),0,1);
  write(0x222c7e,record+0x38,0);return true;
 }
};
}
void source_sound_driver_stream_flush(const SoundDriverOwnersState &s,const SoundDriverOwnersServices &p,const SoundDriverOwnersObserver &o){Owner(s,p,o).flush();}
void source_sound_driver_queue_shutdown(const SoundDriverOwnersState &s,const SoundDriverOwnersServices &p,const SoundDriverOwnersObserver &o){Owner(s,p,o).shutdown();}
void source_sound_driver_release_packets(const SoundDriverOwnersState &s,std::uint32_t r,const SoundDriverOwnersServices &p,const SoundDriverOwnersObserver &o){Owner(s,p,o).release_packets(r);}
void source_sound_driver_complete_packet(const SoundDriverOwnersState &s,std::uint32_t r,std::uint32_t packet,const SoundDriverOwnersServices &p,const SoundDriverOwnersObserver &o){Owner(s,p,o).complete_packet(r,packet);}
bool source_sound_driver_static_buffer_construct(const SoundDriverOwnersState &s,std::uint32_t i,const SoundDriverOwnersServices &p,const SoundDriverOwnersObserver &o){return Owner(s,p,o).static_buffer(i);}
} // namespace halo2
