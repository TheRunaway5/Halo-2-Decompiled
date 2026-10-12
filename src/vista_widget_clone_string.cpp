#include "halo2/vista_widget_clone_string.hpp"
#include "halo2/cache.hpp"

namespace halo2 {
namespace {
void require(std::size_t size, std::uint32_t identity, std::uint64_t at,
    std::uint64_t count) {
  if(!identity || at>size || count>size-at ||
      std::uint64_t(identity)+at+count>(std::uint64_t{1}<<32))
    throw FormatError("Invalid consumed Vista clone string bank");
}
std::uint16_t load(VistaUtf16View bank, std::uint64_t at) {
  require(bank.bytes.size(),bank.identity,at,2);
  return std::uint16_t(std::to_integer<unsigned>(bank.bytes[at]) |
      (std::to_integer<unsigned>(bank.bytes[at+1])<<8));
}
void store(std::span<std::byte> bank,std::size_t at,std::uint16_t value) {
  bank[at]=std::byte(value);bank[at+1]=std::byte(value>>8);
}
std::uint32_t invalid(const VistaUtf16FailureServices& services) {
  if(!services.store_errno || !services.invalid_parameter)
    throw FormatError("Missing consumed Vista string invalid-parameter services");
  services.store_errno(0x16);
  services.invalid_parameter({0,0,0,0,0});
  return 0x16;
}
} //namespace

std::uint32_t vista_utf16_copy_truncate(VistaUtf16Bank destination,
    VistaUtf16View source,std::uint32_t capacity,
    const VistaUtf16FailureServices& services) {
  if(!destination.identity || !capacity)return invalid(services);
  require(destination.bytes.size(),destination.identity,0,std::uint64_t(capacity)*2);
  if(!source.identity) {
    store(destination.bytes,0,0);
    return invalid(services);
  }
  std::uint32_t remaining=capacity;
  std::uint64_t at=0;
  do {
    const auto value=load(source,at);
    store(destination.bytes,at,value);
    at+=2;
    if(!value)return 0;
  }while(--remaining);
  store(destination.bytes,std::size_t(capacity)*2-2,0);
  return 0x50;
}

std::uint32_t vista_utf16_copy(VistaUtf16Bank destination,VistaUtf16View source,
    std::uint32_t capacity,const VistaUtf16FailureServices& services) {
  vista_utf16_copy_truncate(destination,source,capacity,services);
  return destination.identity;
}

std::uint32_t vista_utf16_length(VistaUtf16View source) {
  std::uint64_t at=0;
  while(load(source,at))at+=2;
  const auto bytes=static_cast<std::uint32_t>(at);
  return (bytes>>1)|(bytes&0x80000000u); //Original32-bit SUB then SAR1.
}

VistaWidgetIdentity vista_widget_clone_style_identity(VistaWidgetIdentity widget) {
  return {widget.value+0x78u};
}
std::uint32_t vista_widget_clone_text_identity(VistaWidgetIdentity style) {
  return style.value+0x44u;
}

std::uint32_t vista_widget_clone_string_assign(VistaWidgetIdentity style,
    VistaUtf16View input,const VistaCloneStringServices& services) {
  if(!services.bank || !services.text)
    throw FormatError("Missing consumed Vista clone string service");
  auto bank=services.bank(style);
  require(bank.size(),style.value,0,0x844);
  vista_utf16_copy({style.value+0x44u,bank.subspan(0x44)},input,0x400,services.failure);
  const auto count=vista_utf16_length(services.text(style));
  const bool reset=(std::to_integer<unsigned>(bank[0x3d])&0x80)==0;
  store(bank,0x3e,static_cast<std::uint16_t>(count));
  if(reset)store(bank,0x3c,0);
  return count;
}
} //namespace halo2
