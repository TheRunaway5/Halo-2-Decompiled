#include "halo2/sound_submit.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
template<class T> const T &need(const T &f,const char *why) {
  if(!f)throw std::runtime_error(why);
  return f;
}
class Owner {
  const SoundSubmitState &s;
  const SoundSubmitServices &service;
  const SoundStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,std::size_t size)const {
    auto bytes=need(service.memory,"Original sound submit raw memory unavailable")(address,size);
    if(bytes.size()!=size)throw std::runtime_error("Original sound submit raw memory extent unavailable");
    return bytes;
  }
public:
  Owner(const SoundSubmitState &state,const SoundSubmitServices &services,
        const SoundStartupObserver &observer):s(state),service(services),observe(observer){}
  std::uint32_t read(std::uint32_t address,unsigned size=4)const {
    auto b=memory(address,size);std::uint32_t out=0;
    for(unsigned i=0;i<size;++i)out|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
    return out;
  }
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned size=4)const {
    const auto before=read(address,size);auto b=memory(address,size);
    for(unsigned i=0;i<size;++i)b[i]=std::byte(value>>(8*i));
    if(observe)observe({pc,address,size,before,value&(size==4?0xffffffffu:((1u<<(8*size))-1))});
  }
  bool available()const {
    return read(s.system+0x78,1)&&read(s.system+0x79,1)&&read(s.system+0x7a,1);
  }
  std::uint32_t definition(std::uint32_t sound)const {
    return read(s.tag_table+(sound&0xffff)*16+8);
  }
  bool sound_group(std::uint32_t sound)const {
    const auto index=std::bit_cast<std::int16_t>(std::uint16_t(sound));
    return read(s.tag_table+std::uint32_t(std::int32_t(index)*16))==0x736e6421;
  }
  std::uint32_t playback(std::uint32_t sound)const {
    const auto index=std::bit_cast<std::int8_t>(std::uint8_t(read(definition(sound)+0xd,1)));
    if(index==-1)return 0;
    return read(s.current_ugh+0x34)+std::uint32_t(std::int32_t(index)*0x34);
  }
  std::uint32_t ordinary(std::uint32_t sound)const {
    if(sound==0xffffffffu || !sound_group(sound))return 0;
    return playback(sound);
  }
  std::uint32_t custom(std::uint32_t sound,std::uint32_t interface)const {
    if(interface==0xffffffffu)return ordinary(sound);
    switch(interface>>30) {
      case 0:if(!sound_group(sound))return 0;return playback(sound);
      case 1:{
        if(!read(s.current_matg+0xc0))return 0;
        const auto p=read(s.current_matg+0xc4);if(!p)return 0;
        const auto datum=read(p+0xc);if(datum==0xffffffffu)return 0;
        return read(definition(datum)+4)+4+(interface&0x3fffffffu)*0x38;
      }
      case 2:{
        const auto datum=read(s.current_scenario+0x324);if(datum==0xffffffffu)return 0;
        return read(definition(datum)+4)+4+(interface&0x3fffffffu)*0x38;
      }
      default:return 0;
    }
  }
  bool controller(std::uint32_t sound,std::uint32_t interface,std::uint32_t &output)const {
    const auto custom_row=custom(sound,interface),default_row=ordinary(sound);
    auto chosen=custom_row;
    if(!(custom_row&&read(custom_row+0x2c))) {
      if(interface!=0xffffffffu || !default_row || !read(default_row+0x2c))return false;
      chosen=default_row;
    }
    if(!chosen)return false;
    //21FE7E/85 still dereference originalESI (custom_row), including when the
    //fallback chose EAX. Preserve its actual NULL/context stop; don't fix it.
    if(std::bit_cast<std::int32_t>(read(custom_row+0x2c))<=0)return false;
    const auto request=read(custom_row+0x30);if(!request)return false;
    const auto count=std::bit_cast<std::int32_t>(read(request+8));output=request;
    return count>0;
  }
  static std::int32_t signed32(std::uint32_t v) {return std::bit_cast<std::int32_t>(v);}
  std::uint32_t allocate_pool(std::uint32_t pool)const {
    auto scan=read(pool+0x34);const auto high=read(pool+0x38);auto chosen=0xffffffffu;
    while(signed32(scan)<signed32(high)) {
      const auto word=read(pool+0x48)+std::uint32_t((signed32(scan)>>5)*4);
      if(!(read(word)&(1u<<(scan&31)))) {if(scan!=0xffffffffu)chosen=scan;break;}
      ++scan;if(signed32(scan)>=signed32(read(pool+0x38)))break;
    }
    if(chosen==0xffffffffu) {
      if(signed32(high)>=signed32(read(pool+0x20)) || high==0xffffffffu)return 0xffffffffu;
      chosen=high;
    }
    auto row=read(pool+0x44)+read(pool+0x24)*chosen;
    const auto bitmap=read(pool+0x48)+std::uint32_t((signed32(chosen)>>5)*4);
    write(0x16d32e,bitmap,read(bitmap)|(1u<<(chosen&31)));
    const auto count=read(pool+0x3c),extent=read(pool+0x38);
    write(0x16d33c,pool+0x3c,count+1);write(0x16d33f,pool+0x34,chosen+1);
    if(signed32(extent)<=signed32(chosen))write(0x16d344,pool+0x38,chosen+1);
    const auto stride=read(pool+0x24);unsigned n=0;
    for(;n<(stride&~3u);n+=4)write(0x16d353,row+n,0);
    for(;n<stride;++n)write(0x16d35a,row+n,0,1);
    const auto salt=read(pool+0x40,2);write(0x16d360,row,salt,2);
    write(0x16d363,pool+0x40,salt+1,2);
    if(read(pool+0x40,2)==0xffff)write(0x16d36e,pool+0x40,0x8000,2);
    return (salt<<16)|chosen;
  }
  void release_pool(std::uint32_t pool,std::uint32_t datum)const {
    const auto index=datum&0xffff,stride=read(pool+0x24);auto row=read(pool+0x44)+index*stride;
    if(read(pool+0x2a,1)&8) {
      unsigned n=0;for(;n<(stride&~3u);n+=4)write(0x16d4f1,row+n,0xbabababa);
      for(;n<stride;++n)write(0x16d4f8,row+n,0xba,1);
    }
    const auto bitmap=read(pool+0x48)+(index>>5)*4;
    write(0x16d518,bitmap,read(bitmap)&~(1u<<(index&31)));write(0x16d51a,row,0,2);
    if(signed32(index)<signed32(read(pool+0x34)))write(0x16d524,pool+0x34,index);
    if(index+1==read(pool+0x38)) {
      do {
        row-=read(pool+0x24);const auto next=read(pool+0x38)-1;
        write(0x16d53c,pool+0x38,next);
        if(signed32(next)<=0)break;
      }while(!read(row,2));
    }
    write(0x16d54d,pool+0x3c,read(pool+0x3c)-1);
  }
  std::uint32_t find_controller(std::uint32_t owner)const {
    if(!available() || owner==0xffffffffu)return 0xffffffffu;
    const auto high=read(s.controller_pool+0x38),bitmap=read(s.controller_pool+0x48);
    std::uint32_t start=0;
    while(signed32(start)>=0 && signed32(start)<signed32(high)) {
      if(read(bitmap+(start>>5)*4)&(1u<<(start&31))) {
        const auto stored=read(s.controller_pool+0x44)+read(s.controller_pool+0x24)*start;
        const auto salt=read(stored,2);const auto datum=(salt<<16)|start;
        const auto row=read(s.controller_pool+0x44)+(datum&0xffff)*0x1c;
        if(read(row+4)==owner)return datum;
      }
      ++start;
    }
    return 0xffffffffu;
  }
  std::uint32_t retain_controller(std::uint32_t owner)const {
    if(!available() || owner==0xffffffffu)return 0xffffffffu;
    const auto datum=find_controller(owner);if(datum==0xffffffffu)return datum;
    const auto row=read(s.controller_pool+0x44)+(datum&0xffff)*0x1c;
    auto flags=read(row+2,1);flags=(((flags+1)^flags)&0x7f)^flags;
    write(0x21bb97,row+2,flags,1);return datum;
  }
  std::uint32_t create_controller(std::uint32_t owner)const {
    if(!available() || owner==0xffffffffu)return 0xffffffffu;
    auto datum=find_controller(owner);
    if(datum==0xffffffffu) {datum=allocate_pool(s.controller_pool);if(datum==0xffffffffu)return datum;}
    const auto row=datum==0xffffffffu?s.controller_root:read(s.controller_pool+0x44)+(datum&0xffff)*0x1c;
    auto flags=read(row+2,1);flags=(((flags+1)^flags)&0x7f)^flags;
    write(0x21bc13,row+2,flags,1);
    if((flags&0x7f)==1) {
      const auto source_flag=std::uint8_t(read(s.system+0x7f,1)&1);
      const auto seed=need(service.controller_seed,"Original147A20 sound controller seed producer unavailable")(source_flag);
      write(0x21bfa0,row+4,owner);write(0x21bfa9,row+8,seed);
      write(0x21bfba,row+2,((source_flag==0)?0x80:0)|(read(row+2,1)&0x7f),1);
      write(0x21bfbd,row+3,0x80,1);
      write(0x21bfc6,row+0xc,0);write(0x21bfc8,row+0x10,0);
      write(0x21bfcb,row+0x14,0);write(0x21bfce,row+0x18,0);
    }
    return datum;
  }
  void release_controller(std::uint32_t datum)const {
    if(datum==0xffffffffu)return;
    const auto row=read(s.controller_pool+0x44)+(datum&0xffff)*0x1c;
    const auto original=read(row+2,1);const auto flags=(((original-1)^original)&0x7f)^original;
    write(0x21bdbb,row+2,flags,1);
    if(!(flags&0x7f))release_pool(s.controller_pool,datum);
  }
  void bind_effect(std::uint32_t effect,std::uint32_t raw,
                   std::uint32_t callbacks)const {
    const auto row=read(s.effect_pool+0x44)+(effect&0xffff)*0x14;
    write(0x21fd8b,raw+0x8c,0x30,2);
    auto flags=read(raw);
    if(flags&0x20)write(0x21fd99,raw+0x88,read(raw+0x58));
    else {write(0x21fda1,row+4,read(row+4,1)|8,1);write(0x21fda5,raw+0x88,0);}
    write(0x21fdaf,raw+0x84,effect);flags=read(raw)|0x20;
    write(0x21fdbe,raw+0x58,callbacks);write(0x21fdc5,raw,flags);
    if(flags&0x10) {
      const auto id=retain_controller(read(raw+0x54));
      write(0x21fdd1,row+8,id);
    } else {
      const auto controller_owner=effect|0x4000;
      const auto id=create_controller(controller_owner);
      write(0x21fde9,row+8,id);
      if(id!=0xffffffffu) {write(0x21fdf3,raw+0x54,controller_owner);write(0x21fdf6,raw,read(raw)|0x10);}
    }
  }
  void remove_effect(std::uint32_t effect)const {
    auto row=read(s.effect_pool+0x44)+(effect&0xffff)*0x14;
    const auto controller=read(row+8);
    if(controller!=0xffffffffu) {
      release_controller(controller);
      write(0x21fe27,row+8,0xffffffffu);
    }
    release_pool(s.effect_pool,effect);
  }
  std::uint32_t submit(std::uint32_t sound,std::uint32_t raw,
      std::span<std::byte,160> request,const SoundSubmitConstants &c)const {
    auto interface=(read(raw)&0x100)?read(raw+0x98):0xffffffffu;
    std::uint32_t output=0;auto effect=0xffffffffu;std::uint32_t effect_offset=0;
    if(controller(sound,interface,output)&&read(output+8)) {
      effect=allocate_pool(s.effect_pool);
      if(effect!=0xffffffffu) {
        effect_offset=(effect&0xffff)*0x14;
        const auto row=read(s.effect_pool+0x44)+effect_offset;
        write(0x21f297,row+0x10,output);write(0x21f29c,row+8,0xffffffffu);
        const auto playback_row=custom(sound,interface);
        if(playback_row&&(read(playback_row+8,1)&1))write(0x21f2b2,raw+0xd,read(raw+0xd,1)|1,1);
        bind_effect(effect,raw,c.effect_callbacks);
        std::uint32_t source=0xffffffffu;
        if(available()&&need(service.admit,"Original128380 sound admission unavailable")(sound,request,output)) {
          auto promoted=sound;
          if(!need(service.promote,"Original128610 sound promotion unavailable")(sound,promoted))
            source=need(service.create_source,"Original127750 sound source creation unavailable")(sound,output,request,promoted);
        }
        if(source!=0xffffffffu) {
          const auto result=read(s.effect_pool+0x44)+effect_offset;
          write(0x21f32f,result+3,1,1);write(0x21f333,result+0xc,source);
          write(0x21f33d,result+6,read(raw+4,2),2);
          need(service.update_effect,"Original21F740 sound effect update unavailable")(effect,0);
          return source;
        }
        remove_effect(effect);return 0xffffffffu;
      }
    }
    if(!available())return 0xffffffffu;
    if(!need(service.admit,"Original128380 sound admission unavailable")(sound,request,output))return 0xffffffffu;
    auto promoted=sound;
    if(need(service.promote,"Original128610 sound promotion unavailable")(sound,promoted))return 0xffffffffu;
    return need(service.create_source,"Original127750 sound source creation unavailable")(sound,output,request,promoted);
  }
};
}
std::uint32_t source_sound_custom_playback(const SoundSubmitState &s,std::uint32_t sound,
    std::uint32_t interface,const SoundSubmitServices &service) {return Owner(s,service,{}).custom(sound,interface);}
std::uint32_t source_sound_default_playback(const SoundSubmitState &s,std::uint32_t sound,
    const SoundSubmitServices &service) {return Owner(s,service,{}).ordinary(sound);}
bool source_sound_controller_request(const SoundSubmitState &s,std::uint32_t sound,
    std::uint32_t interface,std::uint32_t &output,const SoundSubmitServices &service,
    const SoundStartupObserver &observe) {return Owner(s,service,observe).controller(sound,interface,output);}
std::uint32_t source_sound_submit(const SoundSubmitState &s,std::uint32_t sound,
    std::uint32_t raw,const SoundSubmitConstants &c,
    const SoundSubmitServices &service,const SoundStartupObserver &observe) {
  const auto backing=need(service.memory,"Original sound submit caller memory unavailable")(raw,160);
  if(backing.size()!=160)throw std::runtime_error("Original sound submit caller memory extent unavailable");
  return Owner(s,service,observe).submit(sound,raw,std::span<std::byte,160>(backing.data(),160),c);
}
} // namespace halo2
