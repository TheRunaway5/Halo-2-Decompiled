#include "halo2/sound_sdk_voice_initialization.hpp"
#include <cfenv>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace halo2 {namespace {
template<class F>const F &need(const F &f,const char *message){if(!f)throw std::runtime_error(message);return f;}
void guarded(const std::function<void(const std::function<void()> &)> &guard,const std::function<void()> &body){
  unsigned calls=0;
  need(guard,"Original SDK voice initialization guard unavailable")([&]{
    if(calls++)throw std::runtime_error("Original SDK voice initialization guard body repeated");
    body();
  });
  if(calls!=1)throw std::runtime_error("Original SDK voice initialization guard body skipped");
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
struct FloatingPointScope {
  std::fenv_t saved;unsigned mxcsr;
  explicit FloatingPointScope(const SoundDriverFloatingPointContext &fp):mxcsr(_mm_getcsr()){
    if((fp.x87_control_word&0x3fu)!=0x3fu||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
      throw std::runtime_error("Unmasked SDK voice initialization floating-point exceptions unavailable");
    if(std::fegetenv(&saved)!=0)throw std::runtime_error("SDK voice initialization floating-point state unavailable");
    _mm_setcsr(fp.mxcsr);asm volatile("fldcw %0"::"m"(fp.x87_control_word):"memory");
  }
  ~FloatingPointScope(){std::fesetenv(&saved);_mm_setcsr(mxcsr);}
};
#endif
class Owner {
  const SoundSdkVoiceInitializationData &c;
  const SoundSdkVoiceInitializationServices &s;
  const SoundDriverStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,unsigned size) const {
    auto raw=need(s.memory,"Original SDK voice initialization memory unavailable")(address,size);
    if(raw.size()!=size)throw std::runtime_error("Original SDK voice initialization extent differs");
    return raw;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4) const {
    std::span<const std::byte> raw;
    for(const auto &region:c.immutable)if(address>=region.identity&&
        std::uint64_t(address)-region.identity<region.bytes.size()){
      const auto offset=std::uint64_t(address)-region.identity;
      if(offset+size>region.bytes.size())throw std::runtime_error("Original SDK voice immutable extent differs");
      raw=std::span<const std::byte>(region.bytes).subspan(offset,size);break;
    }
    if(raw.empty())raw=memory(address,size);
    std::uint32_t value=0;for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,unsigned size,std::uint32_t value) const {
    const auto raw=memory(address,size);std::uint32_t before=0;
    for(unsigned i=0;i<size;++i){before|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);raw[i]=std::byte(value>>(8*i));}
    const auto mask=size==4?0xffffffffu:(1u<<(size*8))-1u;
    if(observe)observe({pc,address,size,before,value&mask});
  }
  void basic_format(std::uint32_t output,std::uint32_t input,bool adpcm) const {
    write(adpcm?0x37da1fu:0x37d9f6u,output,2,adpcm?0x69u:1u);
    write(adpcm?0x37da27u:0x37d9feu,output+2u,1,read(input+2u,1));
    write(adpcm?0x37da2au:0x37da04u,output+3u,1,adpcm?4u:read(input+0xeu,1));
    write(adpcm?0x37da31u:0x37da0au,output+4u,4,read(input+4u));
    write(adpcm?0x37da38u:0x37da11u,output+8u,4,read(input+0xcu,1));
  }
  std::uint32_t convert_format(std::uint32_t output,std::uint32_t input) const {
    const auto tag=read(input,2);
    if(tag==1)basic_format(output,input,false);
    else if(tag==0x69)basic_format(output,input,true);
    else if(tag==0xfffe){
      bool equal=true;
      for(unsigned i=0;i<4;++i)if(read(input+0x18u+i*4u)!=c.pcm_guid[i]){equal=false;break;}
      if(equal)basic_format(output,input,false);
      else {
        equal=true;
        for(unsigned i=0;i<4;++i)if(read(input+0x18u+i*4u)!=c.adpcm_guid[i]){equal=false;break;}
        if(equal)basic_format(output,input,true);
      }
      // Original mask read follows conversion, including input/output aliases.
      return read(input+0x14u);
    }
    return 0;
  }
  std::uint32_t frequency(std::uint32_t rate,const SoundDriverFloatingPointContext &fp) const {
    std::uint32_t result=0;
    guarded(s.under_state_lock,[&]{
      if(rate==48000u)return;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
      FloatingPointScope scope(fp);std::uint32_t scaled;const std::uint32_t octave=0x45800000u;
      if(rate&0x80000000u)asm volatile("fildl %1\n\tfadds %2\n\tfmuls %3\n\tfstps %0"
          :"=m"(scaled):"m"(rate),"m"(c.unsigned_correction_bits),"m"(c.frequency_reciprocal_bits):"st");
      else asm volatile("fildl %1\n\tfmuls %2\n\tfstps %0"
          :"=m"(scaled):"m"(rate),"m"(c.frequency_reciprocal_bits):"st");
      asm volatile("flds %1\n\tflds %2\n\tfyl2x\n\tfistpl %0"
          :"=m"(result):"m"(octave),"m"(scaled):"st","st(1)");
#else
      (void)fp;throw std::runtime_error("Original x87 SDK voice frequency backend unavailable");
#endif
    });
    return result;
  }
  void parent_channel(std::uint32_t identity,std::uint32_t bin) const {
    guarded(s.under_irql_guard,[&]{
      const auto count=read(identity+0x24u);std::uint32_t matched=0;
      for(std::uint32_t i=0;i<count;++i)if(read(identity+0x28u+i,1)==bin)++matched;
      const auto channels=read(identity+0xeu,1);
      if(matched>=channels)return;
      auto remaining=channels-matched;
      if(count+remaining>8u)write(0x37a549u,identity+0x24u,4,8u-remaining);
      while(remaining--){
        const auto index=read(identity+0x24u);
        write(0x37a556u,identity+0x28u+index,1,bin);
        write(0x37a55au,identity+0x24u,4,read(identity+0x24u)+1u);
      }
    });
  }
  void bins(std::uint32_t identity,const std::function<std::uint32_t()> &count,
      const std::function<std::uint32_t(std::uint32_t)> &id,
      const std::function<std::uint32_t(std::uint32_t)> &gain) const {
    std::uint32_t i=0;
    if(count())do {
      const auto bin=id(i)&0xffu;
      write(0x37bccdu,identity+0x28u+i,1,bin);
      const auto level=gain(i);
      ++i;write(0x37bcdcu,identity+0x30u+bin*4u,4,level);
    }while(i<count());
    write(0x37bce7u,identity+0x24u,4,count());
    if(const auto parent=read(identity+0xb0u)){
      const auto data=read(parent+0x1cu);
      if(read(data+8u)&0x82000u)parent_channel(identity,read(data+0xd4u));
    }
    if(!(read(identity+8u,1)&0x10u))return;
    std::uint32_t source;
    if(read(identity+0x24u)>=5u&&read(identity+0x2cu,1)==2u){
      // The original source pointer is loaded after both routing predicates.
      source=read(identity+0xb4u);
      if(!(read(source+0x58u,1)&2u)){
        write(0x37bd2eu,source+0x58u,4,read(source+0x58u)|2u);
        source=read(identity+0xb4u);
        write(0x37bd33u,source+3u,1,read(source+3u,1)|4u);
      }
    }else {
      source=read(identity+0xb4u);
      if(read(source+0x58u,1)&2u){
        write(0x37bd46u,source+0x58u,4,read(source+0x58u)&0xfffffffdu);
        source=read(identity+0xb4u);
        write(0x37bd4cu,source+3u,1,read(source+3u,1)|4u);
      }
    }
  }
  std::uint32_t initialize(std::uint32_t identity,std::uint32_t flags,
      std::uint32_t format_identity,std::uint32_t bins_identity,const SoundDriverFloatingPointContext &fp) const {
    write(0x37cdd9u,identity+8u,4,flags);
    write((flags&0x182000u)||(flags&0x200010u)?0x37cddeu:0x37cdeau,
        identity+0x20u,4,(flags&0x182000u)||(flags&0x200010u)?0u:0x258u);
    if(flags&0x10u){
      const auto allocation=need(s.allocate,"Original SDK source allocation owner unavailable")(0x61645344u,0xa4u,1);
      write(0x37ce18u,identity+0xb4u,4,allocation);
      if(!allocation)return 0x8007000eu;
      for(unsigned i=0;i<18;++i)write(0x37ce31u,allocation+8u+i*4u,4,c.source_defaults[i]);
      auto source=read(identity+0xb4u);
      for(unsigned i=0;i<9;++i)write(0x37ce47u,source+0x80u+i*4u,4,c.environment_defaults[i]);
      source=read(identity+0xb4u);write(0x37ce4fu,source,4,0x07ff0000u);
      source=read(identity+0xb4u);write(0x37ce5cu,source+0x7cu,4,0x007f0000u);
    }
    if(read(identity+8u)&0x400010u){
      const auto allocation=need(s.allocate,"Original SDK parameter allocation owner unavailable")(0x61645344u,0x38u,1);
      write(0x37ce7bu,identity+0xb8u,4,allocation);
      write(0x37ce8fu,identity+0xbu,1,read(identity+0xbu,1)|1u);
      if(!allocation)return 0x8007000eu;
    }
    if(!format(identity,format_identity,bins_identity==0u,fp))mix_bins(identity,bins_identity);
    write(0x37cec2u,identity+0x1cu,4,0u-read(identity+0x20u));
    return 0;
  }
public:
  Owner(const SoundSdkVoiceInitializationData &data,const SoundSdkVoiceInitializationServices &services,
      const SoundDriverStartupObserver &observer):c(data),s(services),observe(observer){}
  void mix_bins(std::uint32_t identity,std::uint32_t payload) const {
    if(!payload){
      if(read(identity+8u)&0x200010u)payload=c.spatial_bins_identity;
      else if(read(identity+0xb0u))payload=c.parent_bins_identity;
      else payload=read(c.channel_bins_table_identity+(read(identity+0xeu,1)>>1)*4u);
    }
    bins(identity,[&]{return read(payload);},[&](auto i){return read(read(payload+4u)+i*8u,1);},
        [&](auto i){return read(read(payload+4u)+i*8u+4u);});
  }
  std::uint32_t format(std::uint32_t identity,std::uint32_t input,std::uint32_t default_bins,
      const SoundDriverFloatingPointContext &fp) const {
    auto mask=convert_format(identity+0xcu,input);std::uint32_t result=0;
    if(mask&&!(read(identity+8u)&0x200010u)&&default_bins){
      std::array<std::uint32_t,8> selected{};unsigned count=0;
      while(mask&&count<8u){
        unsigned bit=0;while(!(mask&(1u<<bit)))++bit;
        selected[count++]=bit;mask&=~(1u<<bit);
      }
      bins(identity,[&]{return count;},[&](auto i){return selected.at(i);},[](auto){return 0u;});
      result=1;
    }
    write(0x37c8a0u,identity+0x18u,4,frequency(read(identity+0x10u),fp));
    return result;
  }
  std::uint32_t buffer(std::uint32_t identity,std::uint32_t input,std::uint32_t size) const {
    if(read(identity+8u)&0x80000000u)write(0x37c8d3u,identity+0xbcu,4,0);
    else if(const auto allocation=read(identity+0xbcu)){
      need(s.release_allocation,"Original SDK allocation release owner unavailable")(allocation);
      write(0x37c8d3u,identity+0xbcu,4,0);
    }
    write(0x37c8deu,identity+0xc0u,4,size);std::uint32_t status=0;
    if(size){
      if(input){
        write(0x37c8edu,identity+8u,4,read(identity+8u)|0x80000000u);
        write(0x37c8f0u,identity+0xbcu,4,input);
      }else {
        write(0x37c8f8u,identity+0xbu,1,read(identity+0xbu,1)&0x7fu);
        const auto allocation=need(s.allocate,"Original SDK buffer allocation owner unavailable")(0x61645344u,size,0);
        write(0x37c908u,identity+0xbcu,4,allocation);if(!allocation)status=0x8007000eu;
      }
    }
    const auto bytes=read(identity+0xc0u);
    write(0x37c1b7u,identity+0xccu,4,0);write(0x37c1beu,identity+0xc4u,4,0);
    write(0x37c1c8u,identity+0xc8u,4,bytes);write(0x37c1ceu,identity+0xd0u,4,bytes);
    return status;
  }
  std::uint32_t descriptor(std::uint32_t identity,std::uint32_t descriptor,const SoundDriverFloatingPointContext &fp) const {
    const auto flags=read(descriptor+4u);
    const auto input=(flags&0x182000u)?c.default_format_identity:read(descriptor+0xcu);
    auto status=initialize(identity,flags,input,read(descriptor+0x10u),fp);
    if(status&0x80000000u)return status;
    const auto mode=read(descriptor+4u);
    if(mode&0x180000u)write(0x37cf29u,identity+0xd4u,4,read(descriptor+0x14u));
    else if(mode&0x2000u)write(0x37cf36u,identity+0xd4u,4,0x1f);
    if(const auto bytes=read(descriptor+8u))status=buffer(identity,0,bytes);
    return status;
  }
};
} // namespace
std::uint32_t source_sound_sdk_initialize_voice_data(std::uint32_t identity,std::uint32_t descriptor,
    const SoundSdkVoiceInitializationData &data,const SoundDriverFloatingPointContext &fp,
    const SoundSdkVoiceInitializationServices &services,const SoundDriverStartupObserver &observer){
  return Owner(data,services,observer).descriptor(identity,descriptor,fp);
}
std::uint32_t source_sound_sdk_voice_format(std::uint32_t identity,std::uint32_t format,std::uint32_t default_bins,
    const SoundSdkVoiceInitializationData &data,const SoundDriverFloatingPointContext &fp,
    const SoundSdkVoiceInitializationServices &services,const SoundDriverStartupObserver &observer){
  return Owner(data,services,observer).format(identity,format,default_bins,fp);
}
void source_sound_sdk_voice_mix_bins(std::uint32_t identity,std::uint32_t bins,const SoundSdkVoiceInitializationData &data,
    const SoundSdkVoiceInitializationServices &services,const SoundDriverStartupObserver &observer){
  Owner(data,services,observer).mix_bins(identity,bins);
}
std::uint32_t source_sound_sdk_voice_buffer(std::uint32_t identity,std::uint32_t buffer,std::uint32_t bytes,
    const SoundSdkVoiceInitializationData &data,const SoundSdkVoiceInitializationServices &services,
    const SoundDriverStartupObserver &observer){
  return Owner(data,services,observer).buffer(identity,buffer,bytes);
}
} // namespace halo2
