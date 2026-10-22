#include "halo2/sound_source_lifecycle.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::int32_t signed32(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::int32_t signed16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
std::int32_t signed8(std::uint32_t v){return std::bit_cast<std::int8_t>(std::uint8_t(v));}
template<class F>const F &need(const F &f,const char *message){if(!f)throw std::runtime_error(message);return f;}
class Owner {
  const SoundSourceLifecycleBindings &b;
  const SoundSourceLifecycleServices &s;
  const SoundStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,unsigned size)const{
    auto out=need(s.memory,"Original sound lifecycle backing unavailable")(address,size);
    if(out.size()!=size)throw std::runtime_error("Original sound lifecycle backing extent unavailable");
    return out;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4)const{
    const auto bytes=memory(address,size);std::uint32_t result=0;
    for(unsigned i=0;i<size;++i)result|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return result;
  }
  void write(std::uint32_t address,std::uint32_t value,unsigned size=4)const{
    const auto before=read(address,size);auto bytes=memory(address,size);
    for(unsigned i=0;i<size;++i)bytes[i]=std::byte(value>>(8*i));
    if(observe)observe({0,address,size,before,value&(size==4?none:((1u<<(size*8))-1))});
  }
  std::uint32_t element(std::uint32_t pool,std::uint32_t datum,std::uint32_t stride)const{
    return read(pool+0x44)+(datum&65535u)*stride;
  }
  std::uint32_t next(std::uint32_t pool,std::uint32_t start)const{
    const auto high=signed32(read(pool+0x38));
    if(signed32(start)<0 || signed32(start)>=high)return none;
    const auto bitmap=read(pool+0x48);
    for(auto i=start;signed32(i)<high;++i)if(read(bitmap+(i>>5)*4)&(1u<<(i&31)))return i;
    return none;
  }
  std::uint32_t datum(std::uint32_t pool,std::uint32_t index)const{
    return (read(read(pool+0x44)+read(pool+0x24)*index,2)<<16)|index;
  }
  void erase(std::uint32_t pool,std::uint32_t handle)const{
    const auto slot=handle&65535u,stride=read(pool+0x24),record=element(pool,handle,stride);
    if(read(pool+0x2a,1)&8){
      unsigned i=0;for(;i+4<=stride;i+=4)write(record+i,0xbabababau);
      for(;i<stride;++i)write(record+i,0xba,1);
    }
    const auto bitmap=read(pool+0x48)+(slot>>5)*4;
    write(bitmap,read(bitmap)&~(1u<<(slot&31)));write(record,0,2);
    if(signed32(slot)<signed32(read(pool+0x34)))write(pool+0x34,slot);
    if(slot+1==read(pool+0x38)){
      auto previous=record-stride;
      for(;;){
        const auto high=read(pool+0x38)-1;write(pool+0x38,high);
        if(signed32(high)<=0 || read(previous,2))break;
        previous-=stride;
      }
    }
    write(pool+0x3c,read(pool+0x3c)-1);
  }
  void detach(std::uint32_t handle)const{
    if(handle==none)return;
    const auto pool=read(b.controllers),record=element(pool,handle,0x1c);
    const auto old=read(record+2,1),value=(old&0x80)|((old-1)&0x7f);
    write(record+2,value,1);if(!(value&0x7f))erase(pool,handle);
  }
  void effects()const{
    const auto pool=read(b.effects);
    for(auto slot=next(pool,0);slot!=none;slot=next(pool,slot+1)){
      const auto handle=datum(pool,slot),record=element(pool,handle,0x14),controller=read(record+8);
      if(controller!=none){detach(controller);write(record+8,none);}
      erase(pool,handle);
    }
  }
  void unpin(std::uint32_t source)const{
    if(!(read(source+4,1)&2))return;
    const auto definition=read(read(b.tag_table)+(read(source+0xc)&65535u)*16+8);
    auto index=std::uint32_t(signed16(read(definition+8,2))+signed8(read(source+0x9c,1)));
    const auto bank=read(b.ugh);
    index=std::uint32_t(signed16(read(read(bank+0x24)+index*12+8,2))+signed8(read(source+0x9d,1)));
    index=std::uint32_t(signed16(read(read(bank+0x2c)+index*16+0xc,2))+signed16(read(source+0x9e,2)));
    const auto cache_datum=read(read(bank+0x44)+index*12+8);
    const auto cache=element(read(b.cache_pool),cache_datum,16);
    write(cache+4,read(cache+4,1)-1,1);write(source+4,read(source+4,1)&0xfd,1);
  }
  void shared_voice(std::uint32_t handle)const{
    if(handle==none)return;
    const auto pool=read(b.shared_voices),record=element(pool,handle,0x6c);
    write(record+0xa,read(record+0xa,2)-1,2);
    if(read(record+0xa,2))return;
    const auto index=read(record+2,2);
    if(index!=65535u){
      const auto signed_index=signed16(index);
      const auto address=read(b.system)+0x70+std::uint32_t(signed_index>>5)*4;
      write(address,read(address)&~(1u<<(index&31)));
    }
    erase(pool,handle);
  }
  void stop_stream(std::uint32_t record)const{
    if(!(read(record+1,1)&7))return;
    const auto identity=read(record+0x24),method=read(read(identity)+0xc);
    const auto status=need(s.buffer_status,"Original sound buffer status service unavailable")(identity,method);
    if(status&0x20000)need(s.buffer_stop,"Original sound buffer stop service unavailable")(read(record+0x24));
    need(s.buffer_stop_ex,"Original sound buffer StopEx service unavailable")(read(record+0x24),3);
    const auto stream=read(record+0x30),flush=read(read(stream)+0x1c);
    need(s.stream_flush,"Original sound stream flush service unavailable")(stream,flush);
    write(record+3,read(record+3,1)|4,1);
  }
  void release_voice(std::uint32_t index)const{
    const auto voice=read(b.static_voices)+std::uint32_t(signed16(index))*0x24;
    write(voice,none);
    const auto bank=read(b.ugh),pool=read(b.cache_pool);
    for(const auto offset:{0x20u,0x1cu}){
      const auto definition=read(voice+offset);
      if(definition){
        const auto chunk=std::uint32_t(signed16(read(definition+0xc,2))+signed16(read(voice+(offset==0x20?0x1a:0x18),2)));
        const auto handle=read(read(bank+0x44)+chunk*12+8),cache=element(pool,handle,16);
        write(cache+4,read(cache+4,1)-1,1);write(voice+offset,0);
      }
    }
    const auto stream_index=read(voice+0xc,2);
    if(stream_index!=65535u){
      stop_stream(read(b.driver)+0xc+std::uint32_t(signed16(stream_index))*0x34);
      const auto bit=std::uint32_t(signed16(read(voice+0xc,2)));
      const auto address=read(b.system)+0x60+std::uint32_t(signed32(bit)>>5)*4;
      write(address,read(address)&~(1u<<(bit&31)));
    }
    shared_voice(read(voice+4));
    write(voice+0xa,0,1);write(voice+0xb,0,1);write(voice+9,0,1);
    write(voice+0xc,65535,2);write(voice+0xe,65535,2);write(voice+4,none);
  }
  void remove(std::uint32_t handle)const{
    const auto pool=read(b.sources),source=element(pool,handle,0xbc),controller=read(source+0xa8);
    if(controller!=none){
      const auto record=element(read(b.controllers),controller,0x1c);
      if(read(source+3,1)==255 && read(record+4)!=none)write(record+3,(read(record+3,1)-1)&0x7f,1);
      detach(read(source+0xa8));
    }
    erase(pool,handle);
  }
