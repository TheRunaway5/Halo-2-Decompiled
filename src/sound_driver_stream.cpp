#include "halo2/sound_driver_stream.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
template<class F>const F &need(const F &f,const char *message) {
  if(!f)throw std::runtime_error(message);
  return f;
}
class Owner {
  const SoundDriverStreamConstants &c;const SoundDriverStreamServices &p;
  const SoundDriverStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,std::size_t size) const {
    auto result=need(p.memory,"Source stream storage unavailable")(address,size);
    if(result.size()!=size)throw std::runtime_error("Source stream storage extent unavailable");
    return result;
  }
  std::uint32_t read(std::uint32_t address,unsigned bytes=4) const {
    const auto data=memory(address,bytes);std::uint32_t value=0;
    for(unsigned i=0;i<bytes;++i)value|=std::to_integer<std::uint32_t>(data[i])<<(8*i);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned bytes=4) {
    auto data=memory(address,bytes);const auto before=read(address,bytes);
    const auto mask=bytes==4?0xffffffffu:((1u<<(8*bytes))-1);value&=mask;
    for(unsigned i=0;i<bytes;++i)data[i]=std::byte(value>>(8*i));
    if(observe)observe({pc,address,bytes,before,value});
  }
public:
  Owner(const SoundDriverStreamConstants &constants,const SoundDriverStreamServices &services,
        const SoundDriverStartupObserver &observer):c(constants),p(services),observe(observer) {}
  void format(std::uint32_t category,std::uint32_t target,std::uint32_t queue_slot) {
    if(category==2) {
      write(0x220588,target,1,2);write(0x22058d,target+2,2,2);
      write(0x220593,target+4,44100);write(0x22059a,target+8,176400);
      write(0x2205a1,target+12,4,2);write(0x2205a7,target+14,16,2);
      write(0x2205ad,target+16,0,2);write(0x2205b3,queue_slot,c.pcm_queue);return;
    }
    write(0x220531,target,0x69,2);write(0x220536,target+14,4,2);
    const auto bits=category==1?c.stereo_bits:c.one_bits;
    // Actual format owner passes only the two immutable integer-valued floats
    // to its complete CRT helper. Do not invent other conversion semantics.
    std::uint32_t channels;
    if(bits==0x3f800000u)channels=1;
    else if(bits==0x40000000u)channels=2;
    else throw std::runtime_error("Source stream CRT operand outside authenticated domain");
    write(0x220553,target+2,channels,2);write(0x220557,target+4,44100);
    write(0x22055e,target+16,2,2);write(0x220564,target+18,64,2);
    const auto alignment=(channels*36)&0xffffu;
    write(0x220570,target+12,alignment,2);write(0x22057d,target+8,alignment*689);
    write(0x220580,queue_slot,c.adpcm_queue);
  }
  bool construct(std::uint32_t driver,std::uint32_t index,std::uint32_t category,
                 const SoundDriverStreamScratch &scratch) {
    const auto descriptor=scratch.descriptor,record=driver+index*0x34+0xc;
    for(unsigned i=0;i<4;++i)write(0x221609+4*i,descriptor+4*i,0);
    write(0x22161a,descriptor+16,0);write(0x22161f,descriptor+20,0);
    write(0x221631,scratch.queue_slot,0);format(category,scratch.format,scratch.queue_slot);
    write(0x221644,descriptor+8,scratch.format);
    const auto queue=read(scratch.queue_slot);
    write(0x221651,descriptor,0);write(0x221659,descriptor+4,2);
    write(0x221661,descriptor+12,c.completion_callback);
    write(0x221669,descriptor+16,index);
    write(0x2b4975,record+0x30,queue);
    const auto method=read(read(queue));
    if(queue==c.pcm_queue&&method==c.pcm_initialize_method)
      write(0x2b4534,queue+4,record);
    else if(!(queue==c.adpcm_queue&&method==c.adpcm_initialize_method))
      throw std::runtime_error("Source stream queue method unavailable");
    write(0x2b4991,descriptor,read(descriptor)|0x20000000u);
    SoundDriverStreamCreateRequest request{read(driver+0x2ab0),descriptor,record+0x24,0,{},{}};
    for(unsigned i=0;i<6;++i)request.descriptor[i]=read(descriptor+4*i);
    const auto format_bytes=memory(request.descriptor[2],20);
    std::copy(format_bytes.begin(),format_bytes.end(),request.format.begin());
    const auto result=need(p.native_create_stream,"Real native stream creation unavailable")(request);
    need(p.native_pause_stream,"Real native stream pause unavailable")(read(record+0x24),0);
    const auto kind=std::uint16_t(category)==2?1u:
        std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(category))));
    const auto flags=read(record+1,1);
    const auto next=(flags&~0x38u)|((kind<<3)&0x38u);
    write(0x2b49d4,record+2,read(record+2,1)&0xc0u,1);
    write(0x2b49d8,record+1,next,1);write(0x2b49db,record+0x28,0xffffffffu);
    write(0x2b49e7,record+0x1c,0);write(0x2b49e9,record+0x20,0);
    return result>=0;
  }
};
}
void source_sound_driver_stream_format(std::uint32_t category,std::uint32_t format,
    std::uint32_t queue_slot,const SoundDriverStreamConstants &c,
    const SoundDriverStreamServices &p,const SoundDriverStartupObserver &o) {
  Owner(c,p,o).format(category,format,queue_slot);
}
bool source_sound_driver_stream_construct(std::uint32_t driver,std::uint32_t index,
    std::uint32_t category,const SoundDriverStreamScratch &scratch,
    const SoundDriverStreamConstants &c,const SoundDriverStreamServices &p,
    const SoundDriverStartupObserver &o) {
  return Owner(c,p,o).construct(driver,index,category,scratch);
}
} // namespace halo2
