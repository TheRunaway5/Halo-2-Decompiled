#include "halo2/sound_driver_listener.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
class Listener {
  std::uint32_t driver_pointer,input;
  const SoundDriverListenerConstants &constants;
  const SoundDriverListenerServices &services;
  const SoundDriverStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,std::size_t extent) const {
    if(!services.memory)throw std::runtime_error("Source listener storage unavailable");
    auto output=services.memory(address,extent);
    if(output.size()!=extent)throw std::runtime_error("Source listener storage extent unavailable");
    return output;
  }
  std::uint32_t read(std::uint32_t address,unsigned extent=4) const {
    const auto bytes=memory(address,extent);std::uint32_t result=0;
    for(unsigned i=0;i<extent;++i)result|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return result;
  }
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned extent=4) const {
    auto output=memory(address,extent);const auto before=read(address,extent);
    for(unsigned i=0;i<extent;++i)output[i]=std::byte(value>>(8*i));
    if(observe)observe({pc,address,extent,before,value});
  }
  static bool within(std::uint32_t first,std::uint32_t second,std::uint32_t epsilon) {
    // Both inputs are original float words. Retain the unspilled 53-bit x87
    // subtraction precision instead of inserting a float rounding step.
    const double difference=double(std::bit_cast<float>(first))-double(std::bit_cast<float>(second));
    return double(std::bit_cast<float>(epsilon))>std::fabs(difference);
  }
  bool equal_words(std::uint32_t first,std::uint32_t second,unsigned count) const {
    for(unsigned i=0;i<count;++i)if(read(first+4*i)!=read(second+4*i))return false;
    return true;
  }
  std::uint32_t default_word(unsigned index) const {
    std::uint32_t value=0;
    for(unsigned j=0;j<4;++j)value|=std::to_integer<std::uint32_t>(constants.default_environment[4*index+j])<<(8*j);
    return value;
  }
  bool equals_default(std::uint32_t address) const {
    for(unsigned i=0;i<18;++i)if(default_word(i)!=read(address+4*i))return false;
    return true;
  }
  void platform(SoundDriverListenerOperation operation,std::uint32_t driver,
                std::span<const std::uint32_t> words) const {
    const auto device=read(driver+0x2ab0);
    if(!services.native_operation)throw std::runtime_error("Real source listener platform operation unavailable");
    services.native_operation({operation,device,words});
  }
public:
  Listener(std::uint32_t pointer,std::uint32_t in,const SoundDriverListenerConstants &c,
           const SoundDriverListenerServices &p,const SoundDriverStartupObserver &o)
      :driver_pointer(pointer),input(in),constants(c),services(p),observe(o){}
  void run() const {
    auto driver=read(driver_pointer);
    if(!read(driver,1)) {
      const std::array<std::uint32_t,4> words{read(input),read(input+8),read(input+4),1};
      platform(SoundDriverListenerOperation::Position,driver,words);driver=read(driver_pointer);
      write(0x2228de,driver+0x2918,read(input));
      write(0x2228e3,driver+0x291c,read(input+4));
      write(0x2228e9,driver+0x2920,read(input+8));
    }
    bool orientation_changed=false;
    for(unsigned i=0;i<6;++i) {
      if(!within(read(input+0xc+4*i),read(driver+0x2924+4*i),constants.orientation_epsilon_bits)) {
        orientation_changed=true;break;
      }
    }
    if(orientation_changed||!read(driver,1)) {
      const std::array<std::uint32_t,7> words{read(input+0xc),read(input+0x14),read(input+0x10),
                                            read(input+0x18),read(input+0x20),read(input+0x1c),1};
      platform(SoundDriverListenerOperation::Orientation,driver,words);driver=read(driver_pointer);
      constexpr std::array<std::uint32_t,6> pcs{0x2229af,0x2229b4,0x2229ba,0x2229c6,0x2229cb,0x2229d1};
      for(unsigned i=0;i<6;++i)write(pcs[i],driver+0x2924+4*i,read(input+0xc+4*i));
    }
    auto channels=std::bit_cast<std::int32_t>(read(input+0x30));
    if(channels>=2)channels=2;
    for(std::int32_t index=0;index<channels;++index) {
      const auto offset=std::uint32_t(index),descriptor=read(input+0x34)+offset*0x18;
      const auto payload=read(descriptor),cached=driver+0x2950+offset*0x48;
      if(!equal_words(payload,cached,18)||!read(driver,1)) {
        for(unsigned i=0;i<18;++i)write(0x222a4d,cached+4*i,read(payload+4*i));
        write(0x222a4f,driver+0x2a08+offset,1,1);
      }
      const auto parameters=driver+0x29e8+offset*0x10;
      if(!equal_words(parameters,descriptor+4,4)||!read(driver,1)) {
        constexpr std::array<std::uint32_t,4> pcs{0x222a77,0x222a7c,0x222a82,0x222a88};
        for(unsigned i=0;i<4;++i)write(pcs[i],parameters+4*i,read(descriptor+4+4*i));
        write(0x222a8b,driver+0x2a0a+offset,1,1);
      }
      const auto weight=driver+0x29e0+offset*4;
      if(!within(read(weight),read(descriptor+0x14),constants.environment_epsilon_bits)||!read(driver,1)) {
        write(0x222ab6,weight,read(descriptor+0x14));write(0x222ab8,driver+0x2a0a+offset,1,1);
      }
    }
    // The source retains negative raw counts here. Do not clamp them into a
    // fabricated zero-channel input; invalid backing must stop naturally.
    for(auto index=channels;index<2;++index) {
      const auto offset=std::uint32_t(index),cached=driver+0x2950+offset*0x48;
      if(!equals_default(cached)||!read(driver,1)) {
        for(unsigned i=0;i<18;++i)write(0x222b33,cached+4*i,default_word(i));
        write(0x222b35,driver+0x2a08+offset,1,1);
      }
      const auto weight=driver+0x29e0+offset*4;
      if(!within(read(weight),0,constants.environment_epsilon_bits)||!read(driver,1)) {
        write(0x222b56,weight,0);write(0x222b5a,driver+0x2a0a+offset,1,1);
      }
    }
  }
};
} // namespace
void source_sound_driver_listener_install(std::uint32_t pointer,std::uint32_t input,
    const SoundDriverListenerConstants &constants,const SoundDriverListenerServices &services,
    const SoundDriverStartupObserver &observe) {
  Listener(pointer,input,constants,services,observe).run();
}
} // namespace halo2
