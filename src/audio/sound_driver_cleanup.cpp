#include "halo2/sound_driver_cleanup.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Cleanup {
  const SoundDriverCleanupBindings &bindings;
  const SoundDriverCleanupServices &services;
  const SoundDriverStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,std::size_t extent) const {
    if(!services.owners.memory)throw std::runtime_error("Source cleanup storage unavailable");
    auto result=services.owners.memory(address,extent);
    if(result.size()!=extent)throw std::runtime_error("Source cleanup storage extent unavailable");
    return result;
  }
  std::uint32_t read(std::uint32_t address,unsigned extent=4) const {
    const auto input=memory(address,extent);std::uint32_t value=0;
    for(unsigned i=0;i<extent;++i)value|=std::to_integer<std::uint32_t>(input[i])<<(8*i);
    return value;
  }
  std::int32_t count(std::uint32_t address) const {
    return std::bit_cast<std::int16_t>(std::uint16_t(read(address,2)));
  }
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned extent=4) const {
    const auto before=read(address,extent);auto output=memory(address,extent);
    for(unsigned i=0;i<extent;++i)output[i]=std::byte(value>>(8*i));
    if(observe)observe({pc,address,extent,before,value});
  }
  std::uint32_t current_driver() const {return read(bindings.driver_pointer_address);}
  SoundDriverOwnersState current_owners() const {
    return {current_driver(),bindings.cache_address,bindings.static_queue_address};
  }
  void stream_release(std::uint32_t identity) const {
    const auto vtable=read(identity),method=read(vtable+4);
    if(!services.owners.native_virtual_call)throw std::runtime_error("Retained source stream release unavailable");
    services.owners.native_virtual_call({identity,vtable,4,method,SoundDriverVirtualThis::Stack,{}});
  }
  void release(SoundDriverCleanupRelease operation,std::uint32_t identity) const {
    if(!services.native_release)throw std::runtime_error("Retained source cleanup release unavailable");
    services.native_release({operation,identity});
  }
public:
  Cleanup(const SoundDriverCleanupBindings &b,const SoundDriverCleanupServices &p,
          const SoundDriverStartupObserver &o):bindings(b),services(p),observe(o){}
  void run() const {
    source_sound_driver_stream_flush(current_owners(),services.owners,observe);
    auto driver=current_driver();
    if(std::uint32_t(driver+0xc)) {
      for(std::int32_t index=0;index<count(driver+4);++index) {
        const auto slot=driver+std::uint32_t(index)*0x34+0x30;
        if(const auto identity=read(slot)) {
          stream_release(identity);driver=current_driver();write(0x220c28,slot,0);
        }
      }
      write(0x220c36,driver+4,0,2);
    }
    if(std::uint32_t(driver+0x1a18)) {
      for(std::int32_t index=0;index<count(driver+6);++index) {
        // ESI retains this record across each real release, even if the
        // source global driver changes. EAX is reloaded independently.
        const auto record=driver+std::uint32_t(index)*0x3c+0x1a18;
        if(const auto identity=read(record+0x34)) {
          release(SoundDriverCleanupRelease::Buffer,identity);
          driver=current_driver();write(0x220c6d,record+0x34,0);
        }
        if(const auto identity=read(record+0x38)) {
          release(SoundDriverCleanupRelease::Buffer,identity);
          driver=current_driver();write(0x220c82,record+0x38,0);
        }
      }
    }
    if(const auto identity=read(driver+0x2ab0)) {
      release(SoundDriverCleanupRelease::Device,identity);
      write(0x220ca7,current_driver()+0x2ab0,0);
    }
    source_sound_driver_queue_shutdown(current_owners(),services.owners,observe);
    driver=current_driver();
    write(0x220cbf,bindings.hardware_queue_pointer_address,0);
    write(0x220cc5,driver,0,1);
  }
};
} // namespace
void source_sound_driver_cleanup(const SoundDriverCleanupBindings &b,
    const SoundDriverCleanupServices &p,const SoundDriverStartupObserver &o) {
  Cleanup(b,p,o).run();
}
} // namespace halo2
