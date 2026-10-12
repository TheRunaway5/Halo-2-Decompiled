#include "halo2/vista_session_storage.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
namespace {
void put(std::span<std::byte> b,std::size_t o,std::uint32_t v,unsigned n=4) {
  for(unsigned i=0;i<n;++i)b[o+i]=std::byte(v>>(8*i));
}
void clear(std::span<std::byte> b,std::size_t o,std::size_t n) {
  std::fill_n(b.begin()+o,n,std::byte{});
}
}
void vista_session_storage_initialize(std::span<std::byte> source,
    std::uint32_t slot,std::uint32_t mode,std::uint32_t network_slot,
    const VistaSessionStorageBindings& bindings) {
  const auto owner5=std::uint64_t(slot)*36+0x14;
  const auto owner6=std::uint64_t(slot)*4;
  if(source.size()<vista_session_source_size || owner5+12>bindings.owner5.size() ||
      owner6+4>bindings.owner6.size())
    throw FormatError("Vista session initialization requires its complete retained owner bindings");
  auto b=source.first(vista_session_source_size);
  put(b,0x14,slot);put(b,0x18,mode);put(b,0x5c,network_slot);
  for(unsigned i=0;i<4;++i)put(b,4+i*4,bindings.owner_words[i]);
  put(bindings.owner6,owner6,bindings.session_word);
  put(bindings.owner5,owner5,bindings.session_word);
  put(bindings.owner5,owner5+4,0xffffffffu);put(bindings.owner5,owner5+8,0xffffffffu);
  put(b,0x1c,0xffffffffu);put(b,0x64,0xffffffffu);put(b,0x6c,0,1);
  for(const auto o:{0x70u,0x24e0u}){clear(b,o,0x2470);put(b,o,0xffffffffu);}
  for(const auto o:{0x4950u,0x4a18u}){clear(b,o,0xc8);put(b,o,0xffffffffu);}
  for(const auto o:{0x4ae0u,0x4b80u}){clear(b,o,0xa0);put(b,o,0xffffffffu);}
  clear(b,0x4c20,0x40);
  for(const auto o:{0x4c60u,0x5f10u}){clear(b,o,0x12b0);put(b,o,0xffffffffu);}
  clear(b,0x71c4,0x1dc);put(b,0x73a0,0);put(b,0x71c0,0xffffffffu);
  clear(b,0x7930,0x34);put(b,0x7964,0xffffffffu);put(b,0x7968,0xffffffffu);
  clear(b,0x7978,0x200);put(b,0x7b78,0);clear(b,0x28,0x20);put(b,0x7b7c,0,1);
}
} // namespace halo2
