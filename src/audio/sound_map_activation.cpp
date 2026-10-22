#include "halo2/sound_map_activation.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
template<class F> const F &need(const F &f,const char *message) {
  if(!f)throw std::runtime_error(message);
  return f;
}
class Owner {
  SoundMapActivationState &s;
  const SoundMapActivationConstants &c;
  const SoundMapActivationServices &services;
  const SoundMapActivationObserver &observe;
  std::span<const std::byte> memory(std::uint32_t address,std::size_t extent)const {
    auto bytes=need(services.read,"Original sound map source memory unavailable")(address,extent);
    if(bytes.size()!=extent)throw std::runtime_error("Original sound map source memory extent unavailable");
    return bytes;
  }
  std::uint32_t read(std::uint32_t address,unsigned extent=4)const {
    const auto bytes=memory(address,extent);std::uint32_t value=0;
    for(unsigned i=0;i<extent;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned extent=4) {
    const auto before=read(address,extent);auto bytes=need(services.writable,"Original sound map mutable backing unavailable")(address,extent);
    if(bytes.size()!=extent)throw std::runtime_error("Original sound map mutable backing extent unavailable");
    for(unsigned i=0;i<extent;++i)bytes[i]=std::byte(value>>(8*i));
    if(observe)observe({pc,address,extent,before,value&(extent==4?0xffffffffu:((1u<<(8*extent))-1))});
  }
  void global(std::uint32_t pc,std::uint32_t address,std::uint32_t &slot,std::uint32_t value) {
    const auto before=slot;slot=value;if(observe)observe({pc,address,4,before,value});
  }
  std::uint32_t globals_record()const {
    return read(s.current_matg_address+0xc0)?read(s.current_matg_address+0xc4):0;
  }
  std::uint32_t tag(std::uint32_t datum)const {
    return need(services.tag_address,"Original sound map tag table unavailable")(std::uint16_t(datum));
  }
  void plugin(std::uint32_t identity) {
    const auto previous=read(s.driver_address+0x2ab4);
    if(previous)need(services.native_plugin_release,"Native sound environment plugin release owner unavailable")(previous);
    const auto driver=s.driver_address;
    const auto policy=std::uint8_t(read(driver+1,1));
    write(0x2204e0,driver+0x2ab4,identity);
    const auto input=memory(driver+0x2a0c,64);
    need(services.native_plugin_initialize,"Native sound environment routing owner unavailable")(
        {identity,driver+0x2a0c,driver+0x2a4c,policy,std::span<const std::byte,64>(input.data(),64)});
    const auto selected=read(s.driver_address+0x2ab4);
    const auto selected_policy=std::uint8_t(read(s.driver_address+1,1));
    const auto parameters=memory(s.runtime_environment_parameters_address,44);
    need(services.native_plugin_environment,"Native sound environment/filter owner unavailable")(
        {selected,s.runtime_environment_parameters_address,selected_policy,
         std::span<const std::byte,44>(parameters.data(),44)});
  }
  void classes() {
    const auto datum=read(globals_record()+0x14);
    if(datum==0xffffffffu)return;
    const auto definition=tag(datum),driver=s.driver_address;
    write(0x2217e8,driver+0x2a0c,read(definition+0x18));
    write(0x2217f5,driver+0x2a10,read(definition+0x1c));
    write(0x2217fd,driver+0x2a14,read(definition+0x20));
    write(0x221807,driver+0x2a18,read(definition+0x24));
    for(unsigned i=0;i<12;++i)write(0x221818,driver+0x2a1c+4*i,read(definition+0x28+4*i));
    plugin(read(driver+0x2ab4));
    const auto system=s.sound_system_address;
    write(0x12758b,system+0x1ec,read(definition+0x48));
    write(0x127594,system+0x1f0,read(definition+0x4c));
    write(0x12759d,system+0x1f4,read(definition+0x50));
  }
  void effects() {
    const auto device=read(s.driver_address+0x2ab0);
    for(unsigned i=0;i<4;++i) {
      const auto address=s.environment_address+0x28+8*i;
      const auto bytes=memory(address,8);
      need(services.native_set_effect_data,"Native sound environment effect-data owner unavailable")(
          {device,4+i,0x20,address,0,std::span<const std::byte,8>(bytes.data(),8)});
    }
    const auto environment=s.environment_address;
    if(environment) {
      write(0x192d15,environment+0x24,0,2);
      write(0x192d1b,environment+0x22,0,2);
    }
  }
public:
  Owner(SoundMapActivationState &state,const SoundMapActivationConstants &constants,
        const SoundMapActivationServices &providers,const SoundMapActivationObserver &observer)
      :s(state),c(constants),services(providers),observe(observer) {}
  void run() {
    const auto ugh=tag(read(globals_record()+0x20)),before_clock=s.sound_system_address;
    global(0x126e1c,0x526464,s.current_ugh_address,ugh);
    write(0x126e22,before_clock+0x1e4,c.one_bits);write(0x126e2a,before_clock+0x7a,1,1);
    const auto clock=need(services.tick_milliseconds,"Original sound map tick owner unavailable")();
    const auto system=s.sound_system_address;
    write(0x126e44,system+0x80,clock);write(0x126e4a,system+0x84,0);
    write(0x126e57,system+0x1f8,0xffffffffu);write(0x126e5d,system+0x200,0xffffffffu);
    write(0x126e63,system+0x1ac,0);write(0x126e6b,system+0x1b0,0xffffffffu);
    write(0x126e71,system+0x1b4,0);write(0x126e79,system+0x1b8,c.ambient_transition_bits);
    write(0x126e81,system+0x1c8,0);write(0x126e89,system+0x1cc,0xffffffffu);
    write(0x126e8f,system+0x1d0,0);write(0x126e97,system+0x1d4,0);
    for(unsigned i=0;i<4;++i)write(0x126eb0,system + 0x8e + 0x48*i,0,1);
    classes();effects();
    const auto driver=s.driver_address,options=s.options_address;
    write(0x126ece,driver+0x2a98,0xffffffffu);write(0x126ed4,driver+0x2a9c,0xffffffffu);
    global(0x126ef1,0x4865e8,s.queue_budget,read(options+8)==3?0x10000:0x4000);
  }
};
}
void source_sound_map_activation(SoundMapActivationState &state,
    const SoundMapActivationConstants &constants,const SoundMapActivationServices &services,
    const SoundMapActivationObserver &observer) {
  Owner(state,constants,services,observer).run();
}
} // namespace halo2
