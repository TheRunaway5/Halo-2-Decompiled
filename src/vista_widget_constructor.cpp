#include "halo2/vista_widget_constructor.hpp"
#include "halo2/vista_widget_activation.hpp"
#include "halo2/cache.hpp"

namespace halo2 {
namespace {
void require(std::span<std::byte> bank, std::uint32_t identity, std::size_t size) {
  if(bank.size()<size)throw FormatError("Truncated consumed Vista widget constructor bank");
  if(!identity || std::uint64_t(identity)+size>(std::uint64_t{1}<<32))
    throw FormatError("Invalid consumed Vista widget constructor identity");
}
void store(std::span<std::byte> bank,std::size_t at,std::uint32_t value,unsigned size=4) {
  for(unsigned i=0;i<size;++i)bank[at+i]=std::byte(value>>(i*8));
}
} //namespace

VistaWidgetIdentity vista_widget_construct(std::span<std::byte> bank,VistaWidgetIdentity identity,
    std::uint32_t kind,std::uint32_t controller,
    const std::function<std::uint32_t()>& clock) {
  require(bank,identity.value,0x70);
  if(!clock)throw FormatError("Missing consumed Vista widget constructor clock");
  store(bank,4,kind);store(bank,8,controller,2);store(bank,0,0x7cf44c);
  store(bank,0xa,0xffff,2);store(bank,0xc,0xffffffff);
  for(auto field:{0x10,0x14,0x18,0x1c})store(bank,field,0);
  store(bank,0x68,0xffff,2);store(bank,0x6a,0,2);
  store(bank,0x6c,0,1);
  for(auto field:{0x6d,0x6e,0x6f})store(bank,field,1,1);
  store(bank,0x20,0);store(bank,0x24,0);
  for(auto field:{0x30,0x2c,0x28})store(bank,field,0x3f800000);
  //611D81 zeros the complete34-byte descriptor, then overwrites these fields.
  VistaWidgetAnimationDescriptor descriptor{};
  store(descriptor,0,0xffffffff);store(descriptor,0xc,1,2);store(descriptor,0x14,0);
  vista_widget_animation_birth(bank,descriptor,clock);
  return identity;
}

VistaWidgetIdentity vista_widget_text_base_construct(std::span<std::byte> bank,VistaWidgetIdentity identity,
    std::uint32_t controller,const std::function<std::uint32_t()>& clock) {
  require(bank,identity.value,0x75);
  vista_widget_construct(bank,identity,6,controller,clock);
  store(bank,0x70,0);store(bank,0x74,0,1);store(bank,0,0x7cfd7c);
  return identity;
}

std::uint32_t vista_widget_string_construct(std::span<std::byte> bank,std::uint32_t identity) {
  require(bank,identity,0x44);
  store(bank,0,0x7d24ec);store(bank,4,0);
  store(bank,0x14,0,2);store(bank,0x16,1,2);store(bank,0x18,0xffffffff);
  store(bank,0x1c,2);store(bank,0x20,0x3f800000);store(bank,0x24,0xffffffff);
  store(bank,0x38,0);store(bank,0x3c,0xffff,2);store(bank,0x3e,0,2);store(bank,0x40,0);
  for(auto field:{0x10,0xc,8})store(bank,field,0x3f800000);
  return identity;
}

VistaWidgetIdentity vista_widget_text_construct(std::span<std::byte> bank,VistaWidgetIdentity identity,
    std::uint32_t controller,const std::function<std::uint32_t()>& clock) {
  require(bank,identity.value,0xbe);
  vista_widget_text_base_construct(bank,identity,controller,clock);
  store(bank,0,0x7cf29c);
  vista_widget_string_construct(bank.subspan(0x78),identity.value+0x78);
  store(bank,0x78,0x7cf22c);store(bank,0xbc,0,2);
  return identity;
}

VistaWidgetIdentity vista_widget_button_construct(std::span<std::byte> bank,VistaWidgetIdentity identity,
    std::uint32_t ordinal,std::uint32_t controller,const std::function<std::uint32_t()>& clock) {
  require(bank,identity.value,0x100);
  vista_widget_construct(bank,identity,3,controller,clock);
  store(bank,0,0x7d39a4);
  vista_widget_string_construct(bank.subspan(0x70),identity.value+0x70);
  store(bank,0x70,0x7cf218);store(bank,0xb4,0,2);
  store(bank,0xf8,ordinal,2);store(bank,0xf4,0);store(bank,0xfc,0); //611442.
  return identity;
}
} //namespace halo2
