#include "halo2/native_vista_widget_cache.hpp"
#include "halo2/cache.hpp"
#include <utility>

namespace halo2 {
namespace {
void scalar_width(std::size_t size) {
  if(size!=1 && size!=2 && size!=4)
    throw FormatError("Vista loaded-cache scalar width must be 1, 2 or 4");
}
} // namespace

NativeVistaWidgetCache::NativeVistaWidgetCache(
    std::shared_ptr<const VistaWidgetCacheBank> original,std::uint32_t base)
    : original_(std::move(original)),base_(base) {
  if(!original_)throw FormatError("Vista loaded-cache owner requires its original asset bank");
  const auto input=original_->bytes(base_,base_,original_->region().size());
  storage_.assign(input.begin(),input.end());
}

std::size_t NativeVistaWidgetCache::offset(std::uint32_t address,std::size_t size) const {
  if(address<base_)throw FormatError("Source address precedes retained Vista loaded cache");
  const auto at=std::size_t(address-base_);
  if(at>storage_.size() || size>storage_.size()-at)
    throw FormatError("Source extent exceeds retained Vista loaded cache");
  return at;
}
std::span<std::byte> NativeVistaWidgetCache::bytes(std::uint32_t address,std::size_t size) {
  return std::span(storage_).subspan(offset(address,size),size);
}
std::span<const std::byte> NativeVistaWidgetCache::bytes(std::uint32_t address,std::size_t size) const {
  return std::span(storage_).subspan(offset(address,size),size);
}
std::uint32_t NativeVistaWidgetCache::read(std::uint32_t address,std::size_t size) const {
  scalar_width(size);std::uint32_t value{};
  const auto data=bytes(address,size);
  for(std::size_t i=0;i<size;++i)
    value|=std::uint32_t(std::to_integer<std::uint8_t>(data[i]))<<(8*i);
  return value;
}
void NativeVistaWidgetCache::write(std::uint32_t address,std::uint32_t value,std::size_t size) {
  scalar_width(size);auto data=bytes(address,size);
  for(std::size_t i=0;i<size;++i)data[i]=std::byte(std::uint8_t(value>>(8*i)));
}
} // namespace halo2