public:
  Owner(const SoundSourceLifecycleBindings &bindings,const SoundSourceLifecycleServices &services,
        const SoundStartupObserver &observer):b(bindings),s(services),observe(observer){}
  void stop(std::uint32_t handle,std::uint32_t reason)const{
    const auto source=element(read(b.sources),handle,0xbc),voice=read(source+0xac,2);
    if(voice!=65535u){release_voice(voice);write(source+0xac,65535,2);}
    unpin(source);
    if(const auto callback=read(source+0x14))if(const auto method=read(callback+0x10)){
      need(s.source_callback,"Original sound source callback unavailable")({callback,method,read(source+0x10),handle,reason});
    }
    remove(handle);
  }
  void reset()const{
    if(!read(read(b.system)+0x78,1))return;
    effects();auto pool=read(b.sources);
    for(auto slot=next(pool,0);slot!=none;slot=next(pool,slot+1)){
      const auto handle=datum(pool,slot),source=element(pool,handle,0xbc);
      if(!read(source+2,1)){stop(handle,10);pool=read(b.sources);}
    }
  }
};
}
void source_sound_round_reset(const SoundSourceLifecycleBindings &b,const SoundSourceLifecycleServices &s,
                             const SoundStartupObserver &o){Owner(b,s,o).reset();}
void source_sound_stop(const SoundSourceLifecycleBindings &b,std::uint32_t source,std::uint32_t reason,
                       const SoundSourceLifecycleServices &s,const SoundStartupObserver &o){Owner(b,s,o).stop(source,reason);}
} // namespace halo2
