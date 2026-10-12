#include "halo2/object_storage_release.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
std::int32_t signed32(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
class Owner {
  const SourceObjectStorageReleaseBindings &b;
  const SourceObjectStorageReleaseServices &s;
  const SourceObjectStorageReleaseObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,unsigned size)const{
    if(!s.memory)throw std::runtime_error("Original object release backing unavailable");
    const auto result=s.memory(address,size);
    if(result.size()!=size)throw std::runtime_error("Original object release backing extent unavailable");
    return result;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4)const{
    const auto bytes=memory(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
  }
  void write(std::uint32_t address,std::uint32_t value,unsigned size=4)const{
    const auto before=read(address,size);const auto bytes=memory(address,size);
    for(unsigned i=0;i<size;++i)bytes[i]=std::byte(value>>(8*i));
    if(observe)observe({address,before,value&(size==4?0xffffffffu:((1u<<(size*8))-1)),size});
  }
  void erase(std::uint32_t pool,std::uint32_t datum)const{
    const auto index=datum&65535u,stride=read(pool+0x24),record=read(pool+0x44)+index*stride;
    if(read(pool+0x2a,1)&8){
      unsigned i=0;for(;i+4<=stride;i+=4)write(record+i,0xbabababau);
      for(;i<stride;++i)write(record+i,0xba,1);
    }
    const auto bitmap=read(pool+0x48)+(index>>5)*4;
    write(bitmap,read(bitmap)&~(1u<<(index&31)));
    write(record,0,2);
    if(signed32(index)<signed32(read(pool+0x34)))write(pool+0x34,index);
    if(index+1==read(pool+0x38)){
      auto previous=record-stride;
      for(;;){
        const auto high=read(pool+0x38)-1;write(pool+0x38,high);
        if(signed32(high)<=0 || read(previous,2))break;
        previous-=stride;
      }
    }
    write(pool+0x3c,read(pool+0x3c)-1);
  }
public:
  Owner(const SourceObjectStorageReleaseBindings &bindings,
        const SourceObjectStorageReleaseServices &services,
        const SourceObjectStorageReleaseObserver &observer):b(bindings),s(services),observe(observer){}
  void release(std::uint32_t datum)const{
    const auto header=read(read(b.object_pool_slot)+0x44)+(datum&65535u)*12;
    const auto body=read(header+8);
    write(header+2,0,1);
    if(body){
      const auto heap=read(b.body_heap_slot);
      write(heap+0x30,read(heap+0x30)+read(body-0x10));
      const auto previous=read(body-4),next=read(body-8);
      if(previous)write(previous+8,next);else write(heap+0x34,next);
      const auto following=read(body-8);
      if(following)write(following+0xc,read(body-4));else write(heap+0x38,read(body-4));
      write(header+8,0);
    }
    // The original reloads this mutable global before invoking16D4C0.
    erase(read(b.object_pool_slot),datum);
  }
};
} // namespace
void release_source_object_storage(const SourceObjectStorageReleaseBindings &b,
    std::uint32_t datum,const SourceObjectStorageReleaseServices &s,
    const SourceObjectStorageReleaseObserver &observer){Owner(b,s,observer).release(datum);}
} // namespace halo2
