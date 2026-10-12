#include "halo2/datum_pool.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <utility>

namespace halo2 {
namespace {
SourceRawDatumPoolServices source_storage(SourceRuntimeMemory &m) {
  return {[&m](auto p,auto n){return m.mutable_view(p,n);},
          [&m](auto p,auto n){return m.view(p,n);}};
}
}
std::uint32_t DatumPool::source_field(unsigned offset,unsigned n) const {
  auto b=source_memory_->view(source_pool_+offset,n);std::uint32_t v=0;
  for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
DatumPool::DatumPool(SourceRuntimeMemory &m,std::uint32_t p):source_memory_(&m),source_pool_(p) {
  m.mutable_view(p,0x4c);
  if(!p || source_field(0x29,1)!=1 || source_field(0x20)==0 || source_field(0x20)>65535 || source_field(0x24)<2 ||
      std::uint64_t(source_field(0x20))*source_field(0x24)>UINT32_MAX)
    throw std::invalid_argument("Source-owned datum pool is not constructed and valid");
  m.mutable_view(source_field(0x44),std::size_t(source_field(0x20))*source_field(0x24));
  m.mutable_view(source_field(0x48),((std::size_t(source_field(0x20))+31)/32)*4);
}
DatumPool::DatumPool(const DatumPool &other) {
  if(other.source_memory_)throw std::logic_error("A source pool view requires a whole-bank checkpoint owner");
  name_=other.name_;capacity_=other.capacity_;stride_=other.stride_;cursor_=other.cursor_;
  high_water_=other.high_water_;count_=other.count_;flags_=other.flags_;next_salt_=other.next_salt_;
  elements_=other.elements_;bitmap_=other.bitmap_;
}
DatumPool &DatumPool::operator=(const DatumPool &other) {
  if(this!=&other){DatumPool staged(other);*this=std::move(staged);}return *this;
}
std::int32_t DatumPool::capacity() const {return source_memory_?std::int32_t(source_field(0x20)):capacity_;}
std::int32_t DatumPool::stride() const {return source_memory_?std::int32_t(source_field(0x24)):stride_;}
std::int32_t DatumPool::cursor() const {return source_memory_?std::int32_t(source_field(0x34)):cursor_;}
std::int32_t DatumPool::high_water() const {return source_memory_?std::int32_t(source_field(0x38)):high_water_;}
std::int32_t DatumPool::count() const {return source_memory_?std::int32_t(source_field(0x3c)):count_;}
std::uint16_t DatumPool::next_salt() const {return source_memory_?std::uint16_t(source_field(0x40,2)):next_salt_;}
void DatumPool::set_next_salt(std::uint16_t v) {
  if(!source_memory_){next_salt_=v;return;}
  auto b=source_memory_->mutable_view(source_pool_+0x40,2);b[0]=std::byte(v);b[1]=std::byte(v>>8);
}
std::span<const std::uint32_t> DatumPool::bitmap() const {
  if(!source_memory_)return bitmap_;
  const auto count=(std::size_t(capacity())+31)/32;const auto address=source_field(0x48);
  const auto b=source_memory_->view(address,count*4);bitmap_view_.resize(count);
  for(std::size_t i=0;i<count;++i){std::uint32_t v=0;for(unsigned j=0;j<4;++j)v|=std::to_integer<std::uint32_t>(b[i*4+j])<<(8*j);bitmap_view_[i]=v;}
  return bitmap_view_;
}
std::span<const std::uint8_t> DatumPool::elements() const {
  if(!source_memory_)return elements_;
  const auto b=source_memory_->view(source_field(0x44),std::size_t(capacity())*stride());
  return {reinterpret_cast<const std::uint8_t*>(b.data()),b.size()};
}
std::span<std::uint8_t> DatumPool::element_storage() {
  if(!source_memory_)return elements_;
  const auto b=source_memory_->mutable_view(source_field(0x44),std::size_t(capacity())*stride());
  return {reinterpret_cast<std::uint8_t*>(b.data()),b.size()};
}
DatumPool::DatumPool(std::string_view name, std::int32_t capacity,
                     std::int32_t stride, std::uint16_t flags)
    : name_(name.substr(0,31)), capacity_(capacity), stride_(stride), flags_(flags) {
  // This native storage adapter covers the recovered player/object pools and
  // bounded capacity fixtures; unsupported storage shapes fail explicitly.
  if (name_.size() < 2 || capacity <= 0 || capacity > 65535 || stride < 2)
    throw std::invalid_argument("Unsupported datum pool storage shape");
  elements_.resize(std::size_t(capacity) * stride);
  bitmap_.resize((std::size_t(capacity) + 31) / 32);
  reset();
}
std::uint16_t DatumPool::salt(std::int32_t index) const {
  const auto at = std::size_t(index) * stride_;
  return std::uint16_t(elements_[at] | (std::uint16_t(elements_[at+1]) << 8));
}
void DatumPool::reset() {
  if(source_memory_){source_raw_datum_reset(source_pool_,source_storage(*source_memory_));return;}
  cursor_ = high_water_ = count_ = 0;
  next_salt_ = std::uint16_t(std::uint8_t(name_[0]) |
      (std::uint16_t(std::uint8_t(name_[1])) << 8) | 0x8000u);
  if (flags_ & 8) std::fill(elements_.begin(), elements_.end(), 0xba);
  for (std::int32_t i=0; i<capacity_; ++i) {
    const auto at = std::size_t(i) * stride_;
    elements_[at] = elements_[at+1] = 0;
  }
  std::fill(bitmap_.begin(), bitmap_.end(), 0);
}
std::uint32_t DatumPool::allocate_element(std::int32_t index) {
  bitmap_[unsigned(index)/32] |= 1u << (unsigned(index)%32);
  ++count_;
  high_water_ = std::max(high_water_, index+1);
  const auto at = std::size_t(index) * stride_;
  std::fill_n(elements_.begin()+at, stride_, 0);
  const auto assigned = next_salt_;
  elements_[at] = std::uint8_t(assigned); elements_[at+1] = std::uint8_t(assigned >> 8);
  ++next_salt_;
  if (next_salt_ == 0xffff) next_salt_ = 0x8000;
  return (std::uint32_t(assigned) << 16) | std::uint32_t(index);
}
std::uint32_t DatumPool::allocate() {
  if(source_memory_)return source_raw_datum_allocate(source_pool_,source_storage(*source_memory_));
  auto index = cursor_;
  while (index < high_water_ &&
      (bitmap_[unsigned(index)/32] & (1u << (unsigned(index)%32)))) ++index;
  if (index >= high_water_) index = high_water_;
  if (index >= capacity_) return none;
  cursor_ = index+1;
  return allocate_element(index);
}
std::uint32_t DatumPool::allocate_at(std::int32_t index) {
  if(source_memory_)return source_raw_datum_allocate_at(source_pool_,index,source_storage(*source_memory_));
  if (index < 0 || index >= capacity_ || salt(index)) return none;
  return allocate_element(index);
}
void DatumPool::release_unchecked(std::uint32_t datum) {
  if(source_memory_){source_raw_datum_release_unchecked(source_pool_,datum,source_storage(*source_memory_));return;}
  const auto index = std::int32_t(datum & 0xffffu);
  assert(index < capacity_);
  const auto at = std::size_t(index) * stride_;
  if (flags_ & 8) std::fill_n(elements_.begin()+at, stride_, 0xba);
  elements_[at] = elements_[at+1] = 0;
  bitmap_[unsigned(index)/32] &= ~(1u << (unsigned(index)%32));
  --count_;
  cursor_ = std::min(cursor_, index);
  while (high_water_ > 0 && !salt(high_water_-1)) --high_water_;
}
std::span<const std::uint8_t> DatumPool::lookup(std::uint32_t datum) const {
  if(source_memory_){const auto p=source_raw_datum_lookup(source_pool_,datum,source_storage(*source_memory_));
    if(!p)return {};const auto b=source_memory_->view(p,stride());return {reinterpret_cast<const std::uint8_t*>(b.data()),b.size()};}
  const auto index = std::int32_t(datum & 0xffffu);
  if (datum == none || index >= high_water_ || !salt(index) || salt(index) != (datum >> 16))
    return {};
  return std::span(elements_).subspan(std::size_t(index)*stride_, stride_);
}
std::span<std::uint8_t> DatumPool::lookup(std::uint32_t datum) {
  if(source_memory_){const auto p=source_raw_datum_lookup(source_pool_,datum,source_storage(*source_memory_));
    if(!p)return {};const auto b=source_memory_->mutable_view(p,stride());return {reinterpret_cast<std::uint8_t*>(b.data()),b.size()};}
  const auto result = std::as_const(*this).lookup(datum);
  if (result.empty()) return {};
  return std::span(elements_).subspan(std::size_t(datum & 0xffffu)*stride_, stride_);
}
} // namespace halo2
