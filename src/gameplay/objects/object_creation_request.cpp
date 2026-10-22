#include "halo2/object_creation_request.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Owner {
  const ObjectCreationRequestServices &services;
  const ObjectCreationRequestObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,std::size_t bytes) const {
    if(!services.memory)throw std::runtime_error("Original object storage provider unavailable");
    auto result=services.memory(address,bytes);
    if(result.size()!=bytes)throw std::runtime_error("Original object storage extent unavailable");
    return result;
  }
  static std::uint32_t word(std::span<const std::byte> data) {
    std::uint32_t result=0;
    for(unsigned i=0;i<data.size();++i)
      result|=std::to_integer<std::uint32_t>(data[i])<<(8*i);
    return result;
  }
  std::uint32_t read(std::uint32_t address,unsigned bytes=4) const {
    return word(memory(address,bytes));
  }
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned bytes=4) {
    auto data=memory(address,bytes);const auto before=word(data);
    if(bytes<4)value&=(1u<<(8*bytes))-1;
    for(unsigned i=0;i<bytes;++i)data[i]=std::byte(value>>(8*i));
    if(observe)observe({pc,address,bytes,before,value});
  }
public:
  Owner(const ObjectCreationRequestServices &p,const ObjectCreationRequestObserver &o)
      :services(p),observe(o) {}
  std::uint32_t body(std::uint32_t pool,std::uint32_t datum,std::uint32_t mask) const {
    if(datum==0xffffffffu)return 0;
    const auto index=datum&0xffffu;
    if(std::int32_t(index)>=std::bit_cast<std::int32_t>(read(pool+0x38)))return 0;
    const auto stride=read(pool+0x24);
    const auto record=stride*index+read(pool+0x44);
    const auto salt=std::uint16_t(read(record,2));
    if(!salt || std::bit_cast<std::int16_t>(salt)!=
                std::bit_cast<std::int16_t>(std::uint16_t(datum>>16)))return 0;
    if(!(mask&(1u<<(read(record+3,1)&31))))return 0;
    return read(record+8);
  }
  void initialize(std::uint32_t target,std::uint32_t definition,std::uint32_t owner,
                  std::uint32_t location,std::uint32_t pool,
                  const ObjectCreationRequestOperands &c) {
    for(unsigned i=0;i<49;++i)write(0xb883e,target+4*i,0);
    write(0xb8844,target,definition);write(0xb8848,target+0xc,0);
    write(0xb884b,target+0x18,0);
    write(0xb8853,target+0x28,word(std::span(c.basis_forward).subspan(0,4)));
    write(0xb8858,target+0x2c,word(std::span(c.basis_forward).subspan(4,4)));
    write(0xb885e,target+0x30,word(std::span(c.basis_forward).subspan(8,4)));
    write(0xb886c,target+0x34,word(std::span(c.basis_up).subspan(0,4)));
    write(0xb8871,target+0x38,word(std::span(c.basis_up).subspan(4,4)));
    write(0xb8877,target+0x3c,word(std::span(c.basis_up).subspan(8,4)));
    write(0xb887d,target+0x58,c.one_bits);write(0xb8882,target+0x74,0);
    write(0xb8885,target+0xb4,0xffff,2);write(0xb888f,target+0x14,0,1);
    const auto owner_body=body(pool,owner,0xffffffffu);
    write(0xb889a,target+0x64,0xffffffffu);write(0xb889d,target+0x5c,0xffffffffu);
    if(owner_body) {
      const auto flags=read(target+0x18);
      write(0xb88a5,target+0x60,owner);
      write(0xb88be,target+0x18,(read(owner_body+0xc0,1)&4)?flags|0x20:flags&~0x20u);
      if((1u<<(read(owner_body+0xaa,1)&31))&3) {
        write(0xb88d9,target+0x5c,read(owner_body+0x13c));
        write(0xb88e3,target+0x64,std::uint32_t(std::int32_t(
            std::bit_cast<std::int16_t>(std::uint16_t(read(owner_body+0x138,2))))));
      }
    } else write(0xb88e8,target+0x60,0xffffffffu);
    if(location) {
      write(0xb88f8,target+0x68,read(location));
      write(0xb88fd,target+0x6c,read(location+4));
      write(0xb8903,target+0x70,read(location+8));
    } else {
      write(0xb8908,target+0x6c,0xffffffffu);write(0xb890b,target+0x68,0xffffffffu);
      write(0xb890e,target+0x70,0xffff,2);
    }
    write(0xb8913,target+0xa,0xff,1);write(0xb8916,target+0xb,0xff,1);
    write(0xb8919,target+8,0xffff,2);write(0xb891d,target+4,0xffffffffu);
    write(0xb8921,target+0x10,0xffffffffu);write(0xb8924,target+0xb8,0,1);
  }
};
}
std::uint32_t source_object_body_from_datum(std::uint32_t pool,
    std::uint32_t datum,std::uint32_t mask,const ObjectCreationRequestServices &p) {
  return Owner(p,{}).body(pool,datum,mask);
}
void source_object_creation_request_initialize(std::uint32_t target,
    std::uint32_t definition,std::uint32_t owner,std::uint32_t location,
    std::uint32_t pool,const ObjectCreationRequestOperands &c,
    const ObjectCreationRequestServices &p,const ObjectCreationRequestObserver &o) {
  Owner(p,o).initialize(target,definition,owner,location,pool,c);
}
} // namespace halo2
