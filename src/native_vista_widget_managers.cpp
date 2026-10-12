#include "halo2/native_vista_widget_managers.hpp"
#include "halo2/cache.hpp"

namespace halo2 {
namespace {
std::size_t offset(std::uint32_t address,std::size_t size) {
  if(address<NativeVistaWidgetManagers::source_base)
    throw FormatError("Source address precedes retained Vista UI owner");
  const auto at=std::size_t(address-NativeVistaWidgetManagers::source_base);
  if(at>NativeVistaWidgetManagers::storage_size||size>NativeVistaWidgetManagers::storage_size-at)
    throw FormatError("Source extent exceeds retained Vista UI owner");
  return at;
}
void scalar(std::size_t size) {
  if(size!=1&&size!=2&&size!=4)throw FormatError("Vista UI scalar width must be1,2 or4");
}
} //namespace
NativeVistaWidgetManagers::NativeVistaWidgetManagers() {
  VistaWidgetManagerLifetimeServices services;
  services.write=[this](auto address,auto value,auto width){write(address,value,width);};
  vista_widget_ui_construct({source_base},services);
}
std::span<std::byte> NativeVistaWidgetManagers::bytes(std::uint32_t address,std::size_t size) {
  return std::span(storage_).subspan(offset(address,size),size);
}
std::span<const std::byte> NativeVistaWidgetManagers::bytes(std::uint32_t address,std::size_t size) const {
  return std::span(storage_).subspan(offset(address,size),size);
}
std::uint32_t NativeVistaWidgetManagers::read(std::uint32_t address,std::size_t size) const {
  scalar(size);std::uint32_t value{};const auto data=bytes(address,size);
  for(std::size_t i=0;i<size;++i)value|=std::uint32_t(std::to_integer<unsigned>(data[i]))<<(8*i);
  return value;
}
void NativeVistaWidgetManagers::write(std::uint32_t address,std::uint32_t value,std::size_t size) {
  scalar(size);auto data=bytes(address,size);
  for(std::size_t i=0;i<size;++i)data[i]=std::byte(std::uint8_t(value>>(8*i)));
}
} //namespace halo2
